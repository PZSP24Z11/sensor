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


def get_all_sensors(session_id) -> Optional[list[Sensor]]:
    try:
        response = requests.get(f"{API_URL}all_sensors/", cookies={"session_id": session_id})
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
        email_notification = request.POST.get("email_notification", "off") == "on"
        try:
            response = requests.post(
                f"{API_URL}register_user/",
                json={
                    "username": username,
                    "email": email,
                    "password": password,
                    "email_notification": email_notification,
                },
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


class ArchiveView(View):
    def get(self, request: HttpRequest) -> HttpResponse:
        session_id = request.COOKIES.get("session_id")
        if not session_id:
            messages.error(request, "User must login to access this resource")
            return redirect("login")

        sensors = get_sensor_list(session_id)

        response = requests.get(f"{API_URL}get_username/", cookies={"session_id": session_id})
        username = response.json()["username"]
        return render(request, "frontend/archive.html", {"sensors": sensors["sensor_list"], "username": username})


class GetMeasurementsView(View):
    def get(self, request: HttpRequest) -> JsonResponse:
        session_id = request.COOKIES.get("session_id")
        if not session_id:
            messages.error(request, "User must login to access this resource")
            return JsonResponse(status=500, data={"error": "you must log in to access this resource"})
        try:
            params = request.GET.dict()
            response = requests.get(f"{API_URL}get_measurements/", cookies={"session_id": session_id}, params=params)
        except Exception as e:
            print(e)

        return JsonResponse(status=200, data=response.json())


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
                next_page = "admin" if is_admin else "user"
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
            messages.error(request, "User must login to access this resource")
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

        return redirect("measurements")


class MeasurementsView(View):
    content = "frontend/measurements.html"

    def get(self, request: HttpRequest) -> HttpResponse:
        session_id = request.COOKIES.get("session_id")
        if not session_id:
            messages.error(request, "User must login to access this resource")
            return redirect("login")

        sensors = get_sensor_list(session_id)
        return render(request, self.content, {"sensors": sensors["sensor_list"]})


class UserMeasurementsView(View):
    content = "frontend/user_measurements.html"

    def get(self, request: HttpRequest) -> HttpResponse:
        session_id = request.COOKIES.get("session_id")
        if not session_id:
            messages.error(request, "User must login to access this resource")
            return redirect("login")

        response = requests.get(f"{API_URL}get_username/", cookies={"session_id": session_id})
        username = response.json()["username"]

        sensors = get_sensor_list(session_id)
        return render(request, self.content, {"username": username, "sensors": sensors["sensor_list"]})


class SensorsView(View):
    content = "frontend/sensors.html"

    def get(self, request: HttpRequest) -> HttpResponse:
        session_id = request.COOKIES.get("session_id")
        if not session_id:
            messages.error(request, "User must login to access this resource")
            return redirect("login")

        sensors = get_all_sensors(session_id)
        return render(request, self.content, {"sensors": sensors["sensors"]})


class ChangeSensorNameView(View):
    def post(self, request: HttpRequest) -> JsonResponse:
        session_id = request.COOKIES.get("session_id")
        if not session_id:
            messages.error(request, "User must login to access this resource")
            return redirect("login")
        try:
            data = json.loads(request.body)
            response = requests.post(f"{API_URL}change_sensor_name/", cookies={"session_id": session_id}, json=data)
            if response.status_code >= 400:
                print(response.json()["message"])
            return JsonResponse(status=response.status_code, data=response.json())
        except Exception as e:
            print(e)
            return redirect("login")


class DeleteSensorView(View):
    def post(self, request: HttpRequest) -> JsonResponse:
        session_id = request.COOKIES.get("session_id")
        if not session_id:
            messages.error(request, "User must login to access this resource")
            return redirect("login")
        try:
            data = json.loads(request.body)
            response = requests.post(f"{API_URL}delete_sensor/", cookies={"session_id": session_id}, json=data)
            if response.status_code >= 400:
                print(response.json()["message"])
            return JsonResponse(status=response.status_code, data=response.json())
        except Exception as e:
            print(e)
            return redirect("login")


class SensorRequestsView(View):
    content = "frontend/sensor_requests.html"

    def get(self, request: HttpRequest) -> HttpResponse:
        session_id = request.COOKIES.get("session_id")
        if not session_id:
            messages.error(request, "User must login to access this resource")
            return redirect("login")
        try:
            response = requests.get(f"{API_URL}pending_sensor_requests/", cookies={"session_id": session_id})
        except Exception as e:
            print(e)
            return redirect("login")

        return render(request, self.content, {"sensor_requests": response.json()["sensor_requests"]})

    def post(self, request: HttpRequest) -> HttpResponse:
        session_id = request.COOKIES.get("session_id")
        if not session_id:
            messages.error(request, "User must login to access this resource")
            return redirect("login")
        try:
            data = json.loads(request.body)
            response = requests.post(
                f"{API_URL}change_sensor_request_status/", json=data, cookies={"session_id": session_id}
            )

            return JsonResponse(status=response.status_code, data=response.json())

        except Exception as e:
            print(e)
            return redirect("login")


class AdminPermissionRequestsView(View):
    content = "frontend/permission_requests.html"

    def get(self, request: HttpRequest) -> HttpResponse:
        session_id = request.COOKIES.get("session_id")
        if not session_id:
            messages.error(request, "User must login to access this resource")
            return redirect("login")
        try:
            response = requests.get(f"{API_URL}pending_permission_requests/", cookies={"session_id": session_id})
        except Exception as e:
            print(e)
            return redirect("login")

        return render(request, self.content, {"requests": response.json()["p_requests"]})

    def post(self, request: HttpRequest) -> HttpResponse:
        session_id = request.COOKIES.get("session_id")
        if not session_id:
            messages.error(request, "User must login to access this resource")
            return redirect("login")
        try:
            data = json.loads(request.body)
            response = requests.post(f"{API_URL}change_prequest/", json=data, cookies={"session_id": session_id})
            print(response.json())
        except Exception as e:
            print(e)
            return JsonResponse(status=500, data={"message": "Internal Server Error"})

        return JsonResponse(status=response.status_code, data=response.json())


class LogoutView(View):
    def get(self, request: HttpRequest) -> HttpResponse:
        session_id = request.COOKIES.get("session_id")
        if not session_id:
            messages.error(request, "Error: no user_id")
            return redirect("login")
        try:
            response = requests.get(f"{API_URL}logout/", cookies={"session_id": session_id})
            if response.status_code == 200:
                response = redirect("login")
                response.delete_cookie("session_id")
                return response
        except Exception as e:
            print(e)
        return JsonResponse(status=500, data={"message": "Log out didnt succeed"})


class UserView(View):
    content = "frontend/user.html"

    def get(self, request: HttpRequest) -> HttpResponse:
        session_id = request.COOKIES.get("session_id")
        if not session_id:
            messages.error(request, "User must login to access this resource")
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

        return redirect("user-measurements")


class UserSensorsView(View):
    content = "frontend/user_sensors.html"

    def get(self, request: HttpRequest) -> HttpResponse:
        session_id = request.COOKIES.get("session_id")
        if not session_id:
            messages.error(request, "User must login to access this resource")
            return redirect("login")

        try:
            response = requests.get(f"{API_URL}all_sensors/", cookies={"session_id": session_id})
            data = response.json()

            response = requests.get(f"{API_URL}get_username/", cookies={"session_id": session_id})
            username = response.json()["username"]
            return render(request, self.content, {"username": username, "sensors": data["sensors"]})
        except Exception as e:
            print(e)
            return redirect("login")

    def post(self, request: HttpRequest) -> HttpResponse:
        session_id = request.COOKIES.get("session_id")
        if not session_id:
            messages.error(request, "User must login to access this resource")
            return redirect("login")

        try:
            data = json.loads(request.body)
            response = requests.post(
                f"{API_URL}submit_permission_request/", json=data, cookies={"session_id": session_id}
            )

            return JsonResponse(status=response.status_code, data=response.json())

        except Exception as e:
            print(e)
            return redirect("login")


class UserPermissionRequestsView(View):
    content = "frontend/user_requests.html"

    def get(self, request: HttpRequest) -> HttpResponse:
        session_id = request.COOKIES.get("session_id")
        if not session_id:
            messages.error(request, "User must login to access this resource")
            return redirect("login")

        try:
            response = requests.get(f"{API_URL}pending_permission_requests/", cookies={"session_id": session_id})
            data = response.json()

            response = requests.get(f"{API_URL}get_username/", cookies={"session_id": session_id})
            username = response.json()["username"]
            return render(request, self.content, {"username": username, "requests": data["p_requests"]})
        except Exception as e:
            print(e)
            return redirect("login")


class LatestSensorMeasurementsView(View):
    def post(self, request: HttpRequest) -> HttpResponse:
        session_id = request.COOKIES.get("session_id")
        if not session_id:
            messages.error(request, "User must login to access this resource")
            return redirect("login")
        try:
            data = json.loads(request.body)
            sensor_id = data.get("id")
            time_range = data.get("range")

            response = requests.post(
                f"{API_URL}latest_sensor_measurements/",
                json={"sensor_id": sensor_id, "range": time_range},
                cookies={"session_id": session_id},
            )

            response_data = response.json()
            if response.status_code == 401:
                messages.error(request, "Server responded with unauthorized user, user logged out")
                return redirect("login")
            else:
                return JsonResponse(status=response.status_code, data=response_data)
        except Exception as e:
            print(e)
            return JsonResponse(status=500, data={"message": "Internal error"})


class PendingSensorRequests(View):
    def get(self, request: HttpRequest) -> HttpResponse:
        session_id = request
        if not session_id:
            messages.error(request, "No session, please log in")
            return redirect("login")

        try:
            response = requests.get(f"{API_URL}pending_sensor_requests/", cookies={"session_id": session_id})
            response_data = response.json()

            return JsonResponse(status=response.status_code, data=response_data)
        except Exception as e:
            print(e)
            return JsonResponse(status=500, data={"message": "Internal error"})


def index(request):
    # return render(request, "frontend/index.html")
    return redirect("login")
