#define ThermeVorlaufTempVorgabePin  4
#define ThermeVentilSperrenPin      24
#define ThermeExternAnfordernPin    26
#define ThermeVentilPin             28
#define ThermeVentilVorlaufPin      29
#define ThermeHappyTempPin          30

#define SteuerungStatusNotfallBetrieb                       0
#define SteuerungStatusSolarBetriebInit                    10
#define SteuerungStatusSolarBetriebWarteAufVentil          20
#define SteuerungStatusSolarBetriebWarteAufVentil2Sek      21
#define SteuerungStatusSolarBetriebWarteAufWWbetrieb       30
#define SteuerungStatusSolarBetriebWarteAufAUSbetrieb      40
#define SteuerungStatusSolarBetrieb                        60

#define PumpennachlaufStart   -40
#define PumpennachlaufBrenner -30
#define Pumpennachlauf        -20
#define AnforderungGesperrt   -10
#define WarteAufAnforderung     0

#include <Bounce.h>

void ThermeInit() {
  pinMode     (ThermeHappyTempPin         , OUTPUT);
  digitalWrite(ThermeHappyTempPin         , LOW   );

  pinMode     (ThermeVentilPin            , INPUT ); 

  pinMode     (ThermeVentilSperrenPin     , OUTPUT);
  digitalWrite(ThermeVentilSperrenPin     , LOW   );

  pinMode     (ThermeVentilVorlaufPin     , OUTPUT);
  digitalWrite(ThermeVentilVorlaufPin     , LOW   );

  pinMode     (ThermeExternAnfordernPin   , OUTPUT);
  digitalWrite(ThermeExternAnfordernPin   , LOW   );

  pinMode     (ThermeVorlaufTempVorgabePin, OUTPUT);
  analogWrite (ThermeVorlaufTempVorgabePin, 0     );
}

void ThermeBrennerGehtAus() {
  if (Values[_SteuerungStatus].ValueX10 > SteuerungStatusSolarBetriebWarteAufVentil) {
    Values[_ThermeVorlaufValue].ValueX10 = PumpennachlaufStart; //Geändert setzen sparen wir uns
    if (thermeLogLevel > 0) Debug.println ("Therme: SteuerungStatusSolarBetriebPumpennachlaufStart");      
  }
}

void ThermeSolarbetriebBeginnt() {
  digitalWrite(ThermeHappyTempPin, HIGH);
  ThermeSetSteuerungStatus( SteuerungStatusSolarBetriebInit);
  if (thermeLogLevel > 0) {
    Debug.println ( F("Therme: SteuerungStatusSolarBetriebInit"));
    Debug.println ( F("        HappyTempPin = HIGH"));
  }
}

void ThermeSolarbetriebEndet () {
  analogWrite (ThermeVorlaufTempVorgabePin, 0     );
  digitalWrite(ThermeExternAnfordernPin   , LOW   );
  digitalWrite(ThermeVentilSperrenPin     , LOW   );
  digitalWrite(ThermeHappyTempPin         , LOW   );
  ThermeSetSteuerungStatus( SteuerungStatusNotfallBetrieb);
  if (thermeLogLevel > 0) Debug.println ("Therme: SteuerungStatusNotfallBetrieb");      
}
#define ThermeUmschaltventilWW 30
#define ThermeUmschaltventilHK 10

Bounce thermeVentilBouncer = Bounce(ThermeVentilPin, 500, true); 
void   ThermeDoEvents() {
  if (thermeVentilBouncer.update()) {
    if (thermeVentilBouncer.read()) 
      ValueX10new1 = ThermeUmschaltventilWW; 
    else 
      ValueX10new1 = ThermeUmschaltventilHK;
    if (setValue(_ThermeUmschaltventilTaster, ValueX10new1) && thermeLogLevel > 1) {
      Debug.print   ( F("Therme: ThermeUmschaltventilTaster = "));
      Debug.print   ( Values[ _ThermeUmschaltventilTaster].ValueX10);
      Debug.println ( F( "Zugewiesen"));
    }
    if (Values[_SteuerungStatus].ValueX10 > SteuerungStatusNotfallBetrieb && thermeVentilBouncer.fallingEdge()) {  // Wir sind nicht mehr im richtigen Modus
      digitalWrite( ThermeVentilSperrenPin, LOW);                                                      // das Ventil freigeben
      ThermeSetSteuerungStatus( SteuerungStatusSolarBetriebWarteAufVentil);                            // und Status  zurücksetzen
      if (thermeLogLevel > 0) Debug.println ( F("Therme: SteuerungStatusSolarBetriebWarteAufVentil"));
    }
    //    warteSeit2 = millis();
  }
  ///////////////////////////////////////////////////////////////////////////
  BerechneSteuerungStatus();
  BerechneThermeVorlaufValue();
  BerechneThermeVentilVorlaufPin();
}

void ThermeSetSteuerungStatus (int16_t value) {
  if ( setValue( _SteuerungStatus, value) && thermeLogLevel > 1) {
    Debug.print   ( F("Therme: SteuerungStatus Zugewiesen: "));
    Debug.println   ( Values[_SteuerungStatus].ValueX10);
  }
}
void BerechneSteuerungStatus() {
  static uint32_t warteSeit1 = 0, warteSeit2 = 0;  
  if (Values[_SteuerungStatus].ValueX10 > SteuerungStatusNotfallBetrieb) {
    if (Values[_SteuerungStatus].ValueX10 < SteuerungStatusSolarBetrieb && millis() - warteSeit1 > 120000) { //Init hat nicht funktioniert --> nochmal
      ThermeSolarbetriebBeginnt();
    }
    switch (Values[_SteuerungStatus].ValueX10) {                        // Ventil steht richtig
    case SteuerungStatusSolarBetriebInit:
      warteSeit1 = millis(); 
      ThermeSetSteuerungStatus( SteuerungStatusSolarBetriebWarteAufVentil);
      break;
    case SteuerungStatusSolarBetriebWarteAufVentil2Sek:
      if( millis() - warteSeit2 > 2000) {                                                    // scheint stabil zu sein
        digitalWrite( ThermeVentilSperrenPin, HIGH);                                         // das Ventil sperren
        if (thermeLogLevel > 1) Debug.println ( F("Therme: ThermeVentilSperrenPin = HIGH"));
        warteSeit2 = millis();
        vitoAngeforderteBetriebsart = ThermeBetriebsartAbschalten;
        ThermeSetSteuerungStatus( SteuerungStatusSolarBetriebWarteAufAUSbetrieb);
      }
      break;
    case SteuerungStatusSolarBetriebWarteAufVentil:
      if (Values[_ThermeUmschaltventilTaster].ValueX10 == ThermeUmschaltventilWW) {
        warteSeit2 = millis();
        ThermeSetSteuerungStatus( SteuerungStatusSolarBetriebWarteAufVentil2Sek);
      } 
      else { //Vielleicht hilft ja den Modus mal auf WW zu schalten?
        vitoAngeforderteBetriebsart = ThermeBetriebsartWW;
        ThermeSetSteuerungStatus( SteuerungStatusSolarBetriebWarteAufWWbetrieb);
      }
      break;
    case SteuerungStatusSolarBetriebWarteAufWWbetrieb:
      if (Values[_ThermeBetriebsart].ValueX10 == ThermeBetriebsartWW * 10){                         //Thermenstatus = WW - so muß nun mal aus hydraulischen Grnden das Ventil stehen
        ThermeSetSteuerungStatus( SteuerungStatusSolarBetriebWarteAufVentil);
      }
      break;
    case SteuerungStatusSolarBetriebWarteAufAUSbetrieb:
      if (Values[_ThermeBetriebsart].ValueX10 == ThermeBetriebsartAbschalten * 10){                //Thermenstatus = Abschalten. Bei externer Anforderung bekomme ich sowieso keine VorlaufTempSoll
        ThermeSetSteuerungStatus( SteuerungStatusSolarBetrieb);
      }
      break;
    }
  }
}

void BerechneThermeVorlaufValue() {
  static uint32_t warteSeit1 = 0;

  ValueX10new1 = Values[_ThermeVorlaufValue].ValueX10;
  switch ( ValueX10new1) { //Pumpennachlauf
  case PumpennachlaufStart:
    analogWrite( ThermeVorlaufTempVorgabePin, 0);
    ValueX10new1 = PumpennachlaufBrenner;
    if (thermeLogLevel > 1) {
      Debug.print  ( F("Therme "));
      Debug.print  ( millis());
      Debug.println( F(": analogWrite( ThermeVorlaufTempVorgabePin, 0); PumpennachlaufBrenner"));
    }
    break;
  case PumpennachlaufBrenner:
    if ( Values[_ThermeBrennerLeistung].ValueX10 == 0) {
      ValueX10new1 = Pumpennachlauf;
      if (thermeLogLevel > 1) {
        Debug.print  ( F("Therme "));
        Debug.print  ( millis());
        Debug.println( F(": Pumpennachlauf"));
      }
    }
    break;
  case Pumpennachlauf:
    //if ( millis() - warteSeit1 > 120000) { // Pumpennachlauf beenden
    if ( Values[ _ThermeVorlaufTempIst].ValueX10 < Values[_ThermeRuecklaufTempIst].ValueX10 + 30) {
      digitalWrite( ThermeExternAnfordernPin, LOW);
      warteSeit1 = millis();
      ValueX10new1 = AnforderungGesperrt;
      if (thermeLogLevel > 1) {
        Debug.print  ( F("Therme "));
        Debug.print  ( millis());
        Debug.println( F(": ExternAnfordern = LOW; AnforderungGesperrt"));
      }
    }
    break;
  case AnforderungGesperrt:
    if (millis() - warteSeit1 > 120000) { //Vor allem Sperre für Ventil
      ValueX10new1 = WarteAufAnforderung;
      if (thermeLogLevel > 1) {
        Debug.print  ( F("Therme "));
        Debug.print  ( millis());
        Debug.println( F(": WarteAufAnforderung"));
      }
    }
    break;
  default:
    tmpUint16_1 = 0;
    if (Values[_HKAnforderung].ValueX10 >= AnforderungTRUE) {
      tmpUint16_1 = Values[ _HKVorlaufTempSoll].ValueX10 + HKHysterese + HKSpreizung * 2;
    }
    if (Values[_WWAnforderung].ValueX10 >= AnforderungTRUE) {
      tmpUint16_1 = max( tmpUint16_1, Values[ _WWSpeicherTempSoll].ValueX10 + WWHysterese + WWSpreizung + max( 0, Values[ _WWSpeicherTempSoll].ValueX10 - Values[ _WWSpeicherTemp1].ValueX10) * 2);
      //                 Wert von HK übernehmen falls grüßer
      //                              Temp muß hoch genug sein, also Soll + Hysterese + Spreizung + (Delta T * 2 aber nur wenn größer 0 - damit Heizen wir mehr wenn großer Verbrauch die Temp. stärker drückt)
    }
    if ( tmpUint16_1 > 1036 / 7) {
      tmpUint16_1 = max( tmpUint16_1,  Values[_ThermeKesselTempIst].ValueX10 - 20);
      //ValueX10new1 = ThermeVorlaufTempVorgabe;
      //ValueX10new1 = ThermeVorlaufTempVorgabeValue + (ThermeVorlaufTempVorgabe - Values[_ThermeVorlaufTempIst].ValueX10) * 255 /1000; // ThermeVorlaufTempVorgabeValue berechnen
      ValueX10new1 = (tmpUint16_1 * 7 - 1036) / 3;//bei 1050 unf 65C Soll entstehen 63
      if ( Values[_ThermeVorlaufValue].ValueX10 != ValueX10new1 ) {
        digitalWrite( ThermeExternAnfordernPin, HIGH);
        analogWrite( ThermeVorlaufTempVorgabePin, ValueX10new1 / 10);               // PWM setzen
        if (thermeLogLevel > 1) {
          Debug.print  ( F("Therme "));
          Debug.print  ( millis());
          Debug.println( F(": B"));
        }
      } 
    }
    else { 
      if (Values[_ThermeVorlaufValue].ValueX10 > 0) {
        ValueX10new1 = PumpennachlaufStart;
        if (thermeLogLevel > 1) {
          Debug.print  ( F("Therme "));
          Debug.print  ( millis());
          Debug.println( F(": A"));
        }
      }
    } 
  }
  if ( setValue( _ThermeVorlaufValue, ValueX10new1) && thermeLogLevel > 1) {
    Debug.print  ( F("Therme "));
    Debug.print  ( millis());
    Debug.print   ( F(": ThermeVorlaufValue Zugewiesen: "));
    Debug.println   ( Values[_ThermeVorlaufValue].ValueX10);
  }
}

void BerechneThermeVentilVorlaufPin(){
  if ( digitalRead( ThermeVentilVorlaufPin) == LOW) {
    if ( Values[_ThermeVorlaufValue].ValueX10 >= WarteAufAnforderung
      && Values[ _ThermeVorlaufTempIst].ValueX10 != ValueUnknown
      && Values[ _SpeicherA1          ].ValueX10 != ValueUnknown
      && Values[ _SpeicherA2          ].ValueX10 != ValueUnknown
      && Values[ _ThermeVorlaufTempIst].ValueX10 >= Values[ _SpeicherA1].ValueX10 + 10) {
      digitalWrite( ThermeVentilVorlaufPin, HIGH);
    }
  } 
  else {
    if (    Values[_ThermeVorlaufValue].ValueX10 >= Pumpennachlauf
         && Values[_ThermeVorlaufValue].ValueX10 < WarteAufAnforderung) {                     //Wenn Therme gerade abgeschalten hat
      //if ( Values[ _ThermeVorlaufTempIst].ValueX10 <= Values[ _SpeicherA2].ValueX10 + 20) {//erst 2° unter SpeicherA2 runter schalten
        digitalWrite( ThermeVentilVorlaufPin, LOW);
      //}
    }
    else { 
      if ( Values[ _ThermeVorlaufTempIst].ValueX10 <= Values[ _SpeicherA1].ValueX10 - 20) { //normal bei 2° unter A1 runter schalten
        digitalWrite( ThermeVentilVorlaufPin, LOW); 
      } 
    }
  }
  if ( setValue( _ThermeVorlaufVentil, (digitalRead( ThermeVentilVorlaufPin) == HIGH)? 10: 0) && thermeLogLevel > 1) {
    Debug.print   ( F("Therme: ThermeVorlaufVentil Zugewiesen: "));
    Debug.println   ( Values[_ThermeVorlaufVentil].ValueX10);
  }
}























