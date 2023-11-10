#include <WiFi.h>
#include <PubSubClient.h>
#include <cppQueue.h>
#include <Wire.h>
#include <ArduinoQueue.h>

const char* ssid = "Xiaomi 12S Pro";
const char* password = "AAAAAAAA";

// configure mqtt connection keywords
const char* mqtt_server = "iot-06z00d3n6c4eo0p.mqtt.iothub.aliyuncs.com";
const char* mqtt_clientid = "k0ae3tXtQp0.test1|securemode=2,signmethod=hmacsha256,timestamp=1697281064836|";
const char* mqtt_username = "test1&k0ae3tXtQp0";
const char* mqtt_password = "aa9e5a7a9eeea231c9cb3bd162d3200638c94d98f34803c430a64b701208f5f7";

// configure sensor parameters
#define VL53L0X_REG_IDENTIFICATION_MODEL_ID         0xc0
#define VL53L0X_REG_IDENTIFICATION_REVISION_ID      0xc2
#define VL53L0X_REG_PRE_RANGE_CONFIG_VCSEL_PERIOD   0x50
#define VL53L0X_REG_FINAL_RANGE_CONFIG_VCSEL_PERIOD 0x70
#define VL53L0X_REG_SYSRANGE_START                  0x00
#define VL53L0X_REG_RESULT_INTERRUPT_STATUS         0x13
#define VL53L0X_REG_RESULT_RANGE_STATUS             0x14
#define address 0x29
byte gbuf[16];

//define client
WiFiClient espClient;
PubSubClient client(espClient);

//define door parameters and key variables
double payload = 0.0;
double count = 0.0;
int dist=0;
double door_length = 300;
double sum = 0.0;

//define the queue for whether the door is occupied at each measurement
//its length can decide the responsiveness to changes in congestion level
ArduinoQueue<int> q(40);

//callback function for sending mqtt request
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

//function to ensure a connection is established to the server
void reconnect(){
  while (!client.connected()) {
    Serial.println("Connecting to MQTT server...");
    if (client.connect(mqtt_clientid, mqtt_username, mqtt_password)) {
      Serial.println("Connected to MQTT server");
      client.subscribe("/sys/k0ae3tXtQp0/test1/thing/event/property/post_reply");
    } else {
      Serial.print("Failed to connect to MQTT server, rc=");
      Serial.print(client.state());
      Serial.println(" retrying in 5 seconds");
      delay(5000);
    }
  }
}


void setup() {
  Serial.begin(115200);
  Wire.begin(19,18);
  WiFi.begin(ssid, password);

  //connect to wifi
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop() {
  //connect to aliyun server
  if (!client.connected()) {
    reconnect();
  }
  //the message and payload to be sent to aliyun iot platform
  char message[200]={0};
  char payloadstr[200]={0};

  //read current distance from the distance sensor
  dist=read();
  //check if enough observations are made
  //this threshold can decide the responsiveness to changes in congestion level
  if(count <= 30){
    //if number is lower than threshold, only push values into the queue
    count+=1.0;

    //check if the current distance reading < half of door width
    if(dist<door_length*0.5){ sum+=1.0; q.enqueue(1);}
    else{ q.enqueue(0);}

    //calculate the percentage of congestion
    payload = sum/count;
  }else{
    //if number is greater than threshold, 
    //push values into the queue and also remove the queue head

    //remove the queue head, and subtract it from the sum
    sum -= q.dequeue();

    if(dist<door_length*0.5) {sum+=1.0; q.enqueue(1);}
    else {q.enqueue(0);}
    payload = sum/count;
  }
  //debug information
  Serial.print("Count: "); Serial.println(count);
  Serial.print("Sum: "); Serial.println(sum);
  Serial.print("Distance: "); Serial.println(dist);
  Serial.print("Payload: "); Serial.println(payload*100);
  
  //the json message to be sent to the server to update parameter
  payload*=100;
  dtostrf(payload, 3, 1, payloadstr);
  sprintf(message, "{\"method\":\"thing.event.property.post\",\"params\":
  {\"test_data\":%s},\"version\":\"1.0.0\"}",payloadstr);

  //send the message to the topic for property update
  client.publish("/sys/k0ae3tXtQp0/test1/thing/event/property/post", message);

  //wait for 1 second
  delay(1000);
}

int read() {
  byte val1 = read_byte_data_at(VL53L0X_REG_IDENTIFICATION_REVISION_ID);
  // Serial_debug.print("Revision ID: "); Serial_debug.println(val1);

  val1 = read_byte_data_at(VL53L0X_REG_IDENTIFICATION_MODEL_ID);
  // Serial_debug.print("Device ID: "); Serial_debug.println(val1);

  val1 = read_byte_data_at(VL53L0X_REG_PRE_RANGE_CONFIG_VCSEL_PERIOD);
  // Serial_debug.print("PRE_RANGE_CONFIG_VCSEL_PERIOD="); Serial_debug.println(val1); 
  // Serial_debug.print(" decode: "); Serial_debug.println(VL53L0X_decode_vcsel_period(val1));

  val1 = read_byte_data_at(VL53L0X_REG_FINAL_RANGE_CONFIG_VCSEL_PERIOD);
  // Serial_debug.print("FINAL_RANGE_CONFIG_VCSEL_PERIOD="); Serial_debug.println(val1);
  // Serial_debug.print(" decode: "); Serial_debug.println(VL53L0X_decode_vcsel_period(val1));

  write_byte_data_at(VL53L0X_REG_SYSRANGE_START, 0x01);

  byte val = 0;
  int cnt = 0;
  while (cnt < 100) { // 1 second waiting time max
    delay(10);
    val = read_byte_data_at(VL53L0X_REG_RESULT_RANGE_STATUS);
    if (val & 0x01) break;
    cnt++;
  }
  // if (val & 0x01) Serial_debug.println("ready"); else Serial_debug.println("not ready");
  if (!(val & 0x01)) return -1;

  read_block_data_at(0x14, 12);
  uint16_t acnt = makeuint16(gbuf[7], gbuf[6]);
  uint16_t scnt = makeuint16(gbuf[9], gbuf[8]);
  uint16_t dist = makeuint16(gbuf[11], gbuf[10]);
  byte DeviceRangeStatusInternal = ((gbuf[0] & 0x78) >> 3);

  // Serial_debug.print("ambient count: "); Serial_debug.println(acnt);
  // Serial_debug.print("signal count: ");  Serial_debug.println(scnt);
  // Serial_debug.print("distance ");       Serial_debug.println(dist);
  // Serial_debug.print("status: ");        Serial_debug.println(DeviceRangeStatusInternal);
  return dist;
}

uint16_t bswap(byte b[]) {
  // Big Endian unsigned short to little endian unsigned short
  uint16_t val = ((b[0] << 8) & b[1]);
  return val;
}

uint16_t makeuint16(int lsb, int msb) {
    return ((msb & 0xFF) << 8) | (lsb & 0xFF);
}

void write_byte_data(byte data) {
  Wire.beginTransmission(address);
  Wire.write(data);
  Wire.endTransmission();
}

void write_byte_data_at(byte reg, byte data) {
  // write data word at address and register
  Wire.beginTransmission(address);
  Wire.write(reg);
  Wire.write(data);
  Wire.endTransmission();
}

void write_word_data_at(byte reg, uint16_t data) {
  // write data word at address and register
  byte b0 = (data &0xFF);
  byte b1 = ((data >> 8) && 0xFF);
    
  Wire.beginTransmission(address);
  Wire.write(reg);
  Wire.write(b0);
  Wire.write(b1);
  Wire.endTransmission();
}

byte read_byte_data() {
  Wire.requestFrom(address, 1);
  while (Wire.available() < 1) delay(1);
  byte b = Wire.read();
  return b;
}

byte read_byte_data_at(byte reg) {
  //write_byte_data((byte)0x00);
  write_byte_data(reg);
  Wire.requestFrom(address, 1);
  while (Wire.available() < 1) delay(1);
  byte b = Wire.read();
  return b;
}

uint16_t read_word_data_at(byte reg) {
  write_byte_data(reg);
  Wire.requestFrom(address, 2);
  while (Wire.available() < 2) delay(1);
  gbuf[0] = Wire.read();
  gbuf[1] = Wire.read();
  return bswap(gbuf); 
}

void read_block_data_at(byte reg, int sz) {
  int i = 0;
  write_byte_data(reg);
  Wire.requestFrom(address, sz);
  for (i=0; i<sz; i++) {
    while (Wire.available() < 1) delay(1);
    gbuf[i] = Wire.read();
  }
}


uint16_t VL53L0X_decode_vcsel_period(short vcsel_period_reg) {
  // Converts the encoded VCSEL period register value into the real
  // period in PLL clocks
  uint16_t vcsel_period_pclks = (vcsel_period_reg + 1) << 1;
  return vcsel_period_pclks;
}

