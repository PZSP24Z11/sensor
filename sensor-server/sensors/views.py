from django.http import JsonResponse, HttpResponse
from .models import Pomiar, Sensor, TypPomiaru, SensorTypPomiaru
from django.views.decorators.csrf import csrf_exempt
import re
import string
import random
from datetime import datetime

type_map = {"T": "Temperature", "H": "Humidity"}


def get_last_pomiar(request):
    try:
        latest_record = Pomiar.objects.latest("data_pomiaru")
        return JsonResponse({"value": latest_record.wartosc_pomiaru})
    except Pomiar.DoesNotExist:
        return JsonResponse({"error": "No data found"}, status=404)


@csrf_exempt
def sensor_register_view(request):
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
def measurements_register_view(request):
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
