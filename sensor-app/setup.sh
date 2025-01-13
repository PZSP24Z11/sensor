#!/bin/sh

echo "> Preparing environment..."
pipx install poetry
poetry install

chmod +x manage.py

echo "> Setup finished!"
echo "> Run the server with ./run.sh"
