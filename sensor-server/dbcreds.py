MYSQL_USERNAME = "djangouser"
MYSQL_PASSWORD = "1234"


def get_credentials() -> tuple[str, str]:
    return MYSQL_USERNAME, MYSQL_PASSWORD
