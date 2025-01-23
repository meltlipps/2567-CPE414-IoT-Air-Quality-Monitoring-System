#include <Arduino.h>         // ไลบรารีพื้นฐานสำหรับโปรแกรม Arduino
#include <Wire.h>            // ไลบรารีสำหรับ I2C communication
#include <Adafruit_GFX.h>    // ไลบรารีกราฟิกสำหรับการแสดงผลบน OLED
#include <Adafruit_SSD1306.h>// ไลบรารีสำหรับ OLED SSD1306
#include <SoftwareSerial.h>  // ไลบรารีสำหรับ Software Serial
#include <OneWire.h>         // ไลบรารีสำหรับการสื่อสาร OneWire
#include <DallasTemperature.h> // ไลบรารีสำหรับเซ็นเซอร์วัดอุณหภูมิ Dallas

// กำหนดค่าพารามิเตอร์สำหรับ OLED
#define SCREEN_WIDTH 128    // กำหนดความกว้างของหน้าจอ OLED
#define SCREEN_HEIGHT 64    // กำหนดความสูงของหน้าจอ OLED

// สร้างวัตถุ OLED SSD1306
Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// กำหนดขา OneWire ที่เชื่อมต่อกับเซ็นเซอร์ Dallas
const int oneWireBus = 4;

// สร้างวัตถุสำหรับการสื่อสาร OneWire และเซ็นเซอร์ Dallas
OneWire oneWire(oneWireBus);
DallasTemperature sensors(&oneWire);

// สร้าง Software Serial สำหรับการสื่อสารผ่าน RX และ TX
HardwareSerial mySerial(2); // ใช้ Serial2 พร้อมระบุ RX = 16, TX = 17

// ตัวแปรเก็บค่าฝุ่น PM1, PM2.5, และ PM10
unsigned int pm1 = 0;
unsigned int pm2_5 = 0;
unsigned int pm10 = 0;

// ตัวแปรสำหรับ Semaphore และ Mutex
SemaphoreHandle_t dataSemaphore;  // Semaphore สำหรับแจ้งว่ามีข้อมูล Sensor ใหม่
SemaphoreHandle_t oledSemaphore;  // Semaphore สำหรับควบคุมการเข้าถึง OLED
SemaphoreHandle_t dataMutex;      // Mutex สำหรับป้องกันการเข้าถึงตัวแปรพร้อมกัน

// ตัวแปรเก็บ Task Handle ของแต่ละ Task
TaskHandle_t sensorTaskHandle;
TaskHandle_t printTaskHandle;
TaskHandle_t oledTaskHandle;

// Task สำหรับอ่านค่าจากเซ็นเซอร์ PM
void readSensorTask(void *parameter)
{
    while (1) // Loop ทำงานตลอดเวลา
    {
        int index = 0;        // ตัวนับตำแหน่งในข้อมูลเซ็นเซอร์
        char value;           // เก็บค่าที่อ่านจากเซ็นเซอร์
        char previousValue;   // เก็บค่าก่อนหน้าเพื่อนำไปคำนวณ

        if (xSemaphoreTake(dataMutex, portMAX_DELAY)) // ล็อก Mutex เพื่อป้องกันการเข้าถึงตัวแปรพร้อมกัน
        {
            while (mySerial.available()) // ตรวจสอบว่ามีข้อมูลจากเซ็นเซอร์หรือไม่
            {
                value = mySerial.read(); // อ่านค่าจากเซ็นเซอร์
                // ตรวจสอบ Data Header ของเซ็นเซอร์
                if ((index == 0 && value != 0x42) || (index == 1 && value != 0x4d))
                {
                    Serial.println("Cannot find the data header."); // แจ้งว่าไม่พบ Data Header
                }
                // เก็บค่า PM1, PM2.5, PM10 ตามตำแหน่งใน Data Frame
                if (index == 4 || index == 6 || index == 8 || index == 10 || index == 12 || index == 14)
                {
                    previousValue = value; // เก็บค่าก่อนหน้า
                }
                else if (index == 5)
                {
                    pm1 = 256 * previousValue + value; // คำนวณค่า PM1
                }
                else if (index == 7)
                {
                    pm2_5 = 256 * previousValue + value; // คำนวณค่า PM2.5
                }
                else if (index == 9)
                {
                    pm10 = 256 * previousValue + value; // คำนวณค่า PM10
                }
                index++; // เพิ่มตัวนับ
            }
            while (mySerial.available())
                mySerial.read(); // ล้างข้อมูลที่เหลืออยู่

            xSemaphoreGive(dataMutex); // ปลดล็อก Mutex
        }

        xSemaphoreGive(dataSemaphore); // แจ้ง Task อื่นว่ามีข้อมูลใหม่พร้อมใช้งาน
        vTaskDelay(pdMS_TO_TICKS(1000)); // หน่วงเวลา 1 วินาที
    }
}

// Task สำหรับพิมพ์ค่าที่อ่านได้
void printTask(void *parameter)
{
    while (1)
    {
        if (xSemaphoreTake(dataSemaphore, portMAX_DELAY)) // รอจนกว่าข้อมูลใหม่พร้อม
        {
            sensors.requestTemperatures(); // เรียกอุณหภูมิจากเซ็นเซอร์ Dallas
            int temperatureC = sensors.getTempCByIndex(0); // อ่านอุณหภูมิในหน่วย C
            int temperatureF = sensors.getTempFByIndex(0); // อ่านอุณหภูมิในหน่วย F

            if (xSemaphoreTake(dataMutex, portMAX_DELAY)) // ล็อก Mutex เพื่อเข้าถึงตัวแปร
            {
                // แสดงข้อมูล PM และอุณหภูมิใน Serial Monitor
                Serial.print("{ ");
                Serial.print("\"pm1\": ");
                Serial.print(pm1);
                Serial.print(" ug/m3, ");
                Serial.print("\"pm2_5\": ");
                Serial.print(pm2_5);
                Serial.print(" ug/m3, ");
                Serial.print("\"pm10\": ");
                Serial.println(" ug/m3 }");
                Serial.println("Temp C");
                Serial.print(temperatureC);
                Serial.println("ºC");
                Serial.println("Temp F");
                Serial.print(temperatureF);
                Serial.println("ºF");

                xSemaphoreGive(dataMutex); // ปลดล็อก Mutex
            }
        }

        vTaskDelay(pdMS_TO_TICKS(1000)); // หน่วงเวลา 1 วินาที
    }
}

// Task สำหรับแสดงค่าบน OLED
void oledshow(void *parameter)
{
    while (1)
    {
        sensors.requestTemperatures(); // อ่านค่าจากเซ็นเซอร์ Dallas
        int temperatureC = sensors.getTempCByIndex(0); // อ่านอุณหภูมิในหน่วย C
        int temperatureF = sensors.getTempFByIndex(0); // อ่านอุณหภูมิในหน่วย F

        if (xSemaphoreTake(oledSemaphore, portMAX_DELAY)) // ล็อกการเข้าถึง OLED
        {
            if (xSemaphoreTake(dataMutex, portMAX_DELAY)) // ล็อก Mutex เพื่อป้องกันการเข้าถึงพร้อมกัน
            {
                // ล้างหน้าจอและแสดงข้อมูล
                oled.clearDisplay();
                oled.drawRect(1, 1, 127, 63, WHITE);
                oled.setTextSize(1);
                oled.setTextColor(WHITE);
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

                xSemaphoreGive(dataMutex); // ปลดล็อก Mutex
            }

            xSemaphoreGive(oledSemaphore); // ปลดล็อก OLED
        }

        vTaskDelay(pdMS_TO_TICKS(2000)); // หน่วงเวลา 2 วินาที
    }
}

// ฟังก์ชันเริ่มต้น
void setup()
{
    Serial.begin(9600); // เริ่ม Serial Monitor
    mySerial.begin(9600, SERIAL_8N1, 16, 17); // เริ่ม Software Serial

    if (!oled.begin(SSD1306_SWITCHCAPVCC, 0x3C)) // เริ่ม OLED
    {
        Serial.println(F("SSD1306 allocation failed")); // แจ้งข้อผิดพลาด
        for (;;);
    }

    // สร้าง Semaphore และ Mutex
    dataSemaphore = xSemaphoreCreateBinary();
    oledSemaphore = xSemaphoreCreateBinary();
    dataMutex = xSemaphoreCreateMutex();

    xSemaphoreGive(oledSemaphore); // ให้สิทธิ์การใช้งาน OLED เริ่มต้น

    // สร้าง Task สำหรับ RTOS
    xTaskCreatePinnedToCore(readSensorTask, "Read Sensor", 2048, NULL, 1, &sensorTaskHandle, 1);
    xTaskCreatePinnedToCore(printTask, "Print Data", 2048, NULL, 1, &printTaskHandle, 1);
    xTaskCreatePinnedToCore(oledshow, "Oled show", 2048, NULL, 1, &oledTaskHandle, 1);
}

void loop()
{
    // Loop หลักไม่มีการใช้งาน เนื่องจาก Task ทำงานทั้งหมด
}
