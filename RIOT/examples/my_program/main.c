#include "stdio.h"
#include "stdlib.h"
#include "shell.h"
#include "led.h"
#include <periph/gpio.h>

#include "ztimer.h"
#include "dht.h"
#include "dht_params.h"
#include "ds18.h"
#include "ds18_params.h"
#include "pcd8544.h"
#include "log.h"

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

pcd8544_t lcd_device;

static int gpio_command(int argc, char **argv)
{
    if (argc < 4) {
        printf("usage: %s <init/set/clear> <port no.> <pin no.>\n", argv[0]);
        return -1;
    }

    int port_no = atoi(argv[2]);
    int pin_no = atoi(argv[3]);

    if (strcmp(argv[1], "init") == 0) {
        printf("GPIO initialization PORT %d, PIN %d\n", port_no, pin_no);

        int result;

        result = gpio_init(GPIO_PIN(port_no, pin_no), GPIO_OUT);

        if (result == 0) {
            printf("Success!\n");
        }
        else {
            printf("Failure!\n");
        }
    }
    else if (strcmp(argv[1], "set") == 0) {
         printf("Set HIGH to PORT %d, PIN %d\n", port_no, pin_no);
         gpio_set(GPIO_PIN(port_no, pin_no));
    }
    else if (strcmp(argv[1], "clear") == 0) {
         printf("Set LOW to PORT %d, PIN %d\n", port_no, pin_no);
         gpio_clear(GPIO_PIN(port_no, pin_no));
    }
    else {
        printf("usage: %s <init/set/clear> <port no.> <pin no.>\n", argv[0]);
    }

    return 0;
}

static int led_command(int argc, char **argv)
{
    if (argc < 3) {
        printf("usage: %s <id> <on|off|toggle>\n", argv[0]);
        return -1;
    }

    int led_id = atoi(argv[1]);

    if (led_id >= LED_NUMOF) {
        printf("This board has %d LEDs\n", LED_NUMOF);
        return -1;
    }

    if (strcmp(argv[2], "on") == 0) {
        led_on(led_id);
    }
    else if (strcmp(argv[2], "off") == 0) {
        led_off(led_id);
    }
    else if (strcmp(argv[2], "toggle") == 0) {
        led_toggle(led_id);
    }
    else {
        printf("usage: %s <id> <on|off|toggle>\n", argv[0]);
    }

    return 0;
}

static int sensor_DHT_command(int argc, char **argv)
{
    if(argc < 2){
        printf("write number as argument\n");
        return -1;
    }
    int iterations = atoi(argv[1]);
    
    printf("PROGRAM SENSOR STARTED\n");

    dht_t sensor;
    int16_t hum = 0;
    int16_t temp = 0;

    if (dht_init(&sensor, &dht_params[0]) == DHT_OK) {
        printf("Sensor Initialized\n");
    }
    else 
    {
        puts("Fail to Initialize Sensor\n");
        return 1;
    }

    for(int i = 0; i < iterations; i++)
    {
        ztimer_sleep(ZTIMER_MSEC, 1000);
        if(dht_read(&sensor, &temp, &hum) == DHT_OK)
        {
            printf("DHT values - temp: %d.%d°C - relative humidity: %d.%d%%\n",
                temp/10, temp%10, hum/10, hum%10);
        }
        else
        {
            printf("Error reading sensor output!\n");
        }

    }

    printf("PROGRAM SENSOR ENDED\n");
    return 0;
}

static int buttons_command(int argc, char** argv)
{
    if(argc > 1) {printf("No need for any arguments: %s",argv[1]);}
    printf("Buttons!\n");


    // Leds
    gpio_init(LED_CONNECTING_PIN, GPIO_OUT);
    gpio_init(LED_ACTIVE_PIN, GPIO_OUT);
    gpio_init(LED_IDLE_PIN, GPIO_OUT);
    gpio_init(LED_ERROR_PIN, GPIO_OUT);
    // Buttons
    gpio_init(BUTTON_DOWN_PIN, GPIO_IN);
    gpio_init(BUTTON_UP_PIN, GPIO_IN);
    gpio_init(BUTTON_CONFIRM_PIN, GPIO_IN);

    uint32_t pulsing_freq_mseconds = 500;
    uint32_t last_toggle_time = ztimer_now(ZTIMER_MSEC);

    while(true)
    {
          
        if(gpio_read(BUTTON_UP_PIN))
        {
            gpio_set(LED_ERROR_PIN);
        }
        else
        {
            gpio_clear(LED_ERROR_PIN);
        }

        if(gpio_read(BUTTON_DOWN_PIN))
        {
            gpio_set(LED_ACTIVE_PIN);
        }
        else
        {
            gpio_clear(LED_ACTIVE_PIN);
        }

        if(gpio_read(BUTTON_CONFIRM_PIN))
        {
            gpio_set(LED_IDLE_PIN);
        }
        else
        {
            gpio_clear(LED_IDLE_PIN);
        }

        uint32_t current_time = ztimer_now(ZTIMER_MSEC);
        if(current_time - last_toggle_time  >= pulsing_freq_mseconds)
        {
            gpio_toggle(LED_CONNECTING_PIN);
            last_toggle_time = current_time;
        }
    }

    printf("End :(\n");
    return 0;
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
    { "gpio", "GPIO pin initialization and set port state HIGH/LOW", gpio_command },
    { "led", "Switch on/off or toggle on-board LEDs", led_command},
    { "sensor", "Turn on DHT sensor program", sensor_DHT_command},
    { "buttons", "Turn on DHT sensor program", buttons_command},
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
    char line_buf[SHELL_DEFAULT_BUFSIZE];
    printf("This board has %d LEDs\n", LED_NUMOF);
    
    LOG(LOG_INFO, "Initializing LCD... \n");
    if (pcd8544_init(&lcd_device, SPI_INTERFACE, LCD_CS, LCD_RESET, LCD_MODE) == 0) 
    {
        LOG(LOG_INFO, "LCD initialized\n");
    }
    else
    {
        LOG(LOG_INFO, "LCD Failed to initialize\n");
    }
    shell_run(commands, line_buf, SHELL_DEFAULT_BUFSIZE);

    return 0;
}
