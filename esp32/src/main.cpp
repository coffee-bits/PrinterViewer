/**
 * @file main.cpp
 * @brief Show ESP32-CAM picture on an ESP32 TFT display together with MQTT data from a (Prusa) printer
 * 
 * @version 1.0
 * @date 2025
 * 
 * @copyright MIT License
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

// Includes
#include <SPI.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <TJpg_Decoder.h>
#include <TFT_eSPI.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "user_config.h"  // Include the configuration header

// Global Variables
WiFiClient   Main_WifiClientHandler;
PubSubClient Main_MgttClientHandler(Main_WifiClientHandler);
TFT_eSPI     Main_DisplayHandler = TFT_eSPI();
String       Main_cameraUrl;
uint8_t      Main_httpStreamBuffer[20000] = { 0 };

// Function Prototypes
void Main_MqttReconnect(void);
void Main_MqttCallback(char* topic, byte* message, unsigned int length);
bool Main_DisplayOutput(int16_t x, int16_t y, uint16_t tempPictureWidth, uint16_t h, uint16_t* bitmap);
void Main_GetCamPicture(void);
void setup(void);
void loop(void);

/**
 * @brief Callback function for JPEG decoder to render the image on the TFT display.
 * 
 * @param x X coordinate of the image block.
 * @param y Y coordinate of the image block.
 * @param tempPictureWidth Width of the image block.
 * @param h Height of the image block.
 * @param bitmap Pointer to the bitmap data.
 * @return true if the next block should be decoded, false otherwise.
 */
bool Main_DisplayOutput(int16_t x, int16_t y, uint16_t tempPictureWidth, uint16_t h, uint16_t* bitmap)
{
    // Stop further decoding as image is running off bottom of screen
    if (y >= Main_DisplayHandler.height()) {
        return false;
    }
    // This function will clip the image block rendering automatically at the TFT boundaries
    Main_DisplayHandler.pushImage(x, y, tempPictureWidth, h, bitmap);
    // Return true to decode next block
    return true;
}


/**
 * @brief Fetches a JPEG picture from the specified URL and displays it on the TFT screen.
 * The provided code snippet is a C++ function that uses an `HTTPClient` object to make an HTTP GET request to a specified
 *  URL (`Main_cameraUrl`). The process begins by initializing the `HTTPClient` object and calling the `begin` method with
 *  the URL. The `GET` method is then called to perform the HTTP request, and the response code is stored in
 *  `tempHttpCodeReturn`.
 * If the response code indicates success (`HTTP_CODE_OK`), the code retrieves the size of the HTTP response stream using
 *  `getSize()`. If the stream length is greater than zero, it proceeds to read the stream. A pointer to the WiFi client
 *  stream is obtained using `getStreamPtr()`, and a buffer (`Main_httpStreamBuffer`) is prepared to store the incoming
 *  data. The remaining length of the stream is tracked by `tempRemainingHttpStreamLength`.
 * A `while` loop is used to read the data from the stream as long as the client is connected and there is data remaining
 *  to be read. Inside the loop, the available bytes in the stream are checked, and if there are any, they are read into
 *  the buffer. The buffer pointer is incremented by the number of bytes read, and the remaining length is decremented
 *  accordingly.
 * Once the stream is successfully read, a message is printed to the serial monitor. The code then attempts to get the
 *  dimensions of the JPEG image using `TJpgDec.getJpgSize()`. If the width of the image is less than or equal to 400
 *  pixels, the image is drawn using `TJpgDec.drawJpg()`. If the image is too large, an error message is printed.
 * If the stream length is zero or the HTTP GET request fails, appropriate error messages are printed to the serial monitor. Finally, the `HTTPClient` object is closed using the `end` method.
 */
void Main_GetCamPicture(void)
{
  HTTPClient TempHttpClient;
  TempHttpClient.begin(Main_cameraUrl);
  int tempHttpCodeReturn = TempHttpClient.GET();

  if (tempHttpCodeReturn != HTTP_CODE_OK) {
    Serial.printf("[HTTP] ERROR: GET failed: %s\n", TempHttpClient.errorToString(tempHttpCodeReturn).c_str());
    return;
  }

  uint32_t tempHttpStreamLength = TempHttpClient.getSize();
  if (tempHttpStreamLength <= 0) {
    Serial.printf("[HTTP] The content could not be processed: %d\n", tempHttpStreamLength);
    return;
  }

  WiFiClient * TempWifiHttpStream_p = TempHttpClient.getStreamPtr();
  uint8_t* tempHttpStreamBuffer_p = Main_httpStreamBuffer;
  int tempRemainingHttpStreamLength = tempHttpStreamLength;

  while (TempHttpClient.connected() && (tempRemainingHttpStreamLength > 0 || tempHttpStreamLength == -1)) {
    size_t tempAvailableTreamBytes = TempWifiHttpStream_p->available();
    if (tempAvailableTreamBytes) {
      int bytesRead = TempWifiHttpStream_p->readBytes(tempHttpStreamBuffer_p, tempAvailableTreamBytes > sizeof(Main_httpStreamBuffer) ? sizeof(Main_httpStreamBuffer) : tempAvailableTreamBytes);
      tempHttpStreamBuffer_p += bytesRead;
      if (tempRemainingHttpStreamLength > 0) {
        tempRemainingHttpStreamLength -= bytesRead;
      }
    }
  }

  Serial.println("ESP32-CAM: stream read successful");
  uint16_t tempPictureWidth = 0, h = 0;
  TJpgDec.getJpgSize(&tempPictureWidth, &h, Main_httpStreamBuffer, tempHttpStreamLength);

  if (tempPictureWidth > 400) {
    Serial.println("Picture has to be smaller or equal CIF 400x296!");
    return;
  }

  TJpgDec.drawJpg(0, 0, Main_httpStreamBuffer, tempHttpStreamLength);

  TempHttpClient.end();
}

/**
 * @brief Initializes the hardware and connects to the WiFi and MQTT server.
 */
void setup(void)
{
    Serial.begin(115200);
    delay(1000);

    /* initializes the display hardware and prepares it for further configuration and use.
    It typically sets up communication protocols and any necessary initial settings. */
    Main_DisplayHandler.begin();
    /* sets the rotation of the display. The parameter 3 specifies the orientation,
    which could correspond to a specific angle (e.g., 270 degrees) depending on the display
    library being used. This ensures that the display content is oriented correctly.*/
    Main_DisplayHandler.setRotation(3);
    /* sets the text color and background color for any text that will be displayed.
    The first parameter 0xFFFF represents the text color (white), and the second parameter
    0x0000 represents the background color (black)*/
    Main_DisplayHandler.setTextColor(0xFFFF, 0x0000);
    /* fills the entire screen with a single color. The parameter TFT_BLACK indicates that
     the screen should be filled with black. This is often used to clear the screen before
     drawing new content. */
    Main_DisplayHandler.fillScreen(TFT_BLACK);
    /* This method configures the byte order for the display. Setting it to true indicates
     that the byte order should be swapped, which might be necessary for certain display
     hardware to correctly interpret color data.*/
    Main_DisplayHandler.setSwapBytes(true);

    // jpeg image scale factor: 1, 2, 4, or 8
    TJpgDec.setJpgScale(1);
    // decoder rendering function
    TJpgDec.setCallback(Main_DisplayOutput);

    // Set WiFi connection
    Serial.print("Connecting to Wifi");
    WiFi.mode(WIFI_STA);
    Serial.print(".");
    WiFi.disconnect();
    Serial.print(".");
    delay(100);
    Serial.print(".");
    Serial.print(CONFIG_ssid);
    Serial.print(".");
    WiFi.begin(CONFIG_ssid, CONFIG_password);
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(500);
    }
    Serial.print("connected with IP address: ");
    IPAddress tempWifiIp = WiFi.localIP();
    Serial.println(tempWifiIp);
    Main_MgttClientHandler.setServer(CONFIG_mqtt_server, 1883);
    Main_MgttClientHandler.setCallback(Main_MqttCallback);
    // generate URL
    Main_cameraUrl = "http://";
    Main_cameraUrl += CONFIG_host;
    Main_cameraUrl += CONFIG_site;
}


/**
 * @brief Callback function for handling incoming MQTT messages.
 * 
 * @param topic The topic of the incoming message.
 * @param message The message payload.
 * @param length The length of the message payload.
 */
void Main_MqttCallback(char* topic, byte* message, unsigned int length)
{
    static double progress = 0.0;
    static double toolTemp = 0.0;
    static double bedTemp = 0.0;
    static String state = "Idle";
    
    String tempReceivedMqttMessage;
    Serial.print("Message arrived on topic: ");
    Serial.print(topic);
    for (unsigned int i = 0U; i < length; i++) {
        tempReceivedMqttMessage += static_cast<char>(message[i]);
    }
    Serial.print(" : ");
    Serial.println(tempReceivedMqttMessage);
    if (String(topic) == CONFIG_mqtt_topic_progress) {
        progress = tempReceivedMqttMessage.toDouble();
    }
    if (String(topic) == CONFIG_mqtt_topic_bed) {
        bedTemp = tempReceivedMqttMessage.toDouble();
    }
    if (String(topic) == CONFIG_mqtt_topic_nozzle) {
        toolTemp = tempReceivedMqttMessage.toDouble();
    }
    if (String(topic) == CONFIG_mqtt_topic_state) {
      state = tempReceivedMqttMessage;
    }
    Main_DisplayHandler.fillRect(401, 0, 80, 320, TFT_BLACK);
    // print text on screen
    // Label: Nozzle
    Main_DisplayHandler.setCursor(401, 0, 2);
    Main_DisplayHandler.setTextSize(1);
    Main_DisplayHandler.setTextColor(TFT_CYAN);
    Main_DisplayHandler.println("Nozzle");
    // Value: Nozzle
    Main_DisplayHandler.setTextSize(2);
    Main_DisplayHandler.setTextColor(TFT_GREENYELLOW);
    Main_DisplayHandler.setCursor(401, Main_DisplayHandler.getCursorY());
    Main_DisplayHandler.print(toolTemp, 1);
    Main_DisplayHandler.println("C");
    // Label: Bed
    Main_DisplayHandler.setCursor(401, Main_DisplayHandler.getCursorY());
    Main_DisplayHandler.setTextSize(1);
    Main_DisplayHandler.setTextColor(TFT_CYAN);
    Main_DisplayHandler.println("Bed");
    // Value: Nozzle
    Main_DisplayHandler.setTextSize(2);
    Main_DisplayHandler.setTextColor(TFT_GREENYELLOW);
    Main_DisplayHandler.setCursor(401, Main_DisplayHandler.getCursorY());
    Main_DisplayHandler.print(bedTemp, 1);
    Main_DisplayHandler.println("C");
    // Label: Chamber
    Main_DisplayHandler.setCursor(401, Main_DisplayHandler.getCursorY());
    Main_DisplayHandler.setTextSize(1);
    Main_DisplayHandler.setTextColor(TFT_CYAN);
    Main_DisplayHandler.println("Chamber");
    // Value: Chamber
    Main_DisplayHandler.setTextSize(2);
    Main_DisplayHandler.setTextColor(TFT_GREENYELLOW);
    Main_DisplayHandler.setCursor(401, Main_DisplayHandler.getCursorY());
    Main_DisplayHandler.println("00.0C");
    // Label: Progress
    Main_DisplayHandler.setCursor(401, Main_DisplayHandler.getCursorY());
    Main_DisplayHandler.setTextSize(1);
    Main_DisplayHandler.setTextColor(TFT_CYAN);
    Main_DisplayHandler.println("Progress");
    // Value: Progress
    Main_DisplayHandler.setTextSize(2);
    Main_DisplayHandler.setTextColor(TFT_GREENYELLOW);
    Main_DisplayHandler.setCursor(401, Main_DisplayHandler.getCursorY());
    Main_DisplayHandler.print(progress, 0);
    Main_DisplayHandler.println("%");

    // Label: state
    Main_DisplayHandler.setCursor(401, Main_DisplayHandler.getCursorY());
    Main_DisplayHandler.setTextSize(1);
    Main_DisplayHandler.setTextColor(TFT_CYAN);
    Main_DisplayHandler.println("State");
    // Value: state
    Main_DisplayHandler.setTextSize(2);
    Main_DisplayHandler.setTextColor(TFT_GREENYELLOW);
    Main_DisplayHandler.setCursor(401, Main_DisplayHandler.getCursorY());
    Main_DisplayHandler.println(state);
}


/**
 * @brief Reconnects to the MQTT server if the connection is lost.
 */
void Main_MqttReconnect(void)
{
    // Loop until we're reconnected
    while (!Main_MgttClientHandler.connected()) {
        Serial.print("Attempting MQTT connection...");
        // Attempt to connect
        if (Main_MgttClientHandler.connect("ESP32DispClient")) {
            Serial.println("connected");
            // Subscribe
            Main_MgttClientHandler.subscribe(CONFIG_mqtt_topic_bed);
            Main_MgttClientHandler.subscribe(CONFIG_mqtt_topic_nozzle);
            Main_MgttClientHandler.subscribe(CONFIG_mqtt_topic_progress);
            Main_MgttClientHandler.subscribe(CONFIG_mqtt_topic_state);
        } else {
            Serial.print("failed, rc=");
            Serial.print(Main_MgttClientHandler.state());
            Serial.println(" try again in 5 seconds");
            // Wait 5 seconds before retrying
            delay(5000);
        }
    }
}


/**
 * @brief Main loop function that handles MQTT connection and fetching JPEG pictures.
 */
void loop(void)
{
    if (!Main_MgttClientHandler.connected()) {
        Main_MqttReconnect();
    }
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("WiFi not connected!");
      ESP.restart();
      delay(2000);
    }
    Main_MgttClientHandler.loop();
    Main_GetCamPicture();
}