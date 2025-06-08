#include <ModbusRTU.h>
#include <DHT.h>

// Modbus RTU settings
#define SLAVE_ID 1
#define SERIAL_BAUD 9600

// Cảm biến DHT11
#define DHTPIN 4  // GPIO4 on ESP32
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// Thanh ghi Modbus
const uint16_t REG_TEMP     = 0;
const uint16_t REG_HUMI     = 1;
const uint16_t REG_CONTROL  = 3; 

// Chân điều khiển LED chiếu sáng
const int LED_LIGHT_PIN = 18;

// Modbus RTU
ModbusRTU mb;

// Sử dụng UART2
HardwareSerial SerialModbus(2);

// Biến thời gian
unsigned long lastUpdateTime = 0;
const unsigned long updateInterval = 1000;

void setup() {
  Serial.begin(115200);
  Serial.println("ESP32 Modbus RTU DHT11 Sensor - LED Control");

  pinMode(RS485_DE_RE, OUTPUT);
  digitalWrite(RS485_DE_RE, LOW);

  // Sử dụng GPIO16 (RX2), GPIO17 (TX2)
  SerialModbus.begin(SERIAL_BAUD, SERIAL_8N1, 16, 17);
  mb.begin(&SerialModbus);
  mb.slave(SLAVE_ID);

  mb.addHreg(REG_TEMP, 0);
  mb.addHreg(REG_HUMI, 0);
  mb.addHreg(REG_CONTROL, 1);

  dht.begin();

  pinMode(LED_LIGHT_PIN, OUTPUT);
  digitalWrite(LED_LIGHT_PIN, HIGH);

  Serial.println("Setup completed");
}

void loop() {
  digitalWrite(RS485_DE_RE, LOW);
  delayMicroseconds(100);

  mb.task();
  yield();

  unsigned long currentTime = millis();
  if (currentTime - lastUpdateTime >= updateInterval) {
    lastUpdateTime = currentTime;

    float temp = dht.readTemperature();
    float humi = dht.readHumidity();

    if (isnan(temp) || isnan(humi)) {
      Serial.println("Failed to read from DHT sensor!");
      return;
    }

    uint16_t ledControl = mb.Hreg(REG_CONTROL);
    digitalWrite(LED_LIGHT_PIN, ledControl ? HIGH : LOW);

    digitalWrite(RS485_DE_RE, HIGH);
    delayMicroseconds(100);

    mb.Hreg(REG_TEMP, (uint16_t)(temp));
    mb.Hreg(REG_HUMI, (uint16_t)(humi));

    delayMicroseconds(100);
    digitalWrite(RS485_DE_RE, LOW);

    Serial.print("Temp: ");
    Serial.print(temp);
    Serial.print(" °C\tHumidity: ");
    Serial.print(humi);
    Serial.print(" %\tLED: ");
    Serial.println(ledControl ? "ON" : "OFF");
  }

  delay(10);
}
