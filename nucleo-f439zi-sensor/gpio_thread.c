#include "pcd8544.h"
#include "ztimer.h"
#include "dht.h"
#include "dht_params.h"
#include "log.h"

#define LCD_RESET           (GPIO_PIN(1, 0))
#define LCD_CS              (GPIO_PIN(1, 1))
#define LCD_MODE            (GPIO_PIN(1, 2))
// Def MOSI (LCD_DIN): PORT_A: 7
// Def SCLK (LCD_CLK): PORT_A: 5
#define SPI_INTERFACE       (SPI_DEV(0))


#define LED_ERR             (GPIO_PIN(PORT_E, 2))
#define LED_ACT             (GPIO_PIN(PORT_E, 4))
#define LED_ACK             (GPIO_PIN(PORT_E, 5))

#define LED_TIMING          100

pcd8544_t lcd_device;
dht_t sensor;
extern int16_t hum;
extern int16_t temp;

extern bool hadError;
extern bool isSending;
extern bool isActive;
extern bool configured;

void *gpio_loop_thread_function(void *arg)
{
    (void)arg;
    char hello_message[32];
    char temp_data[32];
    char hum_data[32];

    while (1)    
    {

        if(isActive && !hadError)
        {
            gpio_clear(LED_ERR);
            gpio_set(LED_ACT);
        }
        else
        {
            gpio_clear(LED_ACT);
        }

        if(hadError)
        {
            gpio_set(LED_ERR);
        }


        if(!configured)
        {
            pcd8544_clear(&lcd_device);
            sprintf(hello_message, "Laczenie...");
            pcd8544_write_s(&lcd_device, 0, 0, hello_message);
        }
        else
        {
            pcd8544_clear(&lcd_device);
            sprintf(hello_message, "Dane z sensora");
            sprintf(temp_data, "Temp.: %d,%d", temp/10, temp%10);
            sprintf(hum_data, "Wilg.: %d,%d", hum/10, hum%10);
            pcd8544_write_s(&lcd_device, 0, 0, hello_message);
            pcd8544_write_s(&lcd_device, 0, 1, temp_data);
            pcd8544_write_s(&lcd_device, 0, 2, hum_data);

            dht_read(&sensor, &temp, &hum);
        }

        ztimer_sleep(ZTIMER_MSEC, 100);
    }
    return NULL;
}

void start_gpio_loop_thread(void)
{
    {
        static char gpio_thread_stack[THREAD_STACKSIZE_MAIN];
        thread_create(gpio_thread_stack, sizeof(gpio_thread_stack), 8, 0, 
                    gpio_loop_thread_function, NULL, "gpio_loop_thread_function");
    }
}

void initialize_lcd(void)
{
    LOG(LOG_INFO, "Initializing LCD... \n");
    gpio_init(LED_ACK, GPIO_OUT);
    gpio_init(LED_ACT, GPIO_OUT);
    gpio_init(LED_ERR, GPIO_OUT);
    if (pcd8544_init(&lcd_device, SPI_INTERFACE, LCD_CS, LCD_RESET, LCD_MODE) == 0) {
        LOG(LOG_INFO, "LCD initialized\n");
    } else {
        LOG(LOG_INFO, "LCD Failed to initialize\n");
        hadError = true;
    }
    pcd8544_poweron(&lcd_device);
}

void initialize_sensor(void)
{
    LOG(LOG_INFO, "Initializing sensor... \n");
    if (dht_init(&sensor, &dht_params[0]) == DHT_OK) {
        LOG(LOG_INFO, "Sensor Initialized\n");
    } else {
        LOG(LOG_INFO, "Failed to Initialize Sensor\n");
        hadError = true;
    }
}
