echo "This script should only be used during development and testing"

openssl ecparam -name prime256v1 -genkey -noout -out ca.key

openssl req -x509 -new -nodes -key ca.key -sha256 -days 3650 -out ca.crt.pem

openssl ecparam -name prime256v1 -genkey -noout -out server.prv.pem

openssl req -new -key server.prv.pem -out server.csr

openssl x509 -req -in server.csr -CA ca.crt.pem -CAkey ca.key -CAcreateserial -out server.crt.pem -days 365 -sha256

echo "Verifying certificate..."
openssl verify -CAfile ca.crt.pem server.crt.pem

openssl x509 -in ca.crt.pem -outform der -out ca.der

openssl x509 -in server.crt.pem -outform der -out server.der

xxd -i ca.der > ./../nucleo-f439zi-sensor/cert.c
