#include <exortix-project-1_inferencing.h>
#include "edge-impulse-sdk/dsp/image/image.hpp"
#include <Arduino.h>
#include "esp_camera.h"
#include "camera_config.h"
#include <WiFi.h>
#include <WiFiClient.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include <HTTPClient.h>

/* Todo List
Todo: POST to PHP server
Todo: Modify to Include More FreeRTOS Tasks
Todo: Make the Design of The Website Prettier
Todo: Break the Code into Small Chunks
Todo: Work on Edge Impulse Model
Todo: Modify to Include FreeRTOS Queues for POST
*/

/* Private variables ------------------------------------------------------- */
static bool debug_nn = false; // Set this to true to see e.g. features generated from the raw signal
static bool is_initialised = false;
uint8_t *snapshot_buf; // points to the output of the capture

/* Function definitions ------------------------------------------------------- */
bool ei_camera_init(void);
void ei_camera_deinit(void);
bool ei_camera_capture(uint32_t img_width, uint32_t img_height, uint8_t *out_buf);
static int ei_camera_get_data(size_t offset, size_t length, float *out_ptr);

void captureAndPostImage(void);
/**/
// void captureImage(void);
// bool uploadImage(uint8_t *buf, size_t img_width, size_t img_height);
// bool uploadImage(camera_fb_t *fb);
/**/

/* Task definitions ------------------------------------------------------- */
//
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

/* WiFi definitions ------------------------------------------------------- */
const char *ssid = "Fares";
const char *password = "fareS123";

String serverName = "192.168.1.13";
String serverPath = "/upload.php";
const int serverPort = 8080;
String serverURL = "http://" + serverName + ":" + serverPort + serverPath;

WiFiClient client;
HTTPClient http;
const int timerInterval = 2000;   // time between each HTTP POST image
unsigned long previousMillis = 0; // last time image was sent

void setup()
{
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); // ? Disable brownout detector

    // ? Replace with FreeRTOS task waiting for Serial to be ready
    // ? Sends notification to cameraInitTask and connectWiFiTask to start
    // ? SerialSetupTask();
    // ! If WebServer is used, then SerialSetupTask is not needed
    Serial.begin(115200);
    while (!Serial)
        ;

    Serial.println("Edge Impulse Inferencing Demo");
    Serial.println("Starting setup");
    snapshotQueue = xQueueCreate(2, sizeof(camera_fb_t));

    xTaskCreate(
        cameraInitTask,
        "cameraInitTask",
        10000, // Stack size (bytes)
        NULL,  // Parameter to pass
        1,     // Task priority
        NULL); // Task handle

    xTaskCreate(
        connectWiFiTask,
        "ConnectWiFiTask",
        10000,
        NULL,
        1,
        &connectWiFiTaskHandle);

    xTaskCreate(
        inferenceTask,
        "InferenceTask",
        10000,
        NULL,
        1,
        &inferenceTaskHandle);

    // xTaskCreate(
    //     captureImageTask,
    //     "CaptureImageTask",
    //     10000,
    //     NULL,
    //     1,
    //     NULL);

    xTaskCreate(
        postDataTask,
        "postDataTask",
        10000,
        NULL,
        2,
        &postDataTaskHandle);
}

// @brief Connect to WiFi network
void connectWiFiTask(void *pvParameters)
{
    WiFi.mode(WIFI_STA);
    Serial.print("\nConnecting to ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print(".");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    Serial.print("\nESP32-CAM IP Address: ");
    Serial.println(WiFi.localIP()); // Show ESP32 IP on serial
    xTaskNotifyGive(inferenceTaskHandle);
    vTaskDelete(NULL);
}

// @brief Initialize camera
void cameraInitTask(void *pvParameters)
{
    ei_camera_init() ? ei_printf("Camera initialized\r\n")
                     : ei_printf("Failed to initialize Camera!\r\n");
    ei_printf("\nStarting continuous inference now...\n");
    xTaskNotifyGive(inferenceTaskHandle);
    vTaskDelete(NULL);
}

/**
 * @brief      Get data and run inferencing
 *
 * @param[in]  debug  Get debug info if true
 */
void inferenceTask(void *pvParameters)
{
    while (1)
    {
        static uint8_t setup = 0;

        // This waits for wifi to be connected and camera to be initialized
        if (setup == 0)
        {
            ulTaskNotifyTake(pdTRUE, portMAX_DELAY); // ! This is a blocking function
            ulTaskNotifyTake(pdTRUE, portMAX_DELAY); // ! This is a blocking function
            setup = 1;
        }

        ei_printf("Inference task started...\n");

        // Perform continuous inference here

        // instead of wait_ms, we'll wait on the signal, this allows threads to cancel us...
        if (ei_sleep(5) != EI_IMPULSE_OK)
        {
            return;
        }

        // used pvPortMalloc instead of malloc to ensure buffer is allocated in RAM
        snapshot_buf = (uint8_t *)pvPortMalloc(EI_CAMERA_RAW_FRAME_BUFFER_COLS * EI_CAMERA_RAW_FRAME_BUFFER_ROWS * EI_CAMERA_FRAME_BYTE_SIZE);

        // check if allocation was successful
        if (snapshot_buf == nullptr)
        {
            ei_printf("ERR: Failed to allocate snapshot buffer!\n");
            return;
        }

        ei::signal_t signal;
        signal.total_length = EI_CLASSIFIER_INPUT_WIDTH * EI_CLASSIFIER_INPUT_HEIGHT;
        signal.get_data = &ei_camera_get_data;

        if (ei_camera_capture((size_t)EI_CLASSIFIER_INPUT_WIDTH, (size_t)EI_CLASSIFIER_INPUT_HEIGHT, snapshot_buf) == false)
        {
            ei_printf("Failed to capture image\r\n");
            vPortFree(snapshot_buf); // used vPortFree instead of free to ensure buffer is freed from RAM
            return;
        }

        // upload every X seconds
        // if (millis() - previousMillis >= timerInterval)
        // {
        //     // uploadImage(snapshot_buf, (size_t)EI_CLASSIFIER_INPUT_WIDTH, (size_t)EI_CLASSIFIER_INPUT_HEIGHT);
        //     captureAndPostImage();
        //     previousMillis = millis();
        // }

        // Run the classifier
        ei_impulse_result_t result = {0};

        EI_IMPULSE_ERROR err = run_classifier(&signal, &result, debug_nn);
        if (err != EI_IMPULSE_OK)
        {
            ei_printf("ERR: Failed to run classifier (%d)\n", err);
            return;
        }

        // print the predictions
        ei_printf("predictions (DSP: %d ms., Classification: %d ms., Anomaly: %d ms.): \n",
                  result.timing.dsp, result.timing.classification, result.timing.anomaly);

#if EI_CLASSIFIER_OBJECT_DETECTION == 1
        bool bb_found = result.bounding_boxes[0].value > 0;
        for (size_t ix = 0; ix < result.bounding_boxes_count; ix++)
        {
            auto bb = result.bounding_boxes[ix];
            if (bb.value == 0)
            {
                continue;
            }

            ei_printf("    %s (%f) [ x: %u, y: %u, width: %u, height: %u ]\n", bb.label, bb.value, bb.x, bb.y, bb.width, bb.height);
        }
        if (!bb_found)
        {
            ei_printf("    No objects found\n");
        }
#else
        for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++)
        {
            ei_printf("    %s: %.5f\n", result.classification[ix].label,
                      result.classification[ix].value);
        }
#endif

#if EI_CLASSIFIER_HAS_ANOMALY == 1
        ei_printf("    anomaly score: %.3f\n", result.anomaly);
#endif

        vPortFree(snapshot_buf); // used vPortFree instead of free to ensure buffer is freed from RAM
    }
}

// take 5 snapshots from the queue but submit 1 POST request to the server
void postDataTask(void *pvParameters)
{
    while (1)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        Serial.println("POST task started");

        camera_fb_t *snapshot;
        while (xQueueReceive(snapshotQueue, &snapshot, portMAX_DELAY))
        {

            http.begin(serverURL);
            String boundary = "---------------------------2049273179963600201993832688"; // This should be a random string
            String body;

            Serial.println("Waiting for snapshot ");

            http.addHeader("Content-Type", "multipart/form-data; boundary=" + boundary);
            body += "--" + boundary + "\r\n";
            body += "Content-Disposition: form-data; name=\"imageFile\"; filename=\"image.jpg\"\r\n";
            body += "Content-Type: image/jpeg\r\n\r\n";
            body += String((char *)snapshot->buf, snapshot->len) + "\r\n";
            body += "--" + boundary + "--\r\n";
            Serial.println("All snapshots received");

            int httpResponseCode = http.POST(body);

            // If success
            if (httpResponseCode == HTTP_CODE_OK)
            {
                // Get the response from the server
                String serverResponse = http.getString();

                // Print the response
                Serial.println("Upload successful!");
                Serial.println(serverResponse);
            }
            else
            {
                Serial.println("Error on sending POST: ");
                Serial.println(httpResponseCode);
            }

            // Free resources
            http.end();
        }

        // Clear the queue
        // vQueueDelete(snapshotQueue);

        // Recreate the queue for the next round
        // snapshotQueue = xQueueCreate(5, sizeof(uint8_t *));

        Serial.println("Queue cleared");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void loop()
{
    // nothing to do here
}

/**
 * @brief   Setup image sensor & start streaming
 *
 * @retval  false if initialisation failed
 */
bool ei_camera_init(void)
{

    if (is_initialised)
        return true;

    // initialize the camera
    esp_err_t err = esp_camera_init(&camera_config);
    if (err != ESP_OK)
    {
        Serial.printf("Camera init failed with error 0x%x\n", err);
        return false;
    }

    sensor_t *s = esp_camera_sensor_get();
    // initial sensors are flipped vertically and colors are a bit saturated
    if (s->id.PID == OV3660_PID)
    {
        s->set_vflip(s, 1);      // flip it back
        s->set_brightness(s, 1); // up the brightness just a bit
        s->set_saturation(s, 0); // lower the saturation
    }

#if defined(CAMERA_MODEL_M5STACK_WIDE)
    s->set_vflip(s, 1);
    s->set_hmirror(s, 1);
#endif

    is_initialised = true;
    return true;
}

void ei_camera_deinit(void)
{

    // deinitialize the camera
    esp_err_t err = esp_camera_deinit();

    if (err != ESP_OK)
    {
        ei_printf("Camera deinit failed\n");
        return;
    }

    is_initialised = false;
    return;
}

bool ei_camera_capture(uint32_t img_width, uint32_t img_height, uint8_t *out_buf)
{
    bool do_resize = false;

    if (!is_initialised)
    {
        ei_printf("ERR: Camera is not initialized\r\n");
        return false;
    }

    camera_fb_t *fb = esp_camera_fb_get();

    if (!fb)
    {
        ei_printf("Camera capture failed\n");
        return false;
    }

    bool converted = fmt2rgb888(fb->buf, fb->len, PIXFORMAT_JPEG, snapshot_buf);

    // add to Queeue
    xQueueSend(snapshotQueue, &fb, portMAX_DELAY);

    // Check if the queue is full
    if (uxQueueMessagesWaiting(snapshotQueue) == 2)
    {
        Serial.println("Queue is full");
        // Signal the task to post snapshots
        xTaskNotifyGive(postDataTaskHandle);
        Serial.println("Task notified");
    }

    esp_camera_fb_return(fb); // return the frame buffer back to the driver for reuse

    if (!converted)
    {
        ei_printf("Conversion failed\n");
        return false;
    }

    if ((img_width != EI_CAMERA_RAW_FRAME_BUFFER_COLS) || (img_height != EI_CAMERA_RAW_FRAME_BUFFER_ROWS))
    {
        do_resize = true;
    }

    if (do_resize)
    {
        ei::image::processing::crop_and_interpolate_rgb888(
            out_buf,
            EI_CAMERA_RAW_FRAME_BUFFER_COLS,
            EI_CAMERA_RAW_FRAME_BUFFER_ROWS,
            out_buf,
            img_width,
            img_height);
    }

    return true;
}

static int ei_camera_get_data(size_t offset, size_t length, float *out_ptr)
{
    // we already have a RGB888 buffer, so recalculate offset into pixel index
    size_t pixel_ix = offset * 3;
    size_t pixels_left = length;
    size_t out_ptr_ix = 0;

    while (pixels_left != 0)
    {
        out_ptr[out_ptr_ix] = (snapshot_buf[pixel_ix] << 16) + (snapshot_buf[pixel_ix + 1] << 8) + snapshot_buf[pixel_ix + 2];

        // go to the next pixel
        out_ptr_ix++;
        pixel_ix += 3;
        pixels_left--;
    }
    // and done!
    return 0;
}

// TODO: Take Queue of 5 snapshots and send them to the server
// TODO: Rename to postDataTask and send Queue of 5 snapshots
// * Currently, it sends only one snapshot to the server (not 5)
void captureAndPostImage()
{
    // Capture image and post it to server
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb)
    {
        Serial.println("Camera capture failed");
        return;
    }

    // Get image size
    size_t img_size = fb->len;
    Serial.printf("Image size: %zuB\n", img_size);

    // Create buffer for image data
    uint8_t *img_buf = (uint8_t *)malloc(img_size);
    if (!img_buf)
    {
        Serial.println("Can't create buffer");
        esp_camera_fb_return(fb);
        return;
    }

    // Copy image data to buffer
    memcpy(img_buf, fb->buf, img_size);

    // Return the frame buffer back to the driver for reuse
    esp_camera_fb_return(fb);

    // Send HTTP POST request using HTTPClient library
    http.begin(serverURL);

    String boundary = "----WebKitFormBoundary7MA4YWxkTrZu0gW"; // This should be a random string
    http.addHeader("Content-Type", "multipart/form-data; boundary=" + boundary);

    String body = "--" + boundary + "\r\n";
    body += "Content-Disposition: form-data; name=\"imageFile\"; filename=\"image.jpg\"\r\n";
    body += "Content-Type: image/jpeg\r\n\r\n";
    body += String((char *)img_buf, img_size) + "\r\n";
    body += "--" + boundary + "--\r\n";

    int httpResponseCode = http.POST(body);

    // If success
    if (httpResponseCode == HTTP_CODE_OK)
    {
        // Get the response from the server
        String serverResponse = http.getString();

        // Print the response
        Serial.println("Upload successful!");
        Serial.println(serverResponse);
    }
    else
    {
        Serial.println("Error on sending POST: ");
        Serial.println(httpResponseCode);
    }

    // Free resources
    http.end();
    free(img_buf);
}
