#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <IRremote.hpp>

#include <Stepper.h>

// initializare LCD
LiquidCrystal_I2C lcd(0x27, 16, 4);

#include "Wire.h"
#include "LiquidCrystal_I2C.h"

#include "Stepper.h"
#define STEPS 64  // Numar de pasi pentru o rotatie completa

const int pinImpuls = 2;  // pin pentru generatorul de impulsuri dreptunghiulare NE555

// pini motoras DC
int mENA = 5;  // PWM
int mIN1 = 3;  // directie inainte
int mIN2 = 4;  // directie inapoi

// pini pentru LED-urile IR (emițătoare) ---
int pinLedIR1 = 6;
int pinLedIR2 = 7;

// pini pentru motorasul pas cu pas
// In1, In2, In3, In4 in the secventa 1-3-2-4
const int mppIN1 = 11;
const int mppIN2 = 10;
const int mppIN3 = 9;
const int mppIN4 = 8;

const int recTelecomanda = 12;  // pinul conectat la OUT de pe VS1838B

const int pinButonFereastra = 13;  // pinul conectat la fereastra
int ultimaPozitieButon;

// Pin Alarma Gaz
int pinAlarmaGaz = A0;

// Pragul de detecție gaz. Daca este mai mare se declanseaza alarma
int pragSenzorGaz = 200;  // 0-1023 (analog)

// --- Pini pentru fotodiode IR (receptoare) ---
int pinDiodaIR1 = A1;
int pinDiodaIR2 = A2;
// Pragul de intensitate a detectiei radiatiei in infrarosu
int pragDiodaIR = 100;

// Pin pentru a activare/dezactivare a placutei Arduino Uno 2 (GSM)
const int pinOptocuplor = A3;

int indexSample = 0;
bool bufferFull = false;

// Trimite jumatate de pas pentru 28BYJ-48
const int steps[8][4] = {
  { 1, 0, 0, 0 },
  { 1, 1, 0, 0 },
  { 0, 1, 0, 0 },
  { 0, 1, 1, 0 },
  { 0, 0, 1, 0 },
  { 0, 0, 1, 1 },
  { 0, 0, 0, 1 },
  { 1, 0, 0, 1 }
};

volatile bool cererePas = false;
int indexPas = 0;

int valDiodaIR1;
int valDiodaIR2;

// viteza motorului DC (poate lua valori intre 0..255)
int vitezaMotorDC = 100;

// temporizare pe intervale a activitatii senzorilor
unsigned long milisec1 = 0;
unsigned long milisec2 = 0;
unsigned long milisec3 = 0;
unsigned long milisec4 = 0;

const long intervalLEDTasta = 100;          // interval pentru LED (in ms)
const long intervalSenzoriFereastra = 200;  // interval pentru senzori fereastra (in ms)
const long intervalGaz = 300;               // interval pentru detectie gaz (in ms)
const long intervalMotorDC = 500;           // interval pentru motor DC (in ms)

volatile bool sistemActiv;
volatile bool alarmaGaz;
volatile bool ledNivel1;
volatile bool ledNivel2;

// ISR pentru impuls de la NE555
void pasISR() {
  cererePas = true;
}

void setup() {

  // activeaza afisarea mesajelor de Arduino
  Serial.begin(9600);

  // seteaza alarma de gaz
  pinMode(pinAlarmaGaz, INPUT);

  // seteaza buton fereastra
  pinMode(pinButonFereastra, INPUT_PULLUP);  // folosit ca intrare pentru buton fereastra

  // seteaza optocuplor
  pinMode(pinOptocuplor, OUTPUT);

  // LED-uri IR
  pinMode(pinLedIR1, OUTPUT);
  pinMode(pinLedIR1, OUTPUT);

  // LED-uri IR permanent active
  digitalWrite(pinLedIR1, HIGH);
  digitalWrite(pinLedIR2, HIGH);

  // motoras pas cu pas
  pinMode(mppIN1, OUTPUT);
  pinMode(mppIN2, OUTPUT);
  pinMode(mppIN3, OUTPUT);
  pinMode(mppIN4, OUTPUT);
  attachInterrupt(digitalPinToInterrupt(pinImpuls), pasISR, RISING);

  // motoras DC
  pinMode(mENA, OUTPUT);
  pinMode(mIN1, OUTPUT);
  pinMode(mIN2, OUTPUT);

  digitalWrite(mIN1, LOW);
  digitalWrite(mIN2, LOW);
  analogWrite(mENA, 0);

  // setare LCD
  lcd.init();
  lcd.backlight();

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("1:");
  lcd.setCursor(2, 0);
  lcd.print("OFF");

  sistemActiv = false;
  alarmaGaz = false;

  IrReceiver.begin(recTelecomanda, ENABLE_LED_FEEDBACK);  // initializează receptorul
  Serial.println("Verifica taste telecomanda");
}

void loop() {

  // Arduino Uno nu suporta multi-threading

  // citeste pozitia (inchis/deschis) butonului de fereastra
  int pozitieButonFereastra = digitalRead(pinButonFereastra);

  // citeste timpul curent al sistemului 
  unsigned long milisecCurent = millis();

  // Simulare multi thread
  if (milisecCurent - milisec1 >= intervalLEDTasta) {
    milisec1 = milisecCurent;

    VerificaTastaApasata();
  }

  if (sistemActiv) {

    if (milisecCurent - milisec2 >= intervalGaz) {
      milisec2 = milisecCurent;

      VerificareSenzorGaz();
    }

    if (alarmaGaz) {

      if (milisecCurent - milisec3 >= intervalSenzoriFereastra) {
        milisec3 = milisecCurent;

        VerificareSenzoriFereastra();
      }

      if (ledNivel2 == false) {
        MotorPasInainte();
      }
      
      if (milisecCurent - milisec4 >= intervalMotorDC) {
        milisec4 = milisecCurent;

        if (pozitieButonFereastra == LOW) {
          //Serial.println("Buton fereastra inchis.");

          // opreste motoras DC
          StopMotorDC();

          // stop alarma pe a doua placa Arduino
          StopOptocuplor();

          ledNivel1 = false;
          ledNivel2 = false;

        } else {
          //Serial.println("Buton fereastra deschis.");

          if(ledNivel1 == true)
          { 
             // mareste putin viteza dupa ce primul nivel de diode a fost depasit
             vitezaMotorDC = 120;
          }
          if(ledNivel2 == true)
          {
            // mareste viteza la maxim dupa ce al doilea nivel de diode a fost depasit
            vitezaMotorDC = 160;
          }
          
          // porneste motorul DC
          StartMotorDC(vitezaMotorDC);

          // motorasul pas cu pas - un pas inainte
          //MotorPasInainte();

          StartOptocuplor();
        }
      }

    } else {
      // opreste motoras DC
      StopMotorDC();

      // stop alarma pe a doua placa Arduino
      StopOptocuplor();

    }

  } else {

    if (pozitieButonFereastra == HIGH) {
      MotorPasInapoi();
      ultimaPozitieButon = pozitieButonFereastra;
    }
    if (ultimaPozitieButon != pozitieButonFereastra) {
      //Serial.println("Fereastra inchisa!");
      lcd.setCursor(9, 1);
      lcd.print("Win:     ");
      lcd.setCursor(9, 1);
      lcd.print("Win:");
      lcd.print("I");
      ultimaPozitieButon = pozitieButonFereastra;
    }
    
  }
}

void VerificaTastaApasata() {

  // verifică dacă a fost recepționat un cod IR
  if (IrReceiver.decode()) {
    
    unsigned long code = IrReceiver.decodedIRData.decodedRawData;
    //Serial.print("Codul: ");
    //Serial.println(code);

    lcd.setCursor(2, 0);

    // corespondenta intre codurile de 32-bit reale si cifrele telecomenzii
    switch (code) {
      case 3125149440UL:
        lcd.print("   ");
        lcd.setCursor(2, 0);
        lcd.print("ON");
        sistemActiv = true;
        Serial.println("1");
        break;  // cod buton 1
      case 3108437760UL:
        //lcd.print("2");
        Serial.println("2");
        break;  // cod buton 2
      case 3091726080UL:
        //lcd.print("3");
        Serial.println("3");
        break;  // cod buton 3 (exemplu, verifică)
      case 3141861120UL:
        //lcd.print("4");
        Serial.println("4");
        break;  // cod buton 4
      case 3208707840UL:
        //lcd.print("5");
        Serial.println("5");
        break;  // cod buton 5
      case 3158572800UL:
        //lcd.print("6");
        Serial.println("6");
        break;  // cod buton 6
      case 4161273600UL:
        //lcd.print("7");
        Serial.println("7");
        break;  // cod buton 7
      case 3927310080UL:
        //lcd.print("8");
        Serial.println("8");
        break;  // cod buton 8
      case 4127850240UL:
        //lcd.print("9");
        Serial.println("9");
        break;  // cod buton 9
      case 3860463360UL:
        lcd.print("   ");
        lcd.setCursor(2, 0);
        lcd.print("OFF");
        sistemActiv = false;
        OpresteSistemAlarma();
        Serial.println("0");
        break;  // cod buton 0
      default:
        Serial.println("Alt buton");
        break;
    }

    IrReceiver.resume();  // asteapta un semnal nou
  }
}

void VerificareSenzorGaz() {

  lcd.setCursor(0, 1);

  int valSenzorGaz = analogRead(pinAlarmaGaz);

  lcd.print("Gaz:     ");
  lcd.setCursor(0, 1);
  lcd.print("Gaz:");
  lcd.print(valSenzorGaz);

  if (valSenzorGaz >= pragSenzorGaz) {
    lcd.print("+"); // valorile de gaz sunt peste prag (pericol)
    alarmaGaz = true;
  } else {
    lcd.print("-"); // valorile de gaz sunt sub prag (normal)
    alarmaGaz = false;
   }
}

void VerificareSenzoriFereastra() {

  // Citește fotodiodele
  valDiodaIR1 = analogRead(pinDiodaIR1);
  valDiodaIR2 = analogRead(pinDiodaIR2);

  if (valDiodaIR1 > pragDiodaIR || valDiodaIR2 > pragDiodaIR) {
    // Afișare valori citite de la diode (pentru a vedea daca senzorii de fereastra sunt centrati si comunica intre ei)
    Serial.print("R1: ");
    Serial.print(valDiodaIR1);
    Serial.print(" | R2: ");
    Serial.print(valDiodaIR2);
    Serial.print(" | Nivel1: ");
    Serial.print(ledNivel1);
    Serial.print(" | Nivel2: ");
    Serial.print(ledNivel2);
    Serial.print(" | Ultima pozitie buton: ");
    Serial.println(ultimaPozitieButon);
  }

  lcd.setCursor(9, 1);
  lcd.print("Win:     ");
  lcd.setCursor(9, 1);
  lcd.print("Win:");

  if (valDiodaIR1 > pragDiodaIR) {
    ledNivel1 = true;
  }

  if (valDiodaIR2 > pragDiodaIR) {
    ledNivel2 = true;
  }

  // === Detectare fascicul IR ntrerupt ===
  if (ledNivel1 == false && ledNivel2 == false) {
    //Serial.println("Fereastra inchisa!");
    lcd.print("I");
  } else if (ledNivel1 == true && ledNivel2 == false) {
    //Serial.println("Fereastra deschisa!");
    lcd.print("D");
  } else if (ledNivel1 == true && ledNivel2 == true) {
    //Serial.println("Fereastra complet deschisa!");
    lcd.print("CD");
  } else {
    // senzorii IR sunt dereglati
    lcd.print(ledNivel1);
    lcd.print(ledNivel2);
  }
}

// porneste motorasul DC
void StartMotorDC(int viteza) {
  // Serial.print("Motor DC pornit");
  lcd.setCursor(8, 0);
  lcd.print("VEN1:   ");
  lcd.setCursor(8, 0);
  lcd.print("VEN1:ON");

  // motor DC inainte
  digitalWrite(mIN1, HIGH);
  digitalWrite(mIN2, LOW);
  analogWrite(mENA, viteza);
  delay(10);
}

// opreste motorasul DC
void StopMotorDC() {
  // Serial.print("Motor DC oprit");
  lcd.setCursor(8, 0);
  lcd.print("VEN1:    ");
  lcd.setCursor(8, 0);
  lcd.print("VEN1:OFF");

  if (vitezaMotorDC != 0) {
    vitezaMotorDC = 0;
    analogWrite(mENA, vitezaMotorDC);
    delay(10);
  }
}

// Motoras pas cu pas - un pas inainte
void MotorPasInainte() {
  MotorPasCuPasInDirectia(-1);
}

// Motoras pas cu pas - un pas inapoi
void MotorPasInapoi() {
  MotorPasCuPasInDirectia(1);
}

// Motoras pas cu pas - in directia
void MotorPasCuPasInDirectia(int directie) {
  if (cererePas) {
    cererePas = false;

    // Facem 2 semipasi (1 pas motor) pentru un impuls NE555 (pentru a deschide mai repede fereastra)
    for (int i = 0; i < 2; i++) {
      // Aplică pasul curent
      digitalWrite(mppIN1, steps[indexPas][0]);
      digitalWrite(mppIN2, steps[indexPas][1]);
      digitalWrite(mppIN3, steps[indexPas][2]);
      digitalWrite(mppIN4, steps[indexPas][3]);

      // Schimbă indexul în funcție de direcție
      indexPas = (indexPas + directie + 8) % 8;

      delay(2);  // mic delay pentru stabilitate
    }
  }
}

void StartOptocuplor() {
  // activeaza a doua placuta Arduino
  digitalWrite(pinOptocuplor, HIGH);
  delay(20);
}

void StopOptocuplor() {
  // dezactiveaza a doua placa Arduino
  digitalWrite(pinOptocuplor, LOW);
  delay(20);
}

void OpresteSistemAlarma() {
  StopMotorDC();
  StopOptocuplor();
}