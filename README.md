# ESP32-CAM Object Detection

## Project Overview

The ESP32-CAM is a low-cost development board with a built-in camera, suitable for various applications such as image processing, object detection, and video streaming. TensorFlow Lite is a lightweight machine learning framework ideal for running inference on microcontrollers. Edge Impulse is a platform for training machine learning models and deploying them to microcontrollers.

This project integrates these technologies to enable object detection on the ESP32-CAM. Captured images are preprocessed, object detection is performed using a TensorFlow Lite model trained with Edge Impulse, and the results are sent to a local PHP server for display on a webpage.

## Screenshots

### Web Interface

<!-- ![Project Screenshot](webpage.png) -->
<img src="webpage.png" width="500" alt="Project Screenshot" />

### ESP32-CAM Module

<!-- ![ESP32-CAM](esp32cam.jpg) -->
<img src="esp32cam.jpg" height="200" alt="ESP32-CAM" />

## Features

- **Capture Images:** Use the ESP32-CAM to capture images.
- **Preprocess Images:** Preprocess the captured images with TensorFlow Lite.
- **Object Detection:** Perform object detection using TensorFlow Lite.
- **Post Results:** Send the results to a local PHP server.
- **Display Results:** Show the results on a webpage with the confidence percentage.


## Technologies Used

- [**ESP32-CAM:**](https://www.espressif.com/en/products/modules/esp32-cam) A low-cost development board with an integrated camera.
- [**TensorFlow Lite:**](https://www.tensorflow.org/lite) A lightweight framework for machine learning inference on microcontrollers.
- [**Edge Impulse:**](https://www.edgeimpulse.com/) A platform for training and deploying machine learning models on edge devices.
- [**PlatformIO:**](https://platformio.org/) Arguably the best IDE for embedded development.
- [**FreeRTOS:**](https://www.freertos.org/) A real-time operating system kernel for embedded devices.