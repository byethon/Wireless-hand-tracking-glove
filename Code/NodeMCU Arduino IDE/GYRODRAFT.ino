#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include <imuFilter.h>
#include <EEPROM.h>
#include <SPI.h>
#include <Mcp320x.h>

#define LED 2
#define EEPROM_SIZE 64
#define CS 0
#define ADC_VREF 3300
#define ADC_CLK 1600000

Adafruit_MPU6050 mpu;
MCP3208 adc(ADC_VREF, CS);

constexpr float GAIN = 0.1;
imuFilter <&GAIN> fusion;

float pitch = 0;
float yaw = 0;
float roll = 0;
float gcal[3];
float acal[3];
float fmaxcal[5];
float fmincal[5];
int cal_mode = 0;
int cal_dur = 2;
uint32_t st = 0;
uint32_t et = 0;
/* Wifi Credentials */

const char* ssid = "Accel/gyro";
const char* password = "IOTIOTIOT";

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
sensors_event_t a, g, t;

void notify(String in) {
  ws.textAll(in);
}

void receive_state(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    if (!strncmp((char*)data, "?/cal", 5)) {
      cal_mode = int((char*)data[5]) - 48;
    }
  }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.println("Connected");
      break;
    case WS_EVT_DISCONNECT:
      Serial.println("Disconnected");
      break;
    case WS_EVT_DATA:
      receive_state(arg, data, len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}

void initWebSocket() {
  ws.onEvent(onEvent);
  server.addHandler(&ws);
}

float read_finger(int finger) {
  uint32_t load_time, f1, f2, f3, f4, f5;
  f1 = 0;
  f2 = 0;
  f3 = 0;
  f4 = 0;
  f5 = 0;
  int itr = 0;
  int avg_window = 3000;
  for (load_time = micros(); micros() - load_time < avg_window && itr < 1024;) {
    itr++;
    if (finger == 1)
      f1 = f1 + adc.read(MCP3208::Channel::SINGLE_0);
    else if (finger == 2)
      f2 = f2 + adc.read(MCP3208::Channel::SINGLE_1);
    else if (finger == 3)
      f3 = f3 + adc.read(MCP3208::Channel::SINGLE_2);
    else if (finger == 4)
      f4 = f4 + adc.read(MCP3208::Channel::SINGLE_3);
    else if (finger == 5)
      f5 = f5 + adc.read(MCP3208::Channel::SINGLE_4);
  }
  return float((f1 + f2 + f3 + f4 + f5) / itr);
}

float angle_conv(int finger) {
  float val = read_finger(finger);
  if (val>fmaxcal[finger-1])
    val=fmaxcal[finger-1];
    else if (val<fmincal[finger-1])
    val=fmincal[finger-1];
  return (fmaxcal[finger-1]-val) / (fmaxcal[finger-1] - fmincal[finger-1])*30;
}

void cal(int x) {
  notify("Calibration Started");
  int itr = 512 * cal_dur;
  if (x == 1) {
    gcal[0] = 0;
    gcal[1] = 0;
    gcal[2] = 0;
    for (int k = 0; k < itr; k++)
    {
      mpu.getEvent(&a, &g, &t);
      gcal[0] = gcal[0] + g.gyro.x / itr;
      gcal[1] = gcal[1] + g.gyro.y / itr;
      gcal[2] = gcal[2] + g.gyro.z / itr;
      delay(2);
      if ((k + 1) % (32 * cal_dur) == 0)
        notify(String((k + 1) * 100.00 / itr) + "% done");
    }
    EEPROM.put(0, gcal[0]);
    EEPROM.put(4, gcal[1]);
    EEPROM.put(8, gcal[2]);
  }
  else if (x == 2) {
    acal[0] = 0;
    acal[1] = 0;
    acal[2] = 0;
    for (int k = 0; k < itr; k++)
    {
      mpu.getEvent(&a, &g, &t);
      acal[0] = acal[0] + a.acceleration.x / itr;
      acal[1] = acal[1] + a.acceleration.y / itr;
      acal[2] = acal[2] + (9.8 - a.acceleration.z) / itr;
      delay(2);
      if ((k + 1) % (32 * cal_dur) == 0)
        notify(String((k + 1) * 100.00 / itr) + "% done");
    }
    EEPROM.put(12, acal[0]);
    EEPROM.put(16, acal[1]);
    EEPROM.put(20, acal[2]);
  }
  else if (x == 3) {
    fmaxcal[0] = 0;
    fmaxcal[1] = 0;
    fmaxcal[2] = 0;
    fmaxcal[3] = 0;
    fmaxcal[4] = 0;
    for (int k = 0; k < itr; k++)
    {
      fmaxcal[0] = fmaxcal[0] + read_finger(1) / itr;
      fmaxcal[1] = fmaxcal[1] + read_finger(2) / itr;
      fmaxcal[2] = fmaxcal[2] + read_finger(3) / itr;
      fmaxcal[3] = fmaxcal[3] + read_finger(4) / itr;
      fmaxcal[4] = fmaxcal[4] + read_finger(5) / itr;
      delay(2);
      if ((k + 1) % (32 * cal_dur) == 0)
        notify(String((k + 1) * 100.00 / itr) + "% done");
    }
    EEPROM.put(24, fmaxcal[0]);
    EEPROM.put(28, fmaxcal[1]);
    EEPROM.put(32, fmaxcal[2]);
    EEPROM.put(36, fmaxcal[3]);
    EEPROM.put(40, fmaxcal[4]);
  }
  else if (x == 4) {
    fmincal[0] = 0;
    fmincal[1] = 0;
    fmincal[2] = 0;
    fmincal[3] = 0;
    fmincal[4] = 0;
    for (int k = 0; k < itr; k++)
    {
      fmincal[0] = fmincal[0] + read_finger(1) / itr;
      fmincal[1] = fmincal[1] + read_finger(2) / itr;
      fmincal[2] = fmincal[2] + read_finger(3) / itr;
      fmincal[3] = fmincal[3] + read_finger(4) / itr;
      fmincal[4] = fmincal[4] + read_finger(5) / itr;
      delay(2);
      if ((k + 1) % (32 * cal_dur) == 0)
        notify(String((k + 1) * 100.00 / itr) + "% done");
    }
    EEPROM.put(44, fmincal[0]);
    EEPROM.put(48, fmincal[1]);
    EEPROM.put(52, fmincal[2]);
    EEPROM.put(56, fmincal[3]);
    EEPROM.put(60, fmincal[4]);
  }
  EEPROM.commit();
  notify("Gyro Calibrated Values X:" + String(gcal[0]) + "Y:" + String(gcal[1]) + "Z:" + String(gcal[2]));
  notify("Accel Calibrated Values X:" + String(acal[0]) + "Y:" + String(acal[1]) + "Z:" + String(acal[2]));
  notify("Accel Calibrated Values X:" + String(fmincal[0]) + "Y:" + String(fmincal[1]) + "Z:" + String(fmincal[2]));
  notify("Accel Calibrated Values X:" + String(fmaxcal[0]) + "Y:" + String(fmaxcal[1]) + "Z:" + String(fmaxcal[2]));
}

void setup(void) {
  EEPROM.begin(EEPROM_SIZE);
  pinMode(LED, OUTPUT);
  pinMode(CS, OUTPUT);
  digitalWrite(CS, HIGH);
  //Serial.begin(921600);
  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("IP Address: ");
  Serial.println(IP);
  initWebSocket();
  server.begin();
  SPISettings settings(ADC_CLK, MSBFIRST, SPI_MODE0);
  SPI.begin();
  SPI.beginTransaction(settings);
  while (!mpu.begin()) {
    notify("Failed to find MPU6050 chip");
    delay(100);
  }
  notify("MPU6050 Found!");
  mpu.setAccelerometerRange(MPU6050_RANGE_4_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_184_HZ);
  EEPROM.get(0, gcal[0]);
  EEPROM.get(4, gcal[1]);
  EEPROM.get(8, gcal[2]);
  EEPROM.get(12, acal[0]);
  EEPROM.get(16, acal[1]);
  EEPROM.get(20, acal[2]);
  EEPROM.get(24, fmaxcal[0]);
  EEPROM.get(28, fmaxcal[1]);
  EEPROM.get(32, fmaxcal[2]);
  EEPROM.get(36, fmaxcal[3]);
  EEPROM.get(40, fmaxcal[4]);
  EEPROM.get(44, fmincal[0]);
  EEPROM.get(48, fmincal[1]);
  EEPROM.get(52, fmincal[2]);
  EEPROM.get(56, fmincal[3]);
  EEPROM.get(60, fmincal[4]);
  mpu.getEvent(&a, &g, &t);
  fusion.setup( a.acceleration.x - acal[0], a.acceleration.y - acal[1], a.acceleration.z - acal[2] );
  notify("Calibration Values X:" + String(gcal[0]) + "Y:" + String(gcal[1]) + "Z:" + String(gcal[2]));
}

void loop(void) {
  et = millis();
  if ((et - st) > 8) {
    st = et;
    String output = String(yaw) + "," + String(pitch) + "," + String(roll) + "," + String(angle_conv(1)) + "," + String(angle_conv(2)) + "," + String(angle_conv(3)) + "," + String(angle_conv(4)) + "," + String(angle_conv(5));
    notify(output);
  }
  mpu.getEvent(&a, &g, &t);
  fusion.update( g.gyro.x - gcal[0], g.gyro.y - gcal[1], g.gyro.z - gcal[2], a.acceleration.x - acal[0], a.acceleration.y - acal[1], a.acceleration.z - acal[2] );
  pitch = fusion.pitch() * 180 / PI;
  yaw = fusion.yaw() * 180 / PI;
  roll = fusion.roll() * 180 / PI;
  if (cal_mode) {
    cal(cal_mode);
    cal_mode = 0;
  }
}
