#include "shell.h"
#include "ztimer.h"
#include "log.h"

bool MOCK_MODE = false;

int16_t hum = 0;
int16_t temp = 0;

bool hadError = false;
bool isSending = false;
bool isActive = false;
bool configured = false;

extern void initialize_sensor(void);
extern void initialize_lcd(void);
extern int initialize_dtls(char* ip);
extern void start_gather_measurements_thread(void);
extern void start_mock_measurements_thread(void);
extern void start_gpio_loop_thread(void);

int start_sensor(int argc, char **argv) {

    if(!MOCK_MODE)
    {
        isActive = true;
        initialize_lcd();
        initialize_sensor();
        start_gpio_loop_thread();
    }

	if (argc != 2) {
		LOG(LOG_ERROR, "Usage: %s <server address>\n", argv[0]);
        hadError = true;
        ztimer_sleep(ZTIMER_MSEC, 1000);
		return -1;
	}


	if (initialize_dtls(argv[1])) {
	    LOG(LOG_ERROR, "DTLS initialization failed!\n");
        hadError = true;
        ztimer_sleep(ZTIMER_MSEC, 1000);
	    return -1;
    }
  
    configured = true;
    ztimer_sleep(ZTIMER_MSEC, 1000);
    
    if(MOCK_MODE)
    {
        start_mock_measurements_thread();
    }
    else
    {
        start_gather_measurements_thread();
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
