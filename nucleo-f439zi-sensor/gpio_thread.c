#include "pcd8544.h"
#include "ztimer.h"
#include "log.h"

#define LCD_RESET           (GPIO_PIN(1, 0))
#define LCD_CS              (GPIO_PIN(1, 1))
#define LCD_MODE            (GPIO_PIN(1, 2))
// Def MOSI (LCD_DIN): PORT_A: 7
// Def SCLK (LCD_CLK): PORT_A: 5
#define SPI_INTERFACE       (SPI_DEV(0))

pcd8544_t lcd_device;
extern int16_t hum;
extern int16_t temp;


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
    {
        static char gpio_thread_stack[THREAD_STACKSIZE_DEFAULT];
        thread_create(gpio_thread_stack, sizeof(gpio_thread_stack), THREAD_PRIORITY_MAIN + 1, 0, 
                    gpio_loop_thread_function, NULL, "gpio_loop_thread_function");
    }
}

void initialize_lcd(void)
{
    LOG(LOG_INFO, "Initializing LCD... \n");
    ztimer_sleep(ZTIMER_MSEC, 2000);
    if (pcd8544_init(&lcd_device, SPI_INTERFACE, LCD_CS, LCD_RESET, LCD_MODE) == 0) {
        LOG(LOG_INFO, "LCD initialized\n");
    } else {
        LOG(LOG_INFO, "LCD Failed to initialize\n");
    }
    pcd8544_poweron(&lcd_device);
}