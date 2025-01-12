from django.urls import path
from .views import sensors_list_view, latest_measurements_view

urlpatterns = [
    path("", sensors_list_view, name="sensors_list"),
    path("latest_measurements/<int:sensor_id>/", latest_measurements_view, name="latest_measurements"),
]
