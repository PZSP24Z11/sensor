#include "pcd8544.h"
#include "ztimer.h"
#include "log.h"

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

pcd8544_t lcd_device;
extern int16_t hum;
extern int16_t temp;

char gpio_thread_stack[THREAD_STACKSIZE_DEFAULT];

void *gpio_loop_thread_function(void *arg)
{
    (void)arg;
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
        printf("LCD Updated: %s, %s\n", temp_data, hum_data);
    }
    return NULL;
}

void start_gpio_loop_thread(void)
{
    thread_create(gpio_thread_stack, sizeof(gpio_thread_stack), THREAD_PRIORITY_MAIN - 1, 0, 
                gpio_loop_thread_function, NULL, "gpio_loop_thread_function");
}

void initialize_lcd(void)
{
    LOG(LOG_INFO, "Initializing LCD... \n");
    if (pcd8544_init(&lcd_device, SPI_INTERFACE, LCD_CS, LCD_RESET, LCD_MODE) == 0) {
        LOG(LOG_INFO, "LCD initialized\n");
    } else {
        LOG(LOG_INFO, "LCD Failed to initialize\n");
    }

    pcd8544_poweron(&lcd_device);
}