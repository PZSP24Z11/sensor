#!/bin/sh

echo "> Preparing database..."
sudo mysql -u root < user.sql

echo "> Preparing environment..."
pipx install poetry
poetry install

echo "> Migrating..."
poetry run python3 manage.py makemigrations
poetry run python3 manage.py migrate

echo "> Creating superuser..."
poetry run python3 manage.py createsuperuser

chmod +x manage.py

echo "> Setup finished!"
echo "> Run the server with ./run.sh"
