
# 🌍 IoT Air Quality Monitoring System

## 📌 Overview
This project is an **IoT-based air quality monitoring system** using **ESP32**. It measures real-time air pollution (**PM1, PM2.5, PM10**) and temperature. The data is displayed on an **OLED screen** and shared through a **web server**.

---

## ✨ Features
✅ Real-time air quality & temperature monitoring  
✅ OLED display for easy reading  
✅ Web server with live data  
✅ **Button control** to change displayed data  
✅ **Auto OLED sleep mode** to save power  
✅ **LED indicators** for air quality (🔴 Red = Bad, 🟡 Yellow = Medium, 🟢 Green = Good)  
✅ **FreeRTOS** for better performance  
✅ **WiFi connectivity** for remote access  

---

## 🛠️ Hardware Components
- **ESP32** - Microcontroller with WiFi
- **PMS7003 Laser Dust Sensor** - Measures PM1, PM2.5, and PM10
- **DS18B20 Temperature Sensor** - Reads temperature
- **OLED SSD1306** - Displays data
- **MH-Sensor-Series** - Extra sensors for environment data
- **LEDs & Switches** - User interface and status display

---

## 💻 Software and Libraries
- **PlatformIO & VSCode** - Development tools
- **Adafruit GFX & SSD1306** - OLED display library
- **Wire & OneWire** - Sensor communication
- **DallasTemperature** - Reads temperature sensor
- **WiFi & WebServer** - For web services
- **PMS7003 Library** - Air quality sensor library
- **FreeRTOS** - Task management

---

## 🚀 Installation & Setup
### **1️⃣ Install PlatformIO**
Make sure **VSCode + PlatformIO** is installed.

### **2️⃣ Clone the Repository**
```sh
git clone https://github.com/your-repo/IoT-Air-Quality.git
cd IoT-Air-Quality


### **3️⃣ Install Dependencies**
```sh
pio run
```

### **4️⃣ Configure WiFi**
Edit `src/main.cpp` and update WiFi settings:
```cpp
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
```

### **5️⃣ Upload Code to ESP32**
```sh
pio run --target upload
```

### **6️⃣ Monitor Serial Output**
```sh
pio device monitor --baud 115200
```

---

## 🌐 Web Server API
| Endpoint  | Description |
|-----------|------------|
| `/`       | Shows web page with live data |
| `/data`   | Returns air quality data in JSON format |

---

## 🔄 Development Process
This project follows **Agile Development**:
1. **Requirement Analysis** - Define system needs.
2. **Planning** - Plan how to build it.
3. **Develop Product** - Write code and connect hardware.
4. **Review & Test** - Check if it works correctly.
5. **Deploy** - Make it ready for real use.

---

## 🚧 Future Plans
🔹 Add deep sleep mode to save power  
🔹 Improve web server with data history  
🔹 Add more sensors for better monitoring  

---

## 💡 Credits
Developed by **แกงส้มปลาหมอคางดำ** as part of the **CPE414 IoT Project**.  
For questions, contact **[Your Email/GitHub Link]**.  

---

