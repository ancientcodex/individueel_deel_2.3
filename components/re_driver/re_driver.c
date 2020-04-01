

#include "re_driver.h"
#include "driver/i2c.h"

#include "esp_log.h"

static const char *TAG = "I2C_RE";


re_driver_err_t re_driver_initialize(RE_t *RE)
{
    esp_err_t ret;
    // setup i2c controller
        i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = RE->sda_pin,
        .scl_io_num = RE->scl_pin,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 20000};
    ret = i2c_param_config(RE->port, &conf);

    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "PARAM CONFIG FAILED");
        return RE_ERR_OK;
    }
    ESP_LOGV(TAG, "PARAM CONFIG done");

    return RE_ERR_OK;
}
re_driver_err_t re_driver_write_register(RE_t *RE, RE_reg_t reg, uint8_t v)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, RE->i2c_address << 1 | WRITE_BIT, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, reg, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, v, ACK_CHECK_EN);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(RE->port, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    if (ret == ESP_FAIL)
    {
        ESP_LOGE(TAG, "ERROR: unable to write to register");
        return RE_ERR_FAIL;
    }
    return RE_ERR_OK;
}
re_driver_err_t re_driver_read_register(RE_t *RE, RE_reg_t reg, uint8_t *data)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (RE->i2c_address << 1) | WRITE_BIT, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, reg, ACK_CHECK_EN);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(RE->port, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    if (ret == ESP_FAIL)
    {
        return RE_ERR_FAIL;
    }

    ESP_LOGI(TAG, "Read first stage: %d", ret);

    cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (RE->i2c_address << 1) | READ_BIT, ACK_CHECK_EN);
    i2c_master_read_byte(cmd, data, 1);
    ret = i2c_master_cmd_begin(RE->port, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    if (ret == ESP_FAIL)
    {
        return RE_ERR_FAIL;
    }

    ESP_LOGI(TAG, "Read second stage: %d", ret);

    return RE_ERR_OK;
}

re_driver_err_t re_driver_write_register16(RE_t *RE,RE_reg_t reg_LSB, uint16_t v)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (RE->i2c_address << 1) | WRITE_BIT, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, reg_LSB, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, v & 0xff, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, v >> 8, ACK_CHECK_EN);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(RE->port, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    if (ret == ESP_FAIL)
    {
        return RE_ERR_FAIL;
    }
    return RE_ERR_OK;
}
re_driver_err_t re_driver_read_register16(RE_t *RE,RE_reg_t reg_LSB, uint16_t *data)
{
    uint8_t LSB = 0x00;
    uint8_t MSB = 0x00;

    //MSB
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (RE->i2c_address << 1), ACK_CHECK_EN);
    i2c_master_write_byte(cmd, reg_LSB + 1, ACK_CHECK_EN);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (RE->i2c_address << 1) | READ_BIT, ACK_CHECK_EN);
    i2c_master_read_byte(cmd, &MSB, 1);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(RE->port, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    if (ret == ESP_FAIL)
    {
        return RE_ERR_FAIL;
    }
    //LSB
    cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (RE->i2c_address << 1), ACK_CHECK_EN);
    i2c_master_write_byte(cmd, reg_LSB, ACK_CHECK_EN);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (RE->i2c_address << 1) | READ_BIT, ACK_CHECK_EN);
    i2c_master_read_byte(cmd, &LSB, 1);
    i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin(RE->port, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    if (ret == ESP_FAIL)
    {
        return RE_ERR_FAIL;
    }
    uint16_t temp = ((uint16_t)MSB << 8 | LSB);
    ESP_LOGI(TAG, "data: %d", temp);
    *data = temp;
    return ret;
}

re_driver_err_t re_driver_getStatus(RE_t *RE, uint8_t *data)
{
   esp_err_t ret = re_driver_read_register(RE, STAT, &(*data));
   if (ret == ESP_FAIL)
    {
        return RE_ERR_FAIL;
    }
    return RE_ERR_OK;
}
re_driver_err_t re_driver_interruptEnable(RE_t *RE, bool v)
{
    esp_err_t ret = v ? re_driver_write_register(RE, STAT, 1) : re_driver_write_register(RE, STAT, 0);
    if (ret == ESP_FAIL)
    {
        return RE_ERR_FAIL;
    }
    return RE_ERR_OK;
}
re_driver_err_t re_driver_setColorRed(RE_t *RE, uint8_t v)
{
    esp_err_t ret = re_driver_write_register(RE, LED_BRIGHTNESS_RED, v);
    if (ret == ESP_FAIL)
    {
        return RE_ERR_FAIL;
    }
    return RE_ERR_OK;
}
re_driver_err_t re_driver_setColorGrn(RE_t *RE, uint8_t v)
{
    esp_err_t ret = re_driver_write_register(RE, LED_BRIGHTNESS_GRN, v);
    if (ret == ESP_FAIL)
    {
        return RE_ERR_FAIL;
    }
    return RE_ERR_OK;
}
re_driver_err_t re_driver_setColorBlu(RE_t *RE, uint8_t v)
{
    esp_err_t ret = re_driver_write_register(RE, LED_BRIGHTNESS_BLU, v);
    if (ret == ESP_FAIL)
    {
        return RE_ERR_FAIL;
    }
    return RE_ERR_OK;
}
re_driver_err_t re_driver_connectColorRed(RE_t *RE, uint16_t v)
{
    esp_err_t ret = re_driver_write_register16(RE, LED_CON_RED_LSB, v);
    if (ret == ESP_FAIL)
    {
        return RE_ERR_FAIL;
    }
    return RE_ERR_OK;
}
re_driver_err_t re_driver_connectColorGrn(RE_t *RE, uint16_t v)
{
    esp_err_t ret = re_driver_write_register16(RE, LED_CON_GRN_LSB, v);
    if (ret == ESP_FAIL)
    {
        return RE_ERR_FAIL;
    }
    return RE_ERR_OK;
}
re_driver_err_t re_driver_connectColorBlu(RE_t *RE, uint16_t v)
{
    esp_err_t ret = re_driver_write_register16(RE, LED_CON_BLU_LSB, v);
    if (ret == ESP_FAIL)
    {
        return RE_ERR_FAIL;
    }
    return RE_ERR_OK;
}
re_driver_err_t re_driver_getCount(RE_t *RE, uint16_t *data)
{
    esp_err_t ret = re_driver_read_register16(RE, RE_COUNT_LSB, &(*data));
    if (ret == ESP_FAIL)
    {
        return RE_ERR_FAIL;
    }
    return RE_ERR_OK;
}
re_driver_err_t re_driver_getDiff(RE_t *RE, uint16_t *data)
{

    esp_err_t ret = re_driver_read_register16(RE, RE_DIFF_LSB, &(*data));
    if (ret == ESP_FAIL)
    {
        return RE_ERR_FAIL;
    }
    ret = re_driver_write_register16(RE, RE_DIFF_LSB, 0x00);
    if (ret == ESP_FAIL)
    {
        return RE_ERR_FAIL;
    }
    return RE_ERR_OK;
}
re_driver_err_t re_driver_getTSLM(RE_t *RE, uint16_t *data)
{
    esp_err_t ret = re_driver_read_register16(RE,RE_TSLM_LSB, &(*data));
    if (ret == ESP_FAIL)
    {
        return RE_ERR_FAIL;
    }
    ret = re_driver_write_register16(RE, RE_TSLB_LSB, 0x00);
    if (ret == ESP_FAIL)
    {
        return RE_ERR_FAIL;
    }
    return RE_ERR_OK;
}
re_driver_err_t re_driver_getTSLB(RE_t *RE, uint16_t *data)
{
    esp_err_t ret = re_driver_read_register16(RE, RE_DIFF_LSB, &(*data));
    if (ret == ESP_FAIL)
    {
        return RE_ERR_FAIL;
    }
    ret = re_driver_write_register16(RE, RE_DIFF_LSB, 0x00);
    if (ret == ESP_FAIL)
    {
        return RE_ERR_FAIL;
    }
    return RE_ERR_OK;
}
bool re_driver_is_connected(RE_t *RE)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, RE->i2c_address << 1 | WRITE_BIT, ACK_CHECK_EN);
    if (i2c_master_stop(cmd) != 0)
        return (false); //Sensor did not ACK
    return (true);
}
re_driver_err_t RE_set_bit(uint8_t bit, RE_reg_t reg)
{
    // dunno
    return RE_ERR_OK;
}