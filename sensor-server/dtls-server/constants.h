#ifndef CONSTANTS_H
#define CONSTANTS_H

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

#define API_REQUEST_ENDPOINT        "http://localhost:8080/sensor/register/"
#define API_MEASUREMENTS_ENDPOINT   "http://localhost:8080/sensor/measurements/"

#endif // CONSTANTS_H
