
#include <string.h>
#include <stdio.h>
#include "dht.h"
#include "dht_params.h"
#include "log.h"
#include "ztimer.h"

dht_t sensor;
extern int16_t hum;
extern int16_t temp;
char gather_thread_stack[THREAD_STACKSIZE_DEFAULT];

extern int send_readings(const char *readings[]);

void *gather_measurements_thread_function(void *arg)
{
	(void)arg;
    char temp_str[32];
    char hum_str[32];
    char temp_data[READING_LEN] = "TXXX0";
    char hum_data[READING_LEN] = "HXXX0";
    while (1)    
    {
        if(dht_read(&sensor, &temp, &hum) == DHT_OK)
        {        
            sprintf(temp_str, "%d", temp);
            sprintf(hum_str, "%d", hum);
            strncpy(&temp_data[1], temp_str, 3);
            strncpy(&hum_data[1], hum_str, 3);
            const char* readings[NUM_READINGS] = {temp_data, hum_data};

            if(send_readings(readings) == 0)
            {
                LOG(LOG_INFO, "Data sent to server: %s, %s\n", temp_data, hum_data);
            }
            else
            {
                LOG(LOG_ERROR, "Error sending data to server: %s, %s\n", temp_data, hum_data);
            }
        }
        else
        {
            LOG(LOG_ERROR, "Error reading sensor output\n");
        }
        ztimer_sleep(ZTIMER_MSEC, SAMPLE_RATE);
    }
    return NULL;
}

void *mock_measurements_thread_function(void *arg)
{
	(void)arg;
    temp = 123;
    hum = 321;
    char temp_str[32];
    char hum_str[32];
    char temp_data[READING_LEN] = "TXXX0";
    char hum_data[READING_LEN] = "HXXX0";

    while (1)    
    {
        temp++;
        hum++;
        
        sprintf(temp_str, "%d", temp);
        sprintf(hum_str, "%d", hum);
		strncpy(&temp_data[1], temp_str, 3);
		strncpy(&hum_data[1], hum_str, 3);;
        const char* readings[NUM_READINGS] = {temp_data, hum_data};

        if(send_readings(readings) == 0)
        {
            LOG(LOG_INFO, "Data sent to server: %s, %s\n", temp_data, hum_data);
        }
        else
        {
            LOG(LOG_ERROR, "Error sending data to server: %s, %s\n", temp_data, hum_data);
        }
        
        ztimer_sleep(ZTIMER_MSEC, SAMPLE_RATE);
    }
    return NULL;
}


void start_gather_measurements_thread(void)
{
    thread_create(gather_thread_stack, sizeof(gather_thread_stack), THREAD_PRIORITY_MAIN - 2, 0, 
                gather_measurements_thread_function, NULL, "gather_measurements_thread_function");
}

void start_mock_measurements_thread(void)
{
    thread_create(gather_thread_stack, sizeof(gather_thread_stack), THREAD_PRIORITY_MAIN - 2, 0, 
                mock_measurements_thread_function, NULL, "mock_measurements_thread_function");
}

void initialize_sensor(void)
{
    LOG(LOG_INFO, "Initializing sensor... \n");
    if (dht_init(&sensor, &dht_params[0]) == DHT_OK) {
        LOG(LOG_INFO, "Sensor Initialized\n");
    } else {
        LOG(LOG_INFO, "Failed to Initialize Sensor\n");
    }
}