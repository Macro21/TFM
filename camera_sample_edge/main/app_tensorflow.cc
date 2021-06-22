// #include "include/app_tensorflow.h"
// #include "include/model_settings.h"
// #include "include/model_data.h"

// #include "tensorflow/lite/micro/kernels/micro_ops.h"
// #include "tensorflow/lite/micro/micro_error_reporter.h"
// #include "tensorflow/lite/micro/micro_interpreter.h"
// #include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
// #include "tensorflow/lite/schema/schema_generated.h"
// #include "tensorflow/lite/version.h"
// #include "tensorflow/lite/micro/kernels/all_ops_resolver.h"
// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
// #include "freertos/semphr.h"
// #include "include/app_camera.h"
// #include "include/normalisation_lookup.h"
// #include "image_util.h"
// #include "mqtt_client.h"

// #define TAG_TF "APP_TENSORFLOW"
// #define GARBAGE_CLASSIFICATION_TOPIC "iot-2/type/ESP-EYE/id/ESP-EYE-001/evt/classification/fmt/txt"

// // Globals, used for compatibility with Arduino-style sketches.
// namespace
// {
// 	tflite::ErrorReporter *error_reporter = nullptr;
// 	const tflite::Model *model = nullptr;
// 	tflite::MicroInterpreter *interpreter = nullptr;

// 	// An area of memory to use for input, output, and intermediate arrays.
// 	constexpr int kTensorArenaSize = 80 * 1024;
// 	static uint8_t tensor_arena[kTensorArenaSize];
// } // namespace

// void normalise_image_buffer(float *dest_image_buffer, uint8 *imageBuffer, int size)
// {
// 	for (int i = 0; i < size; i++)
// 	{
// 		dest_image_buffer[i] = imageBuffer[i] / 255.0f;
// 	}
// }

// void tf_start_inference(void *args)
// {
// 	esp_mqtt_client_handle_t client = (esp_mqtt_client_handle_t)args;
// 	static tflite::MicroErrorReporter micro_error_reporter;
// 	error_reporter = &micro_error_reporter;

// 	model = tflite::GetModel(model_data_tflite);
// 	if (model->version() != TFLITE_SCHEMA_VERSION)
// 	{
// 		ESP_LOGE(TAG_TF, "Model provided is schema version %d %d not equal \n", model->version(), TFLITE_SCHEMA_VERSION);
// 	}

// 	static tflite::ops::micro::AllOpsResolver micro_op_resolver;

// 	static tflite::MicroInterpreter static_interpreter(
// 		model, micro_op_resolver, tensor_arena, kTensorArenaSize, error_reporter);
// 	interpreter = &static_interpreter;

// 	TfLiteStatus allocate_status = interpreter->AllocateTensors();
// 	if (allocate_status != kTfLiteOk)
// 	{
// 		ESP_LOGE(TAG_TF, "AllocateTensors() failed. \n");
// 	}

// 	ESP_LOGI(TAG_TF, "Tensores inicializados. \n");

// 	camera_fb_t *fb = NULL;

// 	while (true)
// 	{
// 		ESP_LOGI(TAG_TF, "Tomando muestra...\n");

// 		struct timeval tv_start, tv_stop;
// 		gettimeofday(&tv_start, NULL);
// 		int64_t time_us_start = (int64_t)tv_start.tv_sec * 1000000L + (int64_t)tv_start.tv_usec;

// 		fb = esp_camera_fb_get();
// 		if (!fb)
// 		{
// 			ESP_LOGE(TAG_TF, "Error al tomar la muestra desde la camara. \n");
// 		}
// 		else
// 		{
// 			if (fb->format != PIXFORMAT_JPEG)
// 			{
// 				//ESP_LOGI(TAG_TF, "Formato foto correcto\n");
// 				uint8_t *temp_buffer = (uint8_t *)malloc(kMaxImageSize);

// 				// --------------- Tensor flow part --------------------------
// 				// We need to resize our 96x96 image to 28x28 pixels as we have trained the model using this input size
// 				image_resize_linear(temp_buffer, fb->buf, kNumRows, kNumCols, kNumChannels, fb->width, fb->height);

// 				// The expected input has to be normalised for the values between 0-1
// 				normalise_image_buffer(interpreter->input(0)->data.f, temp_buffer, kMaxImageSize);

// 				// Free the memory initialised in httpd.c
// 				free(temp_buffer);

// 				// Run the model on this input and make sure it succeeds.
// 				if (kTfLiteOk != interpreter->Invoke())
// 				{
// 					ESP_LOGE(TAG_TF, "Invoke failed. \n");
// 				}

// 				TfLiteTensor *output = interpreter->output(0);

// 				//char *resultJSON = "{\"Carton\": 1, \"Metal\": 2, \"Plastic\": 3, \"Glass\": 4}";
// 				char result[156] = "";

// 				auto resultCarton = "{\"Carton\": ";
// 				auto resultMetal = "\"Metal\":";
// 				auto resultPlastic = "\"Plastic\":";
// 				auto resultGlass = "\"Glass\":";
// 				char c[50];
// 				for (int i = 0; i < kCategoryCount; i++)
// 				{
// 					ESP_LOGI(TAG_TF, "%s: %.2f \n", kCategoryLabels[i], output->data.f[i]);
// 					if (i == 0)
// 					{
// 						strcat(result, resultCarton);
// 						sprintf(c, "%g", output->data.f[i]);
// 						strcat(result, c);
// 						strcat(result, ",");
// 					}
// 					else if (i == 1)
// 					{
// 						strcat(result, resultMetal);
// 						sprintf(c, "%g", output->data.f[i]);
// 						strcat(result, c);
// 						strcat(result, ",");
// 					}
// 					else if (i == 2)
// 					{
// 						strcat(result, resultPlastic);
// 						sprintf(c, "%g", output->data.f[i]);
// 						strcat(result, c);
// 						strcat(result, ",");
// 					}
// 					else
// 					{
// 						strcat(result, resultGlass);
// 						sprintf(c, "%g", output->data.f[i]);
// 						strcat(result, c);
// 						strcat(result, "}");
// 					}
// 				}
// 				ESP_LOGI(TAG_TF, "%s \n ", "-------------------------------------");
// 				// ESP_LOGI(TAG_TF, "JSON: %s", result);
// 				int err = esp_mqtt_client_publish(client, GARBAGE_CLASSIFICATION_TOPIC, (const char *)result, strlen(result), 0, 0);
// 				if (err < 0)
// 				{
// 					ESP_LOGE(TAG_TF, "Error al enviar la imagen por MQTT \n");
// 				}
// 				gettimeofday(&tv_stop, NULL);
// 				int64_t time_us_stop = (int64_t)tv_stop.tv_sec * 1000000L + (int64_t)tv_stop.tv_usec;

// 				double microseconds = (double)(time_us_stop - time_us_start);
// 				double miliseconds = microseconds / 1000;

// 				ESP_LOGI(TAG_TF, "Muestra tomada y enviada. Tiempo %.2f ms \n", miliseconds);
// 			}
// 		}
// 		esp_camera_fb_return(fb);
// 		fb = NULL;

// 		vTaskDelay(3000 / portTICK_RATE_MS);
// 	}
// }
#include "include/app_tensorflow.h"
#include "include/model_settings.h"
#include "include/model_data.h"

#include "tensorflow/lite/micro/kernels/micro_ops.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include "tensorflow/lite/version.h"
#include "tensorflow/lite/micro/kernels/all_ops_resolver.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "include/app_camera.h"
#include "include/normalisation_lookup.h"
#include "image_util.h"
#include "mqtt_client.h"

#define TAG_TF "APP_TENSORFLOW"
#define GARBAGE_CLASSIFICATION_RESULT_TOPIC CONFIG_GARBAGE_CLASSIFICATION_RESULT_TOPIC // "iot-2/type/ESP-EYE/id/ESP-EYE-001/evt/classification/fmt/txt"
#define GARBAGE_IMAGE_TOPIC CONFIG_GARBAGE_IMAGE_TOPIC								   // "iot-2/type/ESP-EYE/id/ESP-EYE-001/evt/image/fmt/txt"

// Globals, used for compatibility with Arduino-style sketches.
namespace
{
	tflite::ErrorReporter *error_reporter = nullptr;
	const tflite::Model *model = nullptr;
	tflite::MicroInterpreter *interpreter = nullptr;

	// An area of memory to use for input, output, and intermediate arrays.
	constexpr int kTensorArenaSize = 73 * 1024;
	static uint8_t tensor_arena[kTensorArenaSize];
} // namespace

void tf_start_inference(void *args)
{
	ESP_LOGI(TAG_TF, "Tarea de inferencia.\n");

	esp_mqtt_client_handle_t client = (esp_mqtt_client_handle_t)args;
	static tflite::MicroErrorReporter micro_error_reporter;
	error_reporter = &micro_error_reporter;

	model = tflite::GetModel(model_data_tflite);
	if (model->version() != TFLITE_SCHEMA_VERSION)
	{
		ESP_LOGE(TAG_TF, "Model provided is schema version %d %d not equal \n", model->version(), TFLITE_SCHEMA_VERSION);
	}

	static tflite::ops::micro::AllOpsResolver micro_op_resolver;

	static tflite::MicroInterpreter static_interpreter(
		model, micro_op_resolver, tensor_arena, kTensorArenaSize, error_reporter);
	interpreter = &static_interpreter;

	TfLiteStatus allocate_status = interpreter->AllocateTensors();
	if (allocate_status != kTfLiteOk)
	{
		ESP_LOGE(TAG_TF, "AllocateTensors() failed. \n");
	}

	ESP_LOGI(TAG_TF, "Tensores inicializados. \n");

	camera_fb_t *fb = NULL;
	struct timeval tv_start, tv_stop;
	int64_t time_us_start;
	uint8_t temp_buffer[kMaxImageSize] = {0};

	ESP_LOGI(TAG_TF, "Input type: %d", interpreter->input(0)->type);
	ESP_LOGI(TAG_TF, "Input dims size: %d", interpreter->input(0)->dims->size);
	ESP_LOGI(TAG_TF, "Input batch: %d", interpreter->input(0)->dims->data[0]);
	ESP_LOGI(TAG_TF, "Input width: %d", interpreter->input(0)->dims->data[1]);
	ESP_LOGI(TAG_TF, "Input height: %d", interpreter->input(0)->dims->data[2]);
	ESP_LOGI(TAG_TF, "Input channels: %d", interpreter->input(0)->dims->data[3]);

	while (true)
	{
		ESP_LOGI(TAG_TF, "Tomando muestra...\n");

		gettimeofday(&tv_start, NULL);
		time_us_start = (int64_t)tv_start.tv_sec * 1000000L + (int64_t)tv_start.tv_usec;

		fb = esp_camera_fb_get();
		if (fb == NULL)
		{
			ESP_LOGE(TAG_TF, "Error al tomar la muestra desde la camara. En tf_start_inference \n");
		}
		else
		{
			ESP_LOGI(TAG_TF, "Original image len: %d", fb->len);

			// Resize image and normalize
			double scaleWidth = (double)kNumCols / (double)96;
			double scaleHeight = (double)kNumRows / (double)96;
			for (int cy = 0; cy < kNumRows; cy++)
			{
				for (int cx = 0; cx < kNumCols; cx++)
				{
					int pixel = (cy * (kNumCols * 3)) + (cx * 3);
					int nearestMatch = (((int)(cy / scaleHeight) * (96 * 3)) + ((int)(cx / scaleWidth) * 3));

					temp_buffer[pixel] = fb->buf[nearestMatch];
					temp_buffer[pixel + 1] = fb->buf[nearestMatch + 1];
					temp_buffer[pixel + 2] = fb->buf[nearestMatch + 2];
				}
			}

			for (size_t i = 0; i <= kMaxImageSize; i += 3)
			{
				interpreter->input(0)->data.f[i] = temp_buffer[i] / 255.0f;
				interpreter->input(0)->data.f[i + 1] = temp_buffer[i + 1] / 255.0f;
				interpreter->input(0)->data.f[i + 2] = temp_buffer[i + 2] / 255.0f;
			}

			// Run the model on this input and make sure it succeeds.
			if (kTfLiteOk != interpreter->Invoke())
			{
				ESP_LOGE(TAG_TF, "La inferencia ha fallado. \n");
			}

			TfLiteTensor *output = interpreter->output(0);

			//char *resultJSON = "{\"Carton\": 1, \"Metal\": 2, \"Plastic\": 3, \"Glass\": 4}";
			char result[156] = "";

			auto resultCarton = "{\"Carton\": ";
			auto resultMetal = "\"Metal\":";
			auto resultPlastic = "\"Plastic\":";
			auto resultGlass = "\"Glass\":";
			char c[50];
			for (int i = 0; i < kCategoryCount; i++)
			{
				ESP_LOGI(TAG_TF, "%s: %.2f \n", kCategoryLabels[i], output->data.f[i]);
				if (i == 0)
				{
					strcat(result, resultCarton);
					sprintf(c, "%g", output->data.f[i]);
					strcat(result, c);
					strcat(result, ",");
				}
				else if (i == 1)
				{
					strcat(result, resultMetal);
					sprintf(c, "%g", output->data.f[i]);
					strcat(result, c);
					strcat(result, ",");
				}
				else if (i == 2)
				{
					strcat(result, resultPlastic);
					sprintf(c, "%g", output->data.f[i]);
					strcat(result, c);
					strcat(result, ",");
				}
				else
				{
					strcat(result, resultGlass);
					sprintf(c, "%g", output->data.f[i]);
					strcat(result, c);
					strcat(result, "}");
				}
			}
			ESP_LOGI(TAG_TF, "%s \n ", "-------------------------------------");

			if (fb->format != PIXFORMAT_JPEG)
			{
				bool jpeg_converted = frame2jpg(fb, 80, &fb->buf, &fb->len);
				if (!jpeg_converted)
				{
					ESP_LOGE(TAG_TF, "Compresion JPEG fallida \n");
				}
			}

			int err = esp_mqtt_client_publish(client, GARBAGE_IMAGE_TOPIC, (const char *)fb->buf, fb->len, 0, 0);
			if (err < 0)
			{
				ESP_LOGE(TAG_TF, "Error al enviar la imagen por MQTT \n");
			}

			err = esp_mqtt_client_publish(client, GARBAGE_CLASSIFICATION_RESULT_TOPIC, (const char *)result, strlen(result), 0, 0);
			if (err < 0)
			{
				ESP_LOGE(TAG_TF, "Error al enviar la imagen por MQTT \n");
			}

			gettimeofday(&tv_stop, NULL);
			int64_t time_us_stop = (int64_t)tv_stop.tv_sec * 1000000L + (int64_t)tv_stop.tv_usec;

			double microseconds = (double)(time_us_stop - time_us_start);
			double miliseconds = microseconds / 1000;

			ESP_LOGI(TAG_TF, "Muestra tomada y enviada. Tiempo %.2f ms \n", miliseconds);
		}
		esp_camera_fb_return(fb);
		fb = NULL;
		extern int task_time;

		// ESP_LOGI(TAG_TF, "El tiempo de muestreo es: %d \n", task_time);

		vTaskDelay(task_time * 1000 / portTICK_RATE_MS);
	}
}