#define VERSION "alpha_testing"

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>

#include <PZEM004Tv30.h>
#include <Wire.h>
#include "SparkFun_SHTC3.h"

#define PIN_POWER 0
#define PIN_SIGNAL 13
#define PIN_SDA 12
#define PIN_SCL 14
#define PZEM_RX_PIN 4
#define PZEM_TX_PIN 5
#define AC_READ_TIMEOUT 200

#define WIFI_SSID "smartcampus@umsida.ac.id"
#define WIFI_PASSWORD "umsida1912"

#define SERVER_HOST "iot.umsida.ac.id"
#define SERVER_TOKEN "f7f9556bd99fa8e15365062ee13ea3dfd4ca8390"
#define TOKEN "1234567890"

#define FIELD_SENSOR_SUHU 0
#define FIELD_SENSOR_KELEMBAPAN 1
#define FIELD_STATUS_AC 2
#define FIELD_COMMAND_AC 3
#define FIELD_SENSOR_TEGANGAN 4
#define FIELD_SENSOR_ARUS 5
#define FIELD_SENSOR_DAYA 6
#define FIELD_SENSOR_FREKUENSI 7
#define FIELD_SENSOR_PF 8

bool req_update = false;
bool req_restart = false;
bool req_signal_ac = false;
bool sensor_ac_status = false;
bool sensor_ac_command = false;
float sensor_room_temp;
float sensor_room_humid;
float sensor_listrik_voltage;
float sensor_listrik_power;
float sensor_listrik_current;
float sensor_listrik_pf;
float sensor_listrik_freq;

String update_url;

AsyncWebServer server(80);

WiFiClient wifi_client;
ESP8266WiFiMulti WiFiMulti;

SHTC3 room_sensor;
SoftwareSerial pzemSWSerial(PZEM_RX_PIN, PZEM_TX_PIN);
PZEM004Tv30 pzem(pzemSWSerial);

void errorDecoder(SHTC3_Status_TypeDef message);
String iot_umsida_get(int field);
void ICACHE_FLASH_ATTR iot_umsida_sender(int field, float value);
void ICACHE_FLASH_ATTR update_sensor();
void ICACHE_FLASH_ATTR ac_controller(bool signal_now);
void ICACHE_FLASH_ATTR update_sensor_ac();
void ICACHE_FLASH_ATTR update_sensor();
void ICACHE_FLASH_ATTR sensor_data_sender();
void ICACHE_FLASH_ATTR do_update();
void ICACHE_FLASH_ATTR webserver();

void setup(){
  Serial.begin(115200);
  Serial.println();
  delay(200);
  //setup sensor
  Serial.println("Setup sensor");
  pinMode(PIN_POWER, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(PIN_SIGNAL, INPUT);
  digitalWrite(PIN_POWER, HIGH);
  Wire.begin(PIN_SDA, PIN_SCL);
  errorDecoder(room_sensor.begin());
  Serial.println();
  // pzem = PZEM004Tv30(pzemSWSerial);
  //setup wifi

  Serial.println(String("Conneceting to ")+WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("WiFi connection timeout");
  }
  webserver();
  Serial.println("Delay 10s for sensor startup");
  for(int i = 10; i > 0; i--){
    Serial.println(i);
    delay(1000);
  }
}

unsigned long last_sensor_update = 0;
void loop(){
  if(req_signal_ac){
    digitalWrite(PIN_POWER, LOW);
    delay(500);
    req_signal_ac = false;
    digitalWrite(PIN_POWER, HIGH);
  }
  if(req_restart){
    Serial.println("Device rebooting");
    ESP.restart();
  }
  if(req_update){
    Serial.println("Device updating");
    do_update();
  }
  if(WiFiMulti.run() == WL_CONNECTED){
    update_sensor_ac();
    if(millis() - last_sensor_update >= 4000){
      update_sensor();
      sensor_data_sender();
      last_sensor_update = millis();
    }
  }
}


void errorDecoder(SHTC3_Status_TypeDef message)                             // The errorDecoder function prints "SHTC3_Status_TypeDef" resultsin a human-friendly way
{
  switch(message)
  {
    case SHTC3_Status_Nominal : Serial.print("Nominal"); break;
    case SHTC3_Status_Error : Serial.print("Error"); break;
    case SHTC3_Status_CRC_Fail : Serial.print("CRC Fail"); break;
    default : Serial.print("Unknown return code"); break;
  }
}



void ICACHE_FLASH_ATTR iot_umsida_sender(int field, float value){
  String result = "";
  String host = String(SERVER_HOST);
  String uri = "/dev/api/";
  uri += "key/"+String(SERVER_TOKEN)+"/";
  uri += "field/"+String(field)+"/sts/"+String(value);
  HTTPClient http;
  Serial.print("Uri :  ");
  Serial.println(uri);
  if(http.begin(wifi_client, host, 8088, uri)){
    int response_code = http.GET();
    
    if(response_code>0){
      Serial.print("HTTP Respose code : ");
      Serial.println(response_code);
      result = http.getString();
      Serial.println(result);
    }
    else{
      Serial.print("HTTP Error code : ");
      Serial.println(response_code);
      Serial.printf("error: %s\n\n", http.errorToString(response_code).c_str());
    }
    
    http.end();
  }
  else{
    Serial.println("Gagal menyambungkan ke IoT UMSIDA");
  }
}


String ICACHE_FLASH_ATTR iot_umsida_get(int field){
  String result = "";
  String host = String(SERVER_HOST);
  String uri = "/dev/api/";
  uri += "key/"+String(SERVER_TOKEN)+"/";
  uri += "field/"+String(field)+"/sts/";
  HTTPClient http;
  Serial.print("Uri :  ");
  Serial.println(uri);
  if(http.begin(wifi_client, host, 8088, uri)){
    int response_code = http.GET();
    
    if(response_code>0){
      Serial.print("HTTP Respose code : ");
      Serial.println(response_code);
      result = http.getString();
      Serial.println(result);
    }
    else{
      Serial.print("HTTP Error code : ");
      Serial.println(response_code);
      Serial.printf("error: %s\n\n", http.errorToString(response_code).c_str());
    }
    
    http.end();
  }
  else{
    Serial.println("Gagal menyambungkan ke IoT UMSIDA");
  }
  return result;
}


//controll AC power pin
void ICACHE_FLASH_ATTR ac_controller(bool signal_now = false){
  req_signal_ac = true;
  // if(uptime - ac_controller_last_run >= ac_controller_run_interval || signal_now){
    // ac_controller_last_run = uptime;
    // if(sensor_ac_status != sensor_ac_command){
      // digitalWrite(PIN_POWER, LOW);
      // delay(1000);
      // digitalWrite(PIN_POWER, HIGH);
    // }else if(signal_now){
    //   digitalWrite(PIN_POWER, LOW);
    //   delay(1000);
    //   digitalWrite(PIN_POWER, HIGH);
    // }
  // }
}


int ac_read_timer = 0;
unsigned long sensor_ac_last_update = 0;
//read AC status from led ping 
void ICACHE_FLASH_ATTR update_sensor_ac(){
  bool sts = digitalRead(PIN_SIGNAL);
  if(millis() - sensor_ac_last_update >= 200){
    if(sts){
      ac_read_timer = 0;
      sensor_ac_status = true;
    }
    else if(!sts && ac_read_timer <= AC_READ_TIMEOUT){
      ac_read_timer+=200;
    }
    else{
      sensor_ac_status = false;
    }
  }
}


void ICACHE_FLASH_ATTR update_sensor(){
  Serial.println("Updating sensor value");
  SHTC3_Status_TypeDef result = room_sensor.update();
  if(room_sensor.lastStatus == SHTC3_Status_Nominal){
    sensor_room_temp = room_sensor.toDegC();
    sensor_room_humid = room_sensor.toPercent();
  }
  else{
    Serial.println("Kesalahan mengukur suhu.");
    // errorDecoder(result);
  }
  float tmp_sensor_tegangan = pzem.voltage();
  float tmp_sensor_arus = pzem.current();
  float tmp_sensor_daya = pzem.power();
  float tmp_sensor_pf = pzem.pf();
  if(isnan(tmp_sensor_tegangan) || isnan(tmp_sensor_arus) || isnan(tmp_sensor_daya) || isnan(tmp_sensor_pf)){
    Serial.println("Kesalahan membaca sensor listrik");
  }else{
    sensor_listrik_voltage = tmp_sensor_tegangan;
    sensor_listrik_current = tmp_sensor_arus;
    sensor_listrik_power = tmp_sensor_daya;
    sensor_listrik_pf = tmp_sensor_pf;
  }
}


//Send data sensor to server
void ICACHE_FLASH_ATTR sensor_data_sender(){

    //sensor suhu
    iot_umsida_sender(FIELD_SENSOR_SUHU, sensor_room_temp);
    //sensor kelembapan
    iot_umsida_sender(FIELD_SENSOR_KELEMBAPAN, sensor_room_humid);
    //status AC
    float tmp_ac_status;
    if(sensor_ac_status){
      tmp_ac_status = 1;
    }else{
      tmp_ac_status = 0;
    }
    iot_umsida_sender(FIELD_STATUS_AC, tmp_ac_status);
    // check ac command
    String result = iot_umsida_get(FIELD_COMMAND_AC);
    if(result != ""){
      DynamicJsonBuffer Json_Buffer;
      JsonObject &json_ac_stat = Json_Buffer.parseObject(result);
      if(result){
        if(json_ac_stat.containsKey("value")){
          if(json_ac_stat["value"] == "1" && !sensor_ac_status){
            req_signal_ac = true;
          }
          else if((json_ac_stat["value"] == "0" || json_ac_stat["0.0"]) && sensor_ac_status){
            req_signal_ac = true;
          }
          // iot_umsida_sender(FIELD_COMMAND_AC, 0);
          
        }
      }
      else{
        Serial.println("Error parsing ac command");
      }
    }
    //sensor tegangan listrik
    iot_umsida_sender(FIELD_SENSOR_TEGANGAN, sensor_listrik_voltage);
    //sensor arus listrik
    iot_umsida_sender(FIELD_SENSOR_ARUS, sensor_listrik_current);
    //sensor daya
    iot_umsida_sender(FIELD_SENSOR_DAYA, sensor_listrik_power);

}




void ICACHE_FLASH_ATTR update_started() {
  Serial.println("OTA UPDATE CALLBACK:  HTTP update process started");
}

void ICACHE_FLASH_ATTR update_finished() {
  Serial.println("OTA UPDATE CALLBACK:  HTTP update process finished");
}

void ICACHE_FLASH_ATTR update_progress(int cur, int total) {
  Serial.printf("OTA UPDATE CALLBACK:  HTTP update process at %d of %d bytes...\n", cur, total);
}

void ICACHE_FLASH_ATTR update_error(int err) {
  Serial.printf("OTA UPDATE CALLBACK:  HTTP update fatal error code %d\n", err);
}

void ICACHE_FLASH_ATTR do_update(){
  WiFiClient wifi_client;
  // ESPhttpUpdate.setLedPin(LED_BUILTIN, LOW);
  // if(ota_server != NULL && firmware_file != NULL){}
  // Add optional callback notifiers
  ESPhttpUpdate.onStart(update_started);
  ESPhttpUpdate.onEnd(update_finished);
  ESPhttpUpdate.onProgress(update_progress);
  ESPhttpUpdate.onError(update_error);
  
  t_httpUpdate_return ret = ESPhttpUpdate.update(wifi_client, update_url);
  // Or:
  
  //t_httpUpdate_return ret = ESPhttpUpdate.update(client, ota_server, 80, firmware_file);
  switch (ret) {
    case HTTP_UPDATE_FAILED:
      Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s\n", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
      // updating = false;
      break;

    case HTTP_UPDATE_NO_UPDATES:
      Serial.println("HTTP_UPDATE_NO_UPDATES");
      // updating = false;
      break;

    case HTTP_UPDATE_OK:
      Serial.println("HTTP_UPDATE_OK");
      break;
  }
}

//================================================WEB_API==================================

void ICACHE_FLASH_ATTR handle_update(AsyncWebServerRequest *request){
  String auth = (request->hasHeader("token")) ? request->getHeader("token")->value() : "false";
  if(auth == TOKEN){
    if(request->hasParam("update_url", true)){
      update_url = request->getParam("update_url",true)->value();
      req_update = true;
      request->send(200, "text/html", "Device will updating soon");
    }else{
      request->send(403, "text/html", "url invalid");
    }
  }
  else{
    request->send(200, "text/html", "Access Denied!");
  }
}

void ICACHE_FLASH_ATTR handle_restart(AsyncWebServerRequest *request){
  String auth = (request->hasHeader("token")) ? request->getHeader("token")->value() : "null";
  if(auth == TOKEN){
    req_restart = true;
    request->send(200, "text/html", "device rebooting");
  }
  else{
    request->send(200, "text/html", "Access Denied!");
  }
}

void ICACHE_FLASH_ATTR handle_sensor_data(AsyncWebServerRequest *request){
  String msg = "<html><head><title>AC Controller Sensor Data</title></head><body>";
  String sts_ac = (sensor_ac_status)?"Meyala" : "Mati";
  msg += "<h3>Status AC "+ sts_ac + "</h3><br><br>";
  msg += "<br>Suhu ruangan       : "+String(sensor_room_temp);
  msg += "<br>Kelembapan ruangan : "+String(sensor_room_humid);
  msg += "<br>Daya listrik       : "+String(sensor_listrik_power);
  msg += "<br>Tegangan listrik   : "+String(sensor_listrik_voltage);
  msg += "<br>Arus listrik   : "+String(sensor_listrik_current);
  msg += "</body></html>";
  request->send(200, "text/html", msg);
}

void ICACHE_FLASH_ATTR handle_controll_ac(AsyncWebServerRequest *request){
  String command = (request->hasParam("perintah")) ? request->getParam("perintah")->value() : "null";
  String msg = "<html><head><title>AC Controller Sensor Data</title></head><body>";
  if(command == "nyala"){
    msg += "AC dinyalakan";
    if(!sensor_ac_command || !sensor_ac_status){
      sensor_ac_command = true;
      ac_controller(true);
    }
  }
  else if(command == "mati"){
    msg += "AC dimatikan";
    if(sensor_ac_command || sensor_ac_status){
      sensor_ac_command = false;
      ac_controller(true);
    }
  }
  else {
    msg += "Perintah tidak diketahui<br>usage : URL?perintah=nyala/mati";
  }
  msg += "</body></html>";
  request->send(200, "text/html", msg);
}

void ICACHE_FLASH_ATTR webserver(){
  server.onNotFound([](AsyncWebServerRequest *request) {
		AsyncWebServerResponse *response = request->beginResponse(404, "text/plain", "Not found");
		request->send(response);
	});
  server.on("/",HTTP_GET, [](AsyncWebServerRequest *request){
    String msg = "<html><head><title>AC Controller</title></head><body>";
    msg += "<h1>Welcome</h1><br><span>"+String(VERSION)+"</span>";
    msg += "</body></html>";
    request->send(200, "text/html", msg);
  });
  server.on("/sensor", HTTP_GET, handle_sensor_data);
  server.on("/ac", HTTP_GET, handle_controll_ac);
  server.on("/update",HTTP_POST, handle_update);
  server.begin();
  Serial.println("Web server started");
} 