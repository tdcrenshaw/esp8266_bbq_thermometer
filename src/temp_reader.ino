//here's where we read the temp
#include <ESP8266WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include <Wire.h>
#include "SSD1306.h"
#include "font.h"

//defining display pins
SSD1306  display(0x3c, D3, D5);

//WiFi info
#define WLAN_SSID       "Pretty Fly For a WiFi"
#define WLAN_PASS       "cubagoodingjr"

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
Adafruit_MQTT_Publish GrillTemp = Adafruit_MQTT_Publish(&mqtt, GrillTempFeed);


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
float TempSamples[0];

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
  Serial.println(TempSamples[0]);
  Serial.println();
  // Serial.println("Meat Temp is: ");
  // Serial.print(TempSamples[1]);

  displaytemp();
  //mqtt_upload();
  delay(1000);
}

void sample() {
//figure out how to return them both //used global variables

    //might could make sizeof work here, but it returns byte size
    //for now I want it to run once, will run twice eventually
    for (int z=0; z<1; z++) {
      // take N samples in a row, with a slight delay
      for (int i=0; i< NUMSAMPLES; i++) {
       samples[i] = analogRead(pin);
       delay(10);
      }

      // average all the samples out
      TempSamples[z] = 0;
      for (int i=0; i< NUMSAMPLES; i++) {
         TempSamples[z] += samples[i];
      }
      TempSamples[z] /= NUMSAMPLES;

      Serial.print("Average analog reading " + String(z) + ": ");
      Serial.println(TempSamples[z]);

    }

}
void convert() {
    for (int z=0; z<1; z++) {
        // convert the value to resistance
        TempSamples[z] = 1023 / TempSamples[z] - 1;
        TempSamples[z] = SERIESRESISTOR / TempSamples[z];
        Serial.print("Thermistor resistance " + String(z) + ": ");
        Serial.println(TempSamples[z]);

        float steinhart;
        steinhart = TempSamples[z] / THERMISTORNOMINAL;     // (R/Ro)
        steinhart = log(steinhart);                  // ln(R/Ro)
        steinhart /= BCOEFFICIENT;                   // 1/B * ln(R/Ro)
        steinhart += 1.0 / (TEMPERATURENOMINAL + 273.15); // + (1/To)
        steinhart = 1.0 / steinhart;                 // Invert
        TempSamples[z] = steinhart * 9 / 5 - 459.67;     // convert to F double check this, I changed it from c to f conversion

        // Serial.print("Temperature ");
        // Serial.print(TempSamples[z]);
        // Serial.println(" *F");

        }
    }


void mqtt_upload() {
    // ping adafruit io a few times to make sure we remain connected
    if(! mqtt.ping(3)) {
      // reconnect to adafruit io
      if(! mqtt.connected())
        connect();
    }
    int current = random(175,600);
    Serial.println("About to hit dat mqtt");
    if (! GrillTemp.publish((int32_t)current))
      Serial.println(F("Grill upload failed."));
    else
      Serial.println(F("Grill upload Success!"));
}


void displaytemp() {

    //for rounding
    TempSamples[0] = TempSamples[0] + .5;
    // TempSamples[1] = TempSamples[1] + .5;

    //convert to int to fit on screen better
    String GrillString = " Grill: " + String(int(TempSamples[0]));
    // String MeatString = "Meat: " + String(int(TempSamples[1]));
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
