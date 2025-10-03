#  ESP32 Air Quality Monitoring System  

This project uses an **ESP32** to monitor **air quality parameters** (PM2.5, PM10, CO₂, NO₂, MQ135 gas sensor, temperature, and humidity) and sends the data to **Firebase Realtime Database**.  
It also outputs key readings (PM2.5, PM10, CO₂) over **UART2** for external devices or displays.  

## 📌 Features
-  **WiFi Connectivity** for cloud data logging.  
-  **Firebase Realtime Database** integration for remote monitoring.  
-  Air quality measurements:
  - PM2.5 (µg/m³)  
  - PM10 (µg/m³)  
  - CO₂ (ppm)  
  - MQ135 gas concentration  
  - NO₂ (calculated in µg/m³)  
-  **Temperature & Humidity** using DHT11.  
-  **UART2 Output** for external display or controller.  
-  **Serial Monitor Debugging** for live data logging.  

##  Hardware Requirements
- **ESP32** development board  
- **PM2.5 / PM10 sensor** (analog type or connected via ADC pins)  
- **CO₂ sensor** (analog type)  
- **MQ135 sensor** (gas sensor)  
- **DHT11 sensor** (temperature & humidity)  
- **Firebase Realtime Database** account  
- WiFi network  



## Pin Connections
| Sensor        | Pin (ESP32) |
|---------------|-------------|
| PM2.5 input   | GPIO 17 (ADC) |
| PM10 input    | GPIO 16 (ADC) |
| CO₂ sensor    | GPIO 32 (ADC) |
| MQ135 sensor  | GPIO 34 (ADC) |
| DHT11 sensor  | GPIO 4 |
| UART2 TX      | GPIO 16 |
| UART2 RX      | GPIO 17 |



##  How It Works

- ESP32 connects to **WiFi**  
- Reads sensors:  
  - PM2.5, PM10 → ADC values mapped to µg/m³  
  - CO₂ → ADC mapped to ppm  
  - MQ135 → raw reading + NO₂ estimation  
  - Temperature & Humidity from **DHT11**  
- Converts MQ135 readings to **NO₂ PPM → µg/m³** using calibration equation  
- Sends JSON data to Firebase at path:  
