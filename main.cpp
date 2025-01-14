#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SoftwareSerial.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#define BLINK_GPIO_2 GPIO_NUM_2
#define SCREEN_WIDTH 128                                       // OLED display width, in pixels
#define SCREEN_HEIGHT 64                                       // OLED display height, in pixels
Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1); // create SSD1306
const int oneWireBus = 4;  //temp s
OneWire oneWire(oneWireBus);//temp
DallasTemperature sensors(&oneWire); //temp
HardwareSerial mySerial(2); // RX, TX
unsigned int pm1 = 0;
unsigned int pm2_5 = 0;
unsigned int pm10 = 0;

/*unsigned int temperatureC = sensors.getTempCByIndex(0);
unsigned int temperatureF = sensors.getTempFByIndex(0);*/
// Task handles
TaskHandle_t sensorTaskHandle;
TaskHandle_t printTaskHandle;
TaskHandle_t oledTaskHandle;

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
            mySerial.read(); // Clear remaining data
        /*Serial.println("index");
        Serial.println(index);
        Serial.println("Value");
        Serial.println(value);
        Serial.println("read");
        Serial.println(mySerial.read());*/
        vTaskDelay(pdMS_TO_TICKS(1000)); // Delay 1 second
        
        
  //delay(5000);
    
    }
}

void printTask(void *parameter)
{
    while (1)
    {
    sensors.requestTemperatures(); 
     int temperatureC = sensors.getTempCByIndex(0);
     int temperatureF = sensors.getTempFByIndex(0); 
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
        Serial.println("Temp C");
        Serial.print(temperatureC);
        Serial.println("ºC");
        Serial.println("Temp F");
        Serial.print(temperatureF);
        Serial.println("ºF");
        vTaskDelay(pdMS_TO_TICKS(1000)); // Delay 1 second
    }
}

void oledshow(void *parameter)
{
    while (1)
    {       
        sensors.requestTemperatures(); 
        int temperatureC = sensors.getTempCByIndex(0);
        int temperatureF = sensors.getTempFByIndex(0); 
            oled.clearDisplay();
            oled.drawRect(1, 1, 127, 63, WHITE);
            oled.setTextSize(1);
            oled.setTextColor(WHITE); // text color
            oled.setCursor(3, 2);
            oled.println("Air sensor");
            oled.setCursor(5, 15);
            oled.println("pm1  :");
            oled.setCursor(40, 15);
            oled.println(pm1);
            oled.setCursor(60, 15);
            oled.println("ug/m3");
            oled.setCursor(5, 25);
            oled.println("pm2.5:");
            oled.setCursor(40, 25);
            oled.println(pm2_5);
            oled.setCursor(60, 25);
            oled.println("ug/m3");
            oled.setCursor(5, 35);
            oled.println("pm10 :");
            oled.setCursor(40, 35);
            oled.println(pm10);
            oled.setCursor(60, 35);
            oled.println("ug/m3");
            oled.setCursor(5, 45);
            oled.println("TempC:");
            oled.setCursor(40, 45);
            oled.println(temperatureC);
            oled.setCursor(60, 45);
            oled.println("C");
            oled.setCursor(5, 55);
            oled.println("TempF:");
            oled.setCursor(40, 55);
            oled.println(temperatureF);
            oled.setCursor(60, 55);
            oled.println("F");
            oled.display();
            delay(2000); // Pause for 2 seconds

    }
}

void setup()
{
    Serial.begin(9600);
    mySerial.begin(9600, SERIAL_8N1, 16, 17);
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!oled.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.

    // Initialize Serial Monitor
    /*Serial.begin(9600); // initialize serial
    mySerial.begin(9600, SERIAL_8N1, 16, 17);
    Serial.println("Setup started");
    oled.clearDisplay(); // clear display
    
    oled.setTextSize(2);      // text size
    oled.setTextColor(WHITE); // text color
    oled.setCursor(50, 20);   // position to display
    oled.drawRect(10, 10, 100, 40, WHITE); // draw a rectangle
    oled.println('0');    // display count
    oled.display();    // show on OLED*/

    // Create FreeRTOS tasks
    xTaskCreatePinnedToCore(
        readSensorTask,    // Task function
        "Read Sensor",     // Name of the task
        2048,              // Stack size in words
        NULL,              // Task input parameter
        1,                 // Priority of the task
        &sensorTaskHandle, // Task handle
        1);                // Core where the task should run

    xTaskCreatePinnedToCore(
        printTask,        // Task function
        "Print Data",     // Name of the task
        2048,             // Stack size in words
        NULL,             // Task input parameter
        1,                // Priority of the task
        &printTaskHandle, // Task handle
        1);               // Core where the task should run

    xTaskCreatePinnedToCore(
        oledshow,        // Task function
        "Oled show",     // Name of the task
        2048,            // Stack size in words
        NULL,            // Task input parameter
        1,               // Priority of the task
        &oledTaskHandle, // Task handle
        1);              // Core where the task should run
}

void loop()
{
}
