from django.http import JsonResponse, HttpResponse, HttpRequest
from django.utils.decorators import method_decorator
from sensors.models import Pomiar, Sensor, TypPomiaru, SensorTypPomiaru, Uzytkownik, UzytkownikManager
from django.contrib.auth.hashers import make_password, check_password
from django.views.decorators.csrf import csrf_exempt, csrf_protect
import json
from django.shortcuts import render, redirect
from django.middleware.csrf import get_token
import re
import string
import random
from datetime import datetime
from django.contrib import messages
from django.contrib.auth.decorators import login_required
from apiserver.anomaly_notifier import send_anomaly_mail
from rest_framework.decorators import api_view
from django.views import View


type_map = {"T": "Temperature", "H": "Humidity"}


def _get_users_with_notifications_by_mac(mac_address: str) -> Uzytkownik.objects:
    return Uzytkownik.objects.filter(
        powiadomienie_email=True, uzytkowniksensor__sensor__adres_MAC=mac_address
    ).distinct()


def handle_anomaly(sensor: Sensor) -> None:
    users = _get_users_with_notifications_by_mac(sensor.adres_MAC)
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
            powiadomienie_email=data["wants_emails"],
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
                    full_name = type_map.get(measurement.upper())
                    if full_name:
                        measurement_type, _ = TypPomiaru.objects.get_or_create(nazwa_pomiaru=full_name)
                        SensorTypPomiaru.objects.get_or_create(sensor=sensor, typ_pomiaru=measurement_type)
                return HttpResponse("1")
            except Sensor.DoesNotExist:
                random_chars = "".join(random.choices(string.ascii_letters + string.digits, k=7))
                current_datetime = datetime.now().strftime("%Y-%m-%d_%H-%M-%S")
                name = f"{random_chars}_{current_datetime}"

                sensor = Sensor.objects.create(nazwa_sensora=name, adres_MAC=mac_address)
                for measurement in measurement_types:
                    full_name = type_map.get(measurement.upper())
                    if full_name:
                        measurement_type, _ = TypPomiaru.objects.get_or_create(nazwa_pomiaru=full_name)
                        SensorTypPomiaru.objects.create(sensor=sensor, typ_pomiaru=measurement_type)
                return HttpResponse("2")

        except Exception:
            return HttpResponse("3")

    return HttpResponse("3")


@csrf_exempt
def measurements_register_view(request: HttpRequest) -> HttpResponse:
    if request.method == "POST":
        try:
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
                full_name = type_map.get(measurement_type.upper())
                if not full_name:
                    return HttpResponse("3")

                try:
                    measurement_type_obj = TypPomiaru.objects.get(nazwa_pomiaru=full_name)
                except TypPomiaru.DoesNotExist:
                    return HttpResponse("3")

                is_allowed = SensorTypPomiaru.objects.filter(sensor=sensor, typ_pomiaru=measurement_type_obj).exists()
                if not is_allowed:
                    return HttpResponse("3")
                Pomiar.objects.create(
                    sensor=sensor,
                    typ_pomiaru=measurement_type_obj,
                    wartosc_pomiaru=float(value),
                    data_pomiaru=datetime.now(),
                )

            return HttpResponse("2")

        except Exception:
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
            username = data.get("username")
            password = data.get("password")
            email = data.get("email")
            status = 200
            message = "Client registered"

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


@csrf_protect
def login_view(request):
    pass
    # if request.method == "POST":
    #     try:
    #         data = json.loads(request.body)

    #         username = data.get("username")
    #         password = data.get("password")

    #         if not username or not password:
    #             return JsonResponse({"error": "Missing required fields"}, status=400)

    #         try:
    #             user = Uzytkownik.objects.get(nazwa_uzytkownika=username)

    #             if user.check_password(password):
    #                 return JsonResponse({"message": "Login successful", "user_id": user.id}, status=200)
    #             else:
    #                 return JsonResponse({"error": "Invalid credentials"}, status=400)

    #         except Uzytkownik.DoesNotExist:
    #             return JsonResponse({"error": "Invalid credentials"}, status=400)

    #     except json.JSONDecodeError:
    #         return JsonResponse({"error": "Invalid JSON"}, status=400)

    #     except Exception as e:
    #         return JsonResponse({"error": str(e)}, status=500)

    # return JsonResponse({"error": "Invalid request method"}, status=405)


@api_view(["GET"])
def get_sensors(request):
    return JsonResponse({"sensors": ["sensor1", "sensor2"]})


def index_view(request: HttpRequest) -> HttpResponse:
    return render(request, "frontend/index.html")


@login_required
@csrf_protect
def dashboard_view(request: HttpRequest) -> HttpResponse:
    print(get_token(request))
    return render(request, "frontend/dashboard.html")
