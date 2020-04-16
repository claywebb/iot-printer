#include <Adafruit_Thermal.h>

#include <ESP8266WiFi.h>

#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include "SoftwareSerial.h"
#include <WiFiManager.h>

#include <WiFiClient.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>

#define LED_PIN 13 // D7
#define BUTTON_PIN 5 // D1

#define TX_PIN 14 // D5 / Arduino transmit  YELLOW WIRE  labeled RX on printer
#define RX_PIN 12 // D6 /  Arduino receive   GREEN WIRE   labeled TX on printer

#define WIDTH 384
#define HEIGHT 10
#define CHUNK_SIZE WIDTH * HEIGHT / 8 
String chunkSizeString = String(CHUNK_SIZE);

#define PRINTER_DELAY 100
 
const String BACKEND_URL = String("192.168.1.15:5000");

SoftwareSerial printerSerial(RX_PIN, TX_PIN); // Declare SoftwareSerial obj first
Adafruit_Thermal printer(&printerSerial);     // Pass addr to printer constructor

int lastInputTime;
int printing = false; 

void initPrinter(int &heatTime, int &heatInterval, char &printDensity, char &printBreakTime){
    //Modify the print speed and heat
    printer.write(27);
    printer.write(55);
    printer.write(7); //Default 64 dots = 8*('7'+1)
    printer.write(heatTime); //Default 80 or 800us
    printer.write(heatInterval); //Default 2 or 20us
    //Modify the print density and timeout
    printer.write(18);
    printer.write(35);
    int printSetting = (printDensity<<4) | printBreakTime;
    printer.write(printSetting); //Combination of printDensity and printBreakTime
}

void setup(){
    Serial.begin(9600);

    pinMode(LED_PIN, OUTPUT);
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    digitalWrite(LED_PIN, HIGH);

    int heatTime = 125;
    int heatInterval = 40;
    char printDensity = 20; 
    char printBreakTime = 2;

    printerSerial.begin(19200);
    printer.begin();
    initPrinter(heatTime, heatInterval, printDensity, printBreakTime);

    WiFiManager wifiManager;
    wifiManager.setDebugOutput(true);
    wifiManager.autoConnect("IoT Printer");    

    lastInputTime = millis();

    digitalWrite(LED_PIN, LOW);
}

void loop(){
    if ((digitalRead(BUTTON_PIN) == LOW) && !printing) {
        printing = true;
        digitalWrite(LED_PIN, HIGH);

        printer.wake();
        printMemo();
        printer.sleep();

        digitalWrite(LED_PIN, LOW);
        printing = false;
    }
}

void printMemo() {
    WiFiClient client;
    HTTPClient http;

    int pos = 0;
    int hasMore = 1;

    int ts = 0;
    
    while (hasMore) {
        if (http.begin(client, "http://" + BACKEND_URL + "/memo?pos=" + pos + 
            "&length=" + chunkSizeString + "&ts=" + ts)) {
            int httpCode = http.GET();
             if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
                const size_t bufferSize = JSON_ARRAY_SIZE(CHUNK_SIZE) + JSON_OBJECT_SIZE(3) + 1850;
                DynamicJsonBuffer jsonBuffer(bufferSize);

                JsonObject& root = jsonBuffer.parseObject(http.getString());
                hasMore = root["hasMore"];
                ts = root["ts"];
                pos += CHUNK_SIZE;

                uint8_t imageBuffer[CHUNK_SIZE];
                for (int i = 0; i < CHUNK_SIZE; i++)
                {
                    imageBuffer[i] = (uint8_t)root["data"][i];
                }

                printer.printBitmap(WIDTH, HEIGHT, imageBuffer, false);
                delay(PRINTER_DELAY);
                
            } else {
                Serial.print("Error retrieving chunk!");
                break;
            }
        } else {
            Serial.print("Error retrieving memo!");
            break;
        }
    }
    printer.feed(2);
}
