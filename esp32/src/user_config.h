/**
 * @file user_config.h
 * @brief Configuration file for WiFi, MQTT, and server addresses.
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

#ifndef USER_CONFIG_H
#define USER_CONFIG_H

// WiFi Configuration
extern const char* CONFIG_ssid;
extern const char* CONFIG_password;

// Server Configuration
extern const char* CONFIG_host;
extern const char* CONFIG_site;
extern const int CONFIG_port;

// MQTT Configuration
extern const char* CONFIG_mqtt_server;
extern const char* CONFIG_mqtt_topic_nozzle;
extern const char* CONFIG_mqtt_topic_bed;
extern const char* CONFIG_mqtt_topic_progress;
extern const char* CONFIG_mqtt_topic_state;
#endif // USER_CONFIG_H
