#include <Arduino.h>

/* Task definitions ------------------------------------------------------- */
void cameraInitTask(void *pvParameters);
void connectWiFiTask(void *pvParameters);
void captureImageTask(void *pvParameters); // TODO: Create a task to capture image
void inferenceTask(void *pvParameters);
void postDataTask(void *pvParameters); // TODO: Create a task to POST data to server (5 images at a time)

/* Task handles ------------------------------------------------------- */
TaskHandle_t cameratInitTaskHandle = NULL;
TaskHandle_t connectWiFiTaskHandle = NULL;
TaskHandle_t inferenceTaskHandle = NULL;
TaskHandle_t postDataTaskHandle = NULL;

QueueHandle_t snapshotQueue;