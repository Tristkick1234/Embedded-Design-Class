/*
  WiFiAccessPoint.ino creates a WiFi access point and provides a web server on it.

  Steps:
  1. Connect to the access point
  2. Point your web browser to http://192.168.4.1/
  3. Press the CHANGE button: the NeoPixel and the webpage both step to the
     next color in the colors array (orange, red, blue, green, yellow, purple).

  Created for arduino-esp32 on 04 July, 2018
  by Elochukwu Ifediora (fedy0)
*/

#include <Arduino.h>
#include <WiFi.h>
#include <NetworkClient.h>
#include <WiFiAP.h>
#include <Adafruit_NeoPixel.h>

#define PIN_NEOPIXEL 38  // Set the GPIO pin where you connected your test LED or comment this line out if your dev board has a built-in LED
#define NUM_LEDS 1

// Set these to your desired credentials.
const char *ssid = "BOBASTICSIDEYE!";
const char *password = "batflash05";

// ---- Colors array (the "states") ----
const int NUM_COLORS = 6;
const char *colorNames[NUM_COLORS] = { "orange", "red", "blue", "green", "yellow", "purple" };
const uint8_t colorRGB[NUM_COLORS][3] = {   // NeoPixel colors
  { 255, 60, 0 },     // orange
  { 255, 0, 0 },      // red
  { 0, 0, 255 },      // blue
  { 0, 255, 0 },      // green
  { 255, 180, 0 },    // yellow
  { 140, 0, 255 }     // purple
};
const char *colorHex[NUM_COLORS] = {        // matching webpage colors
  "#FF8C00", "#E00000", "#0000FF", "#00A000", "#FFD700", "#8000FF"
};
int colorIndex = 0;                         // current state

NetworkServer server(80);
Adafruit_NeoPixel pixel(NUM_LEDS, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);

void showColor() {
  pixel.setPixelColor(0, pixel.Color(colorRGB[colorIndex][0], colorRGB[colorIndex][1], colorRGB[colorIndex][2]));
  pixel.show();
}

void setup() {

  pixel.begin();            // Initialize NeoPixel
  pixel.setBrightness(20);  // Make it less bright!
  showColor();              // start on the first color (orange)

  Serial.begin(115200);
  Serial.println();
  Serial.println("Configuring access point...");

  // You can remove the password parameter if you want the AP to be open.
  // a valid password must have more than 7 characters
  if (!WiFi.softAP(ssid, password)) {
    log_e("Soft AP creation failed.");
    while (1)
      ;
  }
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
  server.begin();

  Serial.println("Server started");
}

void loop() {
  NetworkClient client = server.accept();  // listen for incoming clients

  if (client) {                     // if you get a client,
    Serial.println("New Client.");  // print a message out the serial port
    String currentLine = "";        // make a String to hold incoming data from the client
    while (client.connected()) {    // loop while the client's connected
      if (client.available()) {     // if there's bytes to read from the client,
        char c = client.read();     // read a byte, then
        Serial.write(c);            // print it out the serial monitor
        if (c == '\n') {            // if the byte is a newline character

          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println();

            // the content of the HTTP response follows the header:
            // one button; page background matches the current color
            client.print("<body style=\"font-family:Helvetica,Arial,sans-serif;text-align:center;background:");
            client.print("black");
            client.print("\"><div style=\"background:white;display:inline-block;padding:20px 40px;margin-top:30px;border-radius:12px\">");
            client.print("<h1>ESP32-Access-Point</h1>");
            client.print("<p>State: ");
            client.print(colorNames[colorIndex]);
            client.print("</p>");
            client.print("<a href=\"/C\"><button style=\"background:");
            client.print(colorHex[colorIndex]);
            client.print(";color:white;border:none;padding:16px 40px;font-size:28px\">CHANGE</button></a>");
            client.print("</div></body>");

            // The HTTP response ends with another blank line:
            client.println();
            // break out of the while loop:
            break;
          } else {  // if you got a newline, then clear currentLine:
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }

        // Check to see if the client request was "GET /C" (the CHANGE button):
        // step to the next color in the array, wrapping back to the start
        if (currentLine.endsWith("GET /C")) {
          colorIndex = (colorIndex + 1) % NUM_COLORS;
          Serial.print("State -> ");
          Serial.println(colorNames[colorIndex]);
          showColor();
        }
      }
    }
    // close the connection:
    client.stop();
    Serial.println("Client Disconnected.");
  }
}
