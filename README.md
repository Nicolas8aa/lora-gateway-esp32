## 📡 ESP32 Gateway – Role & Responsibilities

The **ESP32 Gateway** acts as a crucial intermediary between a remote **sensor node** (Arduino Nano + DHT22 + LoRa TX) and the **cloud infrastructure** (Next.js API + Supabase database). Its primary function is to receive sensor data via LoRa and securely forward it to an internet-accessible REST API.

---

### 🔧 How It Works

1. **LoRa Reception**
   The ESP32 is equipped with an SX1278 LoRa RX module that continuously listens for incoming packets. These packets are expected to contain formatted sensor data, typically a string with pipe-separated values, e.g.:

   ```
   <PREFIX>|72.5
   ```

2. **Data Parsing**
   Upon receiving a packet, the ESP32:

   * Verifies the prefix (e.g., `<PREFIX>|`) to confirm it's from a valid node.
   * Extracts the sensor value (e.g., humidity).
   * Captures metadata like RSSI (signal strength) and current timestamp using an NTP client.

3. **Payload Construction**
   The data is serialized into a JSON object using [ArduinoJson](https://arduinojson.org/), with a structure like:

   ```json
   {
     "porcentaje_humedad": "72.5",
     "clave_sensor": "<SENSOR_KEY>"
   }
   ```

4. **Forward to API**
   The ESP32 performs an HTTP `POST` request to a cloud-hosted **Next.js API route**, which acts as the ingestion point. On success, the API validates and stores the data into **Supabase**.

---

### 💡 Why a Gateway?

LoRa devices are **not IP-native**, meaning they can't directly interact with REST APIs or the Internet. The ESP32 Gateway bridges this gap by:

* **Decoding** wireless LoRa packets.
* **Enriching** data with timestamps and signal strength.
* **Forwarding** the data to a cloud database over Wi-Fi using standard HTTP.

This enables:

* Centralized and scalable **data collection**.
* Integration with modern **dashboards and cloud services**.
* Future **OTA updates**, analytics, and alerts.

---

### 📁 Related Files

| File              | Description                                             |
| ----------------- | ------------------------------------------------------- |
| `gateway.ino`     | Core sketch for ESP32 Gateway                           |
| `secrets.h`       | Wi-Fi credentials and API endpoint (not tracked in Git) |
| `lib/ArduinoJson` | JSON serialization used for payloads                    |
| `lib/NTPClient`   | Retrieves accurate timestamps via NTP                   |

---



