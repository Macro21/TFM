/* ESPRESSIF MIT License
 * 
 * Copyright (c) 2018 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
 * 
 * Permission is hereby granted for use on all ESPRESSIF SYSTEMS products, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include <stdio.h>
#include <stddef.h>
#include "app_camera.h"
#include "app_wifi.h"
//#include "app_mdns.h"
//#include "app_sample.h"
#include "esp_sntp.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "include/app_tensorflow.h"

#define TAG "APP_MAIN"

#define SAMPLE_FREQ CONFIG_SAMPLE_FREQ
#define MQTT_KEEPALIVE CONFIG_MQTT_KEEPALIVE
#define MQTT_CLIENT_ID CONFIG_MQTT_CLIENT_ID
#define MQTT_USERNAME CONFIG_MQTT_USERNAME
#define MQTT_PASSWORD CONFIG_MQTT_PASSWORD
#define MQTT_HOST CONFIG_MQTT_HOST
#define MQTT_CONFIG_TOPIC CONFIG_MQTT_CONFIG_TOPIC

static esp_mqtt_client_handle_t client;

int task_time = 5;

void mqtt_error_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;

    ESP_LOGE(TAG, "Ha ocurrido un error. Event_id: %d", event->event_id);
}

void mqtt_disconnected_event_handler()
{
    ESP_LOGW(TAG, "Dispositivo desconectado del broker de MQTT\n");
}

void mqtt_data_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
    ESP_LOGI(TAG, "Cambiando tiempo de muestreo a %d segundos.", atoi(event->data));
    task_time = atoi(event->data);
}

void mqtt_connected_event_handler()
{
    TaskHandle_t *inference_task = NULL;

    ESP_LOGI(TAG, "Conexión establecida con IBM Watson IoT.\n");
    char str[10];
    sprintf(str, "%d", SAMPLE_FREQ);
    int err = esp_mqtt_client_publish(client, MQTT_CONFIG_TOPIC, str, strlen(str), 2, 1);
    if (err < 0)
    {
        ESP_LOGE(TAG, "Error al enviar los parámetros por MQTT \n");
    }

    err = esp_mqtt_client_subscribe(client, MQTT_CONFIG_TOPIC, 1);
    if (err < 0)
    {
        ESP_LOGE(TAG, "Error al suscribirse a la configuracion por MQTT \n");
    }

    ESP_LOGI(TAG, "Clasificación de residuos en proceso...\n");
    //tf_start_inference(client);
    // Start classification task
    BaseType_t taskResult = xTaskCreate(&tf_start_inference, "tf_start_inference", 1024 * 15, client, configMAX_PRIORITIES, inference_task);
    if (taskResult != pdPASS)
    {
        ESP_LOGE(TAG, "Error al crear la tarea de inferencia \n");
        vTaskDelete(inference_task);
    }
}

void mqtt_connect_to_broker(void *connected_callback, void *disconnected_callback)
{
    char uri_buf[256];
    snprintf(uri_buf, sizeof(uri_buf), "%s%s%s%s%s%s", "mqtt://", MQTT_USERNAME, ":", MQTT_PASSWORD, "@", MQTT_HOST);
    esp_log_level_set("*", ESP_LOG_INFO);

    //strcat("mqtt://", MQTT_USERNAME, ":", MQTT_PASSWORD, "@", MQTT_HOST);
    esp_mqtt_client_config_t mqtt_cfg = {
        .uri = uri_buf, //"mqtt://a-p468nr-rhbjswi0sq:lW&gyr+6+(tZfBpUcP@p468nr.messaging.internetofthings.ibmcloud.com",
        .client_id = MQTT_CLIENT_ID,
        .keepalive = MQTT_KEEPALIVE,
    };

    client = esp_mqtt_client_init(&mqtt_cfg);
    if (client == NULL)
    {
        ESP_LOGE(TAG, "El cliente no se ha inicializado correctamente");
    }
    ESP_ERROR_CHECK(esp_mqtt_client_start(client));

    esp_mqtt_client_register_event(client, MQTT_EVENT_ERROR, mqtt_error_event_handler, client);
    esp_mqtt_client_register_event(client, MQTT_EVENT_CONNECTED, connected_callback, client);
    esp_mqtt_client_register_event(client, MQTT_EVENT_DISCONNECTED, disconnected_callback, client);
    esp_mqtt_client_register_event(client, MQTT_EVENT_DATA, mqtt_data_event_handler, client);
}

void wifi_connected_event_handler()
{
    ESP_LOGI(TAG, "Se ha realizado la conexión Wi-Fi con éxito. \n");
    ESP_LOGI(TAG, "Sincronizando fecha...\n");

    // Sincroniza la fecha
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_init();

    ESP_LOGI(TAG, "Fecha sincronizada correctamente. \n");

    ESP_LOGI(TAG, "Conectando con IBM Watson IoT...\n");

    mqtt_connect_to_broker((void *)mqtt_connected_event_handler, (void *)mqtt_disconnected_event_handler);
}

void app_main()
{
    app_camera_init();
    //xTaskCreate(&tf_start_inference, "tf_start_inference", 1024 * 20, client, configMAX_PRIORITIES, NULL);
    app_wifi_main((void *)wifi_connected_event_handler);
}
