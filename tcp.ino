
#include <WiFi.h>
#include <ModbusIP_ESP8266.h>
#include <DHT.h>

// Thông tin WiFi
const char* ssid = "Redmi 10";
const char* password = "05102003";

// Modbus
ModbusIP mb;

// Cảm biến DHT11
#define DHTPIN 21 
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// Thanh ghi Modbus
const uint16_t REG_TEMP = 0;         // Thanh ghi nhiệt độ
const uint16_t REG_HUMI = 1;         // Thanh ghi độ ẩm
const uint16_t REG_LIGHT_CTRL = 2;   // Điều khiển LED mô phỏng đèn phòng (D6)

// Chân điều khiển LED
const int LED_LIGHT_PIN = 19;  // LED mô phỏng đèn phòng


void setup() {
  Serial.begin(115200);

  // Kết nối WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Khởi tạo Modbus TCP server
  mb.server();

  // Khởi tạo các thanh ghi Modbus
  mb.addHreg(REG_TEMP, 0);
  mb.addHreg(REG_HUMI, 0);
  mb.addHreg(REG_LIGHT_CTRL, 1); // Bắt đầu với đèn phòng đang bật

  // Khởi động cảm biến DHT11
  dht.begin();

  // Cấu hình chân GPIO
  pinMode(LED_LIGHT_PIN, OUTPUT);
  digitalWrite(LED_LIGHT_PIN, HIGH); // Bật đèn ban đầu
}

void loop() {
  mb.task(); // Xử lý Modbus

  // Đọc dữ liệu cảm biến
  float humi = dht.readHumidity();
  float temp = dht.readTemperature();

  if (isnan(humi) || isnan(temp)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  // Ghi dữ liệu vào thanh ghi Modbus
  mb.Hreg(REG_TEMP, (uint16_t)temp);
  mb.Hreg(REG_HUMI, (uint16_t)humi);

  // Điều khiển LED đèn phòng qua Modbus
  uint16_t lightCtrl = mb.Hreg(REG_LIGHT_CTRL);
  digitalWrite(LED_LIGHT_PIN, lightCtrl ? HIGH : LOW);

  // In dữ liệu lên Serial
  Serial.print("Temp: ");
  Serial.print(temp);
  Serial.print(" °C\tHumidity: ");
  Serial.print(humi);
  Serial.println(" %");

  delay(2000);
}