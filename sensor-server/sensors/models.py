from __future__ import annotations
from django.db import models
from django.contrib.auth.models import AbstractUser, AbstractBaseUser, BaseUserManager, PermissionsMixin, User


class UzytkownikManager(BaseUserManager):
    """Manager for Uzytkownik model. Handles Uzytkownik and Uzytkownik admin creation"""

    def create_user(self, username: str, password: str = None, **extra_fields: None) -> Uzytkownik:
        if not username:
            raise ValueError("Nazwa użytkownika jest wymagana")
        extra_fields.setdefault("email_notification", False)
        extra_fields.setdefault("is_superuser", False)
        user = self.model(username=username, **extra_fields)
        user.set_password(password)
        user.save(using=self._db)
        return user

    def create_superuser(self, username: str, password: str = None, **extra_fields: None) -> Uzytkownik:
        extra_fields.setdefault("is_superuser", True)
        return self.create_user(username, password, **extra_fields)


class Uzytkownik(AbstractBaseUser, PermissionsMixin):
    username = models.CharField(max_length=150, unique=True)
    email = models.EmailField(unique=True, null=False, blank=False)
    is_active = models.BooleanField(default=True)
    is_superuser = models.BooleanField(default=False)
    email_notification = models.BooleanField(default=False)

    USERNAME_FIELD = "username"
    REQUIRED_FIELDS = ["email"]

    objects = UzytkownikManager()

    def __str__(self) -> str:
        return self.username

    @property
    def is_staff(self) -> bool:
        return self.is_superuser


class Sensor(models.Model):
    nazwa_sensora = models.CharField(max_length=255)
    adres_MAC = models.CharField(max_length=100)


class TypPomiaru(models.Model):
    nazwa_pomiaru = models.CharField(max_length=100)
    jednostka = models.CharField(max_length=50)


class UzytkownikSensor(models.Model):
    uzytkownik = models.ForeignKey(Uzytkownik, on_delete=models.CASCADE)
    sensor = models.ForeignKey(Sensor, on_delete=models.CASCADE)


class SensorTypPomiaru(models.Model):
    sensor = models.ForeignKey(Sensor, on_delete=models.CASCADE)
    typ_pomiaru = models.ForeignKey(TypPomiaru, on_delete=models.CASCADE)


class Pomiar(models.Model):
    sensor = models.ForeignKey(Sensor, on_delete=models.CASCADE)
    typ_pomiaru = models.ForeignKey(TypPomiaru, on_delete=models.CASCADE)
    wartosc_pomiaru = models.FloatField()
    data_pomiaru = models.DateTimeField()


class SensorRequest(models.Model):
    nazwa_sensora = models.CharField(max_length=255)
    adres_MAC = models.CharField(max_length=100, unique=True)
    status = models.CharField(
        max_length=20,
        choices=[("Pending", "Pending"), ("Approved", "Approved"), ("Rejected", "Rejected")],
        default="Pending",
    )
    request_date = models.DateTimeField(auto_now_add=True)

    def __str__(self) -> str:
        return f"Request for {self.nazwa_sensora} ({self.adres_MAC}) - {self.status}"


class SensorPermissionRequest(models.Model):
    uzytkownik = models.ForeignKey(Uzytkownik, on_delete=models.CASCADE)
    sensor = models.ForeignKey(Sensor, on_delete=models.CASCADE)
    status = models.CharField(
        max_length=20,
        choices=[("Pending", "Pending"), ("Approved", "Approved"), ("Rejected", "Rejected")],
        default="Pending",
    )
    request_date = models.DateTimeField(auto_now_add=True)
