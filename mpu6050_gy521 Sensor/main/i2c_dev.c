

#include <string.h>
#include "driver/i2c.h"
#include "esp_err.h"
#include "esp_log.h"

#include "i2c_dev.h"
#include "mpu6050_registers.h"
#include "mpu6050.h"

#define TAG "i2c_dev.c"

#define I2C_CLOCK_SPEED  100000

static int sda_io_pin = 21;
static int scl_io_pin = 22;

/* ----------------------------------------------------- */
bool esp32_i2c_get_sensor_address(uint8_t *sensor_address)
{
    esp_err_t ret = ESP_OK;
    uint8_t address;
    bool status = false;
    for (int i = 0; i < 128; i += 16)
    {
        ESP_LOGD(TAG,"%02x: ", i);
        for (int j = 0; j < 16; j++)
        {
            address = i + j;
            i2c_cmd_handle_t cmd = i2c_cmd_link_create();
            i2c_master_start(cmd);
            i2c_master_write_byte(cmd, (address << 1) | I2C_MASTER_WRITE, true);
            i2c_master_stop(cmd);
            ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, 50 / portTICK_RATE_MS);
            i2c_cmd_link_delete(cmd);
            if (ret == ESP_OK)
            {
                *sensor_address = address;
                ESP_LOGI(TAG,"retrived address 0x%02x : sensor address. 0x%02x ", 
                                                                    address, *sensor_address);
                status = true;
                break;
            }         
        }
    }
    if(status == false)
    {
        ESP_LOGW(TAG,"failed to retrive sensor address ");
    }
    
    return status;
}
/* ----------------------------------------------------- */

void select_register(uint8_t device_address, uint8_t register_address)
{
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (device_address << 1) | I2C_MASTER_WRITE, 1);
	i2c_master_write_byte(cmd, register_address, 1);
	i2c_master_stop(cmd);
	i2c_master_cmd_begin(I2C_NUM, cmd, 1000 / portTICK_PERIOD_MS);
	i2c_cmd_link_delete(cmd);
}
/* ----------------------------------------------------- */
int8_t esp32_i2c_read_bytes( uint8_t device_address, uint8_t register_address, 
                                                        uint8_t size, uint8_t *data)
{
	select_register(device_address, register_address);
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (device_address << 1) | I2C_MASTER_READ, 1);

	if (size > 1)
		i2c_master_read(cmd, data, size - 1, 0);

	i2c_master_read_byte(cmd, data + size - 1, 1);

	i2c_master_stop(cmd);
	i2c_master_cmd_begin(I2C_NUM, cmd, 1000 / portTICK_PERIOD_MS);
	i2c_cmd_link_delete(cmd);

	return (size);
}
/* ----------------------------------------------------- */

int8_t esp32_i2c_read_byte( uint8_t device_address, uint8_t register_address, uint8_t *data)
{
	return (esp32_i2c_read_bytes(device_address, register_address, 1, data));
}

/* ----------------------------------------------------- */

int8_t esp32_i2c_read_bits( uint8_t device_address, uint8_t register_address, 
                                            uint8_t bit_start, uint8_t size, uint8_t *data)
{
	uint8_t bit;
	uint8_t count;

	if ((count = esp32_i2c_read_byte(device_address, register_address, &bit)))
	{
		uint8_t mask = ((1 << size) - 1) << (bit_start - size + 1);

		bit &= mask;
		bit >>= (bit_start - size + 1);
		*data = bit;
	}

	return (count);
}

/* ----------------------------------------------------- */

int8_t esp32_i2c_read_bit( uint8_t device_address, uint8_t register_address, 
                                                    uint8_t bit_number, uint8_t *data)
{
	uint8_t bit;
	uint8_t count = esp32_i2c_read_byte(device_address, register_address, &bit);

	*data = bit & (1 << bit_number);

	return (count);
}

/* ----------------------------------------------------- */

bool esp32_i2c_write_bytes( uint8_t device_address, uint8_t register_address, 
                                                                uint8_t size, uint8_t *data)
{
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (device_address << 1) | I2C_MASTER_WRITE, 1);
	i2c_master_write_byte(cmd, register_address, 1);
	i2c_master_write(cmd, data, size - 1, 0);
	i2c_master_write_byte(cmd, data[size - 1], 1);
	i2c_master_stop(cmd);
	i2c_master_cmd_begin(I2C_NUM, cmd, 1000 / portTICK_PERIOD_MS);
	i2c_cmd_link_delete(cmd);

	return (true);
}

/* ----------------------------------------------------- */

bool esp32_i2c_write_byte( uint8_t device_address, uint8_t register_address, uint8_t data)
{
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (device_address << 1) | I2C_MASTER_WRITE, 1);
	i2c_master_write_byte(cmd, register_address, 1);
	i2c_master_write_byte(cmd, data, 1);
	i2c_master_stop(cmd);
	i2c_master_cmd_begin(I2C_NUM, cmd, 1000 / portTICK_PERIOD_MS);
	i2c_cmd_link_delete(cmd);

	return (true);
}

/* ----------------------------------------------------- */

bool esp32_i2c_write_bits( uint8_t device_address, uint8_t register_address, uint8_t bit_start, uint8_t size, uint8_t data)
{
	uint8_t bit = 0;

	if (esp32_i2c_read_byte(device_address, register_address, &bit) != 0)
	{
		uint8_t mask = ((1 << size) - 1) << (bit_start - size + 1);
		data <<= (bit_start - size + 1);
		data &= mask;
		bit &= ~(mask);
		bit |= data;
		return (esp32_i2c_write_byte(device_address, register_address, bit));
	}
	else
		return (false);
}

/* ----------------------------------------------------- */

bool esp32_i2c_write_bit( uint8_t device_address, uint8_t register_address, uint8_t bit_number, uint8_t data)
{
	uint8_t bit;

	esp32_i2c_read_byte(device_address, register_address, &bit);

	if (data != 0)
		bit = (bit | (1 << bit_number));
	else
		bit = (bit & ~(1 << bit_number));

	return (esp32_i2c_write_byte(device_address, register_address, bit));
}

/* ----------------------------------------------------- */

int8_t esp32_i2c_write_word( uint8_t device_address, uint8_t register_address, uint8_t data)
{
	uint8_t data_1[] = {(uint8_t)(data >> 8), (uint8_t)(data & 0xFF)};

	esp32_i2c_write_bytes(device_address, register_address, 2, data_1);

	return (1);
}

/* ----------------------------------------------------- */

/* ----------------------------------------------------- */
void esp32_i2c_set_pin(int sda_pin, int scl_pin)
{
    /* overwrite defaults pins */
    sda_io_pin = sda_pin;
    scl_io_pin = scl_pin;
}

/* ----------------------------------------------------- */
void esp32_i2c_slave_init(uint16_t slave_address)
{
	esp_err_t ret = ESP_OK;
    i2c_config_t conf_slave;
    conf_slave.sda_io_num = sda_io_pin;
    conf_slave.scl_io_num = scl_io_pin;
	conf_slave.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf_slave.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf_slave.mode = I2C_MODE_SLAVE;
    conf_slave.slave.addr_10bit_en = 0;
    conf_slave.slave.slave_addr = slave_address;
    i2c_param_config(I2C_NUM_0, &conf_slave);
    ret = i2c_driver_install(I2C_NUM_0, conf_slave.mode, 0, 0, 0);
	ESP_ERROR_CHECK(ret==ESP_OK);
}

/* ----------------------------------------------------- */
void esp32_i2c_master_init(void)
{
    esp_err_t ret = ESP_OK;
    i2c_config_t i2c_config = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = sda_io_pin,
        .scl_io_num = scl_io_pin,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_CLOCK_SPEED};

    i2c_param_config(I2C_NUM_0, &i2c_config);
    ret = i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0);
    ESP_ERROR_CHECK(ret == ESP_OK); 
    return;
}
/* ----------------------------------------------------- */
