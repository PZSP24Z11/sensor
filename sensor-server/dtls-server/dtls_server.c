#include <wolfssl/options.h>
#include <wolfssl/ssl.h>
#include <stdio.h>                  /* standard in/out procedures */
#include <curl/curl.h>
#include <stdlib.h>                 /* defines system calls */
#include <string.h>                 /* necessary for memset */
#include <netdb.h>
#include <sys/socket.h>             /* used for all socket calls */
#include <netinet/in.h>             /* used for sockaddr_in6 */
#include <arpa/inet.h>
#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <unistd.h>


#define SERV_PORT   20220
#define MSGLEN      4096

// STATES
#define S_RECIEVE_REQUEST           0 // recieve sensor request
#define S_SEND_REQUEST_TO_SERVER    1
#define S_SEND_SENSOR_ACCEPT        2 // send sensoracc
#define S_RECIEVE_MEASUREMENTS      3 // recieve measurements
#define S_SEND_SENSOR_ACK           4 // send ack
#define S_SEND_REQUEST_SUBMITED     5
#define S_DONE                      6 

// ERROR STATES
#define ES_BAD_REQUEST_FORMAT                   -1
#define ES_BAD_MEASUREMENT_FORMAT               -2
#define ES_API_COMMUNICATION_BAD_REQUEST        -3 // error communicating with API
#define ES_API_COMMUNICATION_BAD_MEASUREMENT    -4

#define ES_API_CONNECTION_REQUEST               -10
#define ES_API_CONNECTION_MEASUREMENT           -11

#define ES_READ_FAILED                          -21
#define ES_WRITE_FAILED                         -20

// MESSAGES CONSTANTS
#define SEPARATOR           '%'
#define MIN_SREQ_LEN        29
#define MAX_SREQ_LEN        34
#define MAC_LEN             17
#define MEASUREMENT_LEN     5   // measurement length - format eg T1750; - temperature 17.50

// API COMMUNICATION - responses
#define SENSOR_KNOWN                1 // sensor was already registered
#define SENSOR_REG_SUB              2 // sensor was not registered, register request submitted
#define BAD_REQUEST                 3 // server responed with bad request  

#define MEASUREMENTS_ACCEPTED       2
#define MEASUREMENTS_REJECTED       3
#define ERROR_IN_API_COMMUNICATION  -1

#define API_REQUEST_ENDPOINT        "http://localhost:8000/sensor/register/"
#define API_MEASUREMENTS_ENDPOINT   "http://localhost:8000/sensor/measurements/"

static int cleanup;
struct sockaddr_in6 servAddr;
struct sockaddr_in6 cliaddr;


bool validate_sreq(const char sreq[], int len){
    const char *prefix = "SENSORREQ";
    int prefix_len = strlen(prefix);
    int mac_start = prefix_len + 1;
    int suffix_start = mac_start + MAC_LEN + 1;

    if (memcmp(sreq, prefix, prefix_len) != 0) {
        return false;
    }

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

struct MemoryStruct {
    char *memory;
    size_t size;
};

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;

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

    struct MemoryStruct chunk;
    chunk.memory = malloc(1);
    chunk.size = 0; 
    
    printf("sending sesnor request to api: %s\n", sreq);

    curl_easy_setopt(curl, CURLOPT_URL, API_REQUEST_ENDPOINT);
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, sreq);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
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

    struct MemoryStruct chunk;
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
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
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

int handle_client(WOLFSSL* ssl, CURL* curl) {
    int recv_len    = 0;
    int state       = S_RECIEVE_REQUEST;
    int cont        = 1;
    int response    = 0;
    char            buff[MSGLEN];
    char            err[] = "ERR";
    char            ack[] = "ACK";
    char            sns_acc[] = "SENSORACC";
    char            mac[MAC_LEN+1];


    while (cont) {
        switch (state)
        {
        case S_RECIEVE_REQUEST:
            recv_len = wolfSSL_read(ssl, buff, sizeof(buff)-1);
            
            if (recv_len < 0){
                int read_err = wolfSSL_get_error(ssl, 0);
                if(read_err != SSL_ERROR_WANT_READ && check_sensor_err(ssl)) {
                    state = ES_READ_FAILED;
                }
            }

            if (recv_len >= sizeof(buff)){
                memset(buff, 0, sizeof(buff));
                printf("Recieved more than buffer size. Recieved: %d", recv_len);
                state = ES_READ_FAILED;
            }

            // for dev
            buff[recv_len] = 0;

            if ((recv_len <= MIN_SREQ_LEN && MAX_SREQ_LEN <= recv_len) || !validate_sreq(buff, recv_len)){
                state = ES_BAD_REQUEST_FORMAT;
                break;
            }

            strncpy(mac, strchr(buff, '%') + 1, MAC_LEN);
            mac[MAC_LEN] = '\0';
            printf("MAC: %s\n", mac);

            state = S_SEND_REQUEST_TO_SERVER;
            break;

        case S_SEND_REQUEST_TO_SERVER:
            response = send_request_to_api(curl, buff, recv_len);

            switch (response)
            {
            case SENSOR_KNOWN:
                printf("sensor is known by API server\n");
                state = S_SEND_SENSOR_ACCEPT;
                break;
            case SENSOR_REG_SUB:
				// state = S_SEND_REQUEST_SUBMITED;
                printf("API server registered sensor\n");
				state = S_SEND_SENSOR_ACCEPT;
                break;
            case BAD_REQUEST:
                state = ES_API_COMMUNICATION_BAD_REQUEST;
                break;
            case ERROR_IN_API_COMMUNICATION:
            default:
                state = ES_API_CONNECTION_REQUEST;
                break;
            }

            break;
        
        case S_SEND_SENSOR_ACCEPT:
            if (wolfSSL_write(ssl, sns_acc, sizeof(sns_acc)) < 0){
                state = ES_WRITE_FAILED;
            } else {
                printf("SENSORACC sent succesfully\n");
                state = S_RECIEVE_MEASUREMENTS;
            }
            break;
        
        case S_RECIEVE_MEASUREMENTS:
            recv_len = wolfSSL_read(ssl, buff, sizeof(buff)-1);
            
            if (recv_len < 0){
                int read_err = wolfSSL_get_error(ssl, 0);
                if(read_err != SSL_ERROR_WANT_READ && check_sensor_err(ssl)) {
                    state = ES_READ_FAILED;
                }
            }

            if (recv_len >= sizeof(buff)){
                memset(buff, 0, sizeof(buff));
                printf("Recieved more than buffer size. Recieved: %d", recv_len);
                state = ES_READ_FAILED;
            }
            buff[recv_len] = 0;

            if (!validate_measurements(buff, recv_len)) {
                state = ES_BAD_MEASUREMENT_FORMAT;
            } else if ((response = send_ms_to_api(curl, mac, buff, recv_len)) != MEASUREMENTS_ACCEPTED){
                if (response == MEASUREMENTS_REJECTED){
                    printf("API server didnt accept sent measurements\n");
                    state = ES_API_COMMUNICATION_BAD_MEASUREMENT;
                } else {
                    printf("Error communicating with API");
                    state = ES_API_CONNECTION_MEASUREMENT;
                }
                break;
            } else {
                printf("API server accepted sent measurements\n");
                state = S_SEND_SENSOR_ACK;
            }
            break;
        
        case S_SEND_SENSOR_ACK:
            if (wolfSSL_write(ssl, ack, sizeof(ack)) < 0){
                state = ES_WRITE_FAILED;
            } else {
                printf("ACK sent succesfully\n");
                state = S_RECIEVE_MEASUREMENTS;
            }
            break;

        case S_SEND_REQUEST_SUBMITED:
            state = S_RECIEVE_MEASUREMENTS;
            break;
            // later to be implemented

        case S_DONE:
            cont = 0;
            break;
// ERRORS
        case ES_BAD_REQUEST_FORMAT:
            if (wolfSSL_write(ssl, err, sizeof(err)) < 0) {
                state = ES_WRITE_FAILED;
            } else {
                printf("bad request format, err msg sent\n");
                state = ES_BAD_REQUEST_FORMAT;
            }
            break;
        
        case ES_BAD_MEASUREMENT_FORMAT:
            if (wolfSSL_write(ssl, err, sizeof(err)) < 0) {
                state = ES_WRITE_FAILED;
            } else {
                printf("bad measurement format, err sent\n");
                state = S_RECIEVE_MEASUREMENTS;
            }
            break;

        case ES_WRITE_FAILED:
            printf("wolfSSL_write fail\n");
            cont = 0;
            break;
        
        case ES_READ_FAILED:
            printf("SSL_read failed\n");
            cont = 0;
            break;
        
        case ES_API_CONNECTION_REQUEST:
            printf("error while communicating with API - could not connect");
            cont = 0;
            break;
        
        case ES_API_CONNECTION_MEASUREMENT:
            printf("error while communicating with API - could not send measurments");
            state = S_RECIEVE_MEASUREMENTS;
            break;
        
        case ES_API_COMMUNICATION_BAD_REQUEST:
            printf("error while communicating with API - bad request");
            state = S_RECIEVE_REQUEST;
            break;

        case ES_API_COMMUNICATION_BAD_MEASUREMENT:
            printf("error while communicating with API - bad measurements");
            state = S_RECIEVE_MEASUREMENTS;
            break;
        default:
            cont = 0;
            break;
        }
    }
    return 0;

}


int main(int argc, char** argv) {
    int             cont = 0;
    char            caCertLoc[] = "../../cert/ca.crt.pem";
    char            servCertLoc[] = "../../cert/server.crt.pem";
    char            servKeyLoc[] = "../../cert/server.prv.pem";
    WOLFSSL_CTX*    ctx;
    int             on = 1;
    int             connfd = 0;
    int             listenfd = 0;   /* Initialize our socket */
    WOLFSSL*        ssl = NULL;
    socklen_t       cliLen;
    socklen_t       len = sizeof(int);
    unsigned char   b[MSGLEN];      /* watch for incoming messages */       
    CURL*           curl;

    wolfSSL_Init();

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if (!curl) {
        fprintf(stderr, "failed to initialize CURL\n");
        return -1;
    }


    ctx = wolfSSL_CTX_new(wolfDTLSv1_2_server_method());
    if (ctx == NULL) {
        printf("wolfSSL_CTX_new error.\n");
        return 1;
    }

    if (wolfSSL_CTX_load_verify_locations(ctx,caCertLoc,0) != SSL_SUCCESS) {
        printf("Error loading %s, please check the file.\n", caCertLoc);
        return 1;
    }

    if (wolfSSL_CTX_use_certificate_file(ctx, servCertLoc, SSL_FILETYPE_PEM) != SSL_SUCCESS) {
        printf("Error loading %s, please check the file.\n", servCertLoc);
        return 1;
    }

    if (wolfSSL_CTX_use_PrivateKey_file(ctx, servKeyLoc, SSL_FILETYPE_PEM) != SSL_SUCCESS) {
        printf("Error loading %s, please check the file.\n", servKeyLoc);
        return 1;
    }

    while (cleanup != 1) {
        listenfd = socket(AF_INET6, SOCK_DGRAM, 0);
        if (listenfd  <= 0 ) {
            printf("error: cannot create socket.\n");
            cleanup = 1;
        }
        printf("info: socket allocated\n");

        /* clear servAddr each loop */
        memset((char *)&servAddr, 0, sizeof(servAddr));

        servAddr.sin6_family      = AF_INET6;
        servAddr.sin6_port        = htons(SERV_PORT);

        /* Eliminate socket already in use error */
        if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, len) < 0) {
            printf("Setsockopt SO_REUSEADDR failed.\n");
            cleanup = 1;
            cont = 1;
        }

        /*Bind Socket*/
        if (bind(listenfd, (struct sockaddr*)&servAddr, sizeof(servAddr)) < 0) {
            printf("Bind failed.\n");
            cleanup = 1;
            cont = 1;
        }

        printf("Awaiting client connection on port %d\n", SERV_PORT);

        cliLen = sizeof(cliaddr);
        connfd = (int)recvfrom(listenfd, (char *)&b, sizeof(b), MSG_PEEK,
                (struct sockaddr*)&cliaddr, &cliLen);

        if (connfd < 0) {
            printf("No clients in que, enter idle state\n");
            continue;
        }
        else if (connfd > 0) {
            if (connect(listenfd, (const struct sockaddr *)&cliaddr,
                        sizeof(cliaddr)) != 0) {
                printf("Udp connect failed.\n");
                cleanup = 1;
                cont = 1;
            }
        }
        else {
            printf("Recvfrom failed.\n");
            cleanup = 1;
            cont = 1;
        }
        printf("Connected!\n");

        /* Create the WOLFSSL Object */
        if ((ssl = wolfSSL_new(ctx)) == NULL) {
            printf("wolfSSL_new error.\n");
            cleanup = 1;
            cont = 1;
        }

        /* set the session ssl to client connection port */
        wolfSSL_set_fd(ssl, listenfd);

        if (wolfSSL_accept(ssl) != SSL_SUCCESS) {

            int e = wolfSSL_get_error(ssl, 0);

            printf("error = %d, %s\n", e, wolfSSL_ERR_reason_error_string(e));
            printf("SSL_accept failed.\n");
            continue;
        }


        
        handle_client(ssl, curl);

        curl_easy_cleanup(curl);
        curl_global_cleanup();

        wolfSSL_set_fd(ssl, 0);
        wolfSSL_shutdown(ssl);
        wolfSSL_free(ssl);

        printf("Client left cont to idle state\n");
        cont = 0;
    }
    
    /* With the "continue" keywords, it is possible for the loop to exit *
     * without changing the value of cont                                */
    if (cleanup == 1) {
        cont = 1;
    }

    if (cont == 1) {
        wolfSSL_CTX_free(ctx);
        wolfSSL_Cleanup();
    }

    return 0;
}
