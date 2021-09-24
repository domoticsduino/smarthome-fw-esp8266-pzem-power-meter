/* 1.0.0 VERSION */

#include <Arduino.h>
#include <ArduinoJson.h>

#include "../include/config.h"

#include <ddcommon.h>
#include <ddwifi.h>
#include <ddmqtt.h>
#include <ddpzem004t.h>

//Wifi
DDWifi wifi(ssid, password, wifihostname, LEDSTATUSPIN);

//MQTT
DDMqtt clientMqtt(DEVICE, MQTT_HOST, MQTT_PORT, MQTT_USER, MQTT_PWD, TOPIC_S, MQTT_QOS, LEDSTATUSPIN);

//PZEM
DDPzem004t pzemWrapper(LEDSTATUSPIN);

PZEM004T pzemPower = PZEM004T(PZEMPIN1, PZEMPIN2);
IPAddress ipPower = IPAddress(192, 168, 1, 1);
float lastPowerTriggerValue;

//JSON
DynamicJsonBuffer jsonBuffer;
JsonObject& configRoot = jsonBuffer.createObject();
JsonObject& root = jsonBuffer.createObject();

unsigned long startMillis;

void createJsonConfig(){
    configRoot["readInterval"] = READ_INTERVAL;
    configRoot["maxSamples"] = MAX_SAMPLES;
}

void printDebugPowerData(DDPZEM004TVal values){
    writeToSerial("POWER READ Success = ", false);
    writeToSerial(values.success ? "True" : "False", true);
    if(values.success){
        writeToSerial("Voltage: ", false);
        writeToSerial(values.voltage, false);
        writeToSerial(" V ", true);
        writeToSerial("Current: ", false);
        writeToSerial(values.current, false);
        writeToSerial(" A ", true);
        writeToSerial("Power: ", false);
        writeToSerial(values.power, false);
        writeToSerial(" W ", true);
        writeToSerial("Energy: ", false);
        writeToSerial(values.energy, false);
        writeToSerial(" Wh ", true);
    }
}   

String generateJsonMessage(DDPZEM004TVal values){
    root["voltage"] = values.voltage;
    root["current"] = values.current;
    root["power"] = values.power;
    root["energy"] = values.energy;
    String json;
    root.printTo(json);
    return json;
}

void setup() {
    createJsonConfig();    
    pinMode(LEDSTATUSPIN, OUTPUT);
    digitalWrite(LEDSTATUSPIN, LOW);
    if(SERIAL_ENABLED)
        Serial.begin(SERIAL_BAUDRATE);
    writeToSerial("ESP8266MCU12 Booting...", true);

    // WIFI
    wifi.connect();

    //MQTT
    clientMqtt.reconnectMQTT(&startMillis);

    //PZEM
    pzemWrapper.init(&pzemPower, ipPower);

    lastPowerTriggerValue = -999.0;
    startMillis = millis();
}

void loop() {

    // Wait a few seconds between measurements.
    if(myDelay(configRoot["readInterval"], &startMillis)){
        clientMqtt.loop();
        DDPZEM004TVal values = pzemWrapper.getValues(&pzemPower, ipPower);
        printDebugPowerData(values);
        writeToSerial("lastPowerTriggerValue ", false);
        writeToSerial(lastPowerTriggerValue, true);
        if(abs(lastPowerTriggerValue - values.power) >= DIFF_POWER_TRIGGER || (lastPowerTriggerValue != values.power && values.power == 0)){
            clientMqtt.sendMessage(TOPIC_P, generateJsonMessage(values), &startMillis);
            lastPowerTriggerValue = values.power;
        }
    }
}