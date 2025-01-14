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
    MeasurementsRegisterView,
    index_view,
    add_user,
    force_anomaly,
    RegisterUserView,
    LoginView,
    LogoutView,
    ValidateSessionView,
    GetSensorsView,
    LatestSensorMeasurementsView,
    PendingSensorRequestsView,
    ChangeSensorRequestStatusView,
    GetAllSensorsView,
    ChangeSensorNameView,
    DeleteSensorView,
    PendingPermissionRequestsView,
    SubmitPermissionRequestView,
    GetUsernameView,
    ChangePermissionRequestStatusView,
    GetMeasurementsView
)

urlpatterns = [
    path("admin/", admin.site.urls),
    path("", index_view, name="index"),
    path("api/last_value/", get_last_pomiar, name="last_value"),
    path("api/add_user/", add_user, name="add_user"),
    path("api/force_anomaly/", force_anomaly, name="force_anomaly"),
    path("sensor/register/", sensor_register_view, name="sensor_register"),
    path("sensor/measurements/", MeasurementsRegisterView.as_view(), name="measurements_register"),
    path("api/register_user/", RegisterUserView.as_view(), name="api-register-user"),
    path("api/get_measurements/", GetMeasurementsView.as_view(), name="api-get-measurements"),
    path("api/login/", LoginView.as_view(), name="api-login"),
    path("api/logout/", LogoutView.as_view(), name="api-logout"),
    path("api/validate_session/", ValidateSessionView.as_view(), name="api-validate-session"),
    path("api/sensors/", GetSensorsView.as_view(), name="api-sensors"),
    path("api/latest_sensor_measurements/", LatestSensorMeasurementsView.as_view(), name="latest-sensor-measurements"),
    path("api/pending_sensor_requests/", PendingSensorRequestsView.as_view(), name="pending-sensor-requests"),
    path(
        "api/change_sensor_request_status/",
        ChangeSensorRequestStatusView.as_view(),
        name="change-sensor-request-status",
    ),
    path("api/all_sensors/", GetAllSensorsView.as_view(), name="api-all-sensors"),
    path("api/change_sensor_name/", ChangeSensorNameView.as_view(), name="api-change-sensor-name"),
    path("api/delete_sensor/", DeleteSensorView.as_view(), name="api-delete-sensor"),
    path("api/submit_permission_request/", SubmitPermissionRequestView.as_view(), name="submit-permission-request"),
    path(
        "api/pending_permission_requests/", PendingPermissionRequestsView.as_view(), name="pending-permission-requests"
    ),
    path("api/get_username/", GetUsernameView.as_view(), name="get-username"),
    path("api/change_prequest/", ChangePermissionRequestStatusView.as_view(), name="change-prequest"),
]
