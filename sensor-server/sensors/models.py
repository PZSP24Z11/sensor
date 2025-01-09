from django.db import models
from django.contrib.auth.hashers import make_password, check_password


class Uzytkownik(models.Model):
    nazwa_uzytkownika = models.CharField(max_length=255, unique=True)
    haslo = models.CharField(max_length=255)
    email = models.EmailField(max_length=255, unique=True)
    powiadomienie_email = models.BooleanField(default=False)

    def set_password(self, raw_password):
        self.haslo = make_password(raw_password)

    def check_password(self, raw_password):
        return check_password(raw_password, self.haslo)


class Sensor(models.Model):
    nazwa_sensora = models.CharField(max_length=255)
    adres_MAC = models.CharField(max_length=100)


class TypPomiaru(models.Model):
    nazwa_pomiaru = models.CharField(max_length=100)


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


class Request(models.Model):
    nazwa_sensora = models.CharField(max_length=255)
    adres_MAC = models.CharField(max_length=100, unique=True)
    status = models.CharField(
        max_length=20,
        choices=[("Pending", "Pending"), ("Approved", "Approved"), ("Rejected", "Rejected")],
        default="Pending",
    )
    request_date = models.DateTimeField(auto_now_add=True)

    def __str__(self):
        return f"Request for {self.nazwa_sensora} ({self.adres_MAC}) - {self.status}"
