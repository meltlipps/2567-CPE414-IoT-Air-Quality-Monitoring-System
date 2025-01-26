#include <Arduino.h>         // ไลบรารีพื้นฐานสำหรับโปรแกรม Arduino
#include <Wire.h>            // ไลบรารีสำหรับ I2C communication
#include <Adafruit_GFX.h>    // ไลบรารีกราฟิกสำหรับการแสดงผลบน OLED
#include <Adafruit_SSD1306.h>// ไลบรารีสำหรับ OLED SSD1306
#include <SoftwareSerial.h>  // ไลบรารีสำหรับ Software Serial
#include <OneWire.h>         // ไลบรารีสำหรับการสื่อสาร OneWire
#include <DallasTemperature.h> // ไลบรารีสำหรับเซ็นเซอร์วัดอุณหภูมิ Dallas

// กำหนดขนาดหน้าจอ OLED
#define SCREEN_WIDTH 128  // ความกว้างของ OLED
#define SCREEN_HEIGHT 64  // ความสูงของ OLED

// สร้างวัตถุสำหรับ OLED
Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// กำหนดขาพินสำหรับการเชื่อมต่อ OneWire
const int oneWireBus = 4;

// สร้างวัตถุสำหรับการสื่อสารผ่าน OneWire และเซ็นเซอร์ Dallas
OneWire oneWire(oneWireBus);
DallasTemperature sensors(&oneWire);

// สร้าง Software Serial สำหรับการสื่อสาร UART
HardwareSerial mySerial(2); // ใช้ UART2 ที่ RX = 16, TX = 17

// ตัวแปรสำหรับเก็บค่าฝุ่น PM1, PM2.5, และ PM10
unsigned int pm1 = 0;
unsigned int pm2_5 = 0;
unsigned int pm10 = 0;

// สร้าง Semaphore และ Mutex สำหรับควบคุมการเข้าถึงทรัพยากรร่วม
SemaphoreHandle_t dataSemaphore;  // แจ้งว่าเซ็นเซอร์มีข้อมูลใหม่
SemaphoreHandle_t oledSemaphore;  // ควบคุมการเข้าถึง OLED
SemaphoreHandle_t dataMutex;      // ป้องกันการเข้าถึงข้อมูลพร้อมกัน

// ตัวแปรเก็บ Task Handle ของแต่ละ Task
TaskHandle_t sensorTaskHandle;  // สำหรับ Task อ่านค่าฝุ่น
TaskHandle_t printTaskHandle;   // สำหรับ Task แสดงผลใน Serial Monitor
TaskHandle_t oledTaskHandle;    // สำหรับ Task แสดงผลบน OLED

// ฟังก์ชันสำหรับเข้าสู่ Deep Sleep
void enterDeepSleep() {
    Serial.println("Entering deep sleep..."); // แจ้งว่า ESP32 กำลังเข้าสู่ Deep Sleep
    esp_sleep_enable_timer_wakeup(10 * 1000000); // ตั้งปลุก ESP32 หลังจาก 10 วินาที
    esp_deep_sleep_start(); // เข้าสู่ Deep Sleep
}

// Task สำหรับอ่านค่าฝุ่นจากเซ็นเซอร์
void readSensorTask(void *parameter) {
    while (1) { // Loop ทำงานตลอดเวลา
        int index = 0;        // ตำแหน่งข้อมูลใน Data Frame
        char value;           // เก็บค่าที่อ่านจากเซ็นเซอร์
        char previousValue;   // เก็บค่าก่อนหน้าเพื่อนำมาคำนวณ

        if (xSemaphoreTake(dataMutex, portMAX_DELAY)) { // ล็อก Mutex เพื่อป้องกันการเข้าถึงข้อมูลพร้อมกัน
            while (mySerial.available()) { // ตรวจสอบว่ามีข้อมูลจากเซ็นเซอร์หรือไม่
                value = mySerial.read(); // อ่านค่าจากเซ็นเซอร์
                if ((index == 0 && value != 0x42) || (index == 1 && value != 0x4d)) { // ตรวจสอบ Data Header
                    Serial.println("Cannot find the data header."); // หากไม่พบ Header ให้แสดงข้อความ
                }
                // เก็บค่าฝุ่น PM1, PM2.5 และ PM10
                if (index == 4 || index == 6 || index == 8 || index == 10 || index == 12 || index == 14) {
                    previousValue = value; // เก็บค่าในตำแหน่งก่อนหน้า
                } else if (index == 5) {
                    pm1 = 256 * previousValue + value; // คำนวณ PM1
                } else if (index == 7) {
                    pm2_5 = 256 * previousValue + value; // คำนวณ PM2.5
                } else if (index == 9) {
                    pm10 = 256 * previousValue + value; // คำนวณ PM10
                }
                index++; // เลื่อนตำแหน่งข้อมูลใน Data Frame
            }
            while (mySerial.available())
                mySerial.read(); // ล้างข้อมูลค้างในบัฟเฟอร์

            xSemaphoreGive(dataMutex); // ปลดล็อก Mutex
        }

        xSemaphoreGive(dataSemaphore); // แจ้ง Task อื่นว่าข้อมูลพร้อม
        vTaskDelay(pdMS_TO_TICKS(1000)); // หน่วงเวลา 1 วินาที
    }
}

// Task สำหรับพิมพ์ค่าฝุ่นและอุณหภูมิใน Serial Monitor
void printTask(void *parameter) {
    while (1) {
        if (xSemaphoreTake(dataSemaphore, portMAX_DELAY)) { // รอจนกว่าข้อมูลใหม่พร้อม
            sensors.requestTemperatures(); // เรียกอุณหภูมิจากเซ็นเซอร์ Dallas
            int temperatureC = sensors.getTempCByIndex(0); // อ่านอุณหภูมิในหน่วย C
            int temperatureF = sensors.getTempFByIndex(0); // อ่านอุณหภูมิในหน่วย F

            if (xSemaphoreTake(dataMutex, portMAX_DELAY)) { // ล็อก Mutex เพื่อเข้าถึงตัวแปร
                // แสดงข้อมูลฝุ่นและอุณหภูมิใน Serial Monitor
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

// Task สำหรับแสดงค่าฝุ่นและอุณหภูมิบน OLED
void oledshow(void *parameter) {
    while (1) {
        sensors.requestTemperatures(); // อ่านอุณหภูมิจากเซ็นเซอร์ Dallas
        int temperatureC = sensors.getTempCByIndex(0); // อ่านอุณหภูมิในหน่วย C
        int temperatureF = sensors.getTempFByIndex(0); // อ่านอุณหภูมิในหน่วย F

        if (xSemaphoreTake(oledSemaphore, portMAX_DELAY)) { // ล็อก Semaphore เพื่อควบคุม OLED
            if (xSemaphoreTake(dataMutex, portMAX_DELAY)) { // ล็อก Mutex สำหรับการอ่านข้อมูล
                // ล้างหน้าจอ OLED และแสดงข้อมูล
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

            xSemaphoreGive(oledSemaphore); // ปลดล็อก Semaphore ของ OLED
        }

        vTaskDelay(pdMS_TO_TICKS(2000)); // หน่วงเวลา 2 วินาที
        enterDeepSleep(); // เข้าสู่ Deep Sleep หลังจบการแสดงผล
    }
}

// ฟังก์ชัน setup
void setup() {
    Serial.begin(9600); // เริ่มต้น Serial Monitor
    mySerial.begin(9600, SERIAL_8N1, 16, 17); // เริ่มต้น Software Serial

    if (!oled.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // เริ่มต้น OLED
        Serial.println(F("SSD1306 allocation failed")); // แจ้งข้อผิดพลาด
        for (;;); // หยุดการทำงานหาก OLED ไม่สามารถเริ่มต้นได้
    }

    // สร้าง Semaphore และ Mutex
    dataSemaphore = xSemaphoreCreateBinary();
    oledSemaphore = xSemaphoreCreateBinary();
    dataMutex = xSemaphoreCreateMutex();

    xSemaphoreGive(oledSemaphore); // ปลดล็อก OLED เริ่มต้น

    // สร้าง Task
    xTaskCreatePinnedToCore(readSensorTask, "Read Sensor", 2048, NULL, 1, &sensorTaskHandle, 1);
    xTaskCreatePinnedToCore(printTask, "Print Data", 2048, NULL, 1, &printTaskHandle, 1);
    xTaskCreatePinnedToCore(oledshow, "Oled show", 2048, NULL, 1, &oledTaskHandle, 1);
}

void loop() {
    // ไม่ต้องใช้งาน loop หลัก เนื่องจาก Task จัดการทุกอย่าง
}
