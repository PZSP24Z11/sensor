#include "stdio.h"
#include "stdlib.h"
#include "shell.h"
#include "led.h"
#include <periph/gpio.h>

#include "ztimer.h"
#include "dht.h"
#include "dht_params.h"
#include "pcd8544.h"
#include "log.h"

bool MOCK_MODE = false;

#define DELAY               (2 * US_PER_SEC)

#define LED_CONNECTING_PIN  (GPIO_PIN(4, 2))
#define LED_ACTIVE_PIN      (GPIO_PIN(4, 4))
#define LED_IDLE_PIN        (GPIO_PIN(4, 5))
#define LED_ERROR_PIN       (GPIO_PIN(4, 6))

#define BUTTON_UP_PIN       (GPIO_PIN(4, 3))
#define BUTTON_CONFIRM_PIN  (GPIO_PIN(5, 8))
#define BUTTON_DOWN_PIN     (GPIO_PIN(5, 7))

#define LCD_RESET           (GPIO_PIN(1, 0))  // PB0 *
#define LCD_CS              (GPIO_PIN(1, 1))  // PB1 *
#define LCD_MODE            (GPIO_PIN(1, 2))  // PB2 *
#define LCD_DIN             (GPIO_PIN(0, 7))  // PA7
#define LCD_CLK             (GPIO_PIN(0, 5))  // PA5
#define SPI_INTERFACE       (0)

#define SAMPLE_RATE         (1000)

dht_t sensor;
pcd8544_t lcd_device;
int16_t hum = 0;
int16_t temp = 0;
char gather_thread_stack[THREAD_STACKSIZE_DEFAULT];
char gpio_thread_stack[THREAD_STACKSIZE_DEFAULT];

void *gather_measurements_thread_function(void *arg)
{
	(void)arg;
    while (1)    
    {
        if(dht_read(&sensor, &temp, &hum) == DHT_OK)
        {
            char temp_data[32];
            char hum_data[32];
            sprintf(temp_data, "T%d", temp);
            sprintf(hum_data, "H%d", hum);
            // SEND DATA HERE
            // Eg. dtlcs.send(temp_data)
            // Eg. dtlcs.send(hum_data)
            // printf("%s, %s\n", temp_data, hum_data);
        }
        else
        {
            printf("Error reading sensor output!\n");
        }
        ztimer_sleep(ZTIMER_MSEC, SAMPLE_RATE);
    }
    return NULL;
}

void *mock_measurements_thread_function(void *arg)
{
	(void)arg;
    temp = 0;
    hum = 0;
    while (1)    
    {
        temp++;
        hum++;
        char temp_data[32];
        char hum_data[32];
        sprintf(temp_data, "T%d", temp);
        sprintf(hum_data, "H%d", hum);
        // SEND DATA HERE
        // Eg. dtlcs.send(temp_data)
        // Eg. dtlcs.send(hum_data)
        // printf("%s, %s\n", temp_data, hum_data);
        ztimer_sleep(ZTIMER_MSEC, SAMPLE_RATE);
    }
    return NULL;
}

void *gpio_loop_thread_function(void *arg)
{
	(void)arg;
    LOG(LOG_INFO, "Initializing LCD... \n");
    if (pcd8544_init(&lcd_device, SPI_INTERFACE, LCD_CS, LCD_RESET, LCD_MODE) == 0) 
    {
        LOG(LOG_INFO, "LCD initialized\n");
    } else {
        LOG(LOG_INFO, "LCD Failed to initialize\n");
    }
    pcd8544_poweron(&lcd_device);
    char hello_message[32];
    char temp_data[32];
    char hum_data[32];

    while (1)    
    {
        ztimer_sleep(ZTIMER_MSEC, SAMPLE_RATE);
        pcd8544_clear(&lcd_device);

        sprintf(hello_message, "Dane z sensora");
        sprintf(temp_data, "Temp.: %d,%d", temp/10, temp%10);
        sprintf(hum_data, "Wilg.: %d,%d", hum/10, hum%10);

        pcd8544_write_s(&lcd_device, 0, 0, hello_message);
        pcd8544_write_s(&lcd_device, 0, 1, temp_data);
        pcd8544_write_s(&lcd_device, 0, 2, hum_data);
    }
    return NULL;
}



static int _contrast(int argc, char **argv)
{
    uint8_t val;

    if (argc < 2) {
        printf("usage: %s VAL [0-127]\n", argv[0]);
        return 1;
    }
    val = atoi(argv[1]);
    pcd8544_set_contrast(&lcd_device, val);
    return 0;
}

static int _temp(int argc, char **argv)
{
    uint8_t val;

    if (argc < 2) {
        printf("usage: %s VAL [0-3]\n", argv[0]);
        return 1;
    }
    val = atoi(argv[1]);
    pcd8544_set_tempcoef(&lcd_device, val);
    return 0;
}

static int _bias(int argc, char **argv)
{
    uint8_t val;

    if (argc < 2) {
        printf("usage: %s VAL [0-7]\n", argv[0]);
        return 1;
    }
    val = atoi(argv[1]);
    pcd8544_set_bias(&lcd_device, val);
    return 0;
}

static int _on(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    pcd8544_poweron(&lcd_device);
    return 0;
}

static int _off(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    pcd8544_poweroff(&lcd_device);
    return 0;
}

static int _clear(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    pcd8544_clear(&lcd_device);
    return 0;
}

static int _invert(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    pcd8544_invert(&lcd_device);
    return 0;
}

static int _riot(int argc, char **argv) {
    (void)argc;
    (void)argv;

    pcd8544_riot(&lcd_device);
    return 0;
}

static int _write(int argc, char **argv)
{
    uint8_t x, y;

    if (argc < 4) {
        printf("usage: %s X Y STRING\n", argv[0]);
        return -1;
    }

    x = atoi(argv[1]);
    y = atoi(argv[2]);

    pcd8544_write_s(&lcd_device, x, y, argv[3]);
    return 0;
}


static const shell_command_t commands[] = {
    { "contrast", "set contrast", _contrast },
    { "temp", "set temperature coefficient", _temp },
    { "bias", "set BIAS value", _bias },
    { "on", "turn display on", _on },
    { "off", "turn display off", _off },
    { "clear", "clear memory", _clear },
    { "invert", "invert display", _invert },
    { "riot", "display RIOT logo", _riot },
    { "write", "write string to display", _write},
    { NULL, NULL, NULL }
};


int main(void)
{
    LOG(LOG_INFO, "--- Starting LCD Test ---\n");
    // Initialize Components
    if(MOCK_MODE == false)
    {
        LOG(LOG_INFO, "Initializing sensor... \n");
        if (dht_init(&sensor, &dht_params[0]) == DHT_OK) {
            LOG(LOG_INFO, "Sensor Initialized\n");
        } else {
            LOG(LOG_INFO, "Failed to Initialize Sensor\n");
            return 1;
        }
    }

    // // Start Measuring Thread
    // LOG(LOG_INFO, "Starting measurements thread\n");
    // if(MOCK_MODE == false)
    // {
    //  thread_create( gather_thread_stack, sizeof(gather_thread_stack), THREAD_PRIORITY_MAIN - 2, 0, 
    //                 gather_measurements_thread_function, NULL, "gather_measurements_thread_function");
    // }
    // else
    // {
    //  thread_create( gather_thread_stack, sizeof(gather_thread_stack), THREAD_PRIORITY_MAIN - 2, 0, 
    //                 mock_measurements_thread_function, NULL, "mock_measurements_thread_function");
    // }

    // Start GPIO Thread
    if(MOCK_MODE == false)
    {
        LOG(LOG_INFO, "Starting GPIO thread \n");
        thread_create(  gpio_thread_stack, sizeof(gpio_thread_stack), THREAD_PRIORITY_MAIN - 1, 0, 
                        gpio_loop_thread_function, NULL, "gpio_loop_thread_function");
    }

    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(commands, line_buf, SHELL_DEFAULT_BUFSIZE);
    return 0;
}
