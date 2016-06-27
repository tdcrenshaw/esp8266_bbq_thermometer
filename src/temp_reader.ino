//here's where we read the temp
#include <ESP8266WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include <Wire.h>
#include "SSD1306.h"
#include "font.h"
#include <WiFiClientSecure.h>

//defining display pins
SSD1306  display(0x3c, D3, D5);

//WiFi info
#define WLAN_SSID       "Pretty Fly For a WiFi"
#define WLAN_PASS       "cubagoodingjr"

//set target temp in F here
int TargetTemp = 135;
int TempTrigger = 0;

// Create an ESP8266 WiFiClient class to connect to the MQTT server.
WiFiClient client;


//adafruit.io credentials
#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883
#define AIO_USERNAME    "tdcrenshaw"
#define AIO_KEY "6223252d03f04a04bcca6538b8fc2470"

// Store the MQTT server, client ID, username, and password in flash memory.
// This is required for using the Adafruit MQTT library.
const char MQTT_SERVER[] PROGMEM    = AIO_SERVER;
// Set a unique MQTT client ID using the AIO key + the date and time the sketch
// was compiled (so this should be unique across multiple devices for a user,
// alternatively you can manually set this to a GUID or other random value).
const char MQTT_CLIENTID[] PROGMEM  = __TIME__ AIO_USERNAME;
const char MQTT_USERNAME[] PROGMEM  = AIO_USERNAME;
const char MQTT_PASSWORD[] PROGMEM  = AIO_KEY;

// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
Adafruit_MQTT_Client mqtt(&client, MQTT_SERVER, AIO_SERVERPORT, MQTT_CLIENTID, MQTT_USERNAME, MQTT_PASSWORD);

// Setup a feed called 'grill-temp' for publishing changes.
// Notice MQTT paths for AIO follow the form: <username>/feeds/<feedname>
const char GrillTempFeed[] PROGMEM = AIO_USERNAME "/feeds/grill-temp";
Adafruit_MQTT_Publish GrillTempMQTT = Adafruit_MQTT_Publish(&mqtt, GrillTempFeed);

//pushbullet info
const char* host = "api.pushbullet.com";
const int httpsPort = 443;
const char* PushBulletAPIKEY = "o.QTZncBKUF0NpAV3qS1ikq8Ftc83PToHO"; //get it from your pushbullet account

// Use web browser to view and copy
// SHA1 fingerprint of the certificate
const char* fingerprint = "2C BC 06 10 0A E0 6E B0 9E 60 E5 96 BA 72 C5 63 93 23 54 B3"; //got it using https://www.grc.com/fingerprints.htm


// which analog pin to connect
//pins will be multiplexed
#define pin A0
// resistance at 25 degrees C
#define THERMISTORNOMINAL 10000
// temp. for nominal resistance (almost always 25 C)
#define TEMPERATURENOMINAL 25
// how many samples to take and average, more takes longer
// but is more 'smooth'
#define NUMSAMPLES 50
// The beta coefficient of the thermistor (usually 3000-4000)
#define BCOEFFICIENT 3950
// the value of the 'other' resistor
#define SERIESRESISTOR 10000

//for now, we'll say one. Change to two when we get another themistor
//done globably so there's no fuckery with returns and arrarys
float GrillTemp;

int samples[NUMSAMPLES];

void setup(void) {
  Serial.begin(115200);
  //analogReference(EXTERNAL);

  display.init();
  display.flipScreenVertically();
  display.setFont(Droid_Sans_Bold_25);
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.drawString(64,20, "Let's Grill!!");
  display.display();

  // Connect to WiFi access point.
  Serial.println(); Serial.println();
  delay(10);
  Serial.print(F("Connecting to "));
  Serial.println(WLAN_SSID);

  WiFi.begin(WLAN_SSID, WLAN_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(F("."));
  }
  Serial.println();

  Serial.println(F("WiFi connected"));
  Serial.println(F("IP address: "));
  Serial.println(WiFi.localIP());

  // connect to adafruit io
  connect();
}

void loop(void) {
  uint8_t i;

  sample();

  convert();
  Serial.print("Grill Temp is: ");
  Serial.println(GrillTemp);
  Serial.println();
  // Serial.println("Meat Temp is: ");
  // Serial.print(REMOVE);

  if (GrillTemp > TargetTemp && TempTrigger == 0){
      SendNotification();
      TempTrigger = 1;
  }

  displaytemp();

  mqtt_upload();

  delay(1000);
}

void sample() {
    //figure out how to return them both //used global variables

    //might could make sizeof work here, but it returns byte size
    //for now I want it to run once, will run twice eventually

    // take N samples in a row, with a slight delay
    for (int i=0; i< NUMSAMPLES; i++) {
    samples[i] = analogRead(pin);
    delay(10);
    }

    // average all the samples out
    GrillTemp = 0;
    for (int i=0; i< NUMSAMPLES; i++) {
     GrillTemp += samples[i];
    }
    GrillTemp /= NUMSAMPLES;

    Serial.print("Average analog reading: ");
    Serial.println(GrillTemp);
}

void convert() {
    // convert the value to resistance
    GrillTemp = 1023 / GrillTemp - 1;
    GrillTemp = SERIESRESISTOR / GrillTemp;
    Serial.print("Thermistor resistance: ");
    Serial.println(GrillTemp);

    float steinhart;
    steinhart = GrillTemp / THERMISTORNOMINAL;     // (R/Ro)
    steinhart = log(steinhart);                  // ln(R/Ro)
    steinhart /= BCOEFFICIENT;                   // 1/B * ln(R/Ro)
    steinhart += 1.0 / (TEMPERATURENOMINAL + 273.15); // + (1/To)
    steinhart = 1.0 / steinhart;                 // Invert
    GrillTemp = steinhart * 9 / 5 - 459.67;     // convert to F double check this, I changed it from c to f conversion

    // Serial.print("Temperature ");
    // Serial.print(GrillTemp);
    // Serial.println(" *F");

    }

void displaytemp() {

    //for rounding
    GrillTemp = GrillTemp + .5;
    // REMOVE = REMOVE + .5;

    //convert to int to fit on screen better
    String GrillString = " Grill: " + String(int(GrillTemp));
    // String MeatString = "Meat: " + String(int(REMOVE));
    display.clear();
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.drawString(3,10, GrillString);
    //    display.drawString(0,33, MeatString);
    display.display();

}

void connect() {

    Serial.print(F("Connecting to Adafruit IO... "));

    int8_t ret;

    while ((ret = mqtt.connect()) != 0) {

    switch (ret) {
      case 1: Serial.println(F("Wrong protocol")); break;
      case 2: Serial.println(F("ID rejected")); break;
      case 3: Serial.println(F("Server unavail")); break;
      case 4: Serial.println(F("Bad user/pass")); break;
      case 5: Serial.println(F("Not authed")); break;
      case 6: Serial.println(F("Failed to subscribe")); break;
      default: Serial.println(F("Connection failed")); break;
    }

    if(ret >= 0)
      mqtt.disconnect();

    Serial.println(F("Retrying connection..."));
    delay(5000);

    }

    Serial.println(F("Adafruit IO Connected!"));

    }

void mqtt_upload() {
    // ping adafruit io a few times to make sure we remain connected
    if(! mqtt.ping(3)) {
      // reconnect to adafruit io
      if(! mqtt.connected())
        connect();
    }

    if (! GrillTempMQTT.publish((int32_t)GrillTemp))
      Serial.println(F("Grill upload failed."));
    else
      Serial.println(F("Grill upload Success!"));
}

void SendNotification() {
    // Use WiFiClientSecure class to create TLS connection
    WiFiClientSecure client;
    Serial.print("connecting to ");
    Serial.println(host);
    if (!client.connect(host, httpsPort)) {
        Serial.println("connection failed");
    return;
    }

    if (client.verify(fingerprint, host)) {
        Serial.println("certificate matches");
    } else {
        Serial.println("certificate doesn't match");
    }
    String url = "/v2/pushes";
    String messagebody = "{\"type\": \"note\", \"title\": \"Meat is Ready!!!\", \"body\": \"The Meat is Currently at the target temp\"}\r\n";
    Serial.print("requesting URL: ");
    Serial.println(url);

    client.print(String("POST ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "Authorization: Bearer " + PushBulletAPIKEY + "\r\n" +
               "Content-Type: application/json\r\n" +
               "Content-Length: " +
               String(messagebody.length()) + "\r\n\r\n");
    client.print(messagebody);



    Serial.println("request sent");

    //print the response

    while (client.available() == 0);

    while (client.available()) {
        String line = client.readStringUntil('\n');
        Serial.println(line);
    }
}
