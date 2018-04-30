#include "cactus_io_DHT22.h"    		    //Library DHT22
#include <Wire.h>               		    //Library bh1750 (SDA dan SCL)
#include <BH1750.h>                     // Library BH1750
#include <OneWire.h>                    // Library EC

/* INISIALISASI PIN AKTUATOR DAN SENSOR */
#define relayPumpAB  13                 // Relay 1 IN4
#define relayHumidifier  12             // Relay 1 IN3
#define relayHeater 11                  // Relay 1 IN2
#define relayLED  10                    // Relay 1 IN1
#define relayFanIn 9                    // Relay 2 IN4
#define relayFanOut 8                   // Relay 2 IN3
#define ledPutih 7                      // Relay 2 IN2
#define relayPeltier  6                 // Relay 2 IN1

#define LED  4                          // LED grow dari IRF540
#define DHT22_PIN 3                     // Sensor Suhu dan Kelembaban
// pin 14 dan 15 (TX3 dan RX3) untuk komunikasi serial
// pin 20 dan 21 (SDA dan SCL) untuk BH1750

#define analogInput A0                  // untuk MQ135
#define sensorPin  A1                   // sensor pH
#define waterLevel A3                   // sensor level air

/* INISIALISASI KONSTANTA */
#define co2Zero 25                      // nilai kalibrasi CO2
#define satuHari 86400000             	// milisekon satu hari
#define StartConvert 0                  // state convert nilai
#define ReadTemperature 1               // state pembacaan temperatur

/* VARIABEL */
byte ECsensorPin = A2;                  // pin A2 untuk sensor EC
byte DS18B20_Pin = 2;                   // pin 2 untuk DS18B20
int bright= 0;                          // variabel brightness LEDGrow menggunakan PWM
unsigned long currentMillis = 0;        // nilai milisekon sekarang
unsigned long previousMillis = 0;       // nilai milisekon sebelumnya
unsigned long previousLED = 0;        	// nilai milisekon penyinaran sebelumnya
float suhuIn;                           // variabel penyimpanan input suhu
int kelembabanIn;                       // variabel penyimpanan input kelembaban
int co2In;                              // variabel penyimpanan input CO2
int intensitasIn;                       // variabel penyimpanan input intensitas cahaya
float phIn;                             // variabel penyimpanan input pH
unsigned long nutrisiIn;                // variabel penyimpanan input konduktivitas nutrisi
unsigned long waktusiramIn;             // variabel penyimpanan input waktu penyiraman
unsigned long penyinaranIn;             // variabel penyimpanan input durasi penyinaran cahaya
String stringInput;                     // variabel penyimpanan input pengguna
String kondisiAktual;                   // variabel penyimpanan kondisi Aktual untuk dikirim serial
String kondisiSuhu;                     // variabel penyimpanan kondisi aktual suhu
String kondisiKelembaban;               // variabel penyimpanan kondisi aktual kelembaban
char charInput[30];                     // tabel penyimpan string input pengguna dari komunikasi serial
char *str;
char *p ;
unsigned long int avgValue;             // variabel penyimpanan nilai rata-rata sensor ph
float b;
int buf[10];                            // aray untuk pH
int tempPh;                             // variabel penyimpanan untuk ph
const byte numReadings = 20;            // jumlah pembacaan untuk EC
unsigned int AnalogSampleInterval=25;   // interval waktu sampling EC
unsigned int printInterval=700;         // interval waktu serial print
unsigned int tempSampleInterval=850;    // interval waktu sampling DS18B20
unsigned int readings[numReadings];     // pembacaan analog input EC
byte index = 0;                         // index pembacaan
unsigned long AnalogValueTotal = 0;     // total nilai pembacaan EC
unsigned int AnalogAverage = 0;         // variabel penyimpanan rata-rata pembacaan analog
unsigned int averageVoltage = 0 ;       // variabel penyimpanan rata-rata tegangan
unsigned long AnalogSampleTime;         // waktu sampling sensor EC
unsigned int printTime;                 // waktu pengiriman nilai sensor EC
unsigned int tempSampleTime;            // varibel waktu sampling suhu
float temperature;                      // variabel penyimpan nilai suhu
float ECcurrent;                        // variabel penyimpan nilai EC
String stateFanIn;                      // kondisi fan in untuk notifikasi
String stateFanOut;                     // kondisi fan out untuk notifikasi
String stateHeater;                     // kondisi heater untuk notifikasi
String stateHumidifier;                 // kondisi humidifier untuk notifikasi
String statePeltier;                    // kondisi peltier untuk notifikasi
String statePump;                       // kondisi pompa nutrisi untuk notifikasi
String statePumpAB;                     // kondisi pompa asam untuk notifikasi
String stateLED;                        // kondisi led grow untuk notifikasi
String levelAir;                        // kondisi tinggi air dalam tanki untuk notifikasi
int sensorValue = 0;                    // variable untuk menampung nilai baca dari sensor dalam bentuk integer
float tinggiAir = 0;                    // variabel untuk menampung ketinggian air


BH1750 lightMeter;
DHT22 dht(DHT22_PIN);
OneWire ds(DS18B20_Pin);                // digital pin 2 untuk sensor intensitas


void setup()
{
  // UNTUK KOMUNIKASI SERIAL
  Serial.begin(115200);                 // komunikasi serial ke raspi
  Serial3.begin(115200);                // komunikasi serial ke nodeMCU

  // PENGATURAN SENSOR CO2 (MQ135)
  pinMode(analogInput, INPUT);
  //pinMode(digitalTrigger, INPUT);

  // PENGATURAN SENSOR TEMPERATUR DAN KELEMBABAN (DHT22)
  dht.begin();

  // PENGATURAN SENSOR INTENSITAS (BH1750)
  lightMeter.begin();

  // PENGATURAN SENSOR EC
  for (byte thisReading = 0; thisReading < numReadings; thisReading++)
    readings[thisReading] = 1;
  TempProcess(StartConvert);   //let the DS18B20 start the convert
  AnalogSampleTime=millis();
  printTime=millis();
  tempSampleTime=millis();
  
  // PENGATURAN SENSOR PH (SEN0161)
  pinMode(sensorPin, INPUT);

  // PENGATURAN PIN OUTPUT DAN RELAY
  pinMode(LED, OUTPUT);
  pinMode(relayHeater, OUTPUT);
  pinMode(relayHumidifier, OUTPUT);
  pinMode(relayFanIn, OUTPUT);
  pinMode(relayFanOut, OUTPUT);
  pinMode(relayLED, OUTPUT);
  pinMode(relayPeltier, OUTPUT);
  pinMode(relayPumpAB, OUTPUT);
  pinMode(ledPutih, OUTPUT);

  // nilai awal sebelum ada input pengguna
  suhuIn = 24;                    // nilai suhu
  kelembabanIn = 70;              // nilai kelembaban
  co2In = 900;                    // nilai CO2
  intensitasIn = 900;             // nilai intensitas
  phIn = 6.50;                    // nilai pH
  nutrisiIn = 1.2;                // nilai EC
  waktusiramIn = 60000;           // setiap 20 menit sekali
  penyinaranIn = 43200000;        // 12 jam nyala, 12 jam mati
  
  digitalWrite(relayHeater, HIGH);
  digitalWrite(relayHumidifier, HIGH);
  digitalWrite(relayFanIn, HIGH);
  digitalWrite(relayFanOut, HIGH);
  digitalWrite(relayLED, HIGH);
  digitalWrite(relayPeltier, HIGH);
  digitalWrite(relayPumpAB, HIGH);
  digitalWrite(ledPutih, HIGH);
}

void loop()
{
  // mengambil nilai milisekon sekarang untuk dibandingkan dengan sebelumnya---------
  Serial.println("---------------------------------------------------------------------");
  currentMillis = millis();
  Serial.print("Waktu milis : ");Serial.println(currentMillis);
  Serial.print("Parameter target : \t");Serial.print(suhuIn);Serial.print(' ');Serial.print(kelembabanIn);Serial.print(' ');Serial.print(co2In);Serial.print(' ');Serial.print(intensitasIn);
  Serial.print(' ');Serial.print(phIn);Serial.print(' ');Serial.print(nutrisiIn);Serial.print(' ');Serial.print(waktusiramIn);Serial.print(' ');Serial.println(penyinaranIn);

  // membaca apakah ada data masuk dari komunikasi serial----------------------------
  if (Serial3.available() > 0)
  {
    // data string dimasukkan ke array, melakukan parsing, dan mengubah ke bentuk int dan float
    // kemudian dimasukkan ke variabel
    // data harus berurutan
    Serial.println("Mendapat input serial");
    stringInput = Serial3.readStringUntil('\n');
    Serial.println(stringInput);
    stringInput.toCharArray(charInput, 30);
    p = charInput;

    /***********parameter ph***********/
    str = strtok (p, " ");
    //Serial.println(str);
    phIn = atof(str);
    Serial.print("inputph = "); Serial.println(phIn);
    p = NULL;
    /***********parameter kelembaban***********/
    str = strtok (p, " ");
    //Serial.println(str);
    kelembabanIn = atoi(str);
    Serial.print("inputkelembaban = "); Serial.println(kelembabanIn);
    p = NULL;
    /***********parameter suhu***********/
    str = strtok (p, " ");
    //Serial.println(str);
    suhuIn = atof(str);
    Serial.print("inputsuhu = "); Serial.println(suhuIn);
    p = NULL;
    /***********parameter intensitas***********/
    str = strtok (p, " ");
    //Serial.println(str);
    intensitasIn = atoi(str);
    Serial.print("inputintensitas = "); Serial.println(intensitasIn);
    p = NULL;
    /***********parameter co2***********/
    str = strtok (p, " ");
    //Serial.println(str);
    co2In = atoi(str);
    Serial.print("inputco2 = "); Serial.println(co2In);
    p = NULL;  
    /***********parameter penyinaran***********/
    str = strtok (p, " ");
    //Serial.println(str);
    penyinaranIn = atoi(str);
    penyinaranIn = penyinaranIn*3600*1000;
    Serial.print("inputilamapenyinaran = "); Serial.println(penyinaranIn);
    p = NULL;
  }

  /******************************* PEMBACAAN KONDISI AKTUAL ***********************************/

  // PEMBACAA SUHU DAN KELEMBABAN-----------------------------------------
  dht.readHumidity();
  dht.readTemperature();

  //variabel tambahan untuk c02----------------------------------------------------
  int co2now[4];                                	//int array for co2 readings
  int co2raw = 0;                               	//int for raw value of co2
  int co2comp = 0;                             		//int for compensated co2
  int co2ppm = 0;                              		//int for calculated ppm
  int average = 0;                              	//int for averaging

  // PEMBACAAN KONDISI C02----------------------------------------------------------
  for (int x = 0; x < 5; x++)                   	//melakukan sampling sebanyak 5 kali
  {
    co2now[x] = analogRead(analogInput);
    delay(100);
  }
  for (int x = 0; x < 5; x++)
  { //menjumlahkan seluruh sampel
    average = average + co2now[x];
  }
  co2raw = average / 5;                         	//mengambil nilai rata-rata
  co2comp = co2raw - co2Zero;                   	//get compensated value
  co2ppm = map(co2comp, 0, 1023, 400, 5000);    	//map value for atmospheric levels

  // PEMBACAAN KONDISI INTENSITAS CAHAYA--------------------------------------------
  uint16_t lux = lightMeter.readLightLevel();
  
  // PEMBACAAN KONDISI NUTRISI------------------------------------------------------
  // Melakukan sampling nilai analog dan mengambil rata-rata
  if(millis()-AnalogSampleTime>=AnalogSampleInterval)  
  {
    AnalogSampleTime=millis();
     // subtract the last reading:
    AnalogValueTotal = AnalogValueTotal - readings[index];
    // read from the sensor:
    readings[index] = analogRead(ECsensorPin);
    // add the reading to the total:
    AnalogValueTotal = AnalogValueTotal + readings[index];
    // advance to the next position in the array:
    index = index + 1;
    // if we're at the end of the array...
    if (index >= numReadings)
    // ...wrap around to the beginning:
    index = 0;
    // calculate the average:
    AnalogAverage = AnalogValueTotal / numReadings;
  }
  // Membaca suhu nutrisi dan melakukan konversi
  // interval dari pembacaan dan konversi harus lebih besar dari 750ms agar nilai suhu akurat
  if(millis()-tempSampleTime>=tempSampleInterval) 
  {
    tempSampleTime=millis();
    temperature = TempProcess(ReadTemperature);  // read the current temperature from the  DS18B20
    TempProcess(StartConvert);                   //after the reading,start the convert for next reading
  }
   /*
   Every once in a while,print the information on the serial monitor.
  */
  if(millis()-printTime>=printInterval)
  {
    printTime=millis();
    averageVoltage=AnalogAverage*(float)5000/1024;
    
    float TempCoefficient=1.0+0.0185*(temperature-25.0);    //temperature compensation formula: fFinalResult(25^C) = fFinalResult(current)/(1.0+0.0185*(fTP-25.0));
    float CoefficientVoltage=(float)averageVoltage/TempCoefficient;   
    
    if(CoefficientVoltage<150)
      Serial.println(" ");   
      //Serial.println("No solution!");   //25^C 1413us/cm<-->about 216mv  if the voltage(compensate)<150,that is <1ms/cm,out of the range
    else if(CoefficientVoltage>3300)
      Serial.println("Out of the range!");  //>20ms/cm,out of the range
    else
    { 
      if(CoefficientVoltage<=448)ECcurrent=7.9*CoefficientVoltage-64.32;   //1ms/cm<EC<=3ms/cm
      else if(CoefficientVoltage<=1457)ECcurrent=7.98*CoefficientVoltage-127;  //3ms/cm<EC<=10ms/cm
      else ECcurrent=7.1*CoefficientVoltage+2278;                           //10ms/cm<EC<20ms/cm
      ECcurrent/=1000;    //convert us/cm to ms/cm
    }
  }
  // PEMBACAAN KONDISI PH-----------------------------------------------------------
  for (int i = 0; i < 10; i++)                  	// mengambil 10 nilai sampel
  {
    buf[i] = analogRead(sensorPin);
    delay(10);
  }
  for (int i = 0; i < 9; i++)                  		// melakukan sorting
  {
    for (int j = i + 1; j < 10; j++)
    {
      if (buf[i] > buf[j])
      {
        tempPh = buf[i];
        buf[i] = buf[j];
        buf[j] = tempPh;
      }
    }
  }
  avgValue = 0;
  for (int i = 2; i < 8; i++)                 		// mengambil rata-rata dari 6 nilai di tengah
    avgValue += buf[i];
  float phValue = (float)avgValue*5.0/1024/6; //mengubah nilai analog mejadi mV
  phValue = 5*phValue;                   		//mengubah mV menjadi nilai pH

  // PEMBACAAN LEVEL AIR---------------------------------------------------------------------
  sensorValue = analogRead(waterLevel); // membaca tengan dari sensor dalam bentuk integer
  tinggiAir = sensorValue*4.0/1023;
  Serial.print("level air : ");Serial.println(tinggiAir);
  if (tinggiAir < 2.0)
  {
    levelAir = "kurang";  
  }
  else
  {
    levelAir = "cukup";
  }
    
  /******************************* PENGATURAN AKTUATOR ***********************************/
  //PENGATURAN FAN IN--------------------------------------------------
  if ((dht.temperature_C > (suhuIn + 0.3)) || (co2ppm < (co2In - 10)))
  {
    digitalWrite(relayFanIn, LOW);
    stateFanIn = "on";
    Serial.println("fan in on");
  }
  else
  {
    digitalWrite(relayFanIn, HIGH);
    stateFanIn = "off";
    Serial.println("fan in off");
  }

  //PENGATURAN FAN OUT--------------------------------------------------
  if ((co2ppm > (co2In + 10)) || (dht.humidity > (kelembabanIn + 0.5)))
  {
    digitalWrite(relayFanOut, LOW);
    stateFanOut = "on";
    Serial.println("fan out on");
  }
  else
  {
    digitalWrite(relayFanOut, HIGH);
    stateFanOut = "off";
    Serial.println("fan out off");
  }

//  PENGATURAN HEATER----------------------------------------------------
  if ((dht.temperature_C < (suhuIn - 0.3)) || (dht.humidity > (kelembabanIn + 10)))
  {
    digitalWrite(relayHeater, LOW);
    stateHeater = "on";
    Serial.println("heater on");
  }
  else
  {
    digitalWrite(relayHeater, HIGH);
    stateHeater = "off";
    Serial.println("heater off");
  }

  //PENGATURAN PELTIER------------------------------------------------
  if (dht.temperature_C > (suhuIn + 0.3))
  {
    digitalWrite(relayPeltier, LOW);
    statePeltier = "on";
    Serial.println("peltier on");
  }
  else
  {
    digitalWrite(relayPeltier, HIGH);
    statePeltier = "off";
    Serial.println("peltier off");
  }
  
  //PENGATURAN HUMIDIFIER------------------------------------------------
  if ((dht.humidity < (kelembabanIn - 0.5)) || (dht.temperature_C > (suhuIn + 0.3)))
  {
    digitalWrite(relayHumidifier, LOW);
    stateHumidifier = "on";
    Serial.println("humidifier on");
  }
  else
  {
    digitalWrite(relayHumidifier, HIGH);
    stateHumidifier = "off";
    Serial.println("humidifier off");
  }

  //PENGATURAN LED----------------------------------------------------------

  // rentang nyala dalam sehari
  if ((unsigned long)(currentMillis - previousLED) <= (unsigned long)(penyinaranIn))
  {
    digitalWrite(relayLED,LOW);
    bright = 255;
    analogWrite(LED, bright);
    stateLED = "on";
    Serial.print("LED nyala : ");
    if ((lux < (intensitasIn - 5)) && (bright < 251))
    {
      bright = bright + 5;
      Serial.println("LED fading in");
    }
    else if ((lux > (intensitasIn + 5)) && (bright > 4))
    {
      bright = bright - 5;
      Serial.println("LED fading out");
    }
    else
    {
      Serial.print("PWM = ");Serial.println(bright);
    }
    analogWrite(LED, bright);
  }

  // rentang mati dalam sehari
  if (((unsigned long)(currentMillis - previousLED) > (unsigned long)(penyinaranIn)) && ((unsigned long)(currentMillis - previousLED) <= satuHari))
  {
    digitalWrite(relayLED, HIGH);
    analogWrite(LED, 0);
    stateLED = "off";
    Serial.println("LED mati");
  }

  // setelah lewat satu hari
  if (((unsigned long)(currentMillis - previousLED) > (unsigned long)(penyinaranIn)) && ((unsigned long)(currentMillis - previousLED) > satuHari))
  {
    previousLED = millis();
    //Serial.println(previousLED);
  }

// PENGAMBILAN FOTO TANAMAN----------------------------------------------------
  while (Serial.available() > 0)
  {
    Serial.read();
    analogWrite(LED, 0);
    digitalWrite(relayLED, HIGH);
    digitalWrite(ledPutih, LOW);
    delay(120000);
    analogWrite(LED, bright);
    digitalWrite(relayLED, LOW);
    digitalWrite(ledPutih, HIGH);
  }
   
  //PENGATURAN POMPA LARUTAN-------------------------------------------------
  if (ECcurrent < (nutrisiIn - 0.5))
  {
    digitalWrite(relayPumpAB, LOW);
    statePumpAB = "on";
    Serial.println("pompa larutan A dan B on");
    delay(500);
    digitalWrite(relayPumpAB, HIGH);
    statePumpAB = "off";
  }
      
  // MENGUBAH INTEGER KE STRING UNTUK DIKIRIM SERIAL--------------------
  kondisiSuhu = dht.temperature_C;
  kondisiKelembaban = dht.humidity;
  kondisiAktual = kondisiSuhu + ' ' + kondisiKelembaban + ' ' + co2ppm + ' ';
  kondisiAktual = kondisiAktual + lux + ' ';
  kondisiAktual = kondisiAktual + phValue + ' ';
  kondisiAktual = kondisiAktual + ECcurrent + ' ';
  kondisiAktual = kondisiAktual + temperature +' ';
  kondisiAktual = kondisiAktual + stateHeater + ' ';
  kondisiAktual = kondisiAktual + stateHumidifier + ' ';
  kondisiAktual = kondisiAktual + stateFanIn + ' ';
  kondisiAktual = kondisiAktual + stateFanOut + ' ';
  kondisiAktual = kondisiAktual + stateLED + ' ';
  kondisiAktual = kondisiAktual + statePumpAB + ' ';
  kondisiAktual = kondisiAktual + statePeltier + ' ';
  kondisiAktual = kondisiAktual + levelAir;
  Serial3.println(kondisiAktual);
  Serial.print("mengirim data serial : \t");
  Serial.println(kondisiAktual);
  Serial.println();
  delay (5000);
}


//ch=0,let the DS18B20 start the convert;ch=1,MCU read the current temperature from the DS18B20.
float TempProcess(bool ch)
{
  //returns the temperature from one DS18B20 in DEG Celsius
  static byte data[12];
  static byte addr[8];
  static float TemperatureSum;
  if(!ch){
          if ( !ds.search(addr)) {
              Serial.println("no more sensors on chain, reset search!");
              ds.reset_search();
              return 0;
          }      
          if ( OneWire::crc8( addr, 7) != addr[7]) {
              Serial.println("CRC is not valid!");
              return 0;
          }        
          if ( addr[0] != 0x10 && addr[0] != 0x28) {
              Serial.print("Device is not recognized!");
              return 0;
          }      
          ds.reset();
          ds.select(addr);
          ds.write(0x44,1); // start conversion, with parasite power on at the end
  }
  else{  
          byte present = ds.reset();
          ds.select(addr);    
          ds.write(0xBE); // Read Scratchpad            
          for (int i = 0; i < 9; i++) { // we need 9 bytes
            data[i] = ds.read();
          }         
          ds.reset_search();           
          byte MSB = data[1];
          byte LSB = data[0];        
          float tempRead = ((MSB << 8) | LSB); //using two's compliment
          TemperatureSum = tempRead / 16;
    }
          return TemperatureSum;  
}
