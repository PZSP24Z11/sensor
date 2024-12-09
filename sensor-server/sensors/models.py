from django.db import models


class Uzytkownik(models.Model):
    nazwa_uzytkownika = models.CharField(max_length=255)
    haslo = models.CharField(max_length=255)
    email = models.EmailField(max_length=255)
    powiadomienie_email = models.BooleanField(default=False)


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
