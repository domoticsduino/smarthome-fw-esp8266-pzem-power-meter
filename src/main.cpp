/* 1.0.0 VERSION */

#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>

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
JsonObject &jsonRoot = jsonBuffer.createObject();
JsonObject &configRoot = jsonRoot.createNestedObject("config");
JsonObject &powerJson = jsonRoot.createNestedObject("power");
JsonObject &jsonInfo = jsonRoot.createNestedObject("info");

// WEB SERVER - OTA
AsyncWebServer server(80);

unsigned long startMillis;

void createJsonConfig()
{
	configRoot["readInterval"] = READ_INTERVAL;
	configRoot["maxSamples"] = MAX_SAMPLES;
}

void printDebugPowerData(DDPZEM004TVal values)
{
	writeToSerial("POWER READ Success = ", false);
	writeToSerial(values.success ? "True" : "False", true);
	if (values.success)
	{
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

String generateJsonMessagePower(DDPZEM004TVal values)
{
	powerJson["voltage"] = values.voltage;
	powerJson["current"] = values.current;
	powerJson["power"] = values.power;
	powerJson["energy"] = values.energy;
	String json;
	powerJson.printTo(json);
	return json;
}

String generateJsonMessageRoot()
{
	String json;
	jsonRoot.printTo(json);
	return json;
}

void setup()
{
	createJsonConfig();
	pinMode(LEDSTATUSPIN, OUTPUT);
	digitalWrite(LEDSTATUSPIN, LOW);
	if (SERIAL_ENABLED)
		Serial.begin(SERIAL_BAUDRATE);
	writeToSerial(USER_SETTINGS_WIFI_HOSTNAME, false);
	writeToSerial(" Booting...", true);
	writeToSerial("FW Version: ", false);
	writeToSerial(AUTO_VERSION, true);

	// WIFI
	wifi.connect();

	//MQTT
	clientMqtt.reconnectMQTT(&startMillis);

	//WEB SERVER
	jsonInfo["name"] = USER_SETTINGS_WIFI_HOSTNAME;
	jsonInfo["version"] = AUTO_VERSION;
	server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
		request->send(200, "application/json", generateJsonMessageRoot());
	});
	AsyncElegantOTA.begin(&server);
	server.begin();
	writeToSerial("Http server started", true);

	//PZEM
	pzemWrapper.init(&pzemPower, ipPower);
	lastPowerTriggerValue = -999.0;
	startMillis = millis();
}

void loop()
{
	AsyncElegantOTA.loop();
	// Wait a few seconds between measurements.
	if (myDelay(configRoot["readInterval"], &startMillis))
	{
		clientMqtt.loop();
		DDPZEM004TVal values = pzemWrapper.getValues(&pzemPower, ipPower);
		printDebugPowerData(values);
		writeToSerial("lastPowerTriggerValue ", false);
		writeToSerial(lastPowerTriggerValue, true);
		if (abs(lastPowerTriggerValue - values.power) >= DIFF_POWER_TRIGGER || (lastPowerTriggerValue != values.power && values.power == 0))
		{
			clientMqtt.sendMessage(TOPIC_P, generateJsonMessagePower(values), &startMillis);
			lastPowerTriggerValue = values.power;
		}
	}
}