/* Play M3U HTTP Living stream

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "sdkconfig.h"
#include "audio_element.h"
#include "audio_pipeline.h"
#include "audio_event_iface.h"
#include "audio_common.h"
#include "http_stream.h"
#include "i2s_stream.h"
#include "mp3_decoder.h"

#include "esp_peripherals.h"
#include "periph_wifi.h"
#include "periph_adc_button.h"
#include "board.h"

#include "re_driver.h"

static const char *TAG = "HTTP_LIVINGSTREAM_EXAMPLE";

RE_t RE;

#define mp3_STREAM_URI "https://icecast-qmusicnl-cdp.triple-it.nl/Qmusic_nl_nonstop_96.mp3"

int RUNNING = 1;
int currentstation = 0;

char stations[5][128] = {
    "https://icecast-qmusicnl-cdp.triple-it.nl/Qmusic_nl_nonstop_96.mp3",
    "https://19643.live.streamtheworld.com/TLPSTR13.mp3",
    "http://19993.live.streamtheworld.com/RADIO10.mp3",
    "https://19993.live.streamtheworld.com/SRGSTR14.mp3",
    "https://20043.live.streamtheworld.com/SRGSTR01.mp3"
    };

void radio_stop();

audio_pipeline_handle_t pipeline;
audio_element_handle_t http_stream_reader, i2s_stream_writer, mp3_decoder;
esp_periph_set_handle_t set;
audio_event_iface_handle_t evt;

int _http_stream_event_handle(http_stream_event_msg_t *msg)
{
    if (msg->event_id == HTTP_STREAM_RESOLVE_ALL_TRACKS)
    {
        return ESP_OK;
    }
    if (msg->event_id == HTTP_STREAM_FINISH_TRACK)
    {
        return http_stream_next_track(msg->el);
    }
    if (msg->event_id == HTTP_STREAM_FINISH_PLAYLIST)
    {
        return http_stream_restart(msg->el);
    }
    return ESP_OK;
}

void wait_wifi()
{
    ESP_LOGI(TAG, "[ 3 ] Start and wait for Wi-Fi network");
    esp_periph_config_t periph_cfg = DEFAULT_ESP_PERIPH_SET_CONFIG();
    set = esp_periph_set_init(&periph_cfg);
    periph_wifi_cfg_t wifi_cfg = {
        .ssid = CONFIG_WIFI_SSID,
        .password = CONFIG_WIFI_PASSWORD,
    };
    esp_periph_handle_t wifi_handle = periph_wifi_init(&wifi_cfg);
    esp_periph_start(set, wifi_handle);
    periph_wifi_wait_for_connected(wifi_handle, portMAX_DELAY);
}
void radio_init()
{
    //configuration of elements like streams and the audiocodec itself
    ESP_LOGI(TAG, "[ 1 ] Start audio codec chip");
    audio_board_handle_t board_handle = audio_board_init();
    audio_hal_ctrl_codec(board_handle->audio_hal, AUDIO_HAL_CODEC_MODE_DECODE, AUDIO_HAL_CTRL_START);

    ESP_LOGI(TAG, "[2.0] Create audio pipeline for playback");
    audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
    pipeline = audio_pipeline_init(&pipeline_cfg);

    ESP_LOGI(TAG, "[2.1] Create http stream to read data");
    http_stream_cfg_t http_cfg = HTTP_STREAM_CFG_DEFAULT();
    http_cfg.event_handle = _http_stream_event_handle;
    http_cfg.type = AUDIO_STREAM_READER;
    http_cfg.enable_playlist_parser = true;
    http_stream_reader = http_stream_init(&http_cfg);

    ESP_LOGI(TAG, "[2.2] Create i2s stream to write data to codec chip");
    i2s_stream_cfg_t i2s_cfg = I2S_STREAM_CFG_DEFAULT();
    i2s_cfg.type = AUDIO_STREAM_WRITER;
    i2s_stream_writer = i2s_stream_init(&i2s_cfg);

    ESP_LOGI(TAG, "[2.3] Create mp3 decoder to decode mp3 file");
    mp3_decoder_cfg_t mp3_cfg = DEFAULT_MP3_DECODER_CONFIG();
    mp3_decoder = mp3_decoder_init(&mp3_cfg);

    ESP_LOGI(TAG, "[2.4] Register all elements to audio pipeline");
    audio_pipeline_register(pipeline, http_stream_reader, "http");
    audio_pipeline_register(pipeline, mp3_decoder, "mp3");
    audio_pipeline_register(pipeline, i2s_stream_writer, "i2s");

    ESP_LOGI(TAG, "[2.5] link elements to audio pipeline");
    audio_pipeline_link(pipeline, (const char *[]){"http", "mp3", "i2s"}, 3);

    ESP_LOGI(TAG, "[2.6] Set up  uri (http as http_stream, mp3 as mp3 decoder, and default output is i2s)");
    audio_element_set_uri(http_stream_reader, mp3_STREAM_URI);
    //check if wifi connected
    wait_wifi();
    //configuration of events & peripherals
    ESP_LOGI(TAG, "[ 4 ] Set up  event listener");
    audio_event_iface_cfg_t evt_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();
    evt = audio_event_iface_init(&evt_cfg);

    ESP_LOGI(TAG, "[4.1] Listening event from all elements of pipeline");
    audio_pipeline_set_listener(pipeline, evt);

    ESP_LOGI(TAG, "[4.2] Listening event from peripherals");
    audio_event_iface_set_listener(esp_periph_set_get_event_iface(set), evt);

    ESP_LOGW(TAG, "[XX] SETUP DONE!");
}

void radio_deinit()
{
    audio_pipeline_unregister(pipeline, http_stream_reader);
    audio_pipeline_unregister(pipeline, i2s_stream_writer);
    audio_pipeline_unregister(pipeline, mp3_decoder);
    /* Stop all peripherals before removing the listener */
    esp_periph_set_stop_all(set);
    audio_event_iface_remove_listener(esp_periph_set_get_event_iface(set), evt);
    /* Make sure audio_pipeline_remove_listener & audio_event_iface_remove_listener are called before destroying event_iface */
    audio_event_iface_destroy(evt);
    /* Release all resources */
    audio_pipeline_deinit(pipeline);
    audio_element_deinit(http_stream_reader);
    audio_element_deinit(i2s_stream_writer);
    audio_element_deinit(mp3_decoder);
    esp_periph_set_destroy(set);
}

void radio_run(void *args)
{
    RUNNING = 1;
    ESP_LOGW(TAG, "[XX] STARTING RADIO");
    audio_pipeline_run(pipeline);
    //init variables
    audio_event_iface_msg_t msg;
    audio_element_info_t music_info = {0};

    while (RUNNING)
    {
        esp_err_t ret = audio_event_iface_listen(evt, &msg, portMAX_DELAY);
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "[ * ] Event interface error : %d", ret);
            continue;
        }
        if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT && msg.source == (void *)mp3_decoder && msg.cmd == AEL_MSG_CMD_REPORT_MUSIC_INFO)
        {
            audio_element_getinfo(mp3_decoder, &music_info);

            ESP_LOGI(TAG, "[ * ] Receive music info from mp3 decoder, sample_rates=%d, bits=%d, ch=%d",
                     music_info.sample_rates, music_info.bits, music_info.channels);

            audio_element_setinfo(i2s_stream_writer, &music_info);
            i2s_stream_set_clk(i2s_stream_writer, music_info.sample_rates, music_info.bits, music_info.channels);
            continue;
        }
        if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT && msg.source == (void *)http_stream_reader && msg.cmd == AEL_MSG_CMD_REPORT_STATUS && (int)msg.data == AEL_STATUS_ERROR_OPEN)
        {
            ESP_LOGW(TAG, "[ * ] Restart stream");

            continue;
        }
    }
    vTaskDelete(NULL);
}

void radio_start()
{
    xTaskCreate(radio_run, "radio", 8 * 1024, NULL, 5, NULL);
}

void radio_stop()
{
    RUNNING = 0;
    audio_pipeline_terminate(pipeline);
    audio_pipeline_remove_listener(pipeline);
}

void radio_restart()
{
    radio_stop();
    vTaskDelay(100 / portTICK_RATE_MS);
    radio_start();
}

void radio_switch_station(char *url)
{
    radio_stop();
    audio_element_set_uri(http_stream_reader, url);
    radio_start();
}

void re_task(void *param)
{
    uint16_t data;
    while (1)
    {
        re_driver_getDiff(&RE, &data);
        if (data > 0)
        {
            if (currentstation == 5)
            {
               currentstation = -1;
            }
            currentstation = currentstation + 1;
            radio_switch_station(stations[currentstation]);
        }
    }
    vTaskDelete(NULL);
}

// Example on how to use radio
void app_main(void)
{
    // standard bootup
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    tcpip_adapter_init();
    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set(TAG, ESP_LOG_DEBUG);

    RE.i2c_address = RE_I2C_ADDR;
    RE.sda_pin = 18;
    RE.scl_pin = 23;

    //start radio
    radio_init();
    re_driver_initialize(&RE);
    radio_start();
    xTaskCreate(re_task, "re_driver", 8 * 1024,NULL, 5, NULL);
    int time = 0;
    while (1)
    {
        ESP_LOGW(TAG, "[XX] PROGRAM IS RUNNING FOR: %d MIN!", time);
        vTaskDelay(60000 / portTICK_RATE_MS);
        time = time + 1;
    }
    radio_stop();
    radio_deinit();
    while (1)
    {
    }
}
