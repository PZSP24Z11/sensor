import smtplib
import ssl
from sensors.models import Sensor, Uzytkownik, Pomiar
from django.db.models import Max
from mailcreds import SMTP_LOGIN, SMTP_PASSWD, MAIL_SENDER

from email.message import EmailMessage


def _set_message_contents(sensor: Sensor) -> EmailMessage:
    msg = EmailMessage()
    value_string = get_latest_pomiary_dict_by_mac(sensor)
    message = f"""The sensor {sensor.nazwa_sensora} has received an anomaly alert!
    Current value is: \n\t{value_string}
    """
    msg.set_content(message)
    return msg


def get_latest_pomiary_dict_by_mac(sensor: Sensor) -> str:
    try:
        
        # Retrieve the latest measurement date for each measurement type
        latest_pomiary = (
            Pomiar.objects.filter(sensor=sensor)
            .values('typ_pomiaru')
            .annotate(date=Max('data_pomiaru'))
        )

        # Build the dictionary with measurement type names as keys and values as values
        latest_pomiary_dict = {
            Pomiar.objects.get(sensor=sensor, typ_pomiaru=item['typ_pomiaru'], data_pomiaru=item['date']).typ_pomiaru.nazwa_pomiaru:
            Pomiar.objects.get(sensor=sensor, typ_pomiaru=item['typ_pomiaru'], data_pomiaru=item['date']).wartosc_pomiaru
            for item in latest_pomiary
        }
        
        msg = ''
        for item_dict in latest_pomiary_dict:
            msg += f"\n{item_dict}: {(float(latest_pomiary_dict[item_dict]) / 100):.2f}"
        return msg
    except Exception as e:
        return str(e)


def send_anomaly_mail(sensor: Sensor, users: Uzytkownik.objects) -> bool:
    try:
        print("Sending anomaly email!")
        user_names = [user.email for user in users]
        recepients = ', '.join(user_names)
        msg = _set_message_contents(sensor)
        msg['Subject'] = "Anomaly detected in one of your sensors!"
        msg['From'] = MAIL_SENDER
        msg['To'] = recepients

        context = ssl.create_default_context()
        with smtplib.SMTP_SSL('smtp.mailgun.org', 465, context=context) as s:
            s.login(SMTP_LOGIN, SMTP_PASSWD)
            s.send_message(msg)
            print("message sent!")
        return True
    except Exception as e:
        print(e)
        return False
