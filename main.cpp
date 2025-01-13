#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SoftwareSerial.h>

HardwareSerial mySerial(2); // RX, TX
unsigned int pm1 = 0;
unsigned int pm2_5 = 0;
unsigned int pm10 = 0;

// Task handles
TaskHandle_t sensorTaskHandle;
TaskHandle_t printTaskHandle;

void readSensorTask(void *parameter)
{
    while (1)   
    {
        int index = 0;
        char value;
        char previousValue;

        while (mySerial.available())
        {
            value = mySerial.read();
            if ((index == 0 && value != 0x42) || (index == 1 && value != 0x4d))
            {
                Serial.println("Cannot find the data header.");
            }
            if (index == 4 || index == 6 || index == 8 || index == 10 || index == 12 || index == 14)
            {
                previousValue = value;
            }
            else if (index == 5)
            {
                pm1 = 256 * previousValue + value;
            }
            else if (index == 7)
            {
                pm2_5 = 256 * previousValue + value;
            }
            else if (index == 9)
            {
                pm10 = 256 * previousValue + value;
            }
            index++;
        }
        while (mySerial.available())
            Serial.println("index");
            Serial.println(index);
            Serial.println("Value");
            Serial.println(value);
            Serial.println("read");
            Serial.println(mySerial.read());
            mySerial.read(); // Clear remaining data
            /*Serial.println("index");
            Serial.println(index);
            Serial.println("Value");
            Serial.println(value);
            Serial.println("read");
            Serial.println(mySerial.read());*/
        vTaskDelay(pdMS_TO_TICKS(1000)); // Delay 1 second
    }
}

void printTask(void *parameter)
{
    while (1) 
    {
        Serial.print("{ ");
        Serial.print("\"pm1\": ");
        Serial.print(pm1);
        Serial.print(" ug/m3, ");
        Serial.print("\"pm2_5\": ");
        Serial.print(pm2_5);
        Serial.print(" ug/m3, ");
        Serial.print("\"pm10\": ");
        Serial.print(pm10);
        Serial.println(" ug/m3 }");

        vTaskDelay(pdMS_TO_TICKS(1000)); // Delay 1 second
    }
}

void setup()
{
    // Initialize Serial Monitor
    Serial.begin(9600); // initialize serial
    mySerial.begin(9600, SERIAL_8N1, 16, 17);
    Serial.println("Setup started");

    // Create FreeRTOS tasks
    xTaskCreatePinnedToCore(
        readSensorTask,  // Task function
        "Read Sensor",   // Name of the task
        2048,            // Stack size in words
        NULL,            // Task input parameter
        1,               // Priority of the task
        &sensorTaskHandle, // Task handle
        1);              // Core where the task should run

    xTaskCreatePinnedToCore(
        printTask,       // Task function
        "Print Data",    // Name of the task
        2048,            // Stack size in words
        NULL,            // Task input parameter
        1,               // Priority of the task
        &printTaskHandle, // Task handle
        1);              // Core where the task should run
}

void loop()
{
    
}
