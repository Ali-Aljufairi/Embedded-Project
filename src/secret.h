#include <Arduino.h>

/* WiFi definitions ------------------------------------------------------- */
const char *ssid = "Fares";
const char *password = "fareS123";

String serverName = "192.168.1.13";
String serverPath = "/upload.php";
const int serverPort = 8080;
String serverURL = "http://" + serverName + ":" + serverPort + serverPath;
