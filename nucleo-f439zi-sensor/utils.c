#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DELIMETER '%'
#define HEXBUF_SIZE 3

/* Formatting MAC addresses to a human-readable form */
void format_mac(char *buf, uint8_t *addr, uint8_t len) {
	char hex_buf[HEXBUF_SIZE];
	for (size_t i = 0; i < len; ++i) {
		sprintf(hex_buf, "%x", addr[i]);
		if (addr[i] < 16) {
			buf[i * 3] = '0';
			memcpy(&buf[i * 3 + 1], hex_buf, 1);
		} else {
			memcpy(&buf[i * 3], hex_buf, 2);
		}
		buf[i * 3 + 2] = ':';
	}

	buf[len * 3 - 1] = (char)0;
}

int format_packet(char *out_buf, uint8_t buf_num, char *bufs[]) {
	uint8_t offset = 0;
	for (size_t i = 0; i < buf_num; ++i) {
		strncpy(&out_buf[offset], bufs[i], strlen(bufs[i]));
		offset += strlen(bufs[i]);
		out_buf[offset] = '%';
		offset++;
	}
	out_buf[offset - 1] = (char)0;
	return 0;

}

