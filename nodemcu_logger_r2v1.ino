/*-----( Import needed libraries )-----*/
#include <DHT_U.h>
#include <DHT.h>
#include <Adafruit_Sensor.h>
#include <OneWire.h>
#include <PubSubClient.h>
#include <ESP8266WiFi.h>

/*-----( Declare Constants and Pin Numbers )-----*/
#define DHTPIN 2       // what digital pin we're connected to
#define DHTTYPE DHT22  // DHT 22(AM2302)

char hexChars[] = "0123456789ABCDEF";
#define HEX_MSB(v) hexChars[(v & 0xf0) >> 4]
#define HEX_LSB(v) hexChars[v & 0x0f]

/*-----( Declare objects )-----*/
// Initialize DHT sensor.
DHT dht(DHTPIN, DHTTYPE);
// Setup a oneWire instance
OneWire  ds(4);  // on pin 4 (a 4.7K resistor is necessary)
// Initialize the Ethernet client object
WiFiClient espClient;
// Initialize mqtt server object
PubSubClient client(espClient);

void setup() {
  Serial.begin(115200);
  setup_wifi();
  client.setServer("192.168.0.10", 1883);
}

void setup_wifi() {
  delay(10);
  WiFi.hostname("hillnode_1");
  // We start by connecting to a WiFi network
  WiFi.begin("hillnet_2.4Ghz", "overthehill");

  Serial.println();
  Serial.println();
  Serial.print("Wait for WiFi... ");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  delay(500);
}

void reconnect()
{
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.println();
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP8266Client", "hillduino", "tuxuser#12")) {
      Serial.println("connected\n");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void loop() {
  //Connect to MQTT
  if (!client.connected())
  {
    reconnect();
  } else {
    client.loop();
  }
  byte i;
  byte present = 0;
  byte type_s;
  byte data[12];
  byte addr[8];
  float celsius;

  if ( !ds.search(addr)) {
    //Serial.println("No more addresses.\n");
    ds.reset_search();

    Serial.println("Chip = AM2302");
    // Reading temperature or humidity takes about 250 milliseconds!
    float h = dht.readHumidity();
    // Read temperature as Celsius (the default)
    float t = dht.readTemperature();

    // Check if any reads failed and exit early (to try again).
    if (isnan(h) || isnan(t)) {
      Serial.println("Failed to read from DHT sensor!");
      return;
    }

    Serial.print("Humidity: ");
    Serial.print(h);
    Serial.print(" %\n");
    client.publish("sensor/dht22/humidity", String(h).c_str(), true);
    Serial.print("Temperature: ");
    Serial.print(t);
    Serial.println(" *C ");
    client.publish("sensor/dht22/temperature", String(t).c_str(), true);
    Serial.print('\n');

    Serial.println("Pausing between search's");
    delay(30000); //delay between finding all the sensors
    return;
  }

  Serial.println("Chip = DS18B20");
  ds.reset();
  ds.select(addr);
  ds.write(0x44, 1);// start conversion, with parasite power on at the end
  delay(1000);      // maybe 750ms is enough, maybe not

  present = ds.reset();
  ds.select(addr);
  ds.write(0xBE);   // Read Scratchpad

  for ( i = 0; i < 9; i++) {  // we need 9 bytes
    data[i] = ds.read();
  }

  // Convert the data to actual temperature
  int16_t raw = (data[1] << 8) | data[0];
  if (type_s) {
    raw = raw << 3; // 9 bit resolution default
    if (data[7] == 0x10) {
      // "count remain" gives full 12 bit resolution
      raw = (raw & 0xFFF0) + 12 - data[6];
    }
  } else {
    byte cfg = (data[4] & 0x60);
    // at lower res, the low bits are undefined, so let's zero them
    if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
    else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
    else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
    // default is 12 bit resolution, 750 ms conversion time
  }

  celsius = (float)raw / 16.0;
  Serial.print("Temperature: ");
  Serial.print(celsius);
  Serial.println(" *C ");
  Serial.print('\n');

  //publish the temp now
  char charTopic[] = "sensor/XXXXXXXXXXXXXXXX/temperature";
  for (i = 0; i < 8; i++) {
    charTopic[7 + i * 2] = HEX_MSB(addr[i]); //7 is where the backlash before XXX
    charTopic[8 + i * 2] = HEX_LSB(addr[i]); //8 is plus one on the above
  }
  char charMsg[10];
  memset(charMsg, '\0', 10);
  dtostrf(celsius, 4, 2, charMsg);
  client.publish(charTopic, charMsg);
  delay(1000); // small delay between publishing
}
