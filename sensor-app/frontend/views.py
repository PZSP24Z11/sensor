import requests
import json
from typing import Optional, TypedDict
from django.http import JsonResponse, HttpResponse, HttpRequest
from django.views import View
from django.contrib import messages
from django.shortcuts import render, redirect
from django.utils.decorators import method_decorator
from django.middleware.csrf import get_token
from django.views.decorators.csrf import csrf_exempt, csrf_protect

API_URL = "http://127.0.0.1:8080/api/"


class Sensor(TypedDict):
    id: int
    nazwa_sensora: str
    adres_MAC: str


def get_sensor_list(session_id) -> Optional[list[Sensor]]:
    try:
        response = requests.get(f"{API_URL}sensors/", cookies={"session_id": session_id})
        return response.json()
    except Exception as e:
        print(e)
        return None


class RegisterUserView(View):
    page = "fronend/register.html"

    def post(self, request: HttpRequest) -> HttpResponse:
        username = request.POST["username"]
        email = request.POST["email"]
        password = request.POST["password"]
        try:
            response = requests.post(
                f"{API_URL}register_user/", json={"username": username, "email": email, "password": password}
            )

            if response.status_code < 400:
                messages.success(request, "Registration successful.")
            else:
                messages.error(request, "Registration failed. Please try again.")
        except Exception:
            messages.error(request, "Could not connect to server")

        return render(request, "frontend/register.html")

    def get(self, request: HttpRequest) -> HttpResponse:
        return render(request, "frontend/register.html")


class LoginView(View):
    content = "frontend/login.html"

    def post(self, request: HttpResponse) -> HttpResponse:
        username, password = request.POST["username"], request.POST["password"]

        try:
            response = requests.post(f"{API_URL}login/", json={"username": username, "password": password})

            if response.status_code < 400:
                response_data = response.json()
                session_id = response_data["session_id"]
                is_admin = response_data["is_admin"]
                next_page = "admin" if is_admin else "dashboard"
                if session_id:
                    response = redirect(next_page)
                    # for development
                    response.set_cookie("session_id", session_id, httponly=True, secure=False)
                    return response

        except Exception as e:
            print(e)

        messages.error(request, "Login failed")
        return render(request, self.content)

    def get(self, request: HttpRequest) -> HttpResponse:
        return render(request, self.content)


class AdminView(View):
    content = "frontend/admin.html"

    def get(self, request: HttpRequest) -> HttpResponse:
        session_id = request.COOKIES.get("session_id")
        if not session_id:
            messages.error(request, "User must login before viewing sensors")
            return redirect("login")

        try:
            response = requests.get(f"{API_URL}validate_session/", cookies={"session_id": session_id})
            if response.status_code < 400:
                response_data = response.json()
                if not all((response_data["is_valid"], response_data["is_admin"])):
                    return redirect("login")
            else:
                messages.error(request, "Invalid session, logged out")
                return redirect("login")
        except Exception as e:
            print(e)
            messages.error(request, "Error communicating with API")
            return redirect("login")

        sensors = get_sensor_list(session_id)
        return render(request, self.content, {"sensors": sensors["sensor_list"]})


class DashboardView(View):
    content = "frontend/dashboard.html"

    def get(self, request: HttpRequest) -> HttpResponse:
        session_id = request.COOKIES.get("session_id")
        if not session_id:
            messages.error(request, "User must login before viewing sensors")
            return redirect("login")

        try:
            response = requests.get(f"{API_URL}validate_session/", cookies={"session_id": session_id})

            if response.status_code < 400:
                response_data = response.json()
                if not all((response_data["is_valid"], not response_data["is_admin"])):
                    return redirect("login")
            else:
                return redirect("login")
        except Exception as e:
            print(e)
            messages.error(request, "Error communicating with API")
            return redirect("login")

        return render(request, self.content)


class LatestSensorMeasurementeView(View):
    def post(self, request: HttpRequest) -> HttpResponse:
        session_id = request.COOKIES.get("session_id")
        if not session_id:
            messages.error(request, "User must login before viewing sensors")
            return redirect("login")
        try:
            data = json.loads(request.body)
            sensor_id = data.get("id")
            response = requests.post(
                f"{API_URL}latest_sensor_measurements/",
                json={"sensor_id": sensor_id},
                cookies={"session_id": session_id},
            )

            if response.status_code < 400:
                response_data = response.json()
                return JsonResponse(status=response.status_code, data={"measurements": response_data["measurements"]})
        except Exception as e:
            print(e)
            return JsonResponse(status=500, data={"message": "Internal error"})


def index(request):
    print(get_token(request))
    return render(request, "frontend/index.html")
