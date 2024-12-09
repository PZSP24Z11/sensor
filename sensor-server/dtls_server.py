import json
import socket
import mbedtls
from mbedtls.tls import DTLSConfiguration




dtls_config_file = "./config/dtls_config.json"


with open(dtls_config_file, "r") as fh:
    config = json.load(fh)

ip = config["server_ipv6"]
port = config["server_port"]
iface = config["server_iface"]
cert_filepath = config["cert_filepath"]
key_filepath = config["key_filepath"]

cert = Certificate()

conf = DTLSConfiguration(certificate_chain=)
