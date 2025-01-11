from django.urls import path
from .views import sensors_list_view, latest_measurements_view, login_view, dashboard_view

urlpatterns = [
    path("", sensors_list_view, name="sensors_list"),
    path("latest_measurements/<int:sensor_id>/", latest_measurements_view, name="latest_measurements"),
    path("login/", login_view, name="login"),
    path("dashboard/", dashboard_view, name="dashboard"),
]
