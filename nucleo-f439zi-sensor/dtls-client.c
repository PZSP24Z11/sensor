#include <wolfssl/ssl.h>
#include <wolfssl/error-ssl.h>
#include <sock_tls.h>
#include <net/sock.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "net/gnrc/netif.h"
#include "log.h"

#define SERVER_PORT 20220
#define APP_DTLS_BUF_SIZE 64

extern const unsigned char ca_der[];
extern const long ca_der_len;

static sock_tls_t tls_socket;
static sock_tls_t *tls_socket_addr = &tls_socket;

static void usage(const char *cmd_name)
{
	LOG(LOG_ERROR, "Usage: %s <server address>\n", cmd_name);
}

int handle_certs(void) {

    wolfSSL_CTX_set_verify(tls_socket_addr->ctx, SSL_VERIFY_PEER, NULL);
	LOG(LOG_INFO, "Loading CA cert\n");

	if (wolfSSL_CTX_load_verify_buffer(
            tls_socket_addr->ctx, 
            ca_der,
            ca_der_len,
            SSL_FILETYPE_ASN1
        ) != SSL_SUCCESS) {
        LOG(LOG_ERROR, "Error loading CA certificate\n");
        return -1;
    }

	LOG(LOG_INFO, "CA Certificate Loaded\n");

    return 0;
}


int restart_session(void) {
	sock_dtls_session_destroy(tls_socket_addr);
	return sock_dtls_session_create(tls_socket_addr);
}

int dtls_client(int argc, char **argv)
{
	int ret = 0;
	char buf[APP_DTLS_BUF_SIZE] = "Hello from DTLS client!";
	char *iface;
	char *addr_str;
	int connect_timeout = 0;
	const int max_connect_timeouts = 5;

	if (argc != 2) {
		usage(argv[0]);
		return -1;
	}

	addr_str = argv[1];
	sock_udp_ep_t local = SOCK_IPV6_EP_ANY;
	sock_udp_ep_t remote = SOCK_IPV6_EP_ANY;

	iface = ipv6_addr_split_iface(addr_str);
	if (!iface) {
		if (gnrc_netif_numof() == 1) {
			remote.netif = (uint16_t)gnrc_netif_iter(NULL)->pid;
		}
	} else {
		gnrc_netif_t *netif = gnrc_netif_get_by_pid(atoi(iface));
		if (netif == NULL) {
			LOG(LOG_ERROR, "ERROR: Invalid Interface\n");
			usage(argv[0]);
			return -1;
		}
		remote.netif = (uint16_t)netif->pid;
	}
	if (ipv6_addr_from_str((ipv6_addr_t *)remote.addr.ipv6, addr_str) == NULL) {
		LOG(LOG_ERROR, "ERROR: Unable to parse destination address\n");
		usage(argv[0]);
		return -1;
	}
	remote.port = SERVER_PORT;
	if (sock_dtls_create(tls_socket_addr, &local, &remote, 0, wolfDTLSv1_2_client_method()) != 0) {
		LOG(LOG_ERROR, "ERROR: Unable to create DTLS sock\n");
	}

	if (handle_certs() < 0)
		return -1;

	LOG(LOG_INFO, "Certificates and verification is set up\n");

	LOG(LOG_INFO, "Creating session...\n");
	if (sock_dtls_session_create(tls_socket_addr) < 0) {
		LOG(LOG_ERROR, "ERROR: Unable to create a DTLS session\n");
		return -1;
	}
	LOG(LOG_INFO, "Session created!\n");
	wolfSSL_dtls_set_timeout_init(tls_socket_addr->ssl, 5);
	LOG(LOG_INFO, "connecting to server...\n");

	do {
		ret = wolfSSL_connect(tls_socket_addr->ssl);
		if ((ret != SSL_SUCCESS)) {
			int error_type = wolfSSL_get_error(tls_socket_addr->ssl, ret);
			if (error_type == SOCKET_ERROR_E) {
				LOG(LOG_WARNING, "Socket Error: reconnecting...\n");
				connect_timeout = 0;
				if (restart_session() < 0)
					return -1;
			}
			if ((error_type == WOLFSSL_ERROR_WANT_READ)
					&& (connect_timeout++ >= max_connect_timeouts)) {
				LOG(LOG_WARNING, "Server not responding: reconnecting...\n");
				connect_timeout = 0;
				if (restart_session() < 0)
					return -1;
			}
		}
	} while (ret != SSL_SUCCESS);

	/* set remote endpoint */
    sock_dtls_set_endpoint(tls_socket_addr, &remote);

	/* send hello message */
	wolfSSL_write(tls_socket_addr->ssl, buf, strlen(buf));

	    /* wait for a reply, indefinitely */
    do {
        ret = wolfSSL_read(tls_socket_addr->ssl, buf, APP_DTLS_BUF_SIZE - 1);
        LOG(LOG_INFO, "wolfSSL_read returned %d\n", ret);
    } while (ret <= 0);
    buf[ret] = (char)0;
    LOG(LOG_INFO, "Received: '%s'\n", buf);

    /* Clean up and exit. */
    LOG(LOG_INFO, "Closing connection.\n");
    sock_dtls_session_destroy(tls_socket_addr);
    sock_dtls_close(tls_socket_addr);
    return 0;
}
