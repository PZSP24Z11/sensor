# Generated by Django 5.1.4 on 2025-01-13 02:54

from django.db import migrations, models


class Migration(migrations.Migration):

    dependencies = [
        ("sensors", "0009_typpomiaru_jednostka"),
    ]

    operations = [
        migrations.AddField(
            model_name="sensor",
            name="zatwierdzony",
            field=models.BooleanField(default=False),
        ),
    ]
