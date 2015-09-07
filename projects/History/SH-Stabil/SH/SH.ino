#define    ethLogLevel 1   //0 nix, 1 Fehlermeldungen, 2 Ein/Ausgehender Datenverkehr, 3 AlleInfos
#define   allgLogLevel 1
#define   vitoLogLevel 1
#define     owLogLevel 0
#define thermeLogLevel 6
#define     hkLogLevel 1
#define     wwLogLevel 1
#define  solarLogLevel 1

#define Debug Serial

#include <EEPROM.h>
//#include <ByteBuffer.h>
//#include <PString.h>
#include <MemoryFree.h>
#include <Clock.h>
Clock g_Clock;

#include <avr/pgmspace.h>
#define myPROGMEM __attribute__((section(".progmem.data")))

void setup() {
  Debug.begin( 57600);
  Debug.println( "Setup");
  AllgInit();
  EthInit();
  VitoInit();
  ThermeInit();
  HKInit();
  WWInit();
//  SolarInit();
  g_Clock.SetTimezoneOffset(1,0); // Setup the clock. In Germany we are 1 hours ahead of GMT 
  Debug.print("freeMemory  = "); 
  Debug.println( freeMemory());
  //  ATS_GetFreeMemory();
}

void loop() {
  AllgDoEvents();
  ethDoEvents();
  vitoDoEvents();
  oneWireDoEvents();
  ThermeDoEvents();
  WWDoEvents();
//  HKDoEvents();
//  SolarDoEvents();
  g_Clock.Maintain();
}







