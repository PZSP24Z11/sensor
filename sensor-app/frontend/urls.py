"""
URL configuration for frontend project.

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

from django.urls import path
from . import views
from .views import (
    RegisterUserView,
    LoginView,
    AdminView,
    UserView,
    LatestSensorMeasurementsView,
    MeasurementsView,
    SensorsView,
    SensorRequestsView,
    AdminPermissionRequestsView,
    LogoutView,
    ChangeSensorNameView,
    DeleteSensorView,
    UserSensorsView,
    UserPermissionRequestsView,
    UserMeasurementsView,
    ArchiveView,
    GetMeasurementsView
)

urlpatterns = [
    path("", views.index, name="index"),
    path("register/", RegisterUserView.as_view(), name="register"),
    path("login/", LoginView.as_view(), name="login"),
    path("admin/", AdminView.as_view(), name="admin"),
    path("user/", UserView.as_view(), name="user"),
    path("user/measurements/", UserMeasurementsView.as_view(), name="user-measurements"),
    path("user/sensors", UserSensorsView.as_view(), name="user-sensors"),
    path("user/permission_requests", UserPermissionRequestsView.as_view(), name="user-permission-requests"),
    path("user/archive/", ArchiveView.as_view(), name="user-archive"),
    path("user/archive/measurements/", GetMeasurementsView.as_view(), name="archive-measurements"),
    path("latest_sensor_measurements/", LatestSensorMeasurementsView.as_view(), name="latest-sensor-measurements"),
    path("admin/measurements/", MeasurementsView.as_view(), name="measurements"),
    path("admin/sensors/", SensorsView.as_view(), name="sensors"),
    path("admin/sensor_requests/", SensorRequestsView.as_view(), name="sensor-requests"),
    path("admin/permission_requests/", AdminPermissionRequestsView.as_view(), name="permission-requests"),
    path("admin/sensors/change_name/", ChangeSensorNameView.as_view(), name="change-sensor-name"),
    path("admin/sensors/delete/", DeleteSensorView.as_view(), name="delete-sensor"),
    path("logout/", LogoutView.as_view(), name="logout"),
]
