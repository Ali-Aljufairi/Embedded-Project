# ESP32-CAM Object Detection with TensorFlow Lite and Edge Impulse

This project demonstrates how to use TensorFlow Lite and Edge Impulse for object detection on an ESP32-CAM. It involves capturing images with the ESP32-CAM, processing them with TensorFlow Lite, and displaying the detection results on a web server.

## Project Overview

The ESP32-CAM is a low-cost development board with a built-in camera, suitable for various applications such as image processing, object detection, and video streaming. TensorFlow Lite is a lightweight machine learning framework ideal for running inference on microcontrollers. Edge Impulse is a platform for training machine learning models and deploying them to microcontrollers.

This project integrates these technologies to enable object detection on the ESP32-CAM. Captured images are preprocessed, object detection is performed using a TensorFlow Lite model trained with Edge Impulse, and the results are sent to a local PHP server for display on a webpage.

## Features

- **Capture Images:** Use the ESP32-CAM to capture images.
- **Preprocess Images:** Preprocess the captured images with TensorFlow Lite.
- **Object Detection:** Perform object detection using TensorFlow Lite.
- **Post Results:** Send the results to a local PHP server.
- **Display Results:** Show the results on a webpage with the confidence percentage.

<!-- ## Project Flow

1. **Image Capture:**
   - The ESP32-CAM captures images.
2. **Image Preprocessing:**
   - TensorFlow Lite preprocesses the captured images.
3. **Object Detection:**
   - TensorFlow Lite performs object detection.
4. **Result Posting:**
   - The results are sent to a local PHP server.
5. **Result Display:**
   - The results are displayed on a webpage. -->

## Screenshots

### ESP32-CAM Module

![ESP32-CAM](esp32cam.jpg)

### Web Interface

![Project Screenshot](webpage.png)

## Technologies Used

- [**ESP32-CAM:**](https://www.espressif.com/en/products/modules/esp32-cam) A low-cost development board with an integrated camera.
- [**TensorFlow Lite:**](https://www.tensorflow.org/lite) A lightweight framework for machine learning inference on microcontrollers.
- [**Edge Impulse:**](https://www.edgeimpulse.com/) A platform for training and deploying machine learning models on edge devices.
- [**PlatformIO:**](https://platformio.org/) Arguably the best IDE for embedded development.
- [**FreeRTOS:**](https://www.freertos.org/) A real-time operating system kernel for embedded devices.
- [**PHP:**](https://www.php.net/) A server-side scripting language used to handle and display the results.


<!-- ## Conclusion

This project successfully integrates computer vision with embedded systems, demonstrating the capability of the ESP32-CAM to perform real-time object detection and making it accessible through a web server. The approach leverages various tools and platforms, providing a comprehensive learning experience in embedded systems programming and machine learning. -->
