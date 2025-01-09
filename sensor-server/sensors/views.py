from django.http import JsonResponse, HttpResponse, HttpRequest
from sensors.models import Pomiar, Sensor, TypPomiaru, SensorTypPomiaru, Uzytkownik
from django.contrib.auth.hashers import make_password, check_password
from django.views.decorators.csrf import csrf_exempt
import json
from django.shortcuts import render, redirect
import re
import string
import random
from datetime import datetime
from django.contrib import messages
from django.contrib.auth.decorators import login_required


type_map = {"T": "Temperature", "H": "Humidity"}


def get_last_pomiar(_: HttpRequest) -> JsonResponse:
    try:
        latest_record = Pomiar.objects.latest("data_pomiaru")
        return JsonResponse({"value": latest_record.wartosc_pomiaru})
    except Pomiar.DoesNotExist:
        return JsonResponse({"error": "No data found"}, status=404)


@csrf_exempt
def add_user(request: HttpRequest) -> JsonResponse:
    if request.method != "POST":
        return JsonResponse({"error": "Invalid request method"}, status=405)

    try:
        print("siur")
        data = json.loads(request.body)
        print('sparsowany siur')
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


def login_view(request: HttpRequest) -> HttpResponse:
    if request.method == "POST":
        username = request.POST.get("username")
        password = request.POST.get("password")

        try:
            user = Uzytkownik.objects.get(nazwa_uzytkownika=username)
            if user.check_password(password):
                request.session["user_id"] = user.id
                messages.success(request, "Login successful!")
                return redirect("dashboard")
            else:
                messages.error(request, "Invalid username or password")
        except Uzytkownik.DoesNotExist:
            messages.error(request, "Invalid username or password")

        return redirect("login")

    return render(request, "frontend/login.html")


def register_view(request: HttpRequest) -> HttpResponse:
    if request.method == "POST":
        username = request.POST.get("username")
        email = request.POST.get("email")
        password = request.POST.get("password")
        confirm_password = request.POST.get("confirmPassword")

        if password != confirm_password:
            messages.error(request, "Passwords do not match")
            return redirect("register")

        if Uzytkownik.objects.filter(email=email).exists():
            messages.error(request, "User with this email already exists")
            return redirect("register")

        if Uzytkownik.objects.filter(nazwa_uzytkownika=username).exists():
            messages.error(request, "User with this username already exists")
            return redirect("register")

        user = Uzytkownik(nazwa_uzytkownika=username, email=email, haslo=make_password(password))
        user.save()

        messages.success(request, "Account created successfully. You can now log in.")
        return redirect("login")

    return render(request, "frontend/register.html")


def index_view(request: HttpRequest) -> HttpResponse:
    return render(request, 'frontend/index.html')


@login_required
def dashboard_view(request: HttpRequest) -> HttpResponse:
    return render(request, "frontend/dashboard.html")
