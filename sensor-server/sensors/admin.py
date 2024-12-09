from django.contrib import admin
from .models import Uzytkownik, Sensor, TypPomiaru, UzytkownikSensor, SensorTypPomiaru, Pomiar


# Register your models here.

admin.site.register(Uzytkownik)
admin.site.register(Sensor)
admin.site.register(TypPomiaru)
admin.site.register(UzytkownikSensor)
admin.site.register(SensorTypPomiaru)
admin.site.register(Pomiar)
