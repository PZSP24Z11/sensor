from django.contrib import admin
from django.contrib.auth.admin import UserAdmin
from .models import Uzytkownik, Sensor, TypPomiaru, UzytkownikSensor, SensorTypPomiaru, Pomiar, SensorRequest


class CustomUserAdmin(UserAdmin):
    model = Uzytkownik
    list_display = ("username", "email", "is_active", "is_superuser")
    list_filter = ("is_active", "is_superuser")
    fieldsets = (
        (None, {"fields": ("username", "email", "password")}),
        ("Permissions", {"fields": ("is_superuser",)}),
        ("Status", {"fields": ("is_active",)}),
    )
    add_fieldsets = (
        (
            None,
            {
                "classes": ("wide",),
                "fields": (
                    "username",
                    "email",
                    "password1",
                    "password2",
                    "is_staff",
                    "is_active",
                    "groups",
                    "is_superuser",
                ),
            },
        ),
    )
    search_fields = ("username", "email")
    ordering = ("username",)


admin.site.register(Uzytkownik, CustomUserAdmin)
admin.site.register(Sensor)
admin.site.register(TypPomiaru)
admin.site.register(UzytkownikSensor)
admin.site.register(SensorTypPomiaru)
admin.site.register(Pomiar)
admin.site.register(SensorRequest)
