# Generated by Django 5.1.4 on 2025-01-12 02:50

from django.db import migrations


class Migration(migrations.Migration):

    dependencies = [
        ("sensors", "0007_remove_uzytkownik_first_name_and_more"),
    ]

    operations = [
        migrations.AlterModelOptions(
            name="uzytkownik",
            options={},
        ),
        migrations.RemoveField(
            model_name="uzytkownik",
            name="date_joined",
        ),
    ]
