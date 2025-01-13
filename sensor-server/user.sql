CREATE USER 'django_user'@'localhost' IDENTIFIED BY 'djangopassword';
CREATE DATABASE sensor_database CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci;
GRANT ALL PRIVILEGES ON sensor_database.* TO 'django_user'@'localhost';
FLUSH PRIVILEGES;
