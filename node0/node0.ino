#include <Creds.h>
#include <ESP8266WiFi.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <MQTT.h>
#include <SimpleDHT.h>


void myDataCb(String& topic, String& data);
void myPublishedCb();
void myDisconnectedCb();
void myConnectedCb();

const char*  clientID = "North_Shop";

int err_cnt = 0;
int cnt = 0;
// for DHT11, 
//      VCC: 5V or 3V
//      GND: GND
//      DATA: 2
int pinDHT11 = D7;
SimpleDHT11 dht11;

// create MQTT object
MQTT myMqtt(clientID, "silver", 1883);

//
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("=========");
  Serial.println("  node0");
  Serial.println("=========");

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
  cnt = 0;
  err_cnt = 0;

  delay(10);
}

boolean pubValue(String what, String sensor, int value)
{
	String topic("/");
    topic += clientID + String("/") + sensor + String("/") + what;
	String valueStr(value);

	// publish value to topic 	
	return myMqtt.publish(topic, valueStr);
}

void loop() {

	byte temperature = 0;
	byte humidity = 0;
  int err = SimpleDHTErrSuccess;
  if (err = dht11.read(pinDHT11, &temperature, &humidity, NULL) != SimpleDHTErrSuccess) 
  {
	  Serial.print("Read DHT11 failed, err="); Serial.println(err); err_cnt++;
  }
  else
  {
	  temperature = ((temperature * 9) / 5) + 32;
	  Serial.print("DHT11, ");
	  Serial.print((int)temperature); Serial.print(" *F, ");
	  Serial.print((int)humidity); Serial.print(" RH ");

	 // boolean result = myMqtt.publish("temperature", value);
	  pubValue("temperature", "DTH11-0", temperature);
	  pubValue("humidity", "DTH11-0", humidity);
  }
  Serial.print("  Reads: "); Serial.print(++cnt);
  Serial.print(", errors: "); Serial.println(err_cnt);
    
  delay(15000);
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



