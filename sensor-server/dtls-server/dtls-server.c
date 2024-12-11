#include <wolfssl/options.h>
#include <wolfssl/ssl.h>
#include <stdio.h>                  /* standard in/out procedures */
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


#define SERV_PORT   20221
#define MSGLEN      4096

// STATES
#define S_RECVREQ       0 // recieve sensor request
#define S_CHECK_REG     1 // check if sensor is registered
#define S_SND_SNSACK    2 // send sensorack
#define S_RECVMS        3 // recieve measurements
#define S_SND_ACK       4 // send ack
#define S_DONE          5 

// 
#define ES_BAD_REQ_FMT  -1
#define ES_BAD_MS_FMT   -2
#define ES_API_COMM     -3 // error communicating with API

#define ES_R_FAILED     -21
#define ES_W_FAILED     -20

// MESSAGES CONSTANTS
#define SEPARATOR           '%'
#define MIN_SREQ_LEN        29
#define MAX_SREQ_LEN        34
#define MAC_LEN             17
#define MEASUREMENT_LEN     4   // measurement length - format eg T1750; - temperature 17.50

static int cleanup;
struct sockaddr_in6 servAddr;
struct sockaddr_in6 cliaddr;


bool validate_sreq(char sreq[], int len){
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
        // if (!isupper(sreq[i]))
        if (!isupper(sreq[i]) && sreq[i] != '\n') // for now - to test working with openssl client
            return false;
    }

    return true;
}

bool validate_measurements(char ms[], int mslen){
    return true;
}

bool sensor_registered_check(char mac[]){
    return true;
}

bool check_sensor_err(WOLFSSL* ssl){
    int readErr = wolfSSL_get_error(ssl, 0);
    return readErr != SSL_ERROR_WANT_READ;
}

int send_to_api(char msg[], int msg_len) {
    // for now just mock
    msg[msg_len] = '\0';
    printf("sending measurements to server: %s", msg);
    return 0;
}

int handle_client(WOLFSSL* ssl) {
    /*
    1 - recievieng SENSORREQ
    2 - 
    */
    int recv_len    = 0;
    int state       = 0;
    int cont        = 1;
    char            buff[MSGLEN];
    char            err[] = "ERR";
    char            ack[] = "ACK";
    char            sns_acc[] = "SENSORACC";
    char            mac[MAC_LEN+1];

    while (cont == 1) {
        switch (state)
        {
        case S_RECVREQ:
            recv_len = wolfSSL_read(ssl, buff, sizeof(buff)-1);
            
            if (recv_len < 0){
                int readErr = wolfSSL_get_error(ssl, 0);
                if(readErr != SSL_ERROR_WANT_READ && check_sensor_err(ssl)) {
                    state = ES_R_FAILED;
                }
            }
            // for dev
            buff[recv_len] = 0;
            printf("Recieved: \"%s\"\n", buff);

            if ((recv_len <= MIN_SREQ_LEN && MAX_SREQ_LEN <= recv_len) || !validate_sreq(buff, recv_len)){
                state = ES_BAD_REQ_FMT;
                break;
            }

            strncpy(mac, strchr(buff, '%') + 1, MAC_LEN);
            mac[MAC_LEN] = '\0';
            printf("MAC: %s\n", mac);


            state = S_CHECK_REG;
            break;

        case S_CHECK_REG:
            if (sensor_registered_check(buff)){
                state = S_SND_SNSACK;
            } else {
                state = ES_BAD_REQ_FMT;
            }
            break;
        
        case S_SND_SNSACK:
            if (wolfSSL_write(ssl, sns_acc, sizeof(sns_acc)) < 0){
                state = ES_W_FAILED;
            } else {
                printf("SENSORACK sent succesfully\n");
                state = S_RECVMS;
            }
            break;
        
        case S_RECVMS:
            recv_len = wolfSSL_read(ssl, buff, sizeof(buff)-1);
            
            if (recv_len < 0){
                int readErr = wolfSSL_get_error(ssl, 0);
                if(readErr != SSL_ERROR_WANT_READ && check_sensor_err(ssl)) {
                    state = ES_R_FAILED;
                }
            }

            if (!validate_measurements(buff, recv_len)){
                state = ES_BAD_MS_FMT;
            } else if (send_to_api(buff, recv_len) != 0){
                state = ES_API_COMM;
            } else {
                state = S_SND_ACK;
            }
            break;
        
        case S_SND_ACK:
            if (wolfSSL_write(ssl, ack, sizeof(ack)) < 0){
                state = ES_W_FAILED;
            } else {
                printf("ACK sent succesfully\n");
                state = S_DONE;
                cont = 0;
            }
            break;

// ERRORS
        case ES_BAD_REQ_FMT:
            if (wolfSSL_write(ssl, err, sizeof(err)) < 0) {
                state = ES_W_FAILED;
            } else {
                printf("bad request format, err msg sent\n");
                cont = 0;
            }
            break;
        
        case ES_BAD_MS_FMT:
            if (wolfSSL_write(ssl, err, sizeof(err)) < 0) {
                state = ES_W_FAILED;
            } else {
                printf("bad measurement format, err sent\n");
                cont = 0;
            }
            break;

        case ES_W_FAILED:
            printf("wolfSSL_write fail\n");
            cont = 0;
            break;
        
        case ES_R_FAILED:
            printf("SSL_read failed\n");
            cont = 0;
            break;
        
        case ES_API_COMM:
            printf("Error while communicating with API");
            cont = 0;
            break;

        default:
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
    int             ret = 0;
    int             on = 1;
    int             res = 1;
    int             connfd = 0;
    int             recvLen = 0;    /* length of message */
    int             listenfd = 0;   /* Initialize our socket */
    WOLFSSL*        ssl = NULL;
    socklen_t       cliLen;
    socklen_t       len = sizeof(int);
    unsigned char   b[MSGLEN];      /* watch for incoming messages */       

    wolfSSL_Init();


    ctx = wolfSSL_CTX_new(wolfDTLSv1_2_server_method());
    if (ctx == NULL) {
        printf("wolfSSL_CTX_new error.\n");
        return 1;
    }
    /* Load CA certificates */
    ret = wolfSSL_CTX_load_verify_locations(ctx,caCertLoc,0);
    if (ret != SSL_SUCCESS) {
        printf("Error loading %s, please check the file.\n", caCertLoc);
        return 1;
    }
    /* Load server certificates */
    ret = wolfSSL_CTX_use_certificate_file(ctx, servCertLoc, SSL_FILETYPE_PEM);
    if (ret != SSL_SUCCESS) {
        printf("Error loading %s, please check the file.\n", servCertLoc);
        return 1;
    }
    /* Load server Keys */
    ret = wolfSSL_CTX_use_PrivateKey_file(ctx, servKeyLoc, SSL_FILETYPE_PEM);
    if (ret != SSL_SUCCESS) {
        printf("Error loading %s, please check the file.\n", servKeyLoc);
        return 1;
    }

    /* Await Datagram */
    while (cleanup != 1) {

        /* Create a UDP/IP socket */
        listenfd = socket(AF_INET6, SOCK_DGRAM, 0);
        if (listenfd  <= 0 ) {
            printf("error: cannot create socket.\n");
            cleanup = 1;
        }
        printf("info: socket allocated\n");

        /* clear servAddr each loop */
        memset((char *)&servAddr, 0, sizeof(servAddr));

        /* host-to-network-long conversion (htonl) */
        /* host-to-network-short conversion (htons) */
        servAddr.sin6_family      = AF_INET6;
        servAddr.sin6_port        = htons(SERV_PORT);

        /* Eliminate socket already in use error */
        res = setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, len);
        if (res < 0) {
            printf("Setsockopt SO_REUSEADDR failed.\n");
            cleanup = 1;
            cont = 1;
        }

        /*Bind Socket*/
        res = bind(listenfd, (struct sockaddr*)&servAddr, sizeof(servAddr));
        if (res < 0) {
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

        handle_client(ssl);

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
