# Generated by Django 5.1.4 on 2025-01-11 21:24

from django.db import migrations, models


class Migration(migrations.Migration):

    dependencies = [
        ("auth", "0012_alter_user_first_name_max_length"),
        ("sensors", "0003_request"),
    ]

    operations = [
        migrations.RenameField(
            model_name="uzytkownik",
            old_name="powiadomienie_email",
            new_name="email_notification",
        ),
        migrations.RemoveField(
            model_name="uzytkownik",
            name="haslo",
        ),
        migrations.RemoveField(
            model_name="uzytkownik",
            name="nazwa_uzytkownika",
        ),
        migrations.AddField(
            model_name="uzytkownik",
            name="groups",
            field=models.ManyToManyField(
                blank=True,
                help_text="The groups this user belongs to. A user will get all permissions granted to each of their groups.",
                related_name="user_set",
                related_query_name="user",
                to="auth.group",
                verbose_name="groups",
            ),
        ),
        migrations.AddField(
            model_name="uzytkownik",
            name="is_active",
            field=models.BooleanField(default=True),
        ),
        migrations.AddField(
            model_name="uzytkownik",
            name="is_admin",
            field=models.BooleanField(default=False),
        ),
        migrations.AddField(
            model_name="uzytkownik",
            name="is_superuser",
            field=models.BooleanField(
                default=False,
                help_text="Designates that this user has all permissions without explicitly assigning them.",
                verbose_name="superuser status",
            ),
        ),
        migrations.AddField(
            model_name="uzytkownik",
            name="last_login",
            field=models.DateTimeField(
                blank=True, null=True, verbose_name="last login"
            ),
        ),
        migrations.AddField(
            model_name="uzytkownik",
            name="password",
            field=models.CharField(
                default="haslo", max_length=128, verbose_name="password"
            ),
            preserve_default=False,
        ),
        migrations.AddField(
            model_name="uzytkownik",
            name="user_permissions",
            field=models.ManyToManyField(
                blank=True,
                help_text="Specific permissions for this user.",
                related_name="user_set",
                related_query_name="user",
                to="auth.permission",
                verbose_name="user permissions",
            ),
        ),
        migrations.AddField(
            model_name="uzytkownik",
            name="username",
            field=models.CharField(default="nickname", max_length=150, unique=True),
            preserve_default=False,
        ),
        migrations.AlterField(
            model_name="uzytkownik",
            name="email",
            field=models.EmailField(max_length=254, unique=True),
        ),
    ]
