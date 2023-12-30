#include <exortix-project-1_inferencing.h>
#include "edge-impulse-sdk/dsp/image/image.hpp"

#define QUEUE_SIZE 5
/* Private variables ------------------------------------------------------- */
static bool debug_nn = false; // Set this to true to see e.g. features generated from the raw signal
static bool is_initialised = false;
uint8_t *snapshot_buf;                            // points to the output of the capture
ei_impulse_result_bounding_box_t *bounding_boxes; // points to the output of the classifier

/* Function definitions ------------------------------------------------------- */
bool ei_camera_init(void);
void ei_camera_deinit(void);
bool ei_camera_capture(uint32_t img_width, uint32_t img_height, uint8_t *out_buf);
static int ei_camera_get_data(size_t offset, size_t length, float *out_ptr);
void captureAndPostImage(void);
