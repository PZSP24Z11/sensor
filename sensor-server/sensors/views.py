from django.http import JsonResponse
from .models import Pomiar, Sensor, TypPomiaru, SensorTypPomiaru
import re
from django.views.decorators.csrf import csrf_exempt


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

            match = re.match(r"([A-Za-z0-9_-]+)%([a-fA-F0-9:]+)%([A-Za-z]*)", data)

            if not match:
                return JsonResponse({"status": 3, "message": "Invalid data format"}, status=400)

            name = match.group(1)
            mac_address = match.group(2)
            measurement_types = match.group(3)

            type_map = {"T": "Temperatura", "W": "Wilgotnosc"}

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
