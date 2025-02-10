
# 🌍 **IoT Air Quality Monitoring System**

This project is an **IoT-based air quality monitoring system**. It measures **PM1.0, PM2.5, PM10**, and **temperature**.  
Data is displayed on an **OLED screen** and via a **WiFi-enabled Web Server**, providing real-time air quality monitoring.

---

## 🚀 **Features**
- 📏 **Accurate measurements**: PM1.0, PM2.5, PM10  
- 🌡 **Temperature monitoring**: Celsius (°C) and Fahrenheit (°F)  
- 📺 **OLED Display** for real-time data visualization  
- 🕹 **User control**: Switch button for navigating OLED menu  
- 🔋 **Power saving mode**: OLED screen turns off when idle  
- 💡 **LED indicators**: Shows air quality levels (Green, Yellow, Red)  
- 🌐 **WiFi & Web Server**: Monitor air quality from a web browser  
- 📡 **Real-time updates**: AJAX-powered live data via JSON API  

---

## 🛠 **Hardware Requirements**

| Component                 | Description                                  |
|---------------------------|----------------------------------------------|
| **ESP32**                 | Microcontroller for system control          |
| **PMS7003 Laser Sensor**  | Measures PM1.0, PM2.5, PM10                 |
| **DS18B20 Temperature Sensor** | Tracks environmental temperature         |
| **OLED Display (SSD1306)**| Displays air quality data                   |
| **LED Indicator**         | Shows air quality levels using colors       |
| **Switch Button**         | Used for OLED menu navigation               |

---

## 💻 **Software Requirements**
- **VS Code** with **PlatformIO** (Recommended)  
- **Arduino IDE** (Optional)  
- **ESP32 Board Manager**  
- **Required Libraries**:  
  - `Adafruit SSD1306`  
  - `Adafruit GFX`  
  - `PMS Library`  
  - `DallasTemperature`  
  - `OneWire`  
  - `Wire`  
  - `WiFi`  
  - `WebServer`  

---

## 🔧 **Installation Guide**

### 1️⃣ **Install Tools**
- Download **[Visual Studio Code](https://code.visualstudio.com/)**  
- Install **PlatformIO Extension** or **Arduino IDE**  
- Add the **ESP32 Board Manager** in Arduino/PlatformIO  

### 2️⃣ **Install Libraries**
If using Arduino IDE:  
- Open **Library Manager**: `Sketch -> Include Library -> Manage Libraries`  
- Search and install required libraries (see list above).  

If using PlatformIO, add this to `platformio.ini`:  
```ini
lib_deps =
    adafruit/Adafruit SSD1306
    adafruit/Adafruit GFX Library
    env/pmsx003
    milesburton/DallasTemperature
    paulstoffregen/OneWire
    wire
    WiFi
    WebServer
