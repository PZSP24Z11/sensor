from django.test import TestCase, Client
from django.urls import reverse
from sensors.models import Sensor, Pomiar


class MeasurementsRegisterViewTests(TestCase):
    def setUp(self):
        self.client = Client()
        self.sensor_measurements_url = reverse('measurements_register')
        self.sensor_register_url = reverse('sensor_register')
        self.sensor_request = "SENSORREQ%00:1A:2B:3C:4D:5E%TH"
        self.measurements_request = "00:1A:2B:3C:4D:5E%T1234%H1200"
        self.invalid_data = "INVALIDDATA"
        self.type_map = {"T": "Temperature", "B": "Humidity"}

    def test_register_measurement(self):
        self.client.post(self.sensor_register_url, data=self.sensor_request, content_type="text/plain")
        self.assertEqual(Sensor.objects.count(), 1)
        measurement_response = self.client.post(self.sensor_measurements_url, data=self.measurements_request, content_type="text/plain")
        self.assertEqual(measurement_response.content.decode(), "2")
        self.assertEqual(Pomiar.objects.count(), 2)

    def test_register_measurement_invalid(self):
        self.client.post(self.sensor_register_url, data=self.sensor_request, content_type="text/plain")
        measurement_response = self.client.post(self.sensor_measurements_url, data=self.invalid_data, content_type="text/plain")
        self.assertEqual(measurement_response.content.decode(), "3")
        self.assertEqual(Pomiar.objects.count(), 0)

    def test_register_measurement_nosensor(self):
        measurement_response = self.client.post(self.sensor_measurements_url, data=self.measurements_request, content_type="text/plain")
        self.assertEqual(measurement_response.content.decode(), "3")
        self.assertEqual(Pomiar.objects.count(), 0)
