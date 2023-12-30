#include <exortix-project-1_inferencing.h>
#include "edge-impulse-sdk/dsp/image/image.hpp"
#include <Arduino.h>
#include "esp_camera.h"
#include <WiFi.h>
#include <WiFiClient.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include <HTTPClient.h>

#include <setup.h>
#include <task.h>
#include <secret.h>
#include <data_handle.h>
#include "camera_config.h"

/* Todo List
Todo: POST to PHP server ✅
Todo: Modify to Include More FreeRTOS Tasks ✅
Todo: Make the Design of The Website Prettier ✅
Todo: Break the Code into Small Chunks ✅
Todo: Work on Edge Impulse Model ✅
Todo: Modify to Include FreeRTOS Queues for POST ✅
*/

WiFiClient client;
HTTPClient http;

void setup()
{
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); // ? Disable brownout detector

    Serial.begin(115200);
    while (!Serial)
        ;

    ei_printf("Vision Vortex Demo\n");
    Serial.println("Starting setup");
    snapshotQueue = xQueueCreate(QUEUE_SIZE, sizeof(camera_fb_t));

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
    ei_printf("\nConnecting to ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED)
    {
        ei_printf(".");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    ei_printf("\nESP32-CAM IP Address: ");
    Serial.println(WiFi.localIP()); // Show ESP32 IP on serial
    xTaskNotifyGive(inferenceTaskHandle);
    vTaskDelete(NULL);
}

// @brief Initialize camera
void cameraInitTask(void *pvParameters)
{
    ei_camera_init() ? ei_printf("Camera initialized\r\n")
                     : ei_printf("Failed to initialize Camera!\r\n");
    xTaskNotifyGive(inferenceTaskHandle);
    vTaskDelete(NULL);
}

/**
 * @brief      Run inferencing
 */
void inferenceTask(void *pvParameters)
{
    while (1)
    {
        ei_printf("\nStarting continuous inference now...\n");
        static uint8_t setup = 0;

        // This waits for wifi to be connected and camera to be initialized
        if (setup == 0)
        {
            ulTaskNotifyTake(pdTRUE, portMAX_DELAY); // ! This is a blocking function
            ulTaskNotifyTake(pdTRUE, portMAX_DELAY); // ! This is a blocking function
            setup = 1;
        }

        // Perform continuous inference here
        ei_printf("Inference task started...\n");

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
            vPortFree(snapshot_buf);
            return; // ? RETURN IN FREERTOS???
        }

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

        bounding_boxes = result.bounding_boxes;
#if EI_CLASSIFIER_OBJECT_DETECTION == 1
        bool bb_found = result.bounding_boxes[0].value > 0;
        for (size_t ix = 0; ix < result.bounding_boxes_count; ix++)
        {
            auto bb = result.bounding_boxes[ix];
            if (bb.value == 0)
            {
                continue;
            }

            ei_printf("    %s (%f) [ x: %u, y: %u, width: %u, height: %u ]\n",
                      bb.label, bb.value, bb.x, bb.y, bb.width, bb.height);
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

        vPortFree(snapshot_buf);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
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

            body += "--" + boundary + "\r\n";
            body += "Content-Disposition: form-data; name=\"boundingBoxes\"\r\n";
            body += "Content-Type: application/json\r\n\r\n";
            body += bb_arrayToJSON(bounding_boxes, EI_CLASSIFIER_MAX_OBJECT_DETECTION_COUNT) + "\r\n";
            body += "--" + boundary + "--\r\n";

            Serial.println(bb_arrayToJSON(bounding_boxes, EI_CLASSIFIER_MAX_OBJECT_DETECTION_COUNT));

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
        vPortFree(bounding_boxes);

        Serial.println("Queue cleared");
        bounding_boxes = (ei_impulse_result_bounding_box_t *)pvPortMalloc(EI_CLASSIFIER_MAX_OBJECT_DETECTION_COUNT * sizeof(ei_impulse_result_bounding_box_t) * 5);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void loop()
{
    // nothing to do here
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
    if (uxQueueMessagesWaiting(snapshotQueue) == QUEUE_SIZE)
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
