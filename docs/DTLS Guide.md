# Instrukcja do konfiguracji klienta i serwera DTLS na maszynie lokalnej

## Certyfikaty

Pierwszym krokiem potrzebnym do stworzenia lokalnego serwera lub klienta jest generacja certyfikatu CA:

```sh
# Klucz prywatny CA
openssl ecparam -name prime256v1 -genkey -noout -out ca.key

# Certyfikat CA
openssl req -x509 -new -nodes -key ca.key -sha256 -days 3650 -out ca.crt
```

Następnie generujemy klucz prywatny serwera i CSR (Certificate Signing Request)

```sh
openssl ecparam -name prime256v1 -genkey -noout -out server.key
openssl req -new -key server.key -out server.csr
```

Wreszcie możemy wygenerować certyfikat serwera

```sh
openssl x509 -req -in server.csr -CA ca.crt -CAkey ca.key -CAcreateserial -out server.crt -days 365 -sha256
```

Ten certyfikat będzie wysyłany do klienta, posiadającego certyfikat CA, w celu weryfikacij serwera.

## Serwer DTLS

Posiadając ważne certyfikaty, możemy uruchomić serwer DTLS, używając polecenia `openssl s_server`

```sh
openssl s_server -dtls1_2 \
  -key server.key \
  -cert server.crt \
  -accept <adres-serwera>:<port> -msg
```

Sensor operować będzie na adresie IPv6, zatem dodatkowo, po adresie IP serwera, musimy podać interfejs sieciowy:

```
[<adres-ip>%<interfejs]:<port>
```

## Klient DTLS

Klient DTLS może zostać uruchomiony za pomocą polecenia:

```sh
openssl s_client -dtls1_2 \
  -CAfile ca.crt \
  -connect \<adres-serwera>:<port> \
  -msg
```

Tak jak w przypadku serwera DTLS, sensor operować będzie na adresach IPv6, zatem należy odpowiednio sformatować adres serwera (jak wyżej)

## RIOT OS na płytce `native`

Możemy również użyć płytki `native`, aby przetestować działanie klienta DTLS napisanego dla sensora. Płytka native porozumiewać się będzie na interfejsie `tap`, zatem należy ten interfejs utworzyć i uruchomić

```sh
sudo ip tuntap add tap0 mode tap
sudo ip link set tap0 up
```

Teraz możemy skompilować kod klienta DTLS:

```sh
# W katalogu z klientem DTLS RIOT OS
make BOARD=native clean all
```

Następnie możemy otworzyć powłokę RIOT OS

```
make term BOARD=native
```

Za pomocą polecenia `dtlsc <adres>` możemy teraz połączyć się z serwerem
