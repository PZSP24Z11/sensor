from django.apps import AppConfig


class SensorsConfig(AppConfig):
    default_auto_field = "django.db.models.BigAutoField"
    name = "sensors"

    def ready(self) -> None:
        from .models import TypPomiaru

        default_types = ["Temperature", "Humidity"]

        for typ in default_types:
            TypPomiaru.objects.get_or_create(nazwa_pomiaru=typ)
