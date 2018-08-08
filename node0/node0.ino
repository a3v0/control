#include <ESP8266WiFi.h>
#include <MQTT.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Creds.h>

#define OWB

void myDataCb(String& topic, String& data);
void myPublishedCb();
void myDisconnectedCb();
void myConnectedCb();
void setup_oneWire();
void printOWAddress(DeviceAddress deviceAddress);
float C2F(float);

#ifdef OWB
/// for oneWire 
// function to print a device address
void printOWAddress(DeviceAddress deviceAddress);

// Setup a oneWire instance to communicate with any OneWire devices 
// (not just Maxim/Dallas temperature ICs)
// Data wire is plugged into pin 2 on the Arduino
#define ONE_WIRE_BUS 5

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
long lastTemp = 0;
long lastMsg = 0;
boolean toggle = false;
float temp0 = 0;
float temp1 = 0;
const int inPin = 5;  // should be ONE_WIRE_BUS
int deviceCount;
// arrays to hold device addresses
DeviceAddress insideThermometer, outsideThermometer;
#endif

const char * clientID = "East_Shop";

// create MQTT object
MQTT myMqtt(clientID, mqtt_server, 1883);

//
void setup() {
  pinMode(BUILTIN_LED, OUTPUT);
  digitalWrite(BUILTIN_LED, HIGH);
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  Serial.println("Connecting to MQTT server");  

  // setup callbacks
  myMqtt.onConnected(myConnectedCb);
  myMqtt.onDisconnected(myDisconnectedCb);
  myMqtt.onPublished(myPublishedCb);
  myMqtt.onData(myDataCb);
  
  Serial.println("connect mqtt...");
  myMqtt.connect();
#ifdef OWB
  setup_oneWire();
#endif
  delay(10);  // not sure we need this anymore but leave it alone
}

//
void loop()
{
  digitalWrite(BUILTIN_LED, toggle);
  toggle = !toggle;

  /*
  String topic("/");
  topic += clientID;
  topic += "/value";
  
  String valueStr(23);

  // publish value to topic
  Serial.println(valueStr);
  boolean result = myMqtt.publish(topic, valueStr);
  */

#ifdef OWB
	oneWireHandler();
#endif 
  delay(1000);
}


/*
 * 
 */
void myConnectedCb()
{
  Serial.println("connected to MQTT server");
}

void myDisconnectedCb()
{
  Serial.println("disconnected. try to reconnect...");
  delay(500);
  myMqtt.connect();
}

void myPublishedCb()
{
  //Serial.println("published.");
}

void myDataCb(String& topic, String& data)
{
  
  Serial.print(topic);
  Serial.print(": ");
  Serial.println(data);
}

#ifdef OWB

// function to print a one wire device address
void printOWAddress(DeviceAddress deviceAddress)
{
	for (uint8_t i = 0; i < 8; i++)
	{
		// zero pad the address if necessary
		if (deviceAddress[i] < 16) Serial.print("0");
		Serial.print(deviceAddress[i], HEX);
	}
	Serial.println();
}

void setup_oneWire()
{
	pinMode(inPin, INPUT);
	sensors.begin();
	lastTemp = 0;
	lastMsg = 0;
	toggle = false;

	deviceCount = sensors.getDeviceCount();
	Serial.print("DeviceCount:");
	Serial.println(deviceCount);

	if (sensors.getAddress(insideThermometer, 0))
		printOWAddress(insideThermometer);
	else
		Serial.println("Unable to find address for Device 0");

	if (sensors.getAddress(outsideThermometer, 1))
		printOWAddress(outsideThermometer);
	else
		Serial.println("Unable to find address for Device 1");

	sensors.setResolution(12);
}

void oneWireHandler(void)
{
	/// oneWire Temperature code 
	String topic;

	long now = millis();
	if (now - lastTemp > (20 * 1000))  // 20 seconds
	{
		lastTemp = now;

		sensors.requestTemperatures(); // Send the command to get temperatures
		temp0 = sensors.getTempCByIndex(0);
		temp0 = C2F(temp0);
		temp1 = sensors.getTempCByIndex(1);
		temp1 = C2F(temp1);
		Serial.println(temp0);
		Serial.println(temp1);

		// If temps are reasonable publish them 

		// if ((temp0 > -20) && (temp0 < 60)) 
		{
			for (int t = 0; t < 5; t++)
			{
				Serial.print("Publish temp0 attempt: ");
				Serial.println(t);
				Serial.print(clientID);
				Serial.println("/_temperature0 ");
				// Serial.println(temp0);
				String valueStr(temp0);
				topic = String(clientID) + String("/T0") + String(temp0).c_str();
				//if (client.publish("T0", , TRUE))
				if (myMqtt.publish(topic, valueStr))
					break;

				Serial.println("Failed");
			}

			if ((temp1 > -50) && (temp1 < 150))  // these are now in F
			{
				for (int t = 0; t < 5; t++)
				{
					Serial.print("Publish temp1 attempt t1");
					Serial.println(t);
					String valueStr(temp1);
					topic = String(clientID) + String("/T1") + String(temp1).c_str();
					//if (client.publish("T1", , TRUE))
					if (myMqtt.publish(topic, valueStr))
						break;
				}
			}
			else Serial.println("ERROR: temp1 out of range");
		}

	}
}

#endif

float C2F(float c)
{
	return(((9 * c) / 5) + 32);
}