import paho.mqtt.client as mqtt
from pymodbus.server.sync import StartTcpServer
from pymodbus.datastore import ModbusSequentialDataBlock
from pymodbus.datastore import ModbusSlaveContext, ModbusServerContext
from threading import Thread
import time
import json

# MQTT cấu hình
MQTT_BROKER = "192.168.1.7"
MQTT_PORT = 1883
MQTT_TOPIC_SENSOR = "sensor/data"
MQTT_TOPIC_CONTROL = "control"

# Địa chỉ thanh ghi
REG_TEMP = 0
REG_HUMI = 1
REG_DEVICE_CTRL = 2  # Điều khiển LED

# Modbus Data Store
store = ModbusSlaveContext(
    di=ModbusSequentialDataBlock(0, [0]*100),
    co=ModbusSequentialDataBlock(0, [0]*100),
    hr=ModbusSequentialDataBlock(0, [0]*100),
    ir=ModbusSequentialDataBlock(0, [0]*100)
)
context = ModbusServerContext(slaves=store, single=True)

# MQTT Client để publish khi Modbus thay đổi
mqtt_client = mqtt.Client()
mqtt_client.connect(MQTT_BROKER, MQTT_PORT, 60)
mqtt_client.loop_start()

# Hàm xử lý tin nhắn MQTT
def on_message(client, userdata, msg):
    try:
        topic = msg.topic
        payload = msg.payload.decode().strip()

        if topic == MQTT_TOPIC_SENSOR:
            data = json.loads(payload)
            temperature = int(data.get("temperature", 0))
            humidity = int(data.get("humidity", 0))

            store.setValues(3, REG_TEMP, [temperature])
            store.setValues(3, REG_HUMI, [humidity])
            print(f"[Sensor] Temp={temperature}, Hum={humidity}")

        elif topic == MQTT_TOPIC_CONTROL:
            device_status = int(payload)
            store.setValues(3, REG_DEVICE_CTRL, [device_status])
            print(f"[MQTT] Device Control Received: {device_status}")

    except Exception as e:
        print(f"Error handling message {msg.topic}: {e}")

# Thread: Subscribe MQTT
def mqtt_subscribe():
    client = mqtt.Client()
    client.on_message = on_message
    client.connect(MQTT_BROKER, MQTT_PORT, 60)

    client.subscribe(MQTT_TOPIC_SENSOR)
    client.subscribe(MQTT_TOPIC_CONTROL)

    client.loop_forever()

# Thread: Kiểm tra Modbus và publish nếu có thay đổi
def check_modbus_changes():
    last_device_status = store.getValues(3, REG_DEVICE_CTRL, count=1)[0]

    while True:
        current_device_status = store.getValues(3, REG_DEVICE_CTRL, count=1)[0]

        if current_device_status != last_device_status:
            print(f"[Modbus] Updated Device Control: {current_device_status}")
            mqtt_client.publish(MQTT_TOPIC_CONTROL, str(current_device_status))
            last_device_status = current_device_status

        time.sleep(1)

# Start Modbus Server
def start_modbus_server():
    print("🟢 Starting Modbus TCP Server on port 10006...")
    StartTcpServer(context, address=("0.0.0.0", 10006))

# Chạy các thread
Thread(target=mqtt_subscribe).start()
Thread(target=check_modbus_changes).start()
start_modbus_server()
