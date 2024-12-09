from django.http import JsonResponse
from .models import Pomiar

def get_last_pomiar(request):
    try:
        latest_record = Pomiar.objects.latest('data_pomiaru')
        return JsonResponse({"value": latest_record.wartosc_pomiaru})
    except Pomiar.DoesNotExist:
        return JsonResponse({"error": "No data found"}, status=404)
