echo "This script should only be used during development and testing"

openssl ecparam -name prime256v1 -genkey -noout -out ca.key

openssl req -x509 -new -nodes -key ca.key -sha256 -days 3650 -out ca.crt

openssl ecparam -name prime256v1 -genkey -noout -out server.key

openssl req -new -key server.key -out server.csr

openssl x509 -req -in server.csr -CA ca.crt -CAkey ca.key -CAcreateserial -out server.crt -days 365 -sha256

echo "Verifying certificate..."
openssl verify -CAfile ca.crt server.crt

openssl x509 -in ca.crt -outform der -out ca.der

xxd -i ca.der > cert.c
