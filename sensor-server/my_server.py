import socket
import ssl
import time
import mbedtls
from contextlib import suppress
from mbedtls.tls import DTLSConfiguration, TLSWrappedSocket, HelloVerifyRequest, ServerContext
from mbedtls.x509 import CRT
from mbedtls._tls import _enable_debug_output, _set_debug_level

mbedtls.cipher.get_supported_ciphers
CERTFILE = "./../cert/server.crt.pem"
ADDRESS = ("127.0.0.1", 8000)
CHUNKSIZE = 512


def make_dtls_connection(sock: TLSWrappedSocket) -> TLSWrappedSocket:
    conn, addr = sock.accept()
    conn.setcookieparam(addr[0].encode("ascii"))
    with suppress(HelloVerifyRequest):
        conn.do_handshake()

    _, (conn, addr) = conn, conn.accept()
    _.close()
    conn.setcookieparam(addr[0].encode("ascii"))
    conn.do_handshake()
    return conn


def echo_handler(conn: TLSWrappedSocket, *, packet_size: int = 4069) -> None:
    while True:
        data = conn.recv(packet_size)
        if data:
            break
        # Avoid tight loop.
        time.sleep(0.01)
    sent = 0
    view = memoryview(data)
    while sent != len(data):
        sent += conn.send(view[sent:])


ca_cert = CRT.from_file("./../cert/ca.crt.pem")
srv_cert = CRT.from_file(CERTFILE)

pk = mbedtls.tls.PrivateKey.from_file("./../cert/server.prv.pem", "root")

conf = DTLSConfiguration(
    validate_certificates=True,
    certificate_chain=((srv_cert), pk),
    ciphers=(
        "TLS-ECDHE-ECDSA-WITH-AES-128-GCM-SHA256",
        "TLS-ECDHE-ECDSA-WITH-AES-256-GCM-SHA384",
        "TLS-ECDHE-RSA-WITH-AES-128-GCM-SHA256",
        "TLS-ECDHE-RSA-WITH-AES-256-GCM-SHA384",
    ),
    lowest_supported_version=mbedtls.tls.DTLSVersion.DTLSv1_2,
    highest_supported_version=mbedtls.tls.DTLSVersion.DTLSv1_2,
)

sock = ServerContext(conf).wrap_socket(socket.socket(socket.AF_INET, socket.SOCK_DGRAM))
_enable_debug_output(sock.context)
_set_debug_level(1)
sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
sock.bind(ADDRESS)

with make_dtls_connection(sock) as conn:
    echo_handler(conn)

sock.close()
