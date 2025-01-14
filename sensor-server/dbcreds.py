MYSQL_USERNAME = "django_user"
MYSQL_PASSWORD = "djangopassword"


def get_credentials() -> tuple[str, str]:
    return MYSQL_USERNAME, MYSQL_PASSWORD
