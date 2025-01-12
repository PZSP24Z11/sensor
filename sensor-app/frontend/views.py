import requests
from django.http import JsonResponse, HttpResponse, HttpRequest
from django.views import View
from django.contrib import messages
from django.shortcuts import render, redirect
from django.utils.decorators import method_decorator
from django.middleware.csrf import get_token
from django.views.decorators.csrf import csrf_exempt, csrf_protect

API_URL = "http://127.0.0.1:8080/api/"


@method_decorator(csrf_exempt, name="dispatch")
class RegisterUserView(View):
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


def login_view(request):
    # print(get_token(request))
    # if request.method == "POST":
    #     username = request.POST["username"]
    #     password = request.POST["password"]
    #     response = requests.post(f"{API_URL}login/", json={"username": username, "password": password})
    #     if response.status_code == 200:
    #         return redirect("dashboard")
    #     return render(request, "frontend/login.html", {"error": response.json().get("error", "Invalid credentials")})
    # return render(request, "frontend/login.html")
    if request.method == "POST":
        csrf_token = get_token(request)  # Generate CSRF token
        print(csrf_token)
        username = request.POST["username"]
        password = request.POST["password"]

        # Send CSRF token in the headers for validation
        headers = {"Content-Type": "application/json", "X-CSRFToken": csrf_token}
        # headers = {"Content-Type": "application/json"}

        response = requests.post(f"{API_URL}login/", json={"username": username, "password": password}, headers=headers)

        if response.status_code == 200:
            return redirect("dashboard")

        print("jestem tu")
        return render(request, "frontend/login.html", {"error": response.status_code})

    return render(request, "frontend/login.html")


@csrf_protect
def dashboard_view(request):
    print(get_token(request))
    return render(request, "frontend/dashboard.html")


def index(request):
    print(get_token(request))
    return render(request, "frontend/index.html")
