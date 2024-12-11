from django.http import JsonResponse
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
                return JsonResponse({"status": 3, "message": "Invalid data format"}, status=400)

            random_chars = "".join(random.choices(string.ascii_letters + string.digits, k=7))
            current_datetime = datetime.now().strftime("%Y-%m-%d_%H-%M-%S")

            name = f"{random_chars}_{current_datetime}"
            mac_address = match.group(1)
            measurement_types = match.group(2)

            sensor, created_sensor = Sensor.objects.get_or_create(nazwa_sensora=name, adres_MAC=mac_address)

            if created_sensor:
                for measurement in measurement_types:
                    full_name = type_map.get(measurement.upper())
                    if full_name:
                        measurement_type, _ = TypPomiaru.objects.get_or_create(nazwa_pomiaru=full_name)
                        SensorTypPomiaru.objects.create(sensor=sensor, typ_pomiaru=measurement_type)
                return JsonResponse({"status": 2, "message": "Sensor registered"})

            for measurement in measurement_types:
                full_name = type_map.get(measurement.upper())
                if full_name:
                    measurement_type, _ = TypPomiaru.objects.get_or_create(nazwa_pomiaru=full_name)
                    SensorTypPomiaru.objects.get_or_create(sensor=sensor, typ_pomiaru=measurement_type)

            return JsonResponse({"status": 1, "message": "Sensor already exists"})

        except Exception as e:
            return JsonResponse({"status": 3, "message": "An error occurred", "error": str(e)}, status=500)

    return JsonResponse({"status": 3, "message": "Method not allowed"}, status=405)


@csrf_exempt
def measurements_register_view(request):
    if request.method == "POST":
        try:
            data = request.body.decode("utf-8")

            match = re.match(r"([a-fA-F0-9:]+)%((?:[A-Za-z]\d+%?)+)", data)
            if not match:
                return JsonResponse({"status": 3, "message": "Invalid data format"}, status=400)

            mac_address = match.group(1)
            measurements = match.group(2)

            try:
                sensor = Sensor.objects.get(adres_MAC=mac_address)
            except Sensor.DoesNotExist:
                return JsonResponse({"status": 3, "message": "Sensor not registered"}, status=404)

            measurements_data = re.findall(r"([A-Za-z])(\d+)", measurements)
            for measurement_type, value in measurements_data:
                full_name = type_map.get(measurement_type.upper())
                if not full_name:
                    return JsonResponse(
                        {"status": 3, "message": f"Measurement type '{measurement_type}' not recognized"},
                        status=400,
                    )

                try:
                    measurement_type_obj = TypPomiaru.objects.get(nazwa_pomiaru=full_name)
                except TypPomiaru.DoesNotExist:
                    return JsonResponse(
                        {"status": 3, "message": f"Measurement type '{full_name}' not found in the database"},
                        status=400,
                    )

                Pomiar.objects.create(
                    sensor=sensor,
                    typ_pomiaru=measurement_type_obj,
                    wartosc_pomiaru=float(value),
                    data_pomiaru=datetime.now(),
                )

            return JsonResponse({"status": 2, "message": "Measurements registered"})

        except Exception as e:
            return JsonResponse({"status": 3, "message": "An error occurred", "error": str(e)}, status=500)

    return JsonResponse({"status": 3, "message": "Method not allowed"}, status=405)
