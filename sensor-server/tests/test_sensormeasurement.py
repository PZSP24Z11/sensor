from django.test import TestCase, Client
from django.urls import reverse
from sensors.models import Sensor, TypPomiaru, SensorTypPomiaru


class MeasurementsRegisterViewTests(TestCase):
    def setUp(self):
        self.client = Client()
        self.sensor_register_url = reverse('sensor_measurements')
        self.sensor_request = "SENSORREQ%00:1A:2B:3C:4D:5E%T"
        self.invalid_type = "SENSORREQ%00:1A:2B:3C:4D:5E%X"
        self.invalid_data = "INVALIDDATA"
        self.type_map = {"T": "Temperature", "B": "Humidity"}

    def test_sensor_register_existing_sensor(self):
        Sensor.objects.create(nazwa_sensora="TestSensor", adres_MAC="00:1A:2B:3C:4D:5E")
        response = self.client.post(self.sensor_register_url, data=self.valid_data, content_type="text/plain")
        self.assertEqual(response.content.decode(), "1")
        self.assertEqual(Sensor.objects.count(), 1)

    def test_sensor_register_new_sensor(self):
        response = self.client.post(self.sensor_register_url, data=self.valid_data, content_type="text/plain")
        self.assertEqual(response.content.decode(), "2")
        self.assertEqual(Sensor.objects.count(), 1)

    def test_sensor_register_invalid_data(self):
        response = self.client.post(self.sensor_register_url, data=self.invalid_data, content_type="text/plain")
        self.assertEqual(response.content.decode(), "3")

    def test_sensor_register_creates_measurement_types(self):
        self.client.post(self.sensor_register_url, data=self.valid_data, content_type="text/plain")
        self.assertEqual(TypPomiaru.objects.filter(nazwa_pomiaru="Temperature").count(), 1)

    def test_sensor_register_creates_sensor_measurement_type(self):
        self.client.post(self.sensor_register_url, data=self.valid_data, content_type="text/plain")
        sensor = Sensor.objects.get(adres_MAC="00:1A:2B:3C:4D:5E")
        self.assertEqual(SensorTypPomiaru.objects.filter(sensor=sensor).count(), 1)

