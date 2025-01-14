from django.http import JsonResponse, HttpResponse, HttpRequest
from django.utils.decorators import method_decorator
from sensors.models import (
    Pomiar,
    Sensor,
    TypPomiaru,
    SensorTypPomiaru,
    Uzytkownik,
    UzytkownikSensor,
    SensorRequest,
    SensorPermissionRequest,
)
from django.contrib.auth.hashers import make_password, check_password
from django.contrib.sessions.models import Session
from django.views.decorators.csrf import csrf_exempt, csrf_protect
from django.contrib.auth import authenticate, login
from typing import Optional
import json
from django.shortcuts import render, redirect
from django.middleware.csrf import get_token
from django.core.paginator import Paginator
from django.db.models import Avg
import re
import string
import random
from datetime import datetime
from django.contrib.auth.decorators import login_required
from apiserver.anomaly_notifier import send_anomaly_mail
from django.views import View


type_map = {"T": ("Temperature", "\u00B0C"), "H": ("Humidity", "%")}


def get_user_from_session(session_key: str) -> Uzytkownik:
    """Get user from session key
    :raises:Session.DoesNotExist: remember to handle it
    """
    session = Session.objects.get(session_key=session_key)
    user_id = session.get_decoded().get("_auth_user_id")
    user = Uzytkownik.objects.get(id=user_id)
    return user


def get_measurement_types(sensor_id: int) -> Optional[list[str]]:
    try:
        sensor = Sensor.objects.get(id=sensor_id)
        measurement_types = TypPomiaru.objects.filter(
            id__in=SensorTypPomiaru.objects.filter(sensor=sensor).values_list("typ_pomiaru_id", flat=True)
        )
        return list(measurement_types.values())
    except Sensor.DoesNotExist:
        return None


def get_latest_measurements(sensor_id: int, max: int = 20):
    sensor = Sensor.objects.get(id=sensor_id)
    measurement_types = TypPomiaru.objects.filter(
        id__in=SensorTypPomiaru.objects.filter(sensor=sensor).values_list("typ_pomiaru_id", flat=True)
    )

    latest_measurements = {}
    for measurement_type in measurement_types:
        measurements = Pomiar.objects.filter(sensor=sensor, typ_pomiaru=measurement_type).order_by("-data_pomiaru")[
            :max
        ]
        latest_measurements[measurement_type.nazwa_pomiaru] = {
            "jednostka": measurement_type.jednostka,
            "pomiary": list(measurements.values())[::-1],
        }
    return latest_measurements


def validate_user_permission(user_id: int, sensor_id: str) -> bool:
    try:
        user = Uzytkownik.objects.get(id=user_id)
        if user.is_superuser:
            return True

        return UzytkownikSensor.objects.filter(uzytkownik_id=user_id, sensor_id=sensor_id).exists()
    except Uzytkownik.DoesNotExist:
        return False


def _get_users_with_notifications_by_mac(mac_address: str) -> Uzytkownik.objects:
    return Uzytkownik.objects.filter(
        email_notification=True, uzytkowniksensor__sensor__adres_MAC=mac_address
    ).distinct()


def handle_anomaly(sensor: Sensor) -> None:
    users = _get_users_with_notifications_by_mac(sensor.adres_MAC)
    if not users:
        print("Anomaly found but no users notified (none connected with e-mail notifications on)")
        return
    send_anomaly_mail(sensor, users)


def get_last_pomiar(_: HttpRequest) -> JsonResponse:
    try:
        latest_record = Pomiar.objects.latest("data_pomiaru")
        return JsonResponse({"value": latest_record.wartosc_pomiaru})
    except Pomiar.DoesNotExist:
        return JsonResponse({"error": "No data found"}, status=404)


# TEST ENDPOINT
@csrf_exempt
def force_anomaly(request: HttpRequest) -> JsonResponse:
    print("in users by mac")
    if request.method != "GET":
        return JsonResponse({"error": "Invalid resquest method"}, status=405)

    print("passed GET check")

    try:
        print("in try")
        mac_address = request.GET.get("mac_address")
        sensor = Sensor.objects.get(adres_MAC=mac_address)
        handle_anomaly(sensor)
        print("request body loaded")
        users = _get_users_with_notifications_by_mac(mac_address)
        print("users received")
        users = [user.email for user in users]
        users = ", ".join(users)
        return JsonResponse({"users": users})
    except Exception as e:
        return JsonResponse({"error": str(e)}, status=400)


@csrf_exempt
def add_user(request: HttpRequest) -> JsonResponse:
    if request.method != "POST":
        return JsonResponse({"error": "Invalid request method"}, status=405)

    try:
        data = json.loads(request.body)
        Uzytkownik.objects.create(
            nazwa_uzytkownika=data["name"],
            haslo=make_password(data["password"]),
            email=data["email"],
            email_notification=data["wants_emails"],
        )
        return JsonResponse({"message": "User created successfully"}, status=201)
    except Exception as e:
        return JsonResponse({"error": str(e)}, status=400)


@csrf_exempt
def sensor_register_view(request: HttpRequest) -> HttpResponse:
    if request.method == "POST":
        try:
            data = request.body.decode("utf-8")

            match = re.match(r"SENSORREQ%([a-fA-F0-9:]+)%([A-Za-z]*)", data)

            if not match:
                return HttpResponse("3")

            mac_address = match.group(1)
            measurement_types = match.group(2)

            try:
                sensor = Sensor.objects.get(adres_MAC=mac_address)
                for measurement in measurement_types:
                    m_type = type_map.get(measurement.upper())
                    full_name = m_type[0]
                    if full_name:
                        measurement_type, _ = TypPomiaru.objects.get_or_create(
                            nazwa_pomiaru=full_name, jednostka=m_type[1]
                        )
                        SensorTypPomiaru.objects.get_or_create(sensor=sensor, typ_pomiaru=measurement_type)
                return HttpResponse("1")
            except Sensor.DoesNotExist:
                if not SensorRequest.objects.filter(adres_MAC=mac_address).exclude(status="Approved").exists():
                    random_chars = "".join(random.choices(string.ascii_letters + string.digits, k=7))
                    current_datetime = datetime.now().strftime("%Y-%m-%d_%H-%M-%S")
                    name = f"{random_chars}_{current_datetime}"

                    SensorRequest.objects.create(nazwa_sensora=name, adres_MAC=mac_address)
                return HttpResponse("2")

        except Exception as e:
            print(e)
            return HttpResponse("3")

    return HttpResponse("3")


@method_decorator(csrf_exempt, name="dispatch")
class MeasurementsRegisterView(View):
    def __init__(self) -> None:
        self._anomaly_cooldown = 0

    def post(self, request: HttpRequest) -> HttpResponse:
        if request.method == "POST":
            try:
                print("in register view")
                data = request.body.decode("utf-8")

                match = re.match(r"([a-fA-F0-9:]+)%((?:[A-Za-z]\d+%?)+)", data)
                if not match:
                    return HttpResponse("3")

                mac_address = match.group(1)
                measurements = match.group(2)

                try:
                    sensor = Sensor.objects.get(adres_MAC=mac_address)
                except Sensor.DoesNotExist:
                    return HttpResponse("3")

                measurements_data = re.findall(r"([A-Za-z])(\d+)", measurements)
                for measurement_type, value in measurements_data:
                    full_name = type_map.get(measurement_type.upper())[0]
                    if not full_name:
                        return HttpResponse("3")

                    try:
                        measurement_type_obj = TypPomiaru.objects.get(nazwa_pomiaru=full_name)
                    except TypPomiaru.DoesNotExist:
                        return HttpResponse("3")

                    is_allowed = SensorTypPomiaru.objects.filter(sensor=sensor, typ_pomiaru=measurement_type_obj).exists()
                    if not is_allowed:
                        return HttpResponse("3")

                    average_pomiar_value = Pomiar.objects.filter(
                        sensor=sensor, 
                        typ_pomiaru=measurement_type_obj
                        ).order_by('-data_pomiaru')[:50].aggregate(Avg('wartosc_pomiaru'))['wartosc_pomiaru__avg']
                    Pomiar.objects.create(
                        sensor=sensor,
                        typ_pomiaru=measurement_type_obj,
                        wartosc_pomiaru=float(value),
                        data_pomiaru=datetime.now(),
                    )
                    print(f"average reading: {average_pomiar_value}")
                    print(f"this reading: {float(value)}")
                    if float(value) >= 1.1 * average_pomiar_value and not self._anomaly_cooldown:
                        self._anomaly_cooldown = 100
                        print("ANOMALY DETECTED!")
                        handle_anomaly(sensor)
                    self._anomaly_cooldown -= 1

                return HttpResponse("2")

            except Exception as e:
                print(e)
                return HttpResponse("3")

        return HttpResponse("3")


def sensors_list_view(request: HttpRequest) -> HttpResponse:
    sensors = Sensor.objects.all()
    return render(request, "sensors/sensors_list.html", {"sensors": sensors})


def latest_measurements_view(request: HttpRequest, sensor_id: int) -> JsonResponse:
    try:
        measurements = Pomiar.objects.filter(sensor_id=sensor_id).order_by("-wartosc_pomiaru")[:10]
        data = [
            {
                "typ_pomiaru": measurement.typ_pomiaru.nazwa_pomiaru,
                "jednostka": measurement.typ_pomiaru.jednostka,
                "wartosc": measurement.wartosc_pomiaru,
                "data": measurement.data_pomiaru.strftime("%Y-%m-%d %H:%M:%S"),
            }
            for measurement in measurements
        ]
        return JsonResponse({"measurements": data})
    except Exception as e:
        return JsonResponse({"error": str(e)}, status=500)


@method_decorator(csrf_exempt, name="dispatch")
class RegisterUserView(View):
    def post(self, request: HttpRequest) -> JsonResponse:
        try:
            data = json.loads(request.body)
            username, password, email = data.get("username"), data.get("password"), data.get("email")
            status, message = 201, "Client registered"

            if not all((username, password, email)):
                status = 400
                message = "Missing one or more require fields"
            elif Uzytkownik.objects.filter(username=username).exists():
                status = 400
                message = f"Username {username} already exist"
            else:
                Uzytkownik.objects.create_user(username, password, email=email)
        except json.JSONDecodeError as e:
            print(e)
            status = 400
            message = str(e)
        except Exception as e:
            print(e)
            status = 500
            message = "Internal error creating user"

        return JsonResponse(status=status, data={"message": message})


@method_decorator(csrf_exempt, name="dispatch")
class LoginView(View):
    def post(self, request: HttpRequest) -> JsonResponse:
        try:
            data = json.loads(request.body)
            username, password = data.get("username"), data.get("password")
            status, message, session_id = 200, "Login successful", ""

            user = authenticate(request, username=username, password=password)
            if user is not None:
                login(request, user)
                session_id = request.session.session_key
                if not session_id:
                    request.session.create()
                    session_id = request.session.session_key
            else:
                status, message = 401, "Invalid credentials"
        except json.JSONDecodeError as e:
            status, message = 400, str(e)
        except Exception as e:
            print(e)
            status, message = 500, "Internal server error"

        return JsonResponse(
            status=status, data={"message": message, "session_id": session_id, "is_admin": user.is_superuser}
        )


@method_decorator(csrf_exempt, name="dispatch")
class GetMeasurementsView(View):
    def get(self, request: HttpRequest) -> JsonResponse:
        sensor_id = request.GET.get("sensor_id")
        reading_type = request.GET.get("type")
        page_number = request.GET.get("page")

        readings = Pomiar.objects.filter(sensor_id=sensor_id, typ_pomiaru=reading_type).order_by("-data_pomiaru")
        paginator = Paginator(readings, 10)

        result = paginator.get_page(page_number)
        readings_data = [
            {"timestamp": reading.data_pomiaru.strftime("%Y-%m-%d %H:%M:%S"), "value": reading.wartosc_pomiaru}
            for reading in result
        ]

        return JsonResponse(
            {
                "results": readings_data,
                "count": paginator.count,
                "num_pages": paginator.num_pages,
                "current_page": result.number,
            }
        )


@method_decorator(csrf_exempt, name="dispatch")
class GetUsernameView(View):
    def get(self, request: HttpRequest) -> JsonResponse:
        session_id = request.COOKIES.get("session_id")
        if not session_id:
            return JsonResponse(status=401, data={"message": "Unauthorized user - no session_id"})
        try:
            user = get_user_from_session(session_id)
            print(f"{user.username} {user.email}")
            return JsonResponse(status=200, data={"username": user.username})
        except Uzytkownik.DoesNotExist:
            return JsonResponse(status=401, data={"message": "Unauthorized user - invalid session token"})
        except Exception as e:
            print(e)
            return JsonResponse(status=500, data={"message": "Internal server error"})


@method_decorator(csrf_exempt, name="dispatch")
class LogoutView(View):
    def get(self, request: HttpRequest) -> JsonResponse:
        session_id = request.COOKIES.get("session_id")
        status, message = 200, "Logout Successful"
        if not session_id:
            (status,) = 401, "Unauthorized user - no session_id"
        else:
            try:
                session = Session.objects.get(session_key=session_id)
                session.delete()
            except Session.DoesNotExist:
                status, message = 400, "Invalid session ID"
            except Exception as e:
                print(e)
                status, message = 500, "Internal server error"

        return JsonResponse(status=status, data={"message": message})


@method_decorator(csrf_exempt, name="dispatch")
class ValidateSessionView(View):
    def get(self, request: HttpRequest) -> JsonResponse:
        status, is_valid, is_admin = 200, True, False
        session_id = request.COOKIES.get("session_id")
        try:
            if session_id:
                user = get_user_from_session(session_id)
                is_admin = user.is_superuser
            else:
                status, is_valid = 400, False
        except Session.DoesNotExist:
            status, is_valid = 401, False
        except Exception as e:
            print(e)
            status = 500

        return JsonResponse(status=status, data={"is_valid": is_valid, "is_admin": is_admin})


@method_decorator(csrf_exempt, name="dispatch")
class GetSensorsView(View):
    def get(self, request: HttpRequest) -> JsonResponse:
        session_id = request.COOKIES.get("session_id")
        if not session_id:
            return JsonResponse(status=401, data={"message": "Unauthorized user - no session_id"})
        try:
            user = get_user_from_session(session_id)
            if user.is_superuser:
                sensors = Sensor.objects.all()
            else:
                sensors = Sensor.objects.filter(
                    id__in=UzytkownikSensor.objects.filter(uzytkownik=user).values_list("sensor", flat=True)
                )
            sensors = list(sensors.values())
            for sensor in sensors:
                sensor["measurement_types"] = get_measurement_types(sensor["id"])

            return JsonResponse(status=200, data={"sensor_list": sensors})
        except Session.DoesNotExist:
            return JsonResponse(status=401, data={"message": "Invalid session_id"})
        except Exception as e:
            print(e)
            return JsonResponse(status=500, data={"message": "Internal server error"})


@method_decorator(csrf_exempt, name="dispatch")
class GetAllSensorsView(View):
    def get(self, request: HttpRequest) -> JsonResponse:
        session_id = request.COOKIES.get("session_id")
        if not session_id:
            return JsonResponse(status=401, data={"message": "Unauthorized user - no session_id"})
        try:
            get_user_from_session(session_id)  # if no exception - user exists
            sensors = list(Sensor.objects.all().values())
            for sensor in sensors:
                sensor["measurement_types"] = get_measurement_types(sensor["id"])

            return JsonResponse(status=200, data={"sensors": sensors})
        except Session.DoesNotExist:
            return JsonResponse(status=401, data={"message": "Invalid session_id"})
        except Exception as e:
            print(e)
            return JsonResponse(status=500, data={"message": "Internal server error"})


@method_decorator(csrf_exempt, name="dispatch")
class LatestSensorMeasurementsView(View):
    def post(self, request: HttpRequest) -> JsonResponse:
        session_id = request.COOKIES.get("session_id")

        if not session_id:
            return JsonResponse(status=401, data={"message": "Unauthorized user - no session_id"})
        try:
            user = get_user_from_session(session_id)

            data = json.loads(request.body)
            sensor_id = data.get("sensor_id")
            if not validate_user_permission(user.id, sensor_id):
                return JsonResponse(status=401, data={"message": "User doesnt have permission"})
            measurements = get_latest_measurements(sensor_id)
            return JsonResponse(status=200, data={"measurements": measurements})
        except Session.DoesNotExist:
            return JsonResponse(status=401, data={"message": "Invalid session_id"})
        except Sensor.DoesNotExist:
            return JsonResponse(status=400, data={"message": "Sensor does not exist"})
        except Exception as e:
            print(e)
            return JsonResponse(status=500, data={"message": "Internal server error"})


@method_decorator(csrf_exempt, name="dispatch")
class ChangeSensorRequestStatusView(View):
    def post(self, request: HttpRequest) -> JsonResponse:
        session_id = request.COOKIES.get("session_id")
        if not session_id:
            return JsonResponse(status=401, data={"message": "Unauthorized user - no session_id"})

        try:
            user = get_user_from_session(session_id)
            if not user.is_superuser:
                return JsonResponse(status=401, data={"message": "User doesnt have privileges to perform operation"})

            data = json.loads(request.body)
            sesnor_request_id = data["sensor_request_id"]
            set_status = data["status"]

            if set_status not in ("Rejected", "Approved"):
                return JsonResponse(
                    status=400,
                    data={
                        "meassage": "Bad request: status can only be changed to one of the following: Rejected, Accepted"
                    },
                )

            sreq = SensorRequest.objects.get(id=sesnor_request_id)
            sreq.status = set_status
            if sreq.status == "Approved":
                Sensor.objects.create(nazwa_sensora=sreq.nazwa_sensora, adres_MAC=sreq.adres_MAC)

            sreq.save()

            return JsonResponse(status=200, data={"message": "Status changed successfully"})
        except SensorRequest.DoesNotExist:
            return JsonResponse(status=400, data={"message": "Sensor Request doesnt exist"})
        except Session.DoesNotExist:
            return JsonResponse(stataus=401, data={"message": "Unauthorizes session"})
        except Exception as e:
            print(e)
            return JsonResponse(status=500, data={"message": "Internal server error"})


@method_decorator(csrf_exempt, name="dispatch")
class PendingSensorRequestsView(View):
    def get(self, request: HttpRequest) -> JsonResponse:
        session_id = request.COOKIES.get("session_id")
        if not session_id:
            return JsonResponse(status=401, data={"message": "Unauthorized user - no session_id"})
        try:
            user = get_user_from_session(session_id)
            if not user.is_superuser:
                return JsonResponse(status=401, data={"message": "User doesnt have privileges to perform operation"})

            sesnor_requests = list(SensorRequest.objects.filter(status="Pending").values())
            return JsonResponse(status=200, data={"sensor_requests": sesnor_requests})
        except Session.DoesNotExist:
            return JsonResponse(stataus=401, data={"message": "Unauthorizes session"})
        except Exception as e:
            print(e)
            return JsonResponse(status=500, data={"message": "Internal server error"})


@method_decorator(csrf_exempt, name="dispatch")
class DeleteSensorView(View):
    def post(self, request: HttpRequest) -> JsonResponse:
        session_id = request.COOKIES.get("session_id")
        if not session_id:
            return JsonResponse(status=401, data={"message": "Unauthorized user - no session_id"})
        try:
            user = get_user_from_session(session_id)
            if not user.is_superuser:
                return JsonResponse(status=401, data={"message": "User doesnt have privileges to perform operation"})
            data = json.loads(request.body)
            sensor_id = data["sensor_id"]
            if not sensor_id:
                return JsonResponse(status=400, data={"message": "Bad format: sensor id required"})

            sensor = Sensor.objects.get(id=sensor_id)
            sensor.delete()

            return JsonResponse(status=200, data={"message": "Sensor deleted"})
        except Sensor.DoesNotExist:
            return JsonResponse(stataus=400, data={"message": "Invalid sensor id"})
        except Session.DoesNotExist:
            return JsonResponse(stataus=401, data={"message": "Unauthorizes session"})
        except json.JSONDecodeError as e:
            print(e)
            return JsonResponse(status=400, data={"message": "Expected JSON format"})
        except Exception as e:
            print(e)
            return JsonResponse(status=500, data={"message": "Internal server error"})


@method_decorator(csrf_exempt, name="dispatch")
class ChangeSensorNameView(View):
    def post(self, request: HttpRequest) -> JsonResponse:
        session_id = request.COOKIES.get("session_id")
        if not session_id:
            return JsonResponse(status=401, data={"message": "Unauthorized user - no session_id"})
        try:
            user = get_user_from_session(session_id)
            if not user.is_superuser:
                return JsonResponse(status=401, data={"message": "User doesnt have privileges to perform operation"})
            data = json.loads(request.body)
            sensor_id = data["sensor_id"]
            sensor_name = data["name"]
            if not all([sensor_id, sensor_name]):
                return JsonResponse(status=400, data={"message": "Bad format sensor id and non empty required"})

            sensor = Sensor.objects.get(id=sensor_id)
            sensor.nazwa_sensora = sensor_name
            sensor.save()
            return JsonResponse(status=200, data={"message": "Changed name"})
        except Sensor.DoesNotExist:
            return JsonResponse(stataus=400, data={"message": "Invalid sensor id"})
        except Session.DoesNotExist:
            return JsonResponse(stataus=401, data={"message": "Unauthorized session"})
        except json.JSONDecodeError as e:
            print(e)
            return JsonResponse(status=400, data={"message": "Expected JSON format"})
        except Exception as e:
            print(e)
            return JsonResponse(status=500, data={"message": "Internal server error"})


@method_decorator(csrf_exempt, name="dispatch")
class PendingPermissionRequestsView(View):
    def get(self, request: HttpRequest) -> JsonResponse:
        session_id = request.COOKIES.get("session_id")
        if not session_id:
            return JsonResponse(status=401, data={"message": "Unauthorized user - no session_id"})
        try:
            user = get_user_from_session(session_id)
            if user.is_superuser:
                permission_requests = SensorPermissionRequest.objects.filter(status="Pending")
            else:
                permission_requests = SensorPermissionRequest.objects.filter(status="Pending", uzytkownik=user.id)

            response_data = []
            for prequest in permission_requests:
                sensor = Sensor.objects.get(id=prequest.sensor.id)
                user = Uzytkownik.objects.get(id=prequest.uzytkownik.id)
                response_data.append(
                    {
                        "id": prequest.id,
                        "username": user.username,
                        "email": user.email,
                        "sensor_name": sensor.nazwa_sensora,
                        "sensor_MAC": sensor.adres_MAC,
                        "status": prequest.status,
                    }
                )

            return JsonResponse(status=200, data={"p_requests": response_data})
        except Session.DoesNotExist:
            return JsonResponse(status=401, data={"message": "Unauthorized session"})
        except Exception as e:
            print(e)
            return JsonResponse(status=500, data={"message": "Internal server error"})


@method_decorator(csrf_exempt, name="dispatch")
class SubmitPermissionRequestView(View):
    def post(self, request: HttpRequest) -> JsonResponse:
        session_id = request.COOKIES.get("session_id")
        if not session_id:
            return JsonResponse(status=401, data={"message": "Unauthorized user - no session_id"})
        try:
            user = get_user_from_session(session_id)

            data = json.loads(request.body)
            sensor_id = data["sensor_id"]

            sensor = Sensor.objects.get(id=sensor_id)
            if SensorPermissionRequest.objects.filter(uzytkownik=user, sensor=sensor, status="Pending").exists():
                return JsonResponse(status=400, data={"message": "Pending request already exists for this sensor"})
            SensorPermissionRequest.objects.create(uzytkownik=user, sensor=sensor, status="Pending")

            return JsonResponse(status=201, data={"message": "Request Created"})
        except json.JSONDecoder:
            return JsonResponse(status=400, data={"message": "Invalid data format"})
        except Session.DoesNotExist:
            return JsonResponse(status=401, data={"message": "Unauthorized session"})
        except Exception as e:
            print(e)
            return JsonResponse(status=500, data={"message": "Internal server error"})


@method_decorator(csrf_exempt, name="dispatch")
class ChangePermissionRequestStatusView(View):
    def post(self, request: HttpRequest) -> JsonResponse:
        session_id = request.COOKIES.get("session_id")
        if not session_id:
            return JsonResponse(status=401, data={"message": "Unauthorized user - no session_id"})
        try:
            user = get_user_from_session(session_id)
            if not user.is_superuser:
                return JsonResponse(
                    status=401, data={"message": "You need to be administrator to perform this operation"}
                )
            data = json.loads(request.body)
            p_request_id = data["request_id"]
            status = data["status"]

            if not p_request_id or not status or status not in ("Approved", "Rejected"):
                return JsonResponse(status=400, data={"message": "Both request id and status fields are required"})

            permission_request = SensorPermissionRequest.objects.get(id=p_request_id)
            user = Uzytkownik.objects.get(id=permission_request.uzytkownik.id)

            if status == "Approved":
                print("in approved")
                UzytkownikSensor.objects.create(
                    uzytkownik=permission_request.uzytkownik, sensor=permission_request.sensor
                )

            permission_request.status = status
            permission_request.save()

            return JsonResponse(status=200, data={"message": "Request updated successfully"})
        except json.JSONDecodeError as e:
            print(e)
            return JsonResponse(status=400, data={"message": "Expected json value"})
        except Session.DoesNotExist:
            return JsonResponse()


def index_view(request: HttpRequest) -> HttpResponse:
    return render(request, "frontend/index.html")
