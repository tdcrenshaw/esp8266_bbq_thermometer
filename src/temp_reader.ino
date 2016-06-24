//here's where we read the temp
	#import esp8266

// which analog pin to connect
//pins will be multiplexed
#define TempPin A0
// resistance at 25 degrees C
#define THERMISTORNOMINAL 10000
// temp. for nominal resistance (almost always 25 C)
#define TEMPERATURENOMINAL 25
// how many samples to take and average, more takes longer
// but is more 'smooth'
#define NUMSAMPLES 5
// The beta coefficient of the thermistor (usually 3000-4000)
#define BCOEFFICIENT 3950
// the value of the 'other' resistor
#define SERIESRESISTOR 10000

//for now, we'll say one. Change to two when we get another themistor
//done globably so there's no fuckery with returns and arrarys
float TempSamples[1];

int samples[NUMSAMPLES];

void setup(void) {
  Serial.begin(9600);
  analogReference(EXTERNAL);
}

void loop(void) {
  uint8_t i;

  sample(TempPin);

  convert();


  Serial.println("Grill Temp is: ");
  Serial.print(TempSamples[0]);

  Serial.println("Meat Temp is: ");
  Serial.print(TempSamples[1]);

  delay(1000);
}

void sample(pin) {
//figure out how to return them both //used global variables

    //might could make sizeof work here, but it returns byte size
    //for now I want it to run once, will run twice eventually
    for (z=0; z<1; z++) {
      // take N samples in a row, with a slight delay
      for (i=0; i< NUMSAMPLES; i++) {
       samples[i] = analogRead(THERMISTORPIN);
       delay(10);
      }

      // average all the samples out
      TempSamples[z] = 0;
      for (i=0; i< NUMSAMPLES; i++) {
         TempSamples[z] += samples[i];
      }
      TempSamples[z] /= NUMSAMPLES;

      Serial.print("Average analog reading ");
      Serial.println(TempSamples[z]);

    }

}
void convert() {
    for (z=0; z<1; z++) {
        // convert the value to resistance
        TempSamples[z] = 1023 / TempSamples[z] - 1;
        TempSamples[z] = SERIESRESISTOR / TempSamples[z];
        Serial.print("Thermistor resistance ");
        Serial.println(TempSamples[z]);

        float steinhart;
        steinhart = TempSamples[z] / THERMISTORNOMINAL;     // (R/Ro)
        steinhart = log(steinhart);                  // ln(R/Ro)
        steinhart /= BCOEFFICIENT;                   // 1/B * ln(R/Ro)
        steinhart += 1.0 / (TEMPERATURENOMINAL + 273.15); // + (1/To)
        steinhart = 1.0 / steinhart;                 // Invert
        steinhart = steinhart * 9 / 5 - 459.67;     // convert to F double check this, I changed it from c to f conversion

        Serial.print("Temperature ");
        Serial.print(steinhart);
        Serial.println(" *F");
        }
    }
}
