#include <wolfssl/ssl.h>

#include "shell.h"
#include "msg.h"
#include "log.h"

#ifdef WITH_RIOT_SOCKETS
#error RIOT-OS is set to use sockets but this DTLS app is configured for socks.
#endif

#define MAIN_QUEUE_SIZE     (8)
static msg_t _main_msg_queue[MAIN_QUEUE_SIZE];

extern int dtls_client(int argc, char **argv);

static const shell_command_t shell_commands[] = {
	{ "dtlsc", "Start a DTLS Client", dtls_client },
	{ NULL, NULL, NULL }
};


int main(void)
{
	msg_init_queue(_main_msg_queue, MAIN_QUEUE_SIZE);
	LOG(LOG_INFO, "RIOT wolfSSL DTLS client\n");
	wolfSSL_Init();
	wolfSSL_Debugging_ON();

	/* start shell */
	LOG(LOG_INFO, "All up, running the shell now\n");
	char line_buf[SHELL_DEFAULT_BUFSIZE];
	shell_run(shell_commands, line_buf, SHELL_DEFAULT_BUFSIZE);

	return 0;
}
