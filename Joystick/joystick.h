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

#define CONFIG_ADC_CHANNEL  ADC_UNIT_1

typedef enum 
{
    UNKNWON = -1,
    CENTRE = 0,
    NORTH = 1,
    NORTH_EAST,
    NORTH_WEST,
    SOUTH,
    SOUTH_EAST,
    SOUTH_WEST,
    EAST,
    WEST,
}direction_t;

typedef struct 
{
    char *strDirection;
    direction_t direction;
}check_direction_t;

typedef struct js_axis
{
    int jX;
    int jY;
}js_axis_t;

long map(long x, long in_min, long in_max, long out_min, long out_max);
void set_adc_read_at_centre(int value);
void set_axis_full_range(long axisHighValue, long axisLowRange);
void filter_axis_values(int jX, int jY, js_axis_t *axis_value);
void set_channel_and_button(adc_channel_t Xchannel, adc_channel_t Ychannel, const int button_pin);
check_direction_t checkDirection(int jX, int jY);