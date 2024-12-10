#include "log.h"

#include <sock_tls.h>

#ifndef NUM_READINGS
  #define NUM_READINGS 2
#endif

#ifndef READING_LEN
  #define READING_LEN 5
#endif

extern int dtls_client(char *addr_str);
extern int verify_sensor(void);
extern int clean_up(void);
extern int send_readings(const char *readings[]);

static void usage_sensor(const char *cmd_name)
{
	LOG(LOG_ERROR, "Usage: %s <server address>\n", cmd_name);
}

int start_sensor(int argc, char **argv) {
	LOG(LOG_INFO, "Establishing connection to server\n");

	if (argc != 2) {
		usage_sensor(argv[0]);
		return -1;
	}

	/* Establish connection with DTLS server */
	if (dtls_client(argv[1])) {
		LOG(LOG_ERROR, "ERROR: dtls_client failed, exitting...\n");
		return -1;
	}


	/* Request permission to send data */
	if (verify_sensor()) {
		LOG(LOG_ERROR, "ERROR: Sensor not accepted by host, exitting...\n");
		clean_up();
		return -1;
	}

	while (true) {

		/* MOCKED SENSOR DATA */
		char r1[READING_LEN] = "T1234";
		char r2[READING_LEN] = "H0013";
		const char *readings[NUM_READINGS] = {r1, r2};

		if (send_readings(readings)) {
			LOG(LOG_ERROR, "ERROR: Unable to send data\n");
			clean_up();
			return -1;
		}
	}
	
	/* Close connection, destroy session */
	clean_up();
	return 0;
}

int dtls_client_cmd(int argc, char **argv) {
	start_sensor(argc, argv);
	return 0;
}
