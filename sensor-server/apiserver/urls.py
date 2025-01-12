"""
URL configuration for apiserver project.

The `urlpatterns` list routes URLs to views. For more information please see:
    https://docs.djangoproject.com/en/5.1/topics/http/urls/
Examples:
Function views
    1. Add an import:  from my_app import views
    2. Add a URL to urlpatterns:  path('', views.home, name='home')
Class-based views
    1. Add an import:  from other_app.views import Home
    2. Add a URL to urlpatterns:  path('', Home.as_view(), name='home')
Including another URLconf
    1. Import the include() function: from django.urls import include, path
    2. Add a URL to urlpatterns:  path('blog/', include('blog.urls'))
"""

from django.contrib import admin
from django.urls import path, include
from sensors.views import (
    get_last_pomiar,
    sensor_register_view,
    measurements_register_view,
    index_view,
    add_user,
    force_anomaly,
    RegisterUserView,
    LoginView,
    LogoutView,
    ValidateSessionView,
    GetSensorsView,
    LatestSensorMeasurementsView,
)

urlpatterns = [
    path("admin/", admin.site.urls),
    path("", index_view, name="index"),
    path("api/last_value/", get_last_pomiar, name="last_value"),
    path("api/add_user/", add_user, name="add_user"),
    path("api/force_anomaly/", force_anomaly, name="force_anomaly"),
    path("sensor/register/", sensor_register_view, name="sensor_register"),
    path("sensor/measurements/", measurements_register_view, name="measurements_register"),
    path("api/register_user/", RegisterUserView.as_view(), name="api-register-user"),
    path("api/login/", LoginView.as_view(), name="api-login"),
    path("api/logout/", LogoutView.as_view(), name="api-logout"),
    path("api/validate_session/", ValidateSessionView.as_view(), name="api-validate-session"),
    path("api/sensors/", GetSensorsView.as_view(), name="api-sensors"),
    path("api/latest_sensor_measurements/", LatestSensorMeasurementsView.as_view(), name="latest-sensor-measurements"),
]
