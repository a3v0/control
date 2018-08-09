/*
 *
 *     M  A  S  T  E  R
 *
 */

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

float C2F(float);
boolean toggle = false;

#ifdef OWB
/// for oneWire 
// function prototypes
// function to print a device address
void printOWAddress(DeviceAddress deviceAddress);
void setupOneWire();
void printOWAddress(DeviceAddress deviceAddress);

// Setup a oneWire instance to communicate with any OneWire devices 
// (not just Maxim/Dallas temperature ICs)
// Data wire is plugged into pin 2 on the Arduino
#define ONE_WIRE_BUS 5

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
long lastTemp = 0;
long lastMsg = 0;

const int inPin = 5;  // should be ONE_WIRE_BUS
int deviceCount;
const int MAX_ONE_WIRE_DEVICES = 8;  // hard coded here and in DallasTemperature.h  "typedef uint8_t DeviceAddress[8];"
// arrays to hold device addresses
// this is special.  The names like insideThermometer are defined here as array indexes.
// each node needs to have these changed to suite
////DeviceAddress insideThermometer, outsideThermometer, anotherThermometer;
DeviceAddress device[8]; // 8 devices
#endif

const char * clientID = "master";

// create MQTT object
MQTT myMqtt(clientID, mqtt_server, 1883);

//
void setup() {
  pinMode(BUILTIN_LED, OUTPUT);
  digitalWrite(BUILTIN_LED, HIGH);
  Serial.begin(115200);
  delay(1000);

  Serial.print("\n\nConnecting to ");
  Serial.print(ssid);
  
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.print("\nWiFi connected IP address: ");
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
  setupOneWire();
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

void setupOneWire()
{
	pinMode(inPin, INPUT);
	sensors.begin();
	lastTemp = 0;
	lastMsg = 0;
	toggle = false;

	deviceCount = sensors.getDeviceCount();
	if (deviceCount > MAX_ONE_WIRE_DEVICES)
	{
		Serial.print("WARNING: Number of oneWire devices exceeds deviceCountMax");
		deviceCount = MAX_ONE_WIRE_DEVICES;
	}

	Serial.print("DeviceCount:");
	Serial.println(deviceCount);

	for (int i = 0; i < deviceCount; i++)  // device is an array of OW devices
	{
		if (sensors.getAddress(device[i], i))
			printOWAddress(device[i]);
		else
		{
			Serial.print("Unable to find address for Device ");
			Serial.println(i);
		}
			
	}

	sensors.setResolution(12);
}

void oneWireHandler(void)
{
	/// oneWire Temperature code 
	String topic;
	float temperature;

	long now = millis();
	if (now - lastTemp > (20 * 1000))  // 20 seconds
	{
		lastTemp = now;

		sensors.requestTemperatures(); // Send the command to get temperatures
		for (int i = 0; i < deviceCount; i++)
		{
			temperature = sensors.getTempFByIndex(i);
			Serial.print("Device ");  Serial.print(i); Serial.print(": ");
			Serial.print(temperature);
			// publish regardless of value. let upstream figure it out
			topic = String(clientID) + String("/temp") + String(i);
			Serial.print(" /"); Serial.print(topic); 
			String valueStr(temperature);
			for (int t = 0; t < 5; t++)
			{
				if (myMqtt.publish(topic, valueStr))
				{
					Serial.print(" Published");
					break;
				}
				Serial.println("F ");
			}
			Serial.println("");
		}
		/*
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
		*/

	}
}

#endif

