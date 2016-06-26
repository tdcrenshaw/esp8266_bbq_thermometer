//Tyler Crenshaw
//insert open software info here
#include <ESP8266WiFi.h>

#include <Wire.h>
#include "SSD1306.h"
#include "font.h"

//defining display pins
SSD1306  display(0x3c, D3, D5);

//WiFi info
#define WLAN_SSID       "Pretty Fly For a WiFi"
#define WLAN_PASS       "cubagoodingjr"

//sparkfun connection info
const char* host = "data.sparkfun.com";
const char* publicKey = "RM4KV6gqxWcGv5gwvgnb";
const char* privateKey = "lzNeK5qpEkCeyKrByrvY";

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

  //put uploader in an interrupt so we don't get banned from sparkfun
  //http_upload();

  //delay(1000);
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
// void http_upload() {
//     Serial.println("trying http upload");
//     // Create an ESP8266 WiFiClient class to connect to the http server.
//     WiFiClient client;
//     const int httpPort = 80;
//     if (!client.connect(host, httpPort)) {
//       Serial.println("connection failed");
//       return;
//     }
//       // We now create a URI for the request
//       String url = "/input/";
//       url += publicKey;
//       url += "?private_key=";
//       url += privateKey;
//       url += "&grilltemp=";
//       url += TempSamples[0];
//
//       Serial.print("Requesting URL: ");
//       Serial.println(url);
//
//       // This will send the request to the server
//       client.print(String("GET ") + url + " HTTP/1.1\r\n" +
//                    "Host: " + host + "\r\n" +
//                    "Connection: close\r\n\r\n");
//       delay(10);
//
//       // Read all the lines of the reply from server and print them to Serial
//       while(client.available()){
//         String line = client.readStringUntil('\r');
//         Serial.print(line);
//       }
//
//       Serial.println();
//       Serial.println("closing connection");
// }
