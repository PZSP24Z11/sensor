# Generated by Django 5.1.4 on 2025-01-12 02:02

from django.db import migrations


class Migration(migrations.Migration):

    dependencies = [
        ("sensors", "0006_alter_uzytkownik_options_uzytkownik_date_joined_and_more"),
    ]

    operations = [
        migrations.RemoveField(
            model_name="uzytkownik",
            name="first_name",
        ),
        migrations.RemoveField(
            model_name="uzytkownik",
            name="last_name",
        ),
    ]
