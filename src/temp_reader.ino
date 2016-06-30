//here's where we read the temp
#include <ESP8266WiFi.h>
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
int TargetTemp = 700;
int TempTrigger = 0;

//sparkfun info
const char* host = "data.sparkfun.com";
const char* publicKey = "RM4KV6gqxWcGv5gwvgnb";
const char* privateKey = "lzNeK5qpEkCeyKrByrvY";

//pushbullet info
const char* pbhost = "api.pushbullet.com";
const int httpsPort = 443;
const char* PushBulletAPIKEY = "o.QTZncBKUF0NpAV3qS1ikq8Ftc83PToHO"; //get it from your pushbullet account

// Use web browser to view and copy
// SHA1 fingerprint of the certificate
const char* fingerprint = "2C BC 06 10 0A E0 6E B0 9E 60 E5 96 BA 72 C5 63 93 23 54 B3"; //got it using https://www.grc.com/fingerprints.htm


// Create an ESP8266 WiFiClient class to connect to the MQTT server.
WiFiClient client;

// which analog pin to connect
//pins will be multiplexed
#define pin A0
// #define MeatPin D1
// #define GrillPin D2

// resistance at 25 degrees C
#define THERMISTORNOMINAL 2000000
// temp. for nominal resistance (almost always 25 C)
#define TEMPERATURENOMINAL 25
// how many samples to take and average, more takes longer
// but is more 'smooth'
#define NUMSAMPLES 50
// The beta coefficient of the thermistor (usually 3000-4000)
#define BCOEFFICIENT 3423
// the value of the 'other' resistor
#define SERIESRESISTOR 10000



float GrillTemp;
float MeatTemp;

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

  SampleGrill();
  SampleMeat();

  convert();

  Serial.print("Grill Temp is: ");
  Serial.println(GrillTemp);
  Serial.println();
  Serial.print("Meat Temp is: ");
  Serial.println(MeatTemp);
  Serial.println();

  if (MeatTemp > TargetTemp && TempTrigger == 0){
      SendNotification();
      TempTrigger = 1;
  }

  displaytemp();

  uploaddata();

  delay(1000);
}

void SampleGrill() {

    pinMode(5,INPUT);
    pinMode(4,OUTPUT);
    digitalWrite(4,LOW);
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
    GrillTemp = 1023 / GrillTemp - 1;
    GrillTemp = SERIESRESISTOR / GrillTemp;
    Serial.print("Thermistor resistance: ");
    Serial.println(GrillTemp);

}

void SampleMeat() {

    pinMode(4,INPUT);
    pinMode(5,OUTPUT);
    digitalWrite(5,LOW);
    //might could make sizeof work here, but it returns byte size
    //for now I want it to run once, will run twice eventually
    // take N samples in a row, with a slight delay
    for (int i=0; i< NUMSAMPLES; i++) {
    samples[i] = analogRead(pin);
    delay(10);
    }

    // average all the samples out
    MeatTemp = 0;
    for (int i=0; i< NUMSAMPLES; i++) {
     MeatTemp += samples[i];
    }
    MeatTemp /= NUMSAMPLES;

    Serial.print("Average analog reading: ");
    Serial.println(MeatTemp);
    MeatTemp = 1023 / MeatTemp - 1;
    MeatTemp = SERIESRESISTOR / MeatTemp;
    Serial.print("Thermistor resistance: ");
    Serial.println(MeatTemp);
}

void convert() {

    float steinhart;
    steinhart = GrillTemp / THERMISTORNOMINAL;     // (R/Ro)
    steinhart = log(steinhart);                  // ln(R/Ro)
    steinhart /= BCOEFFICIENT;                   // 1/B * ln(R/Ro)
    steinhart += 1.0 / (TEMPERATURENOMINAL + 273.15); // + (1/To)
    steinhart = 1.0 / steinhart;                 // Invert
    GrillTemp = steinhart * 9 / 5 - 459.67;     // K to F conversion

    steinhart = MeatTemp / THERMISTORNOMINAL;     // (R/Ro)
    steinhart = log(steinhart);                  // ln(R/Ro)
    steinhart /= BCOEFFICIENT;                   // 1/B * ln(R/Ro)
    steinhart += 1.0 / (TEMPERATURENOMINAL + 273.15); // + (1/To)
    steinhart = 1.0 / steinhart;                 // Invert
    MeatTemp = steinhart * 9 / 5 - 459.67;     // K to F conversion
    }

void displaytemp() {
    //clear old data off screen

    display.clear();
    //for rounding
    GrillTemp = GrillTemp + .5;

    //convert to int to fit on screen better
    String GrillString = " Grill: " + String(int(GrillTemp));
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.drawString(3,10, GrillString);

    //for rounding
    MeatTemp = MeatTemp + .5;

    //convert to int to fit on screen better
    String MeatString = " Meat: " + String(int(MeatTemp));
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.drawString(3,33, MeatString);

    display.display();
}
void uploaddata() {

    // We now create a URI for the request
    String url = "/input/";
    url += publicKey;
    url += "?private_key=";
    url += privateKey;
    url += "&grilltemp=";
    url += GrillTemp;
    url += "&meattemp=";
    url += MeatTemp;

    Serial.print("Requesting URL: ");
    Serial.println(url);

    const int httpPort = 80;
    if (!client.connect(host, httpPort)) {
      Serial.println("connection failed");
      return;
    }
    // This will send the request to the server
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + host + "\r\n" +
                 "Connection: close\r\n\r\n");
    delay(10);

    // Read all the lines of the reply from server and print them to Serial
    while(client.available()){
      String line = client.readStringUntil('\r');
      Serial.print(line);
    }

    Serial.println();
    Serial.println("closing connection");

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
    String messagebody = "{\"type\": \"note\", \"title\": \"Meat is Ready!!!\", \"body\": \"The Meat is currently at " + String(MeatTemp) + " degrees F\"}\r\n";
    Serial.print("requesting URL: ");
    Serial.println(url);

    client.print(String("POST ") + url + " HTTP/1.1\r\n" +
               "Host: " + pbhost + "\r\n" +
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
