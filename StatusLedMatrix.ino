/*
NeoPixelOpenHab

Change the color of a NeoPixel Strip using OpenHab and MQTT!
Uses the PubSubClient library located at https://github.com/Imroy/pubsubclient

----------

Copyright (c) 2015 Jordan Bunker

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

#include <Adafruit_NeoPixel.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>


/* Adafruit NeoPixel Settings */

#define PIXEL_PIN    2    // Digital IO pin connected to the NeoPixels.

#define WIDTH 8 // Columns
#define HEIGHT 8 // Rows

#define PIXEL_COUNT 64    // 64 pixels in the matrix

// Parameter 1 = number of pixels in strip,  neopixel stick has 8
// Parameter 2 = pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_RGB     Pixels are wired for RGB bitstream
//   NEO_GRB     Pixels are wired for GRB bitstream, correct for neopixel stick
//   NEO_KHZ400  400 KHz bitstream (e.g. FLORA pixels)
//   NEO_KHZ800  800 KHz bitstream (e.g. High Density LED strip), correct for neopixel stick & rings
Adafruit_NeoPixel strip = Adafruit_NeoPixel(PIXEL_COUNT, PIXEL_PIN, NEO_GRB + NEO_KHZ800);

/* MQTT Topic Settings */
// Channels for the statuses
String mervi_status = "home/status_led_matrix/mervi/#";
String ville_status = "home/status_led_matrix/ville/#";

// Color shapes
int m_shape[] = {9, 10, 11, 21, 27, 36, 42, 52, 53, 54};
int v_shape[] = {13, 17, 31, 32, 46, 50};

/* Network Settings */
// Change for your WiFi network
const char *ssid =	"xxxx";		// cannot be longer than 32 characters!
const char *pass =	"xxxx";		//

// Update these with values suitable for your network.
IPAddress server(192, 168, 0, 106);


// Initializes the colors to zero
uint32_t mervi_color = 0;
uint32_t ville_color = 0;

// Initializes update status to false
volatile bool updateLights = false;

#define BUFFER_SIZE 100

void callback(const MQTT::Publish& pub) {
  if (pub.has_stream()) {
    uint8_t buf[BUFFER_SIZE];
    int read;
    while (read = pub.payload_stream()->read(buf, BUFFER_SIZE)) {
      Serial.write(buf, read);
    }
    pub.payload_stream()->stop();
    Serial.println("");
  } else
    // Read the color string
    String values = pub.payload_string();
    int c1 = pub.payload_string().indexOf(',');
    int c2 = pub.payload_string().indexOf(',',c1+1);
    int red = pub.payload_string().toInt();
    int green = pub.payload_string().substring(c1+1).toInt();
    int blue = pub.payload_string().substring(c2+1).toInt();

    // Set the correct color
    if (pub.topic() == "home/status_led_matrix/mervi") {
      mervi_color = strip.Color(red, green, blue);
      updateLights = true;
    }
    else if (pub.topic() == "home/status_led_matrix/ville") {
      ville_color = strip.Color(red, green, blue);
      updateLights = true;
    }
}

WiFiClient wclient;
PubSubClient client(wclient, server);

void setup() {
  // Setup console
  Serial.begin(9600);
  delay(10);
  strip.begin();

  setAllLedsOff();
}

void setAllLedsOff() {
  for(uint16_t i=0; i<strip.numPixels(); ++i) {
    strip.setPixelColor(i, strip.Color(0, 0, 0));
    strip.show();
  }
}

void changeColor() {
  if (updateLights) {
    // Update mervi lights
    int arraySize = sizeof(m_shape) / sizeof(m_shape[0]);
    for(uint16_t i=0; i < arraySize; ++i) {
      strip.setPixelColor(m_shape[i], mervi_color);
      strip.show();
    }

    // Update ville lights
    arraySize = sizeof(v_shape) / sizeof(v_shape[0]);
    for(uint16_t i=0; i < arraySize; ++i) {
      strip.setPixelColor(v_shape[i], ville_color);
      strip.show();
    }

    updateLights = false;
  }
}

void loop() {
  // Create connection to wifi
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.begin(ssid, pass);
    if (WiFi.waitForConnectResult() != WL_CONNECTED) {
      return;
    }
  }

  // When connected to wifi, create connection to 
  if (WiFi.status() == WL_CONNECTED) {
    if (!client.connected()) {
      if (client.connect(MQTT::Connect("status_led_matrix").set_auth("xxxxusernamexxxx", "xxxxpasswordxxxx"))) {
	      client.set_callback(callback);
	      client.subscribe(mervi_status);
        client.subscribe(ville_status);
      }
      else {
        delay(100);
      }
    }
  }

  // Remain the connection for client by looping
  if (client.connected()) {
    client.loop();
  }
  
  changeColor();
}
