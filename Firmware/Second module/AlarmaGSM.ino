#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <SoftwareSerial.h>

#include "Wire.h"
#include "LiquidCrystal_I2C.h"

LiquidCrystal_I2C lcd(0x27, 16, 4);

const int pinOptoReleu = 5;
const int pinOptocuplor = 6;
const int pinBistabil = 7;
const int pinSunetAlarma = 12;

// RX și TX conectate la modul SIM800L
SoftwareSerial sim800(10, 11);  // RX, TX

int wait = 0;
int Do = 262, Re = 294, Mi = 330, Fa = 349, Sol = 392,
    La = 440, Si = 494, Do2 = 524;

unsigned long milisec1 = 0;
unsigned long milisec2 = 0;
unsigned long milisec3 = 0;
unsigned long milisec4 = 0;

const long intervalReadOctocuplor = 100;  // interval pentru citire optocuplor (in ms)
const long intervalLumini = 200;          // interval pentru activare vizuala (in ms)
const long intervalSunet = 300;           // interval pentru activare sonora (in ms)
const long intervalSMS = 30000;           // interval pentru trimitere SMS (in ms)

void setup() {

  Serial.begin(9600);  // viteza pentru PC
  sim800.begin(9600);  // viteza SIM800L 
  //Serial.println("Trimite AT:");

  pinMode(pinOptoReleu, OUTPUT);
  digitalWrite(pinOptoReleu, LOW);  // motorul OPRIT la pornire (depinde dacă releul e low-trigger)

  pinMode(pinOptocuplor, INPUT);  // dacă ieșirea e deschis-collector, poți folosi INPUT_PULLUP

  pinMode(pinBistabil, OUTPUT);
  digitalWrite(pinBistabil, LOW);
  pinMode(pinSunetAlarma, OUTPUT);

  lcd.init();
  lcd.backlight();

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("2:");
}

void loop() {

  int valOptocuplor = digitalRead(pinOptocuplor);

  unsigned long milisecCurent = millis();

  // Simulare multi thread
  if (milisecCurent - milisec1 >= intervalReadOctocuplor) {
    milisec1 = milisecCurent;
    AfiseazaStareOctocuplor(valOptocuplor);
  }

  if (milisecCurent - milisec2 >= intervalSunet) {
    milisec2 = milisecCurent;
    ActiveazaSunet(valOptocuplor);
  }

  if (milisecCurent - milisec3 >= intervalLumini) {
    milisec3 = milisecCurent;
    ComutaBistabili(valOptocuplor);
  }

  ActiveazaOptoReleu(valOptocuplor);

  if (milisecCurent - milisec4 >= intervalSMS) {
    milisec4 = milisecCurent;
    TrimiteSMS(valOptocuplor);
  }
}

void AfiseazaStareOctocuplor(int valoareOctocuplor) {
  lcd.setCursor(2, 0);
  lcd.print("   ");
  lcd.setCursor(2, 0);
  if (valoareOctocuplor == LOW) {
    lcd.print("ON");
  } else {
    lcd.print("OFF");
  }
  delay(200);
}

void ComutaBistabili(int valoareOctocuplor) {
  lcd.setCursor(8, 0);
  lcd.print("LEDS:   ");
  lcd.setCursor(8, 0);
  lcd.print("LEDS:");

  if (valoareOctocuplor == LOW) {
    lcd.print("ON");
    // Impuls pentru comutare
    digitalWrite(pinBistabil, HIGH);
    delay(100);  // impuls scurt
    digitalWrite(pinBistabil, LOW);
    delay(100);  // pauză între comutări
  } else {
    lcd.print("OFF");
  }
}

void ActiveazaSunet(int valoareOctocuplor) {
  
  if (valoareOctocuplor == LOW) {
    wait = 200;
    tone(pinSunetAlarma, Sol, wait);
    delay(200);
    tone(pinSunetAlarma, Re, wait);
    delay(200);
    tone(pinSunetAlarma, Fa, wait);
    delay(200);
    wait = 500;
    tone(pinSunetAlarma, Sol, wait);
    delay(500);
  } else {
    noTone(pinSunetAlarma);
  }

  delay(100);
}

void TrimiteSMS(int valoareOctocuplor) {
  lcd.setCursor(0, 1);
  lcd.print("SMS:   ");
  lcd.setCursor(0, 1);
  lcd.print("SMS:");

  // Serial.println("=== Test functionalitate pentru modulul GSM SIM800L V2.0 ===");

  // delay(1000);                    // asteaptapentru inițializare
  // TrimiteComenziGSM("AT");        // test comunicare
  // TrimiteComenziGSM("AT+GMR");    // versiune firmware
  // TrimiteComenziGSM("AT+CPIN?");  // status SIM
  // TrimiteComenziGSM("AT+CSQ");    // semnal GSM
  // TrimiteComenziGSM("AT+CREG?");  // inregistrare in retea
  // TrimiteComenziGSM("AT+CBC");    // alimentare / baterie

  if (valoareOctocuplor == LOW) {
    // trimite SMS la telefonul
    TrimiteComenziGSM("AT");         // test comunicare
    TrimiteComenziGSM("AT+CSQ");     // semnal GSM
    TrimiteComenziGSM("AT+CREG?");   // inregistrare in retea
    TrimiteComenziGSM("AT+CMGF=1");  // spcifica mesajul in mod text

    sim800.print("AT+CMGS=\"0787674842\"\r"); // numarul de telefon

    sim800.println("Alarma Gaz.");  // mesajul SMS
    delay(300);

    sim800.write(26);  // 26 = CTRL+Z, se trimite mesajul
    lcd.print("ON"); // SMS-ul a fost trimis
  } else {
    lcd.print("OFF");
  }
  delay(500);
}

void ActiveazaOptoReleu(int valoareOctocuplor) {
  lcd.setCursor(8, 1);
  lcd.print("VEN2:   ");
  lcd.setCursor(8, 1);
  lcd.print("VEN2:");

  if (valoareOctocuplor == LOW) {
    // Activeaza al doilea ventilator
    digitalWrite(pinOptoReleu, LOW);

    lcd.print("ON");
  } else {
    digitalWrite(pinOptoReleu, HIGH);
    lcd.print("OFF");
  }
  delay(500);
}

void TrimiteComenziGSM(String cmd) {
  sim800.println(cmd);
  delay(500);
  while (sim800.available()) {
    Serial.write(sim800.read());
  }
}


