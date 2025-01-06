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
#include <pthread.h>
#include "constants.h"
#include "helpers.h"


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
                state = S_RECIEVE_REQUEST;
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

typedef struct {
    WOLFSSL* ssl;
    CURL* curl;
} client_args_t;


void* handle_client_thread(void* arg) {
    client_args_t* client_args = (client_args_t*)arg;
    handle_client(client_args->ssl, client_args->curl);
    wolfSSL_shutdown(client_args->ssl);
    wolfSSL_free(client_args->ssl);
    free(client_args);
    return NULL;
}


static int cleanup;
struct sockaddr_in6 server_address;
struct sockaddr_in6 client_address;


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
    socklen_t       cli_len;
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

        /* clear server_address each loop */
        memset((char *)&server_address, 0, sizeof(server_address));

        server_address.sin6_family      = AF_INET6;
        server_address.sin6_port        = htons(SERV_PORT);

        /* Eliminate socket already in use error */
        if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, len) < 0) {
            printf("Setsockopt SO_REUSEADDR failed.\n");
            cleanup = 1;
            cont = 1;
        }

        /*Bind Socket*/
        if (bind(listenfd, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) {
            printf("Bind failed.\n");
            cleanup = 1;
            cont = 1;
        }

        printf("Awaiting client connection on port %d\n", SERV_PORT);

        cli_len = sizeof(client_address);
        connfd = (int)recvfrom(listenfd, (char *)&b, sizeof(b), MSG_PEEK,
                (struct sockaddr*)&client_address, &cli_len);

        if (connfd < 0) {
            printf("No clients in que, enter idle state\n");
            continue;
        } else if (connfd > 0) {
            if (connect(listenfd, (const struct sockaddr *)&client_address,
                        sizeof(client_address)) != 0) {
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

        client_args_t* client_args = malloc(sizeof(client_args_t));
        client_args->ssl = ssl;
        client_args->curl = curl;

        pthread_t client_thread;
        if (pthread_create(&client_thread, NULL, handle_client_thread, client_args) != 0) {
            printf("failed to create thread\n");
            wolfSSL_free(ssl);
            free(client_args);
            continue;
        }

        pthread_detach(client_thread);
    }
    

    wolfSSL_CTX_free(ctx);
    wolfSSL_Cleanup();
    curl_easy_cleanup(curl);
    curl_global_cleanup();

    return 0;
}
