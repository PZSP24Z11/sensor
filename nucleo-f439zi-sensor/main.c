#include "shell.h"
#include "ztimer.h"
#include "log.h"

bool MOCK_MODE = true;

int16_t hum = 0;
int16_t temp = 0;

extern void initialize_sensor(void);
extern void initialize_lcd(void);
extern int initialize_dtls(char* ip);
extern void start_gather_measurements_thread(void);
extern void start_mock_measurements_thread(void);
extern void start_gpio_loop_thread(void);

int start_sensor(int argc, char **argv) {

	if (argc != 2) {
		LOG(LOG_ERROR, "Usage: %s <server address>\n", argv[0]);
		return -1;
	}

	if (initialize_dtls(argv[1])) {
	    LOG(LOG_ERROR, "DTLS initialization failed!\n");
	    return -1;
    }

    if(MOCK_MODE)
    {
        start_mock_measurements_thread();
        LOG(LOG_INFO, "Mock measurements initialized\n");
    }
    else
    {
        initialize_sensor();
        initialize_lcd();
        LOG(LOG_INFO, "Sensor and LCD initialized\n");

        start_gather_measurements_thread();
        start_gpio_loop_thread();
    }

	return 0;
}

static const shell_command_t shell_commands[] = {
	{ "sensor", "Start the sensor", start_sensor },
	{ NULL, NULL, NULL }
};

int main(void)
{
    LOG(LOG_INFO, "--- Starting Senosr App ---\n");
    LOG(LOG_INFO, "sensor <serwer-ip-address>: start app\n");
	LOG(LOG_INFO, "All up, running the shell now\n");
	char line_buf[SHELL_DEFAULT_BUFSIZE];
	shell_run(shell_commands, line_buf, SHELL_DEFAULT_BUFSIZE);
    return 0;
}
