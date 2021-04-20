#include "FS.h"
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <DNSServer.h>
#include <WiFiManager.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <RBDdimmer.h>
#include <DHT.h>

#define LAMP 5
#define POWER_LED 2
#define WIFI_LED 15
#define CLIENT_ID "ESP_%06X"
#define ZEROCROSS 4
#define PIR 13
#define DHTPIN 3
#define DHTTYPE DHT22

char mqtt_server[40];
char mqtt_port[6];
char mqtt_user[32];
char device_id[6];
char mqtt_pass[32];

const char* control_topic;
const char* lamp_topic;
const char* move_topic;
const char* timer_topic;
const char* start_topic;
const char* stop_topic;
const char* control_response_topic;

String control_var = "/control";
String lamp_var = "/lamp";
String move_var = "/move";
String timer_var = "/timer";
String start_var = "/start";
String stop_var = "/stop";

char dev_name[50];
boolean clean_g = false;

WiFiClient espClient;
PubSubClient client(espClient);

bool saveConfig = false;
int value = 0;

unsigned long previousFadeMillis;

const long utcOffsetInSeconds = 7200;
boolean timer = 0;
String startTimer = "00:00:00";
String stopTimer = "00:00:00";

const int button = 12;
const int reset_button = 14;
int temp = 0;
bool lamp_flag = 0;
bool pir_flag = 0;
byte reset_flag = 0;
bool is_sent = 0;
bool restart_device = 0;

const char* server_name = "http://192.168.0.106:3333";
String manual_host_name = "";
String move_host_name = "";
String reset_host_name = "";
String temperature_host_name = "";
const char* final_manual_name;
const char* final_move_name;
const char* final_reset_name;
const char* final_temperature_name;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

dimmerLamp dimmer(LAMP, ZEROCROSS);

DHT dht(DHTPIN, DHTTYPE);
float t = 0.0;
float h = 0.0;
unsigned long previousMillis = 0;
const long interval = 10000;

void saveConfigToFileFn() {
  Serial.println("Should save config");
  saveConfig = true;
}

void callback(char* topic, byte* payload, unsigned int msg_length) {
  String temp_user = String(mqtt_user);
  String temp_id = String(device_id);
      
  String final_control = temp_user + "/" + temp_id + control_var;
  control_topic = final_control.c_str();

  String final_lamp = temp_user + "/" + temp_id + lamp_var;
  lamp_topic = final_lamp.c_str();

  String final_move = temp_user + "/" + temp_id + move_var;
  move_topic = final_move.c_str();

  String final_timer = temp_user + "/" + temp_id + timer_var;
  timer_topic = final_timer.c_str();

  String final_start = temp_user + "/" + temp_id + timer_var + start_var;
  start_topic = final_start.c_str();
      
  String final_stop = temp_user + "/" + temp_id + timer_var + stop_var;
  stop_topic = final_stop.c_str();

  String final_control_response = temp_user + "/" + temp_id + control_var + "/response";
  control_response_topic = final_control_response.c_str();
  
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for ( int i = 0; i < msg_length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
  
  if(strcmp(topic, control_topic) == 0) {
    char format[16];
    snprintf(format, sizeof format, "%%%ud", msg_length);

    int payload_value = 0;
    if (sscanf((const char *) payload, format, &payload_value) == 1) {
      Serial.println(payload_value);
      client.publish(control_response_topic, "1");
      delay(1);
    }
   }
    Serial.print("control topic value: ");
    Serial.println(control_topic);
    
   if(strcmp(topic, lamp_topic) == 0) {
     char format[16];
     snprintf(format, sizeof format, "%%%ud", msg_length);
  
     int payload_value = 0;
     if (sscanf((const char *) payload, format, &payload_value) == 1) {
       Serial.println(payload_value);
       //analogWrite(LAMP, payload_value);
       if(payload_value > 0){ 
       dimmer.setState(ON);
       dimmer.setPower(payload_value);
       } else {
        dimmer.setState(OFF);
        dimmer.setPower(payload_value);
       }
       delay(1);
     }
   }

   if(strcmp(topic, move_topic) == 0) {
    if ((char)payload[0] == 't') {
      value = 1;
    } else if((char)payload[0] == 'f'){
      value = 0;
      analogWrite(LAMP, 0);
    }
   }

    if(strcmp(topic, timer_topic) == 0) {
      if ((char)payload[0] == 't') {
        timer = 1;
      } else if((char)payload[0] == 'f') {
        timer = 0;
        digitalWrite(LAMP, LOW);
      }
    }

    if(strcmp(topic, start_topic) == 0) {
      String startMsg = String((char*)payload);
      startMsg.remove(5,4);
      startTimer = startMsg;
      startTimer += ":00";
    }

    if(strcmp(topic, stop_topic) == 0) {
      String stopMsg = String((char*)payload);
      stopMsg.remove(5,4);
      stopTimer = stopMsg;
      stopTimer += ":00";
    }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String lampId = (String)ESP.getChipId();
    if (client.connect(lampId.c_str()))
    {
      Serial.println("connected");
      
      String temp_user = String(mqtt_user);
      String temp_id = String(device_id);
      
      String final_control = temp_user + "/" + temp_id + control_var;
      control_topic = final_control.c_str();

      String final_lamp = temp_user + "/" + temp_id + lamp_var;
      lamp_topic = final_lamp.c_str();

      String final_move = temp_user + "/" + temp_id + move_var;
      move_topic = final_move.c_str();

      String final_timer = temp_user + "/" + temp_id + timer_var;
      timer_topic = final_timer.c_str();

      String final_start = temp_user + "/" + temp_id + timer_var + start_var;
      start_topic = final_start.c_str();
      
      String final_stop = temp_user + "/" + temp_id + timer_var + stop_var;
      stop_topic = final_stop.c_str();

      client.subscribe(control_topic, 1);
      client.subscribe(lamp_topic, 1);
      client.subscribe(move_topic, 1);
      client.subscribe(timer_topic, 1);
      client.subscribe(start_topic, 1);
      client.subscribe(stop_topic, 1);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(2500);
    }
  }
}

ICACHE_RAM_ATTR void light() {
  if(digitalRead(LAMP) == 0) {
    digitalWrite(LAMP, HIGH);
  } else {
    digitalWrite(LAMP, LOW);
  }
  lamp_flag = 0;
  if(value == 1) {
    value = 0;
  }
}

ICACHE_RAM_ATTR void reload() {
  reset_flag++;
  if(reset_flag == 2) {
  WiFi.disconnect(true);
  SPIFFS.format();
  Serial.println("Reset...");
  ESP.restart();
  }
}

ICACHE_RAM_ATTR void pir_interrupt() {
  Serial.println("PIR interrupt");
  if(value == 1) {
    if(digitalRead(PIR) == HIGH) {
      digitalWrite(LAMP, HIGH);
    } else {
      digitalWrite(LAMP, LOW);
    }
    pir_flag = 1;
  }
}

void setup() {
  pinMode(LAMP, OUTPUT);
  dimmer.begin(NORMAL_MODE, OFF);
  pinMode(POWER_LED, OUTPUT);
  digitalWrite(POWER_LED, HIGH);
  pinMode(button, INPUT_PULLUP);
  pinMode(reset_button, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(button), light, RISING);
  attachInterrupt(digitalPinToInterrupt(reset_button), reload, RISING);
  attachInterrupt(digitalPinToInterrupt(PIR), pir_interrupt, CHANGE);
  dht.begin();
  Serial.begin(115200);
  sprintf(dev_name, CLIENT_ID, ESP.getChipId());
  if (clean_g) {
    Serial.println(F("\n\nWait... I am formatting the Flash!"));
    SPIFFS.format();
    Serial.println(F("Done!"));
    WiFi.disconnect(true);
  }
  if (SPIFFS.begin()) {
    Serial.println("mounted file system");
    if (SPIFFS.exists("/config.json")) {
      Serial.println("reading config file");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        Serial.println("opened config file");
        size_t size = configFile.size();
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if (json.success()) {
          Serial.println("\nparsed json");

          strcpy(mqtt_server, json["mqtt_server"]);
          strcpy(mqtt_port, json["mqtt_port"]);
          strcpy(mqtt_user, json["mqtt_user"]);
          strcpy(device_id, json["device_id"]);
          strcpy(mqtt_pass, json["mqtt_pass"]);
        } else {
          Serial.println("failed to load json config");
        }
      }
    }
  } else {
    Serial.println("failed to mount FS");
  }
  WiFiManagerParameter custom_mqtt_server("server", "mqtt server", mqtt_server, 40);
  WiFiManagerParameter custom_mqtt_port("port", "mqtt port", mqtt_port, 5);
  WiFiManagerParameter custom_mqtt_user("user", "mqtt user", mqtt_user, 32);
  WiFiManagerParameter custom_device_id("id", "device id", device_id, 6);
  WiFiManagerParameter custom_mqtt_pass("pass", "mqtt pass", mqtt_pass, 32);


  WiFiManager wifiManager;

  File is_config = SPIFFS.open("/config.json", "r");
  if(!is_config) {
    Serial.println("Config file not found, I am resetting WIFI settings.");
    wifiManager.resetSettings();
  }
 
  wifiManager.setCustomHeadElement("<style>input{margin-bottom: 2px;}body{background-color: #00142d; color: white;}button{background-color: #d9534f;}</style>");

  wifiManager.setSaveConfigCallback(saveConfigToFileFn);

  wifiManager.addParameter(&custom_mqtt_server);
  wifiManager.addParameter(&custom_mqtt_port);
  wifiManager.addParameter(&custom_mqtt_user);
  wifiManager.addParameter(&custom_device_id);
  wifiManager.addParameter(&custom_mqtt_pass);

  String lampId = "E-Lampa" + (String)ESP.getChipId();
  if (!wifiManager.autoConnect(lampId.c_str())) {
    Serial.println(F("failed to connect and hit timeout"));
    delay(3000);
    ESP.reset();
    delay(5000);
  }

  Serial.println(F("WiFi is connected now..."));

  strcpy(mqtt_server, custom_mqtt_server.getValue());
  strcpy(mqtt_port, custom_mqtt_port.getValue());
  strcpy(mqtt_user, custom_mqtt_user.getValue());
  strcpy(device_id, custom_device_id.getValue());
  strcpy(mqtt_pass, custom_mqtt_pass.getValue());

  if (saveConfig) {
    Serial.println("saving config");
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
    json["mqtt_server"] = mqtt_server;
    json["mqtt_port"] = mqtt_port;
    json["mqtt_user"] = mqtt_user;
    json["device_id"] = device_id;
    json["mqtt_pass"] = mqtt_pass;

    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
      Serial.println(F("failed to open config file for writing"));
    }
    json.printTo(Serial);
    json.printTo(configFile);
    configFile.close();
  }
  Serial.println(F("My IP address: "));
  Serial.println(WiFi.localIP());
  timeClient.begin();
  pinMode(WIFI_LED, OUTPUT);
  digitalWrite(WIFI_LED, HIGH);
  client.setServer(mqtt_server, atoi(mqtt_port));
  Serial.println(mqtt_server);
  Serial.println(mqtt_port);
  client.setCallback(callback);
  Serial.print("Test id: ");
  Serial.println(lampId);
  manual_host_name = (String)server_name + "/things/lamps/" + (String)device_id + "/manualLamp";
  move_host_name = (String)server_name + "/things/lamps/" + (String)device_id + "/toggleMove";
  reset_host_name = (String)server_name + "/things/lamps/" + (String)device_id + "/reset";
  temperature_host_name = (String)server_name + "/things/lamps/" + (String)device_id + "/temperatureSensor";
  final_manual_name = manual_host_name.c_str();
  final_move_name = move_host_name.c_str();
  final_reset_name = reset_host_name.c_str();
  final_temperature_name = temperature_host_name.c_str();
}

void loop() {
  timeClient.update();
  Serial.println(timeClient.getFormattedTime());
  Serial.println(startTimer);
  Serial.println(stopTimer);
  Serial.println(value);

  if(WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    if(restart_device == 0) {
      Serial.println("Restart");
      http.begin(final_reset_name);
      http.addHeader("Content-Type", "application/x-www-form-urlencoded");
      int httpCode = http.POST("restart=1");

      if(httpCode > 0) {
        String payload = http.getString();
        Serial.println(payload);
        Serial.println("Control is 0");
      }
      restart_device = 1;
    }
    
    if (analogRead(LAMP) > 0 && lamp_flag == 0) {
      http.begin(final_manual_name);
      http.addHeader("Content-Type", "application/x-www-form-urlencoded");
      int httpCode = http.POST("lamp=1");

      if (httpCode > 0) {
        String payload = http.getString();
        Serial.println(payload);
        Serial.println("Lamp on");
      }
      lamp_flag = 1;
    }
    

    if (analogRead(LAMP) == 0  && lamp_flag == 0) {
      http.begin(final_manual_name);
      http.addHeader("Content-Type", "application/x-www-form-urlencoded");
      int httpCode = http.POST("lamp=0");

      if (httpCode > 0) {
        String payload = http.getString();
        Serial.println(payload);
        Serial.println("Lamp off");
      }
     lamp_flag = 1;
    }

    if(value == 1 && pir_flag == 1) {
      http.begin(final_move_name);
      http.addHeader("Content-Type", "application/x-www-form-urlencoded");
      if(digitalRead(PIR) == HIGH) {
        int httpCode = http.POST("is_move=1");
        if (httpCode > 0) {
          String payload = http.getString();
          Serial.println(payload);
        }
      } else {
        int httpCode = http.POST("is_move=0");
        if (httpCode > 0) {
          String payload = http.getString();
          Serial.println(payload);
        }
      }
      pir_flag = 0;
    }

  unsigned long currentMillis = millis();
  if(currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    
    float newT = dht.readTemperature();
    float newH = dht.readHumidity();
    if(isnan(newT) && isnan(newH)) {
      Serial.println("Failed to read from DHT sensor");
    } else {
      t = newT;
      h = newH;
      Serial.println(t);
      Serial.println(h);
      http.begin(final_temperature_name);
      http.addHeader("Content-Type", "application/x-www-form-urlencoded");
      int httpCode = http.POST("temperature=" + (String)t + "&humidity=" + (String)h);
      if(httpCode > 0) {
        String payload = http.getString();
        Serial.println(payload);
      }
    }
  }

    Serial.println(reset_flag);
    if(reset_flag == 1 && is_sent == 0) {
      http.begin(final_reset_name);
      http.addHeader("Content-Type", "application/x-www-form-urlencoded");
      int httpCode = http.POST("control=0");

      if(httpCode > 0) {
        String payload = http.getString();
        Serial.println(payload);
        Serial.println("Control is 0");
      }
      is_sent = 1;
    }
    http.end();
  } 
  
  if (timer == 1) {
    if (timeClient.getFormattedTime() == startTimer) {
      digitalWrite(LAMP, HIGH);
    } else if (timeClient.getFormattedTime() == stopTimer)
      digitalWrite(LAMP, LOW);
  }

  while (!client.connected()) {
    reconnect();
  }

  client.loop();
  delay(50);
}
