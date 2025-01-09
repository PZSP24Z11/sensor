from django.test import TestCase, Client
from django.urls import reverse
from django.contrib.auth.hashers import check_password
from sensors.models import Uzytkownik


class RegisterViewTests(TestCase):
    def setUp(self):
        self.client = Client()
        self.register_url = reverse('register')
        self.login_url = reverse('login')
        self.user_data = {
            'username': 'testuser',
            'email': 'test@example.com',
            'password': 'password123',
            'confirmPassword': 'password123'
        }

    def test_register_view_get(self):
        response = self.client.get(self.register_url)
        self.assertEqual(response.status_code, 200)
        self.assertTemplateUsed(response, 'frontend/register.html')

    def test_register_successful(self):
        response = self.client.post(self.register_url, self.user_data)
        self.assertRedirects(response, self.login_url)
        self.assertTrue(Uzytkownik.objects.filter(email='test@example.com').exists())

    def test_register_password_mismatch(self):
        self.user_data['confirmPassword'] = 'wrongpassword'
        response = self.client.post(self.register_url, self.user_data)
        self.assertRedirects(response, self.register_url)
        self.assertFalse(Uzytkownik.objects.filter(email='test@example.com').exists())

    def test_register_duplicate_email(self):
        Uzytkownik.objects.create(nazwa_uzytkownika='existinguser', email='test@example.com', haslo='mockedhashedpassword')
        response = self.client.post(self.register_url, self.user_data)
        self.assertRedirects(response, self.register_url)

    def test_register_duplicate_username(self):
        Uzytkownik.objects.create(nazwa_uzytkownika='testuser', email='unique@example.com', haslo='mockedhashedpassword')
        response = self.client.post(self.register_url, self.user_data)
        self.assertRedirects(response, self.register_url)

    def test_password_hashed_on_save(self):
        self.client.post(self.register_url, self.user_data)
        user = Uzytkownik.objects.get(email='test@example.com')
        self.assertTrue(check_password('password123', user.haslo))

