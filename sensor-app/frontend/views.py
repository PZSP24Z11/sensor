import requests
from django.shortcuts import render, redirect

API_URL = "http://127.0.0.1:8080/api/"


def register_view(request):
    if request.method == "POST":
        username = request.POST["username"]
        email = request.POST["email"]
        password = request.POST["password"]
        response = requests.post(
            f"{API_URL}register/", json={"username": username, "email": email, "password": password}
        )
        if response.status_code == 201:
            return redirect("login")
        return render(request, "frontend/register.html", {"error": response.json().get("error", "Registration failed")})
    return render(request, "frontend/register.html")


def login_view(request):
    if request.method == "POST":
        username = request.POST["username"]
        password = request.POST["password"]
        response = requests.post(f"{API_URL}login/", json={"username": username, "password": password})
        if response.status_code == 200:
            return redirect("dashboard")
        return render(request, "frontend/login.html", {"error": response.json().get("error", "Invalid credentials")})
    return render(request, "frontend/login.html")


def dashboard_view(request):
    return render(request, "frontend/dashboard.html")


def index(request):
    return render(request, "frontend/index.html")
