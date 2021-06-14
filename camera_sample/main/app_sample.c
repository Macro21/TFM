#include "esp_timer.h"
#include "esp_camera.h"
#include "img_converters.h"
#include "fb_gfx.h"
#include "driver/ledc.h"
#include "sdkconfig.h"
#include "app_camera.h"
#include "esp_log.h"
#include "app_sample.h"
#include "esp_err.h"
#include <stdlib.h>
#include "nvs_flash.h"
#include "mqtt_client.h"
#include "time.h"
#include <stdio.h>
#include <inttypes.h>

#define GARBAGE_CLASSIFICATION_TOPIC CONFIG_GARBAGE_CLASSIFICATION_TOPIC
static const char *TAG = "APP_SAMPLE";

void monitor(void *args)
{

    esp_mqtt_client_handle_t client = (esp_mqtt_client_handle_t)args;
    camera_fb_t *fb = NULL;
    size_t _jpg_buf_len = 0;
    uint8_t *_jpg_buf = NULL;

    ESP_LOGI(TAG, "Tomando muestra... \n");

    struct timeval tv_start, tv_stop;
    gettimeofday(&tv_start, NULL);
    int64_t time_us_start = (int64_t)tv_start.tv_sec * 1000000L + (int64_t)tv_start.tv_usec;

    fb = esp_camera_fb_get();
    if (!fb)
    {
        ESP_LOGE(TAG, "Error al tomar la muestra desde la camara. \n");
    }

    if (fb->format != PIXFORMAT_JPEG)
    {
        bool jpeg_converted = frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len);
        if (!jpeg_converted)
        {
            ESP_LOGE(TAG, "Compresion JPEG fallida \n");
        }
    }
    else
    {
        _jpg_buf_len = fb->len;
        _jpg_buf = fb->buf;
    }

    int err = esp_mqtt_client_publish(client, GARBAGE_CLASSIFICATION_TOPIC, (const char *)_jpg_buf, _jpg_buf_len, 0, 0);
    if (err < 0)
    {
        ESP_LOGE(TAG, "Error al enviar la imagen por MQTT \n");
    }
    esp_camera_fb_return(fb);
    gettimeofday(&tv_stop, NULL);
    int64_t time_us_stop = (int64_t)tv_stop.tv_sec * 1000000L + (int64_t)tv_stop.tv_usec;

    double microseconds = (double)(time_us_stop - time_us_start);
    double miliseconds = microseconds / 1000;

    ESP_LOGI(TAG, "Muestra tomada y enviada. Tiempo %.2f ms \n", miliseconds);
}