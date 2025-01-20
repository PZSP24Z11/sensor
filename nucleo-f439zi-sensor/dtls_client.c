#include <wolfssl/ssl.h>
#include <wolfssl/error-ssl.h>
#include <sock_tls.h>
#include <net/sock.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>

#include "net/gnrc/netif.h"
#include "log.h"

#include <wolfssl/wolfcrypt/asn.h>
#include <wolfssl/wolfcrypt/error-crypt.h>
#include <wolfssl/wolfcrypt/types.h>
#include "periph/rtc.h"
#include <time.h>
#include "xtimer.h"

#define ERR_TIMEOUT -1
#define ERR_RESPONSE -2

#define SERVER_PORT 20220
#define APP_DTLS_BUF_SIZE 64

#ifndef NUM_READINGS
  #define NUM_READINGS 2
#endif
#ifndef READING_LEN
  #define READING_LEN 6
#endif

#define MAIN_QUEUE_SIZE     (8)

extern void format_mac(char *buf, uint8_t *addr, uint8_t len);
extern int format_packet(char *out_buf, uint8_t buf_num, char *bufs[]);
extern const unsigned char ca_der[];
extern const int ca_der_len;

static uint16_t netif_pid;
static sock_tls_t tls_socket;
static sock_tls_t *tls_socket_addr = &tls_socket;
static msg_t _main_msg_queue[MAIN_QUEUE_SIZE];

int decode_utctime(const unsigned char *bytes, size_t len, struct tm *tm_date) {
    int year, month, day, hour, minute, second;
    /* 2 bytes of trailing + 13 bytes of date */
    if (len != 15) {
        return -1;
    }

    int ret = sscanf((char *)bytes, "%2d%2d%2d%2d%2d%2d", &year, &month, &day, &hour, &minute, &second);
    if (ret != 6) {
        return ret;
    }

    if (year < 50) {
        year += 2000;
    } else {
        year += 1900;
    }

    tm_date->tm_year = year - 1900;
    tm_date->tm_mon = month - 1;
    tm_date->tm_mday = day;
    tm_date->tm_hour = hour;
    tm_date->tm_min = minute;
    tm_date->tm_sec = second;
    tm_date->tm_isdst = -1;

    return 0;
}

int handle_certs(void) {
    DecodedCert cert;
	int ret;
	struct tm tm_before;
	struct tm tm_after;
	struct tm tm_rtc;
	time_t rtc_time_t, before_time_t, after_time_t;
	char tm_str[50];

    // disable verification of ca cert (will be done manually)
    wolfSSL_CTX_set_verify(tls_socket_addr->ctx, SSL_VERIFY_NONE, NULL);

	LOG(LOG_INFO, "Loading CA cert\n");
	LOG(LOG_INFO, "CA cert len: %d\n", ca_der_len);

	ret = wolfSSL_CTX_load_verify_buffer(
            tls_socket_addr->ctx, 
            ca_der,
            ca_der_len,
            SSL_FILETYPE_ASN1);
    if (ret != SSL_SUCCESS && ret != -150) {
        LOG(LOG_INFO, "Error loading certificate\n");
        LOG(LOG_INFO, "Error code: %d\n", ret);
        return -1;
    }
    //wolfSSL_CTX_set_verify(tls_socket_addr->ctx, SSL_VERIFY_PEER, NULL); temporary!

	InitDecodedCert(&cert, ca_der, ca_der_len, NULL);
	ret = ParseCert(&cert, CERT_TYPE, NO_VERIFY, NULL);

	if (ret != 0) {
        FreeDecodedCert(&cert);
        return -1;
    }
    
    ret = decode_utctime(cert.afterDate + 2, cert.afterDateLen, &tm_after);
    if (ret != 0) {
        return -1;
    }
	strftime(tm_str, sizeof(tm_str), "%Y-%m-%d %H:%M:%S", &tm_after);
	LOG(LOG_INFO, "Certificate after date: %s\n", tm_str);

    ret = decode_utctime(cert.beforeDate + 2, cert.beforeDateLen, &tm_before);
    if (ret != 0) {
        return -1;
    }
	strftime(tm_str, sizeof(tm_str), "%Y-%m-%d %H:%M:%S", &tm_before);
	LOG(LOG_INFO, "Certificate before date: %s\n", tm_str);

	rtc_get_time(&tm_rtc);

	strftime(tm_str, sizeof(tm_str), "%Y-%m-%d %H:%M:%S", &tm_rtc);
	LOG(LOG_INFO, "Current date: %s\n", tm_str);

	before_time_t = mktime(&tm_before);
	after_time_t = mktime(&tm_after);
	rtc_time_t = mktime(&tm_rtc);

	if (before_time_t > rtc_time_t) {
	    LOG(LOG_ERROR, "Server certificate not yet valid!\n");
        FreeDecodedCert(&cert);
	    return -1;
    }

    if (after_time_t < rtc_time_t ) {
        LOG(LOG_ERROR, "Server Certificate no longer valid!\n");
        FreeDecodedCert(&cert);
        return -1;
    }

    LOG(LOG_INFO, "Certificate is valid!\n");

    FreeDecodedCert(&cert);

    return 0;
}

int restart_session(void) {
	sock_dtls_session_destroy(tls_socket_addr);
	return sock_dtls_session_create(tls_socket_addr);
}

int clean_up(void) {
	/* Clean up and exit. */
    LOG(LOG_INFO, "Closing connection.\n");
    sock_dtls_session_destroy(tls_socket_addr);
    sock_dtls_close(tls_socket_addr);
    return 0;
}

int dtls_client(char *addr_str)
{
	int ret = 0;
	char *iface;

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
			return -1;
		}
		remote.netif = (uint16_t)netif->pid;
	}
	netif_pid = remote.netif;

	if (ipv6_addr_from_str((ipv6_addr_t *)remote.addr.ipv6, addr_str) == NULL) {
		LOG(LOG_ERROR, "ERROR: Unable to parse destination address\n");
		return -1;
	}
	remote.port = SERVER_PORT;
	if (sock_dtls_create(tls_socket_addr, &local, &remote, 0, wolfDTLSv1_2_client_method()) != 0) {
		LOG(LOG_ERROR, "ERROR: Unable to create DTLS sock\n");
	}

	if (handle_certs() < 0) {
	    LOG(LOG_ERROR, "Error while processing certificates!\n");
	    sock_dtls_close(tls_socket_addr);
		return -1;
	}

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
				if (restart_session() < 0)
					return -1;
			}
			if (error_type == WOLFSSL_ERROR_WANT_READ) {
				LOG(LOG_WARNING, "Server not responding: reconnecting...\n");
				if (restart_session() < 0)
					return -1;
			} else if (error_type == -150){
                LOG(LOG_INFO, "Before Date invalid, manually checking the dates\n");
                wolfSSL_CTX_set_verify(tls_socket_addr->ctx, SSL_VERIFY_NONE, NULL);
                if (restart_session() < 0)
                    return -1;
            } else {
                LOG(LOG_ERROR, "Unknown error occured! %d\n", error_type);
                return -1;
            }
		}
	} while (ret != SSL_SUCCESS);

	/* set remote endpoint */
    sock_dtls_set_endpoint(tls_socket_addr, &remote);

    LOG(LOG_INFO, "Connection established.\n");
    return 0;
}

int verify_sensor(void) {
	int confirmation = 0;
	int ret = 0;
	char buf[APP_DTLS_BUF_SIZE] = "SENSORREQ";
	char req_buf[10] = "SENSORREQ";
	char type_buf[3] = "TH";

	/* Get MAC address */
	gnrc_netif_t *netif = gnrc_netif_get_by_pid(netif_pid);
	uint8_t *mac_addr = netif->l2addr;
	uint8_t mac_len = netif->l2addr_len;
	uint8_t mac_buf_len = 3 * mac_len;

	char mac_buf[mac_buf_len];
	char *bufs[] = {req_buf, mac_buf, type_buf};

	/* format MAC address properly */
	format_mac(mac_buf, mac_addr, mac_len);

	LOG(LOG_INFO, "MAC ADDRESS: %s\n", mac_buf);

	format_packet(buf, 3, bufs);

	/* send sensor request */
	ret = wolfSSL_write(tls_socket_addr->ssl, buf, strlen(buf));
	LOG(LOG_INFO, "wolfSSL_write returned with %d\n", ret);

	/* wait for a request accept */
	while (!confirmation) {
		do {
			ret = wolfSSL_read(tls_socket_addr->ssl, buf, APP_DTLS_BUF_SIZE - 1);
			LOG(LOG_INFO, "wolfSSL_read returned %d\n", ret);
		} while (ret <= 0);
		buf[ret] = (char)0;
		LOG(LOG_INFO, "Received: '%s'\n", buf);
		if (!strcmp(buf, "SENSORACC")) {
			confirmation = 1;
		}
	}

	return 0;
}

int send_readings(char *readings[]) {
	int16_t ret = 0;
	uint8_t ack = 0;
	char ack_buf[5];
	char buf[READING_LEN * (NUM_READINGS + 1)];

	uint8_t errors = 0;
	uint8_t timeouts = 0;
	const uint8_t max_ack_timeouts = 10;
	const uint8_t max_errors = 3;

	format_packet(buf, NUM_READINGS, readings);

	LOG(LOG_INFO, "Crafted packet: '%s'\n", buf);

	/* Sending and confirmation handling */
	do {

		/* re-send packet every iteration */
		ret = wolfSSL_write(tls_socket_addr->ssl, buf, strlen(buf));
		LOG(LOG_INFO, "Sensor readings sent: %d characters\n", ret);
		LOG(LOG_INFO, "Awaiting server ACK...\n");

		/* Read into the ack buf */
		ret = wolfSSL_read(tls_socket_addr->ssl, ack_buf, 4);

		/* Check if ACK or ERR*/
		if (ret >= 4) {
			/* Null-terminate, just in case */
			ack_buf[ret] = (char)0;

			if (!strcmp(ack_buf, "ACK")) {
				ack = 1;
			} else if (!strcmp(ack_buf, "ERR")) {
				errors++;
				LOG(LOG_WARNING, "Server reports error with reading! (%d/%d)\n", errors, max_errors);
				if (errors >= max_errors) {
					LOG(LOG_ERROR, "Too many errors! stopping\n");
					return ERR_RESPONSE;
				}
			}
		} else if (ret < 0) {
			timeouts++;
			if (timeouts == max_ack_timeouts) {
				LOG(LOG_ERROR, "No response from server, timed out! (%d/%d)\n", timeouts, max_ack_timeouts);
				return ERR_TIMEOUT;
			}
		} else {
			LOG(LOG_WARNING, "Received invalid server response\n");
		}
	} while (!ack);

	LOG(LOG_INFO, "ACK received\n");
	return 0;
}

int initialize_dtls(char* ip)
{
	msg_init_queue(_main_msg_queue, MAIN_QUEUE_SIZE);
	wolfSSL_Init();
	wolfSSL_Debugging_ON();

	if (dtls_client(ip)) {
		LOG(LOG_ERROR, "ERROR: dtls_client failed, exitting...\n");
		return -1;
	}
	LOG(LOG_INFO, "DTLS Init complete\n");

	if (verify_sensor()) {
		LOG(LOG_ERROR, "ERROR: Sensor not accepted by host, exitting...\n");
		clean_up();
		return -1;
	}
	
	return 0;
}
