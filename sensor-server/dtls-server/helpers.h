#ifndef HELPERS_H
#define HELPERS_H

#include <wolfssl/ssl.h>
#include <curl/curl.h>
#include <stddef.h>
#include <stdbool.h>
#include <curl/curl.h>


bool validate_sreq(const char sreq[], int len);
bool validate_measurements(const char ms[], int ms_len);
bool check_sensor_err(WOLFSSL* ssl);
size_t write_memory_callback(void *contents, size_t size, size_t nmemb, void *userp);
int send_request_to_api(CURL* curl, char sreq[], int len);
int send_ms_to_api(CURL* curl, char mac[], char ms[], int ms_len);

struct memory_struct {
    char *memory;
    size_t size;
};

#endif // HELPERS_H