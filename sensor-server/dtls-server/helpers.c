#include <wolfssl/options.h>
#include <wolfssl/ssl.h>
#include <stdio.h>                  /* standard in/out procedures */
#include <curl/curl.h>
#include <stdlib.h>                 /* defines system calls */
#include <string.h>
#include <stdbool.h>
#include "helpers.h"
#include "constants.h"


bool validate_sreq(const char sreq[], int len){
    const char *prefix = "SENSORREQ";
    int prefix_len = strlen(prefix);
    int mac_start = prefix_len + 1;
    int suffix_start = mac_start + MAC_LEN + 1;

    if (memcmp(sreq, prefix, prefix_len) != 0)
        return false;

    if (sreq[prefix_len] != SEPARATOR || sreq[mac_start + MAC_LEN] != SEPARATOR) {
        return false;
    }

    for (int i = 0; i < 17; i++) {
        char c = sreq[mac_start + i];
        if (i % 3 == 2) {
            if (c != ':') 
                return false;
        } else  if (!isxdigit(c)){
            return false;
        }
    }

    for (int i = suffix_start; i < len; i++) {
        if (!isupper(sreq[i]))
        // if (!isupper(sreq[i]) && sreq[i] != '\n') // for now - to test working with openssl client
            return false;
    }

    return true;
}

bool validate_measurements(const char ms[], int ms_len){
    char measurement[MEASUREMENT_LEN + 1];
    int i = 0;

    if (ms_len < MEASUREMENT_LEN)
        return false;

    while (i < ms_len) {
        int j = 0;

        while (i < ms_len && ms[i] != '%' && j < MEASUREMENT_LEN) {
            measurement[j++] = ms[i++];
        }
        measurement[j] = 0;

        if (j != 5 || !isupper(measurement[0]))
            return false;
        for (int k = 1; k < MEASUREMENT_LEN; k++) {
            if (!isdigit(measurement[k]))
            // if (!isdigit(measurement[k]) && measurement[k] != '\n') // for development - communication with openssl client
                return false;
        }

        if (i < ms_len && ms[i] == '%')
            i++;
    }

    return true;
}

bool check_sensor_err(WOLFSSL* ssl) {
    int readErr = wolfSSL_get_error(ssl, 0);
    return readErr != SSL_ERROR_WANT_READ;
}

size_t write_memory_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct memory_struct *mem = (struct memory_struct *)userp;

    char *ptr = realloc(mem->memory, mem->size + realsize + 1);
    if(ptr == NULL) {
        // out of memory!
        printf("not enough memory (realloc returned NULL)\n");
        return 0;
    }

    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    return realsize;
}

int send_request_to_api(CURL* curl, char sreq[], int len) {
    CURLcode res;
    int result;

    struct memory_struct chunk;
    chunk.memory = malloc(1);
    chunk.size = 0; 
    
    printf("sending sesnor request to api: %s\n", sreq);

    curl_easy_setopt(curl, CURLOPT_URL, API_REQUEST_ENDPOINT);
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, sreq);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_memory_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

    res = curl_easy_perform(curl);

    if(res != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        result = ERROR_IN_API_COMMUNICATION;
    } else {
        printf("sent sensor request to api: %s\n", sreq);
        result = atoi(chunk.memory);
    }

    free(chunk.memory);
    return result;
}

int send_ms_to_api(CURL* curl, char mac[], char ms[], int ms_len) {
    CURLcode res;
    int result;

    struct memory_struct chunk;
    chunk.memory = malloc(1);
    chunk.size = 0; 

    char msg_to_api[MAC_LEN + ms_len + 2];
    // build message to API
    strncpy(msg_to_api, mac, MAC_LEN);
    msg_to_api[MAC_LEN] = SEPARATOR;
    msg_to_api[MAC_LEN+1] = '\0';
    strncat(msg_to_api, ms, ms_len);

    msg_to_api[MAC_LEN + ms_len + 2] = '\0';
    printf("sending measurements to API server: %s\n", msg_to_api);

    curl_easy_setopt(curl, CURLOPT_URL, API_MEASUREMENTS_ENDPOINT);
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    printf("msg to api: %s\n", msg_to_api);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, msg_to_api);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_memory_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

    res = curl_easy_perform(curl);

    if(res != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
    } else {
        result = atoi(chunk.memory);
    }

    free(chunk.memory);
    return result;
}
