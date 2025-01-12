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
from .views import RegisterUserView, LoginView, AdminView, DashboardView, LatestSensorMeasurementeView

urlpatterns = [
    path("", views.index, name="index"),
    path("register/", RegisterUserView.as_view(), name="register"),
    path("login/", LoginView.as_view(), name="login"),
    path("admin/", AdminView.as_view(), name="admin"),
    path("dashboard/", DashboardView.as_view(), name="dashboard"),
    path("latest_sensor_measurements/", LatestSensorMeasurementeView.as_view(), name="latest-sensor-measurements"),
]
