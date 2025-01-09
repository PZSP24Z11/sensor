from django.test import TestCase
from sensor.models import Uzytkownik


class UserTestCase(TestCase):
    def setUp(self) -> None:
        Uzytkownik.objects.create(
                nazwa_uzytkownika="john",
                haslo="mocked-password-hash",
                email="john@example.com",
                powiadomienia_email=True
        )
