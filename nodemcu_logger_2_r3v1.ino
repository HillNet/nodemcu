/*-----( Import needed libraries )-----*/
#include <OneWire.h>
#include <DallasTemperature.h>
#include <DHT_U.h>
#include <DHT.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

/*-----( Declare Constants and Pin Numbers )-----*/
#define DHTPIN 0       // 2 digital pin
#define DHTTYPE DHT22  // DHT 22(AM2302)
#define ONE_WIRE_BUS 2 // GPIO3=Rx
#define OLED_RESET LED_BUILTIN //4
#define tho "sensor/dht22/humidity"
#define tto "sensor/dht22/temperature"
#define tt1 "sensor/28FF531294160563/temperature"
//#define tt2 "sensor/28FF52E793160524/temperature"
//#define tt3 "sensor/28FF0E3E8C1603CA/temperature"


/*-----( Declare objects )-----*/
// OLED display
Adafruit_SSD1306 display(OLED_RESET);

// DHT 22(AM2302)
DHT dht(DHTPIN, DHTTYPE);

// DS18B20 sensor
OneWire oneWire(ONE_WIRE_BUS); //oneWire instance to communicate with any OneWire devices
DallasTemperature sensors(&oneWire);// Pass address of our oneWire instance to Dallas Temperature.
DeviceAddress Probe01 = { 0x28, 0xFF, 0x53, 0x12, 0x94, 0x16, 0x05, 0x63 };
//DeviceAddress Probe02 = { 0x28, 0xFF, 0x52, 0xE7, 0x93, 0x16, 0x05, 0x24 };
//DeviceAddress Probe03 = { 0x28, 0xFF, 0x0E, 0x3E, 0x8C, 0x16, 0x03, 0xCA };

// Initialize the Ethernet client object
WiFiClient espClient;

// Initialize mqtt server object
PubSubClient client(espClient);

void setup() {
  Serial.begin(9600);
  setup_wifi();
  client.setServer("192.168.X.X", 1883);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.display();
  delay(1000);
  display.clearDisplay();
  display.display();
  // DS18B20 sensor
  // Resolution, 9 to 12 bits (lower is faster)
  sensors.setResolution(Probe01, 10);
  //sensors.setResolution(Probe02, 10);
  //sensors.setResolution(Probe03, 10);
  // DHT 22(AM2302)
  dht.begin();
}

void setup_wifi() {
  delay(10);
  WiFi.hostname("hostname");
  // We start by connecting to a WiFi network
  WiFi.begin("SSID", "passwd");
  delay(500);
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    // Attempt to connect
    if (client.connect("ESP8266Client", "uname", "passwd")) {
      Serial.println("connected\n");
    } else {
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void displayPage1() {
  sensors.requestTemperatures();
  float t1 = sensors.getTempC(Probe01);
  //float t2 = sensors.getTempC(Probe02);
  //float t3 = sensors.getTempC(Probe03);
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(2);
  display.setCursor(0, 10);
  display.print("Ti:");
  display.print(t1);
  display.print(" C");
  delay(2000);
  // Publish
  client.publish(tt1, String(t1).c_str(), true);
}

void displayPage2() {
  delay(2000);
  // Reading takes about 250 milliseconds!
  float ho = dht.readHumidity();
  float to = dht.readTemperature();

  // Check if any reads failed and exit early (to try again).
  if (isnan(ho) || isnan(to)) {
    display.clearDisplay(); 
    display.setTextColor(WHITE);
    display.setTextSize(1);
    display.setCursor(0, 10);
    display.print("Failed to read from  DHT sensor!");
    return;
  }
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(2);
  display.setCursor(0, 10);
  display.print("Ho:");
  display.print(ho);
  display.print(" %");
  display.setCursor(0, 40);
  display.print("To:");
  display.print(to);
  display.print(" C");
  delay(2000);
  // Publish
  client.publish(tho, String(ho).c_str(), true);
  delay(1000);
  client.publish(tto, String(to).c_str(), true);
}

void loop() {
  // Wait a few seconds between measurements.
  // delay(5000); //reset to 30000
  // Connect to MQTT
  if (!client.connected())
  {
    reconnect();
  } else {
    client.loop();
  }
  // Display
  displayPage1();
  display.display();
  delay(1000);
  displayPage2();
  display.display();
}
