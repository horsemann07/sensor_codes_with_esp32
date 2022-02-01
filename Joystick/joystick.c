/**
 * @file joystick.c
 * @author Raghav Jha (raghavjha1531@gmail.com)
 * @brief
 * @version 0.1
 * @date 12-11-2021
 *
 * @copyright Copyright (c) 2021
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "driver/gpio.h"
#include "joystick.h"

#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG
#include "esp_log.h"

#define TAG "joystick.c"

#define DEFAULT_VREF 1100

/* defualt values */
/* ADC capture width is 12Bit. Only ESP32 is supported. */
const adc_bits_width_t width = ADC_WIDTH_BIT_10;
static esp_adc_cal_characteristics_t *adc_chars = NULL;

/* No input attenumation, ADC can measure up to approx. 800 mV. */
const adc_atten_t atten = ADC_ATTEN_DB_11;
const adc_unit_t unit = CONFIG_ADC_CHANNEL;

static xQueueHandle js_button_event = NULL;

static int adc_read_at_mid = 460;
static long axis_high = 512;
static long axis_low = -512;

static adc_channel_t x_axis_channel = ADC1_CHANNEL_6;
static adc_channel_t y_axis_channel = ADC1_CHANNEL_7;
static uint32_t button_gpio = 25;

/* --------- function prototypes -------------*/
static uint32_t readJoystickChannel(adc_channel_t channel, const char *axis);

/**************************************************************************/

/*--------------------------------------------------- */
long map(long x, long in_min, long in_max, long out_min, long out_max)
{
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}
/*--------------------------------------------------- */
void set_channel_and_button(adc_channel_t Xchannel, adc_channel_t Ychannel, const int button_pin)
{
    x_axis_channel = Xchannel;
    y_axis_channel = Ychannel;
    button_gpio = button_pin;
}
/*--------------------------------------------------- */
void set_adc_read_at_centre(int value)
{
    if (value == 0)
    {
        ESP_LOGE(TAG, ",midpoint can't set to 0 or NULL");
        return;
    }
    adc_read_at_mid = value;
}
/*--------------------------------------------------- */
void set_axis_full_range(long axisHighValue, long axisLowRange)
{
    axis_high = axisHighValue;
    axis_low = axisLowRange;
}
/*--------------------------------------------------- */
void filter_axis_values(int jX, int jY, js_axis_t *axis_value)
{

    int upper_high_range = axis_high + 10;
    int lower_high_range = axis_high - 10;
    int upper_low_range = axis_low + 10;
    int lower_low_range = axis_low - 10;

    if ((jX <= 10) && (jX >= -10))
    {
        jX = 0;
    }
    else if (jX > axis_high)
    {
        jX = axis_high;
    }
    else if (jX < axis_low)
    {
        jX = axis_low;
    }
    else if ((upper_high_range <= jX) && (jX >= lower_high_range))
    {
        jX = axis_high;
    }

    else if ((upper_low_range == jX) && (jX >= lower_low_range))
    {
        jX = axis_low;
    }
    if ((jY <= 10) && (jY >= -10))
    {
        jY = 0;
    }
    else if (jY > axis_high)
    {
        jY = axis_high;
    }
    else if (jY < axis_low)
    {
        jY = axis_low;
    }
    else if (((upper_high_range <= jY) && (jY >= lower_high_range)))
    {
        jY = axis_high;
    }

    else if ((upper_low_range == jY) && (jY >= lower_low_range))
    {
        jY = axis_low;
    }

    axis_value->jX = jX;
    axis_value->jY = jY;

    return;
}
/*--------------------------------------------------- */
check_direction_t checkDirection(int jX, int jY)
{
    check_direction_t joyStickDir = {0};
    joyStickDir.direction = UNKNWON;
    joyStickDir.strDirection = NULL;
    if (jX == 0 && jY == 0)
    {
        ESP_LOGD(TAG, "CENTRE");
        joyStickDir.strDirection = "CENTRE";
        joyStickDir.direction = CENTRE;
    }
    else if ((jX == axis_high) && (jY == 0))
    {
        ESP_LOGD(TAG, "EAST");
        joyStickDir.strDirection = "EAST";
        joyStickDir.direction = EAST;
    }
    else if ((jX == axis_low) && (jY == 0))
    {
        ESP_LOGD(TAG, "WEST");
        joyStickDir.strDirection = "WEST";
        joyStickDir.direction = WEST;
    }
    else if ((jX == 0) && (jY == axis_high))
    {
        ESP_LOGD(TAG, "NORTH");
        joyStickDir.strDirection = "NORTH";
        joyStickDir.direction = NORTH;
    }
    else if ((jX == 0) && (jY == axis_low))
    {
        ESP_LOGD(TAG, "SOUTH");
        joyStickDir.strDirection = "SOUTH";
        joyStickDir.direction = SOUTH;
    }
    // ++
    else if ((jX > 0) && (jX <= axis_high) && (jY > 0) && (jY <= axis_high))
    {
        ESP_LOGD(TAG, "NORTHEAST");
        joyStickDir.strDirection = "NORTHEAST";
        joyStickDir.direction = NORTH_EAST;
    }
    //-+
    else if ((jX < 0) && (jX >= axis_low) && (jY > 0) && (jY <= axis_high))
    {
        ESP_LOGD(TAG, "NORTHWEST");
        joyStickDir.strDirection = "NORTHWEST";
        joyStickDir.direction = NORTH_WEST;
    }
    //--
    else if ((jX < 0) && (jX >= axis_low) && (jY < 0) && (jY >= axis_low))
    {
        ESP_LOGD(TAG, "SOUTHWEST");
        joyStickDir.strDirection = "SOUTHWEST";
        joyStickDir.direction = SOUTH_WEST;
    }
    //+-
    else if ((jX > 0) && (jX <= axis_high) && (jY < 0) && (jY >= axis_low))
    {
        ESP_LOGD(TAG, "SOUTHEAST");
        joyStickDir.strDirection = "SOUTHEAST";
        joyStickDir.direction = SOUTH_EAST;
    }
    else
    {
        ESP_LOGD(TAG, "UNKNOWN");
        joyStickDir.strDirection = "UNKNOWN";
        joyStickDir.direction = UNKNWON;
    }
    return joyStickDir;
}
/*--------------------------------------------------- */
static uint32_t readJoystickChannel(adc_channel_t channel, const char *axis)
{
    // Configure ADC
    if (unit == ADC_UNIT_1)
    {
        adc1_config_width(width);
        adc1_config_channel_atten(channel, atten);
    }
    else
    {
        adc2_config_channel_atten((adc2_channel_t)channel, atten);
    }

    // Characterize ADC
    adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
    esp_adc_cal_value_t val_type = esp_adc_cal_characterize(unit, atten, width, DEFAULT_VREF, adc_chars);

    uint32_t adc_reading = 0;

    if (unit == ADC_UNIT_1)
    {
        adc_reading = adc1_get_raw((adc1_channel_t)channel);
    }
    else
    {
        int raw;
        adc2_get_raw((adc2_channel_t)channel, width, &raw);
        adc_reading = raw;
    }
#if (LOG_LOCAL_LEVEL == ESP_LOG_DEBUG)
    // Convert adc_reading to voltage in mV
    uint32_t voltage = esp_adc_cal_raw_to_voltage(adc_reading, adc_chars);
    ESP_LOGD(TAG, "%s: Raw: %d\tVoltage: %dmV", axis, adc_reading, voltage);
    return adc_reading;
#else
    return adc_reading;
#endif
}
/*--------------------------------------------------- */

void joystick_task(void *arg)
{
    (void)arg;
    js_axis_t axis_value = {0};
    check_direction_t direction = {0};
    uint32_t js_button;
    while (true)
    {
        int jXRead = 0, jYRead = 0, jX = 0, jY = 0;

        jXRead = (int)readJoystickChannel(x_axis_channel, "x-axis");
        jYRead = (int)readJoystickChannel(y_axis_channel, "y-axis");

        ESP_LOGD(TAG, ":adc x-axis: %d\ty-axis: %d", jXRead, jYRead);

        jX = (int)map(jXRead, 0, adc_read_at_mid * 2, axis_low, axis_high);
        jY = (int)map(jYRead, 0, adc_read_at_mid * 2, axis_low, axis_high);

        filter_axis_values(jX, jY, &axis_value);
        jX = axis_value.jX;
        jY = axis_value.jY;
        ESP_LOGI(TAG, "x-axis: %d\ty-axis: %d", jX, jY);
        direction = checkDirection(jX, jY);
        ESP_LOGI(TAG, "direction %d: %s", direction.direction, direction.strDirection);

        if (xQueueReceive(js_button_event, &js_button, pdMS_TO_TICKS(1000)) == pdPASS)
        {
            ESP_LOGI(TAG, "GPIO[%d] intr, val: %d\n", js_button, gpio_get_level(js_button));
        }
    }
    vTaskDelete(NULL);
}
/*--------------------------------------------------------*/
void joystick_button_config(void)
{
    gpio_config_t joystick_button_config;
    joystick_button_config.intr_type = GPIO_PIN_INTR_POSEDGE;
    joystick_button_config.mode = GPIO_MODE_INPUT;
    joystick_button_config.pin_bit_mask = (1ULL << button_gpio);
    joystick_button_config.pull_down_en = 1;
    joystick_button_config.pull_up_en = 0;

    gpio_config(&joystick_button_config);

    gpio_install_isr_service(0);
    gpio_isr_handler_add(button_gpio, gpio_isr_handler, (void *)button_gpio);
}

static void IRAM_ATTR gpio_isr_handler(void *arg)
{
    uint32_t gpio_num = (uint32_t)arg;

    /* use queue to send the data of the pin */
    xQueueSendFromISR(js_button_event, &gpio_num, NULL);
}

void joystick_config(void)
{
    set_channel_and_button(ADC1_CHANNEL_6, ADC1_CHANNEL_7, 25);
    set_axis_full_range(512, -512);
    
    joystick_button_config();

    js_button_event = xQueueCreate(10, sizeof(uint32_t));
    if(js_button_event == NULL)
    {
        ESP_LOGE(TAG," failed to create the joystick button interrupt queue.");
    }
}
/*--------------------------------------------------- */
void app_main(void)
{
    // BaseType_t xReturn = pdPASS;
    // xReturn = xTaskCreate(joystick_task,"JoySticK",1024*3, NULL, 5, NULL);
    // if(xReturn == pdFAIL)
    // {
    //     ESP_LOGE(TAG,"failed to create JoyStick task ");
    // }
    joystick_config();
    joystick_task(NULL);
}
/*--------------------------------------------------- */