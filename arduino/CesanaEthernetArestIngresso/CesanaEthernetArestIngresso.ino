/* 
  This a simple example of the aREST Library for Arduino (Uno/Mega/Due/Teensy)
  using the Ethernet library (for example to be used with the Ethernet shield). 
  See the README file for more details.
 
 Sensore di temperatura LM35 su pin A1,A3
 Sensore NC su    pin 9
 Rele cancello    pin 2
 Relé cancelletti pin 3
 
 created 22 Aprile 2015
 modified X MAggio 2015
 by Tom Rugg 
*/

// Libraries
#include <SPI.h>
#include <Ethernet.h>
#include "aREST.h"
#include <avr/wdt.h>

#define PIN_RELE_CANCELLO    2//led verde
#define PIN_RELE_CANCELLETTI 3//led rosso

/* contatto magnetico normalmente chiuso con resistenza di pull-up interna 
porta di ingresso
          contatto    NC
                  .--o---o--o GND
                  |    /\
 Pin 8 o-----------
 */
#define NC_CONT_1  9

/* Sensor test sketch
    for more information see http://www.ladyada.net/make/logshield/lighttemp.html
    */
//LM35 Pin Variables
#define tempPin A2
#define tempPin_su_scheda A2 //the analog pin the LM35's Vout (sense) pin is connected to
//the resolution is 10 mV / degree centigrade with a
float temperatureC = 0.0; // the analog reading from the sensor
float temperatureC_su_scheda = 0.0;
// Variables to be exposed to the API
int LM35sensor = 0;
int LM35sensor_su_scheda = 0;
  
//Variabili
unsigned long currentMillis = 0L;
unsigned long previousMillisTemp = 0L;
unsigned long EventoIngressoMillis = 0L;
  
// Enter a MAC address for your controller below.
byte mac[] = { 0x90, 0xA2, 0xDA, 0x0E, 0xFE, 0x40 };

// IP address in case DHCP fails
IPAddress ip(192,168,1,200);

// Ethernet server
EthernetServer server(80);

//variabili per pushingbox.com accesso con account gmail
char serverName[] = "api.pushingbox.com";
char DEVID1[] = "vD6CBA2419476BA2";

// Create aREST instance
aREST rest = aREST();


void setup(void)
{  
  delay( 50 );   // allow some time (50 ms) after powerup and sketch start, for the Wiznet W5100 Reset IC to release and come out of reset.
  // Start Serial
  //---Serial.begin(115200);
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }
  
  // setto i contatti NC 
  pinMode(NC_CONT_1, INPUT);           // set pin to input porta di ingresso
  digitalWrite(NC_CONT_1, HIGH);       // turn on pullup resistors
  
  // initialize the digital pin's as an output.
  pinMode(PIN_RELE_CANCELLO, OUTPUT);            // Relay 1 
  digitalWrite(PIN_RELE_CANCELLO, LOW);
  pinMode(PIN_RELE_CANCELLETTI, OUTPUT);         // Relay 2
  digitalWrite(PIN_RELE_CANCELLETTI, LOW);
  
  // Init variables and expose them to REST API
  rest.variable("LM35sensor",&LM35sensor);
  rest.variable("LM35sensor_su_scheda",&LM35sensor_su_scheda);

  // Function to be exposed
  rest.function("portoncino",portoncinoControl);
  rest.function("cancello",cancelloControl);
  
  // Give name and ID to device
  rest.set_id("001");
  rest.set_name("ingresso");
  
  //setto variabili temeperatura
  LM35sensor = analogRead(tempPin);
  LM35sensor_su_scheda = analogRead(tempPin_su_scheda);
  //tom modifiche adottate per ovviare al blocco della scheda ethernet
  pinMode(4,OUTPUT);
  digitalWrite(4,HIGH);

  pinMode(10,OUTPUT);
  digitalWrite(10,HIGH);
  delay(10000); // attendi 10 secondi per dare tempo di chiudere la porta 
  Ethernet.begin(mac, ip);
  // Start the Ethernet connection and the server
  /*if (Ethernet.begin(mac) == 0) {
    //---Serial.println("Failed to configure Ethernet using DHCP");
    // no point in carrying on, so do nothing forevermore:
    // try to congifure using IP address instead of DHCP:
    Ethernet.begin(mac, ip);
  }*/
  server.begin();
  //---Serial.print("server is at ");
  //---Serial.println(Ethernet.localIP());
  allarmeMailPushingbox("Start_servizio:", "192,168,1,200");

  // Start watchdog
  wdt_enable(WDTO_8S);
}

void loop() {  
  currentMillis=millis();
  
  // listen for incoming clients
  EthernetClient client = server.available();
  rest.handle(client);
  wdt_reset();
  
  //sensori interni
  readIngresso();
  readTemp();
}

// Custom function accessible by the API
int portoncinoControl(String command) {
  //---Serial.print("portoncinoControl: ");
  //---Serial.println(command);
  // Get state from command
  //int state = command.toInt();
  
  digitalWrite(PIN_RELE_CANCELLETTI,HIGH);
  delay(2000);
  digitalWrite(PIN_RELE_CANCELLETTI,LOW);
  return 1;
}

int cancelloControl(String command){
  //---Serial.print("cancelloControl: ");
  //---Serial.println(command);
  digitalWrite(PIN_RELE_CANCELLO,HIGH);
  delay(2000);
  digitalWrite(PIN_RELE_CANCELLO,LOW);
  return 2;
}

void readIngresso(){
  int ingresso = digitalRead(NC_CONT_1);
  if(ingresso){
    delay(200);
    if(digitalRead(NC_CONT_1) && ((currentMillis-EventoIngressoMillis) > 300000L)){
      EventoIngressoMillis = currentMillis;
      allarmeMailPushingbox("Apertura_porta_di_ingresso:", "ALLARME_intrusione!");
    }
  }
}

void readTemp() {
  LM35sensor += analogRead(tempPin);
  LM35sensor /=2;
  LM35sensor_su_scheda += analogRead(tempPin_su_scheda);
  LM35sensor_su_scheda /=2;
  if(currentMillis - previousMillisTemp > 60000) {// un minuto
    // save the last time control
    previousMillisTemp = currentMillis;
    float volts = 0.0;
    //getting the voltage reading from the temperature sensor
    //LM35sensor = analogRead(tempPin);
    //lettura valore del sensore LM35 messo sull'ingresso
    //analogico A1
    volts = ( LM35sensor/1024.0)*5.0; //formula per ottenere la tensione di uscita dell'LM35 in volts
    temperatureC =volts*100.0;// valore espresso in gradi Celsius (l'out del sensore è 10mv per grado) 
    // print out the voltage
 //Serial.print(volts); Serial.println(" volts");
    // now print out the temperature
 //Serial.print(temperatureC); Serial.println(" sensore di temperatura esterno C");
    temperatureC_su_scheda = ( LM35sensor_su_scheda/1024.0)*500.0;
    // now print out the temperature
 //Serial.print(temperatureC_su_scheda); Serial.println(" sensore di temperatura su scheda C");
    
    if(temperatureC > 45){
      char buff[10];
      //dtostrf(FLOAT,WIDTH, PRECISION,BUFFER);
      dtostrf(temperatureC, 2, 2, buff);
      allarmeMailPushingbox("Temperatura_in_ingresso:", buff);
    }
    if(temperatureC_su_scheda > 50){
      char buff[10];
      dtostrf(temperatureC_su_scheda, 2, 2, buff);
      allarmeMailPushingbox("Temperatura_su_scheda_arduino:", buff);
    }
  }
}


void allarmeMailPushingbox(String sensore, String msg){
  /*
  http://api.pushingbox.com/pushingbox?devid=vD6CBA2419476BA2&sensore='lm35'&messaggio='temperatura superiore 50 gradi'
  */
    EthernetClient client_eth;
    int tentativi=0;
 allarmeMail:   
    client_eth.stop();
    //Serial.println("connecting pushingbox...");
    if (client_eth.connect(serverName, 80) && (tentativi < 3)) {
      //Serial.println("connected");
      //Serial.println("sendind request");
      client_eth.print("GET /pushingbox?devid=");
      client_eth.print(DEVID1);
      client_eth.print("&sensore=");
      client_eth.print(sensore);
      client_eth.print("&messaggio=");
      client_eth.print(msg);
      client_eth.print(" ");
      client_eth.println(" HTTP/1.1");
      client_eth.print("Host: ");
      client_eth.println(serverName);
      client_eth.println("User-Agent: Arduino");
      client_eth.println();
      tentativi++;
    } else {
      tentativi++;
      delay(1000);
      goto allarmeMail;
    }
    client_eth.stop();
}

