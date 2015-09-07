#define HKPumpeAnPin         31


void HKInit(){
  pinMode     (HKPumpeAnPin        , OUTPUT   );
  digitalWrite(HKPumpeAnPin        , LOW      );

  Serial2.begin(9600);

}

void HKSolarbetriebBeginnt() {
  digitalWrite(HKPumpeAnPin, HIGH);
} 
void HKSolarbetriebEndet() {
  digitalWrite(HKPumpeAnPin, LOW );
} 

void   HKDoEvents() {
  HKAnforderung();
  HKVentil ();
}

void HKAnforderung() {
  ValueX10new1 = AnforderungNOT_INITIALIZED;
  static uint32_t AnforderungSeit;
  if (Values[_HKAnforderung].ValueX10 < AnforderungTRUE) { // Es ist noch keine Anforderung aktiv -> prüfen was gegen einschalten spricht
    if      ( Values[_HKVorlaufTemp1].ValueX10      == ValueUnknown                       ) ValueX10new1 = AnforderungFALSE_Temp1_UNKNOWN;
    else if ( Values[_HKVorlaufTemp1].ValueX10      >= Values[_HKVorlaufTempSoll].ValueX10) ValueX10new1 = AnforderungFALSE_Temp1;
    else if ( Values[_SpeicherA2    ].ValueX10      == ValueUnknown                       ) ValueX10new1 = AnforderungFALSE_Temp2_UNKNOWN;
    else if ( Values[_SpeicherA2    ].ValueX10 - 10 >= Values[_HKVorlaufTempSoll].ValueX10) ValueX10new1 = AnforderungFALSE_Temp2; 
    else if ( Values[_SpeicherA3    ].ValueX10      == ValueUnknown                       ) ValueX10new1 = AnforderungFALSE_Temp3_UNKNOWN;
    else if ( millis() - AnforderungSeit > 300000                                         ) ValueX10new1 = AnforderungFALSE_Zeitlimit; 
    else                                                                                    ValueX10new1 = AnforderungTRUE;
    if ( ValueX10new1 < AnforderungFALSE_Zeitlimit){ // Die Einschaltkriterien sind nicht erfüllt
      AnforderungSeit = millis();      //Wartezeit  zurücksetzen
    }
  } 
  else {                                           //Die Anforderung ist bereits aktiv -> kömmer nu endlich AUS?
    if      ( Values[_SpeicherA1].ValueX10 < Values[_HKVorlaufTempSoll].ValueX10 + HKHysterese) ValueX10new1 = AnforderungTRUE_Temp1;
    else if ( Values[_SpeicherA2].ValueX10 < Values[_HKVorlaufTempSoll].ValueX10 + HKHysterese) ValueX10new1 = AnforderungTRUE_Temp2;
    else if ( Values[_SpeicherA3].ValueX10 < Values[_HKVorlaufTempSoll].ValueX10              ) ValueX10new1 = AnforderungTRUE_Temp3;
    else                                                                              ValueX10new1 = AnforderungFALSE_Ausschaltkriterien;
  }
  if ( setValue( _HKAnforderung, ValueX10new1) && hkLogLevel > 1) {
    Debug.print   ( F("Therme: HKAnforderung Zugewiesen: "));
    Debug.println ( Values[_HKAnforderung].ValueX10);
  }
}

#define posIntervall 10000  // 10Sek
uint32_t HKVentilRechnenMillis;
int16_t vDelta = 0, vDeltaAlt = 0;
void HKVentil() {
  if ( Values[_HKVorlaufTempSoll].ValueX10 == ValueUnknown
    ||  Values[_HKVorlaufTemp1].ValueX10   == ValueUnknown
    ||  Values[_HKVorlaufTemp2].ValueX10   == ValueUnknown
    ||  Values[_HKRuecklaufTemp2].ValueX10 == ValueUnknown) { //Wenn die Vorlauftemperatur nicht gesetzt wurde können wir nichts regeln
      HKVentilRechnenMillis = millis() - posIntervall;
    } else {
    if (millis() - HKVentilRechnenMillis >= posIntervall ) {
      HKVentilRechnenMillis = millis();
      
      ValueX10new1 = Values[_HKVorlaufValue].ValueX10;
      vDelta = vDeltaAlt + (Values[_HKVorlaufTemp2].ValueX10 - Values[_HKVorlaufTempSoll].ValueX10);
#define vDeltaFaktor 20      
      tmpInt32_2 = Values[_HKVorlaufTempSoll].ValueX10 - vDelta / vDeltaFaktor;
#define vDeltaPlusMinusX10 30
      if ( vDelta > vDeltaPlusMinusX10 * vDeltaFaktor) 
        vDelta = vDeltaPlusMinusX10 * vDeltaFaktor;
      else if ( vDelta < -vDeltaPlusMinusX10 * vDeltaFaktor)
        vDelta = -vDeltaPlusMinusX10 * vDeltaFaktor;
       vDeltaAlt = vDelta;
      if ( Values[_HKVorlaufTemp2].ValueX10 > 600) {  // Zudrehen!!!!
        if (hkLogLevel > 0) Debug.println ("HK: ! >600 = soKALTwiesGEHT");
        ValueX10new1 = 0;
      }     
      else if ( Values[_HKVorlaufTemp1].ValueX10 == tmpInt32_2) {   // Bei Rücklauf = Soll drehen wir einfach auf Vorlauf sonst gibts #DIV/0
        if (hkLogLevel > 0) Debug.println ("HK: Sonderfall 1");
        ValueX10new1 =  1000;
      } 
      else if ( Values[_HKVorlaufTemp1].ValueX10 == Values[_HKRuecklaufTemp2].ValueX10) {  // Bei Vorlauf = Rücklauf gibts auch #DIV/0. Wir machen vorsichtshalber erstmal garnischt
        if (hkLogLevel > 0) Debug.println ("HK: Sonderfall 2");
      } 
      else {
#define Fak       100 //Vermeidet Rundungsdifferenzen beim dividieren
#define Schritte 1000
        Debug.print  ( F ("V="));
        Debug.print  (     Values[_HKVorlaufTemp1].ValueX10    );
        Debug.print  ( F (" R="));
        Debug.print  (      Values[_HKRuecklaufTemp2].ValueX10 );
        Debug.print  ( F (" S="));
        Debug.print  (      Values[_HKVorlaufTempSoll].ValueX10);
        Debug.print  ( F (" ("));
        Debug.print  (      vDelta                             );
        tmpInt32_1 = Fak * (Values[_HKRuecklaufTemp2].ValueX10 - tmpInt32_2) / ( tmpInt32_2 - Values[_HKVorlaufTemp1].ValueX10);
        Debug.print  ( F (") tmpInt32_1.1="));
        Debug.print  (     tmpInt32_1   );
        tmpInt32_1 = (int32_t)Schritte * tmpInt32_1 / (Fak + tmpInt32_1);
        Debug.print  ( F (" tmpInt32_1.2="));
        Debug.print  (     tmpInt32_1   );
        ValueX10new1 = min( Schritte, max( 0, tmpInt32_1));
        Debug.print  ( F (" ValueX10new1 "));
        Debug.println  (      ValueX10new1   );
        //        ValueX10new1 = sqrt(ValueX10new1 * abs(ValueX10new1 + 0.001 * (bPos - iPos)));
        //        Debug.print  ( F (" -> "));
        //        Debug.println( ValueX10new1);
      }
      if ( setValue( _HKVorlaufValue, ValueX10new1)) {
        if ( hkLogLevel > 1) {
          Debug.print   ( F("Therme: _HKVorlaufValue Zugewiesen: "));
          Debug.println ( Values[_HKVorlaufValue].ValueX10);
        }
      }
      Serial2.print( ValueX10new1);
      Serial2.println( '#');
    }
  }
}








