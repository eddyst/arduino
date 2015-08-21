#define    ethLogLevel 2   //0 nix, 1 Fehlermeldungen, 2 Ein/Ausgehender Datenverkehr, 3 AlleInfos
#define    aDCLogLevel 4   //0 nix, 1 Fehlermeldungen + unbekannte Addressen, 2 alle Buswechsel u. readings, 3 Statuswechsel, 4  Zuordnung + alles

#define Debug Serial
#define useDHCP false

#include <Controllino.h>
#include <SPI.h>
#include <Ethernet.h>

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0x00, 0x01 };   // MAC address.
/*IPAddress ip(192,168,1, 177);
IPAddress dnsServer(192,168,1, 1);
IPAddress gateway(192,168,1, 1);*/
IPAddress ip       ( 10,  0,  6, 103);
IPAddress dnsServer( 10,  4,  1,   1);
IPAddress gateway  ( 10,  4,  1,   1);
IPAddress subnet   (255,  0,  0,   0);

EthernetServer server(23); //Port festlegen
EthernetClient clients[4];

const uint8_t pinCount = 9;
//volatile uint32_t zaehler1 = 0;

void setup() {
  // put your setup code here, to run once:
  Debug.begin( 115200);
  Debug.println( F( "Setup"));
  EthInit();
  DoWorkInit();
}

void loop() {
  // put your main code here, to run repeatedly:
  DoWork();
  ethDoEvents();
}

void ethNewClient(byte i){
  Debug.print  (F("New client"));
  Debug.println(i);
  
  for (uint8_t pin = 0; pin < pinCount; pin++) {
    PrintPin(pin);
  }
}
