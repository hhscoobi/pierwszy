#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#define SUPLADEVICE_CPP
#include <SuplaDevice.h>

#include <DoubleResetDetector.h> //Bilioteka by Stephen Denne
#define DRD_TIMEOUT 5// Number of seconds after reset during which a subseqent reset will be considered a double reset.
#define DRD_ADDRESS 0 // RTC Memory Address for the DoubleResetDetector to use
DoubleResetDetector drd(DRD_TIMEOUT, DRD_ADDRESS);

#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>
extern "C" {
#include "user_interface.h"
}
// Include the libraries we need
#include <OneWire.h>
#include <DallasTemperature.h>
#include <EEPROM.h>
#define ONE_WIRE 2 // GPIO na którym jest magistrala 1-wire
#define TEMPERATURE_PRECISION 12  // rozdzielczość czujnika DS 9 -12 bit

OneWire oneWire(ONE_WIRE);
DallasTemperature sensors(&oneWire);

char Supla_server[40];
char Location_id[15];
char Location_Pass[20];
int aktualny_status; 
String status_msg = "";
String old_status_msg = "";
    
String esid = "";  
String epass = "";

const char* Config_Wifi_name = "SUPLA-TS_DS";
const char* Config_Wifi_pass = "12345678";

const char* THINGSPEAK_host = "api.thingspeak.com"; // Adres serwera THINGSPEAK
byte DS_mean_co_max = 10; //Wartosc okreslajaca po ilu razach zgłaszany jest blad -127 stopni
float Temp_THINGSPEAK_DS0  = 0; float Temp_THINGSPEAK_DS1  = 0; float Temp_THINGSPEAK_DS2  = 0; float Temp_THINGSPEAK_DS3  = 0;
float Temp_THINGSPEAK_DS4  = 0; float Temp_THINGSPEAK_DS5  = 0; float Temp_THINGSPEAK_DS6  = 0; float Temp_THINGSPEAK_DS7  = 0;
double Temp_SUPLA_DS0 = 0;double Temp_SUPLA_DS1 = 0; double Temp_SUPLA_DS2 = 0; double Temp_SUPLA_DS3 = 0;
double Temp_SUPLA_DS4 = 0;double Temp_SUPLA_DS5 = 0; double Temp_SUPLA_DS6 = 0; double Temp_SUPLA_DS7 = 0;
byte DS_mean_co = 0;

word Licznik_iteracji = 0;

WiFiClient client;
const char* host = "supla";
const char* update_path = "/firmware";
const char* update_username = "admin";
const char* update_password = "supla";
String My_guid = "";
String My_mac = "";
String custom_Supla_server = "";
String Location_id_string = "";
String custom_Location_Pass = "";
bool DHCP = 0;
bool Nowy_pomiar = 0;

ESP8266WebServer httpServer(80);

const String SuplaDevice_setName = "SUPLA / ThingSpeak v1.1";

String content;
String e_api_ts_key;
String zmienna = "";
int THSP_counter_time = 0;


byte DS18b20_1[8]; byte DS18b20_2[8]; byte DS18b20_3[8]; byte DS18b20_4[8]; byte DS18b20_5[8]; byte DS18b20_6[8]; byte DS18b20_7[8]; byte DS18b20_8[8];
byte DS18b20_1_err = 0;
byte DS18b20_2_err = 0;
byte DS18b20_3_err = 0;
byte DS18b20_4_err = 0;
byte DS18b20_5_err = 0;
byte DS18b20_6_err = 0;
byte DS18b20_7_err = 0;
byte DS18b20_8_err = 0;

String e_ds18b20_id_1 = "";
String e_ds18b20_id_2 = "";
String e_ds18b20_id_3 = "";
String e_ds18b20_id_4 = "";
String e_ds18b20_id_5 = "";
String e_ds18b20_id_6 = "";
String e_ds18b20_id_7 = "";
String e_ds18b20_id_8 = "";

int zmienna_int = 0;

byte nowe_dane_dla_thingspeak = 0;
ESP8266HTTPUpdateServer httpUpdater;

byte Modul_tryb_konfiguracji = 0;

//*********************************************************************************************************
void setup() {
  EEPROM.begin(4096);
  Serial.begin(115200);
  delay(100);
  Serial.print(" ");
  Serial.println(" ");
  Serial.print(" ");
  Serial.println(" ");
  Serial.print(" ");
  
  if (drd.detectDoubleReset()) {
    //drd.stop();
    Tryb_konfiguracji();
  }

  delay(5000);
  drd.stop();
  
  Pokaz_zawartosc_eeprom();

  Odczytaj_parametry_IP();
  
  if (WiFi.status() != WL_CONNECTED) { 
    WiFi_up();
  }
  
  WiFi.hostname("Supla/TS-8xDS");
  WiFi.softAPdisconnect(true); // wyłączenie rozgłaszania sieci ESP

  
  
  // Inicjalizacja DS18B20
  sensors.begin();
  Odczytaj_temperature();

  uint8_t mac[WL_MAC_ADDR_LENGTH];
  WiFi.macAddress(mac);
  
  char GUID[SUPLA_GUID_SIZE] = {0x20,0x18,0x10,0x30, 
                                mac[WL_MAC_ADDR_LENGTH - 6], 
                                mac[WL_MAC_ADDR_LENGTH - 5], 
                                mac[WL_MAC_ADDR_LENGTH - 4], 
                                mac[WL_MAC_ADDR_LENGTH - 3], 
                                mac[WL_MAC_ADDR_LENGTH - 2], 
                                mac[WL_MAC_ADDR_LENGTH - 1],
                                0x04,0x12,0x34,0x56,0x78,0x91};
  
  My_guid = "20181030-"+String(mac[WL_MAC_ADDR_LENGTH - 6],HEX) + String(mac[WL_MAC_ADDR_LENGTH - 5],HEX) + "-" + String(mac[WL_MAC_ADDR_LENGTH - 4],HEX) + String(mac[WL_MAC_ADDR_LENGTH - 3],HEX) + "-" + String(mac[WL_MAC_ADDR_LENGTH - 2],HEX) + String(mac[WL_MAC_ADDR_LENGTH - 1],HEX) + "-041234567890";
  My_mac = String(mac[WL_MAC_ADDR_LENGTH - 6],HEX) +":"+ String(mac[WL_MAC_ADDR_LENGTH - 5],HEX) +":"+ String(mac[WL_MAC_ADDR_LENGTH - 4],HEX) +":"+ String(mac[WL_MAC_ADDR_LENGTH - 3],HEX) +":"+ String(mac[WL_MAC_ADDR_LENGTH - 2],HEX) +":"+ String(mac[WL_MAC_ADDR_LENGTH - 1],HEX);
 

  SuplaDevice.addDS18B20Thermometer();  // DS na GPIO02
  SuplaDevice.addDS18B20Thermometer();  
  SuplaDevice.addDS18B20Thermometer();  
  SuplaDevice.addDS18B20Thermometer();  
  SuplaDevice.addDS18B20Thermometer();  
  SuplaDevice.addDS18B20Thermometer();  
  SuplaDevice.addDS18B20Thermometer(); 
  SuplaDevice.addDS18B20Thermometer();
  SuplaDevice.setTemperatureCallback(&get_temperature);
  SuplaDevice.setStatusFuncImpl(&status_func);
  
  Odczytaj_parametry_autentykacji();
  int Location_id = Location_id_string.toInt();

  if(custom_Supla_server != ""){
    strcpy(Supla_server, custom_Supla_server.c_str());
    strcpy(Location_Pass, custom_Location_Pass.c_str());

    SuplaDevice.setName("Supla/TS-8xDS");
    SuplaDevice.begin(GUID,              // Global Unique Identifier 
                      mac,               // Ethernet MAC address
                      Supla_server,      // SUPLA server address
                      Location_id,        // Location ID 
                      Location_Pass);    // Location Password
  }
  
  
  Serial.println();
  Serial.println("Uruchamianie serwera aktualizacji...");
  WiFi.mode(WIFI_STA);

  createWebServer();  

  httpUpdater.setup(&httpServer, update_path, update_username, update_password);
  httpServer.begin();
  
  
}

//*********************************************************************************************************

void loop() {
    if (WiFi.status() != WL_CONNECTED) { 
    WiFi_up();
  }
  SuplaDevice.setTemperatureCallback(&get_temperature);
  ++Licznik_iteracji;
  delay(1);

  if (Nowy_pomiar == 1) {
      SuplaDevice.iterate(); 
      Licznik_iteracji = 0;
      Nowy_pomiar = 0;
      Odczytaj_temperature();
  }
  if(Licznik_iteracji == 100){
    Licznik_iteracji = 0;
    SuplaDevice.iterate();
    switch (aktualny_status) { 
      case 2:  status_msg = "ALREADY INITIALIZED";      break;
      case 3:  status_msg = "CB NOT ASSIGNED";          break;
      case 4:  status_msg = "INVALID GUID";             break;
      case 5:  status_msg = "UNKNOWN SERVER_ADDRESS";   break;
      case 6:  status_msg = "UNKNOWN LOCATION_ID";      break;
      case 7:  status_msg = "INITIALIZED";              break;
      case 8:  status_msg = "CHANNEL LIMIT EXCEEDED";   break;
      case 9:  status_msg = "DISCONNECTED";             break;
      case 10: status_msg = "REGISTER IN PROGRESS";     break;
      case 11: status_msg = "ITERATE FAIL";             break;
      case 12: status_msg = "PROTOCOL VERSION ERROR";   break;
      case 13: status_msg = "BAD CREDENTIALS";          break;
      case 14: status_msg = "TEMPORARILY UNAVAILABLE";  break;
      case 15: status_msg = "LOCATION CONFLICT";        break;
      case 16: status_msg = "CHANNEL CONFLICT";         break;
      case 17: status_msg = "REGISTERED AND READY";     break;
      case 18: status_msg = "DEVICE IS DISABLED ";      break;
      case 19: status_msg = "LOCATION IS DISABLED";     break;
      case 20: status_msg = "DEVICE LIMIT EXCEEDED";    break; 
    }

    if(old_status_msg != status_msg){
      Serial.print(status_msg);
      old_status_msg = status_msg; 
    }
    
  }

if(Licznik_iteracji == 0 or Licznik_iteracji == 20 or Licznik_iteracji == 40 or Licznik_iteracji == 60 or Licznik_iteracji == 80){
  httpServer.handleClient();
}

  Odczytaj_pojedynczy_eeprom(190);
  if(zmienna == "1"){
    if(nowe_dane_dla_thingspeak == 10){
    nowe_dane_dla_thingspeak = 0;  
    e_api_ts_key = "";  for (int i = 200; i < 216; ++i)   {e_api_ts_key += char(EEPROM.read(i));}
    Serial.print("API: ");
    Serial.println(e_api_ts_key);
          
    Serial.println("Wysylam do Thingspeak");
    if (client.connect(THINGSPEAK_host,80)) {  //   "184.106.153.149" or api.thingspeak.com
      String postStr = e_api_ts_key;
      postStr +="&field1=" + String(Temp_THINGSPEAK_DS0);
      postStr +="&field2=" + String(Temp_THINGSPEAK_DS1);
      postStr +="&field3=" + String(Temp_THINGSPEAK_DS2);
      postStr +="&field4=" + String(Temp_THINGSPEAK_DS3);
      postStr +="&field5=" + String(Temp_THINGSPEAK_DS4);
      postStr +="&field6=" + String(Temp_THINGSPEAK_DS5);
      postStr +="&field7=" + String(Temp_THINGSPEAK_DS6);
      postStr +="&field8=" + String(Temp_THINGSPEAK_DS7);
      postStr += "\r\n\r\n";

      client.print("POST /update HTTP/1.1\n");
      client.print("Host: api.thingspeak.com\n");
      client.print("Connection: close\n");
      client.print("X-THINGSPEAKAPIKEY: "+ e_api_ts_key +"\n");
      client.print("Content-Type: application/x-www-form-urlencoded\n");
      client.print("Content-Length: ");
      client.print(postStr.length());
      client.print("\n\n");
      client.print(postStr);
      }
    client.stop();
    }
   }     
 }

//*********************************************************************************************************

// Supla.org ethernet layer
    int supla_arduino_tcp_read(void *buf, int count) {
        _supla_int_t size = client.available();
       
        if ( size > 0 ) {
            if ( size > count ) size = count;
            return client.read((uint8_t *)buf, size);
        };
        
        return -1;
    };
    
    int supla_arduino_tcp_write(void *buf, int count) {
        return client.write((const uint8_t *)buf, count);
    };
    
    bool supla_arduino_svr_connect(const char *server, int port) {
          return client.connect(server, 2015);
    }
    
    bool supla_arduino_svr_connected(void) {  
          return client.connected();
    }
    
    void supla_arduino_svr_disconnect(void) {
         client.stop();
    }
    
    void supla_arduino_eth_setup(uint8_t mac[6], IPAddress *ip) {
      WiFi_up();
    }


SuplaDeviceCallbacks supla_arduino_get_callbacks(void) {
  SuplaDeviceCallbacks cb;

  cb.tcp_read = &supla_arduino_tcp_read;
  cb.tcp_write = &supla_arduino_tcp_write;
  cb.eth_setup = &supla_arduino_eth_setup;
  cb.svr_connected = &supla_arduino_svr_connected;
  cb.svr_connect = &supla_arduino_svr_connect;
  cb.svr_disconnect = &supla_arduino_svr_disconnect;
  cb.get_temperature = &get_temperature;
  cb.get_temperature_and_humidity = NULL;
  cb.get_rgbw_value = NULL;
  cb.set_rgbw_value = NULL;

  return cb;
}

//*********************************************************************************************************

void createWebServer()
{    
  httpServer.on("/", []() {
                                      content = "<!DOCTYPE HTML>";
                                      content += "<meta http-equiv='content-type' content='text/html; charset=UTF-8'>";
                                      content += "<meta name='viewport' content='width=device-width,initial-scale=1,maximum-scale=1,user-scalable=no'>";
                                      content += "<style>body{font-size:14px;font-family:HelveticaNeue,'Helvetica Neue',HelveticaNeueRoman,HelveticaNeue-Roman,'Helvetica Neue Roman',TeXGyreHerosRegular,Helvetica,Tahoma,Geneva,Arial,sans-serif;font-weight:400;font-stretch:normal;background:#00d151;color:#fff;line-height:20px;padding:0}.s{width:460px;margin:0 auto;margin-top:calc(50vh - 340px);border:solid 3px #fff;padding:0 10px 10px;border-radius:3px}#l{display:block;max-width:150px;height:155px;margin:-80px auto 20px;background:#00d151;padding-right:5px}#l path{fill:#000}.w{margin:3px 0 16px;padding:5px 0;border-radius:3px;background:#fff;box-shadow:0 1px 3px rgba(0,0,0,.3)}h1,h3{margin:10px 8px;font-family:HelveticaNeueLight,HelveticaNeue-Light,'Helvetica Neue Light',HelveticaNeue,'Helvetica Neue',TeXGyreHerosRegular,Helvetica,Tahoma,Geneva,Arial,sans-serif;font-weight:300;font-stretch:normal;color:#000;font-size:23px}h1{margin-bottom:14px;color:#fff}span{display:block;margin:10px 7px 14px}i{display:block;font-style:normal;position:relative;border-bottom:solid 1px #00d151;height:42px}i:last-child{border:none}label{position:absolute;display:inline-block;top:0;left:8px;color:#00d151;line-height:41px;pointer-events:none}input,select{width:calc(100% - 145px);border:none;font-size:16px;line-height:40px;border-radius:0;letter-spacing:-.5px;background:#fff;color:#000;padding-left:144px;-webkit-appearance:none;-moz-appearance:none;appearance:none;outline:0!important;height:40px}select{padding:0;float:right;margin:1px 3px 1px 2px}button{width:100%;border:0;background:#000;padding:5px 10px;font-size:16px;line-height:40px;color:#fff;border-radius:3px;box-shadow:0 1px 3px rgba(0,0,0,.3);cursor:pointer}.c{background:#ffe836;position:fixed;width:100%;line-height:80px;color:#000;top:0;left:0;box-shadow:0 1px 3px rgba(0,0,0,.3);text-align:center;font-size:26px;z-index:100}@media all and (max-height:920px){.s{margin-top:80px}}@media all and (max-width:900px){.s{width:calc(100% - 20px);margin-top:40px;border:none;padding:0 8px;border-radius:0}#l{max-width:80px;height:auto;margin:10px auto 20px}h1,h3{font-size:19px}i{border:none;height:auto}label{display:block;margin:4px 0 12px;color:#00d151;font-size:13px;position:relative;line-height:18px}input,select{width:calc(100% - 10px);font-size:16px;line-height:28px;padding:0 5px;border-bottom:solid 1px #00d151}select{width:100%;float:none;margin:0}}</style>";
                                      content += "<div class='s'><svg version='1.1' id='l' x='0' y='0' viewBox='0 0 200 200' xml:space='preserve'><path d='M59.3,2.5c18.1,0.6,31.8,8,40.2,23.5c3.1,5.7,4.3,11.9,4.1,18.3c-0.1,3.6-0.7,7.1-1.9,10.6c-0.2,0.7-0.1,1.1,0.6,1.5c12.8,7.7,25.5,15.4,38.3,23c2.9,1.7,5.8,3.4,8.7,5.3c1,0.6,1.6,0.6,2.5-0.1c4.5-3.6,9.8-5.3,15.7-5.4c12.5-0.1,22.9,7.9,25.2,19c1.9,9.2-2.9,19.2-11.8,23.9c-8.4,4.5-16.9,4.5-25.5,0.2c-0.7-0.3-1-0.2-1.5,0.3c-4.8,4.9-9.7,9.8-14.5,14.6c-5.3,5.3-10.6,10.7-15.9,16c-1.8,1.8-3.6,3.7-5.4,5.4c-0.7,0.6-0.6,1,0,1.6c3.6,3.4,5.8,7.5,6.2,12.2c0.7,7.7-2.2,14-8.8,18.5c-12.3,8.6-30.3,3.5-35-10.4c-2.8-8.4,0.6-17.7,8.6-22.8c0.9-0.6,1.1-1,0.8-2c-2-6.2-4.4-12.4-6.6-18.6c-6.3-17.6-12.7-35.1-19-52.7c-0.2-0.7-0.5-1-1.4-0.9c-12.5,0.7-23.6-2.6-33-10.4c-8-6.6-12.9-15-14.2-25c-1.5-11.5,1.7-21.9,9.6-30.7C32.5,8.9,42.2,4.2,53.7,2.7c0.7-0.1,1.5-0.2,2.2-0.2C57,2.4,58.2,2.5,59.3,2.5z M76.5,81c0,0.1,0.1,0.3,0.1,0.6c1.6,6.3,3.2,12.6,4.7,18.9c4.5,17.7,8.9,35.5,13.3,53.2c0.2,0.9,0.6,1.1,1.6,0.9c5.4-1.2,10.7-0.8,15.7,1.6c0.8,0.4,1.2,0.3,1.7-0.4c11.2-12.9,22.5-25.7,33.4-38.7c0.5-0.6,0.4-1,0-1.6c-5.6-7.9-6.1-16.1-1.3-24.5c0.5-0.8,0.3-1.1-0.5-1.6c-9.1-4.7-18.1-9.3-27.2-14c-6.8-3.5-13.5-7-20.3-10.5c-0.7-0.4-1.1-0.3-1.6,0.4c-1.3,1.8-2.7,3.5-4.3,5.1c-4.2,4.2-9.1,7.4-14.7,9.7C76.9,80.3,76.4,80.3,76.5,81z M89,42.6c0.1-2.5-0.4-5.4-1.5-8.1C83,23.1,74.2,16.9,61.7,15.8c-10-0.9-18.6,2.4-25.3,9.7c-8.4,9-9.3,22.4-2.2,32.4c6.8,9.6,19.1,14.2,31.4,11.9C79.2,67.1,89,55.9,89,42.6z M102.1,188.6c0.6,0.1,1.5-0.1,2.4-0.2c9.5-1.4,15.3-10.9,11.6-19.2c-2.6-5.9-9.4-9.6-16.8-8.6c-8.3,1.2-14.1,8.9-12.4,16.6C88.2,183.9,94.4,188.6,102.1,188.6z M167.7,88.5c-1,0-2.1,0.1-3.1,0.3c-9,1.7-14.2,10.6-10.8,18.6c2.9,6.8,11.4,10.3,19,7.8c7.1-2.3,11.1-9.1,9.6-15.9C180.9,93,174.8,88.5,167.7,88.5z'/></svg>";
   
                                      content += "<h1><center>" + String(SuplaDevice_setName) + "</center></h1>";
                                      content += "<font size='2'>STATUS: " + status_msg + "</font><br>";
                                      content += "<font size='2'>GUID: " + My_guid + "</font><br>";
                                      content += "<font size='2'>MAC:  " + My_mac + "</font><br>";
  long rssi = WiFi.RSSI();            content += "<font size='2'>RSSI: " + String(rssi) + "</font>";
                                      content += "<form method='post' action='set0'>";
                                      content += "<div class='w'>";
                                      content += "<h3>Ustawienia WIFI</h3>";
  Odczytaj_zakres_eeprom(0, 32);      content += "<i><input name='wifi_ssid' value='" + String(zmienna.c_str()) + "'length=32><label>Nazwa sieci</label></i>";
  Odczytaj_zakres_eeprom(32, 96);     content += "<i><input type='password' name='wifi_pass' value='" + String(zmienna.c_str()) + "'minlength='8' required length=64><label>Hasło</label></i>";
                                      content += "</div>";
                                      content += "<div class='w'>"; 
                                      content += "<h3>Ustawienia modulu"; 
  if (DHCP == 1){                     content += " (DHCP)";}
                                      content += "</h3>";
  Odczytaj_zakres_eeprom(4000, 4015); content += "<i><input name='ip_address' value='" + String(zmienna.c_str()) + "'length=15><label>IP:</label></i>";
  Odczytaj_zakres_eeprom(4015, 4030); content += "<i><input name='ip_mask' value='" + String(zmienna.c_str()) + "'length=15><label>Maska:</label></i>";
  Odczytaj_zakres_eeprom(4030, 4045); content += "<i><input name='ip_gateway' value='" + String(zmienna.c_str()) + "'length=15><label>Brama:</label></i>";
                                      content += "</div>";
                                      content += "<div class='w'>";
                                      content += "<h3>Ustawienia SUPLA</h3>";
  Odczytaj_zakres_eeprom(300, 350);   content += "<i><input name='Supla_Server_ADR' value='" + String(zmienna.c_str()) + "'length=32><label>Adres serwera</label></i>";
  Odczytaj_zakres_eeprom(350, 360);   content += "<i><input name='Supla_Location_ID' value='" + String(zmienna.c_str()) + "'length=10><label>ID Lokalizacji</label></i>";
  Odczytaj_zakres_eeprom(360, 410);   content += "<i><input type='password' name='Supla_Location_Pass' value='" + String(zmienna.c_str()) + "'minlength='4' required length=50><label>Hasło Lokalizacji</label></i>";
                                      content += "</div>";
                                      content += "<div class='w'>";
                                      content += "<h3>THINGSPEAK API</h3>";
  Odczytaj_zakres_eeprom(200, 216);   content += "<i><input name='api_key_1' value='" + String(zmienna.c_str()) + "'length=16><label>Klucz API</label></i>";
  Odczytaj_pojedynczy_eeprom(190);    content += "<i><input name='thingspeak_on_off' value='" + String(zmienna.c_str()) + "'length=1><label>1=On, 0=Off</label></i>";
                                      content += "</div>";
                                      
                                      content += "<div class='w'>";
  if (sensors.isParasitePowerMode()){ content += "<h3>Konfiguracja DS18b20 (Tryb pasożytniczy)</h3>";
                                }else{content += "<h3>Konfiguracja DS18b20</h3>";
  }
  Odczytaj_ID_urzadzen_jako_string(); content += "<i><input name='ds18b20_id_1' value='" + String(e_ds18b20_id_1) + "'minlength='16' required maxlength='16' required length=16><label>ID1 ";if(e_ds18b20_id_1 == "FFFFFFFFFFFFFFFF"){content += "</label></i>";DS18b20_1_err = 0;}else{content += String(Temp_THINGSPEAK_DS0) + "&deg;C ";if(DS18b20_1_err >= 1){content += "<font color='red'>E:"+String(DS18b20_1_err)+"</font>";}content += "</label></i>";}
                                      content += "<i><input name='ds18b20_id_2' value='" + String(e_ds18b20_id_2) + "'minlength='16' required maxlength='16' length=16><label>ID2 ";if(e_ds18b20_id_2 == "FFFFFFFFFFFFFFFF"){content += "</label></i>";DS18b20_2_err = 0;}else{content += String(Temp_THINGSPEAK_DS1) + "&deg;C ";if(DS18b20_2_err >= 1){content += "<font color='red'>E:"+String(DS18b20_2_err)+"</font>";}content += "</label></i>";}
                                      content += "<i><input name='ds18b20_id_3' value='" + String(e_ds18b20_id_3) + "'minlength='16' required maxlength='16' length=16><label>ID3 ";if(e_ds18b20_id_3 == "FFFFFFFFFFFFFFFF"){content += "</label></i>";DS18b20_3_err = 0;}else{content += String(Temp_THINGSPEAK_DS2) + "&deg;C ";if(DS18b20_3_err >= 1){content += "<font color='red'>E:"+String(DS18b20_3_err)+"</font>";}content += "</label></i>";}
                                      content += "<i><input name='ds18b20_id_4' value='" + String(e_ds18b20_id_4) + "'minlength='16' required maxlength='16' length=16><label>ID4 ";if(e_ds18b20_id_4 == "FFFFFFFFFFFFFFFF"){content += "</label></i>";DS18b20_4_err = 0;}else{content += String(Temp_THINGSPEAK_DS3) + "&deg;C ";if(DS18b20_4_err >= 1){content += "<font color='red'>E:"+String(DS18b20_4_err)+"</font>";}content += "</label></i>";}
                                      content += "<i><input name='ds18b20_id_5' value='" + String(e_ds18b20_id_5) + "'minlength='16' required maxlength='16' length=16><label>ID5 ";if(e_ds18b20_id_5 == "FFFFFFFFFFFFFFFF"){content += "</label></i>";DS18b20_5_err = 0;}else{content += String(Temp_THINGSPEAK_DS4) + "&deg;C ";if(DS18b20_5_err >= 1){content += "<font color='red'>E:"+String(DS18b20_5_err)+"</font>";}content += "</label></i>";}
                                      content += "<i><input name='ds18b20_id_6' value='" + String(e_ds18b20_id_6) + "'minlength='16' required maxlength='16' length=16><label>ID6 ";if(e_ds18b20_id_6 == "FFFFFFFFFFFFFFFF"){content += "</label></i>";DS18b20_6_err = 0;}else{content += String(Temp_THINGSPEAK_DS5) + "&deg;C ";if(DS18b20_6_err >= 1){content += "<font color='red'>E:"+String(DS18b20_6_err)+"</font>";}content += "</label></i>";}
                                      content += "<i><input name='ds18b20_id_7' value='" + String(e_ds18b20_id_7) + "'minlength='16' required maxlength='16' length=16><label>ID7 ";if(e_ds18b20_id_7 == "FFFFFFFFFFFFFFFF"){content += "</label></i>";DS18b20_7_err = 0;}else{content += String(Temp_THINGSPEAK_DS6) + "&deg;C ";if(DS18b20_7_err >= 1){content += "<font color='red'>E:"+String(DS18b20_7_err)+"</font>";}content += "</label></i>";}
                                      content += "<i><input name='ds18b20_id_8' value='" + String(e_ds18b20_id_8) + "'minlength='16' required maxlength='16' length=16><label>ID8 ";if(e_ds18b20_id_8 == "FFFFFFFFFFFFFFFF"){content += "</label></i>";DS18b20_8_err = 0;}else{content += String(Temp_THINGSPEAK_DS7) + "&deg;C ";if(DS18b20_8_err >= 1){content += "<font color='red'>E:"+String(DS18b20_8_err)+"</font>";}content += "</label></i>";}

                                      content += "</div>";
                                      content += "<button type='submit'>Zapisz</button></form>";
                                      content += "<br>";
                                      content += "<a href='/search'><button>Szukaj DS</button></a>";
                                      content += "<br><br>";
                                      content += "<a href='/firmware_up'><button>Aktualizacja</button></a>";
                                      content += "<br><br>"; 
                                      content += "<form method='post' action='reboot'>";
                                      content += "<button type='submit'>Restart</button></form></div>";
                                      content += "<br><br>";
  httpServer.send(200, "text/html", content);
  });


  httpServer.on("/search", []() {
                                      content = "<!DOCTYPE HTML>";
                                      content += "<meta http-equiv='content-type' content='text/html; charset=UTF-8'>";
                                      content += "<meta name='viewport' content='width=device-width,initial-scale=1,maximum-scale=1,user-scalable=no'>";
                                      content += "<style>body{font-size:14px;font-family:HelveticaNeue,'Helvetica Neue',HelveticaNeueRoman,HelveticaNeue-Roman,'Helvetica Neue Roman',TeXGyreHerosRegular,Helvetica,Tahoma,Geneva,Arial,sans-serif;font-weight:400;font-stretch:normal;background:#00d151;color:#fff;line-height:20px;padding:0}.s{width:460px;margin:0 auto;margin-top:calc(50vh - 340px);border:solid 3px #fff;padding:0 10px 10px;border-radius:3px}#l{display:block;max-width:150px;height:155px;margin:-80px auto 20px;background:#00d151;padding-right:5px}#l path{fill:#000}.w{margin:3px 0 16px;padding:5px 0;border-radius:3px;background:#fff;box-shadow:0 1px 3px rgba(0,0,0,.3)}h1,h3{margin:10px 8px;font-family:HelveticaNeueLight,HelveticaNeue-Light,'Helvetica Neue Light',HelveticaNeue,'Helvetica Neue',TeXGyreHerosRegular,Helvetica,Tahoma,Geneva,Arial,sans-serif;font-weight:300;font-stretch:normal;color:#000;font-size:23px}h1{margin-bottom:14px;color:#fff}span{display:block;margin:10px 7px 14px}i{display:block;font-style:normal;position:relative;border-bottom:solid 1px #00d151;height:42px}i:last-child{border:none}label{position:absolute;display:inline-block;top:0;left:8px;color:#00d151;line-height:41px;pointer-events:none}input,select{width:calc(100% - 145px);border:none;font-size:16px;line-height:40px;border-radius:0;letter-spacing:-.5px;background:#fff;color:#000;padding-left:144px;-webkit-appearance:none;-moz-appearance:none;appearance:none;outline:0!important;height:40px}select{padding:0;float:right;margin:1px 3px 1px 2px}button{width:100%;border:0;background:#000;padding:5px 10px;font-size:16px;line-height:40px;color:#fff;border-radius:3px;box-shadow:0 1px 3px rgba(0,0,0,.3);cursor:pointer}.c{background:#ffe836;position:fixed;width:100%;line-height:80px;color:#000;top:0;left:0;box-shadow:0 1px 3px rgba(0,0,0,.3);text-align:center;font-size:26px;z-index:100}@media all and (max-height:920px){.s{margin-top:80px}}@media all and (max-width:900px){.s{width:calc(100% - 20px);margin-top:40px;border:none;padding:0 8px;border-radius:0}#l{max-width:80px;height:auto;margin:10px auto 20px}h1,h3{font-size:19px}i{border:none;height:auto}label{display:block;margin:4px 0 12px;color:#00d151;font-size:13px;position:relative;line-height:18px}input,select{width:calc(100% - 10px);font-size:16px;line-height:28px;padding:0 5px;border-bottom:solid 1px #00d151}select{width:100%;float:none;margin:0}}</style>";
                                      content += "<div class='s'><svg version='1.1' id='l' x='0' y='0' viewBox='0 0 200 200' xml:space='preserve'><path d='M59.3,2.5c18.1,0.6,31.8,8,40.2,23.5c3.1,5.7,4.3,11.9,4.1,18.3c-0.1,3.6-0.7,7.1-1.9,10.6c-0.2,0.7-0.1,1.1,0.6,1.5c12.8,7.7,25.5,15.4,38.3,23c2.9,1.7,5.8,3.4,8.7,5.3c1,0.6,1.6,0.6,2.5-0.1c4.5-3.6,9.8-5.3,15.7-5.4c12.5-0.1,22.9,7.9,25.2,19c1.9,9.2-2.9,19.2-11.8,23.9c-8.4,4.5-16.9,4.5-25.5,0.2c-0.7-0.3-1-0.2-1.5,0.3c-4.8,4.9-9.7,9.8-14.5,14.6c-5.3,5.3-10.6,10.7-15.9,16c-1.8,1.8-3.6,3.7-5.4,5.4c-0.7,0.6-0.6,1,0,1.6c3.6,3.4,5.8,7.5,6.2,12.2c0.7,7.7-2.2,14-8.8,18.5c-12.3,8.6-30.3,3.5-35-10.4c-2.8-8.4,0.6-17.7,8.6-22.8c0.9-0.6,1.1-1,0.8-2c-2-6.2-4.4-12.4-6.6-18.6c-6.3-17.6-12.7-35.1-19-52.7c-0.2-0.7-0.5-1-1.4-0.9c-12.5,0.7-23.6-2.6-33-10.4c-8-6.6-12.9-15-14.2-25c-1.5-11.5,1.7-21.9,9.6-30.7C32.5,8.9,42.2,4.2,53.7,2.7c0.7-0.1,1.5-0.2,2.2-0.2C57,2.4,58.2,2.5,59.3,2.5z M76.5,81c0,0.1,0.1,0.3,0.1,0.6c1.6,6.3,3.2,12.6,4.7,18.9c4.5,17.7,8.9,35.5,13.3,53.2c0.2,0.9,0.6,1.1,1.6,0.9c5.4-1.2,10.7-0.8,15.7,1.6c0.8,0.4,1.2,0.3,1.7-0.4c11.2-12.9,22.5-25.7,33.4-38.7c0.5-0.6,0.4-1,0-1.6c-5.6-7.9-6.1-16.1-1.3-24.5c0.5-0.8,0.3-1.1-0.5-1.6c-9.1-4.7-18.1-9.3-27.2-14c-6.8-3.5-13.5-7-20.3-10.5c-0.7-0.4-1.1-0.3-1.6,0.4c-1.3,1.8-2.7,3.5-4.3,5.1c-4.2,4.2-9.1,7.4-14.7,9.7C76.9,80.3,76.4,80.3,76.5,81z M89,42.6c0.1-2.5-0.4-5.4-1.5-8.1C83,23.1,74.2,16.9,61.7,15.8c-10-0.9-18.6,2.4-25.3,9.7c-8.4,9-9.3,22.4-2.2,32.4c6.8,9.6,19.1,14.2,31.4,11.9C79.2,67.1,89,55.9,89,42.6z M102.1,188.6c0.6,0.1,1.5-0.1,2.4-0.2c9.5-1.4,15.3-10.9,11.6-19.2c-2.6-5.9-9.4-9.6-16.8-8.6c-8.3,1.2-14.1,8.9-12.4,16.6C88.2,183.9,94.4,188.6,102.1,188.6z M167.7,88.5c-1,0-2.1,0.1-3.1,0.3c-9,1.7-14.2,10.6-10.8,18.6c2.9,6.8,11.4,10.3,19,7.8c7.1-2.3,11.1-9.1,9.6-15.9C180.9,93,174.8,88.5,167.7,88.5z'/></svg>";
   
                                      content += "<h1><center>" + String(SuplaDevice_setName) + "</center></h1>";
                                      content += "<font size='2'>GUID: " + My_guid + "</font><br>";
                                      content += "<font size='2'>MAC:  " + My_mac + "</font><br>";
  long rssi = WiFi.RSSI();            content += "<font size='2'>RSSI: " + String(rssi) + "</font>";
                                      content += "<div class='w'>";
                                      content += "<h3>Lista dostępnych DS18b20";


  byte zmienna_byte;
  byte addr[8];
  byte Liczba_urzadzen = 0;
  String String_pomocniczy = "";
  String String_pomocniczy2 = "";
  while(oneWire.search(addr)) {
  ++Liczba_urzadzen;
  }
  oneWire.reset_search();
  content += " (" + String(Liczba_urzadzen) + ")</h3>";
  while(oneWire.search(addr)) {
    
    content += "<i><input name='' value='";
    for( zmienna_byte = 0; zmienna_byte < 8; zmienna_byte++) {if (addr[zmienna_byte] < 16) {String_pomocniczy += ("0");}String_pomocniczy += String(addr[zmienna_byte], HEX);zmienna = String_pomocniczy;Zamien_litery();String_pomocniczy = zmienna;}
    content += String_pomocniczy; 
    if(e_ds18b20_id_1 == String_pomocniczy){String_pomocniczy2 = "ID1";goto NEXT;}else{String_pomocniczy2 = "<font color='red'>NOWY</font>";}
    if(e_ds18b20_id_2 == String_pomocniczy){String_pomocniczy2 = "ID2";goto NEXT;}else{String_pomocniczy2 = "<font color='red'>NOWY</font>";}
    if(e_ds18b20_id_3 == String_pomocniczy){String_pomocniczy2 = "ID3";goto NEXT;}else{String_pomocniczy2 = "<font color='red'>NOWY</font>";}
    if(e_ds18b20_id_4 == String_pomocniczy){String_pomocniczy2 = "ID4";goto NEXT;}else{String_pomocniczy2 = "<font color='red'>NOWY</font>";}
    if(e_ds18b20_id_5 == String_pomocniczy){String_pomocniczy2 = "ID5";goto NEXT;}else{String_pomocniczy2 = "<font color='red'>NOWY</font>";}
    if(e_ds18b20_id_6 == String_pomocniczy){String_pomocniczy2 = "ID6";goto NEXT;}else{String_pomocniczy2 = "<font color='red'>NOWY</font>";}
    if(e_ds18b20_id_7 == String_pomocniczy){String_pomocniczy2 = "ID7";goto NEXT;}else{String_pomocniczy2 = "<font color='red'>NOWY</font>";}
    if(e_ds18b20_id_8 == String_pomocniczy){String_pomocniczy2 = "ID8";goto NEXT;}else{String_pomocniczy2 = "<font color='red'>NOWY</font>";}
     

    NEXT:
    String_pomocniczy = "";
    content += "'length=32><label>" + String_pomocniczy2 + "</label></i>";
    String_pomocniczy2 = "";
  }
  oneWire.reset_search();
                                     
  content += "</div>";
  content += "<br>";
  content += "<a href='/'><button>POWRÓT</button></a></div>";          
  content += "<br><br>";
  httpServer.send(200, "text/html", content);
  });

  httpServer.on("/set0", []() {

      String qssid = "";
      String qpass = "";
      String qip_address  = "";
      String qip_mask  = "";
      String qip_gateway  = "";
      String qSupla_Server_ADR = "";
      String qSupla_Location_ID = "";
      String qSupla_Location_Pass = "";
      String qapi_key_1  = "";
      String qthingspeak_on_off = "";
      String qds18b20_id_1 = "";
      String qds18b20_id_2 = "";
      String qds18b20_id_3 = "";
      String qds18b20_id_4 = "";
      String qds18b20_id_5 = "";
      String qds18b20_id_6 = "";
      String qds18b20_id_7 = "";
      String qds18b20_id_8 = "";

      qssid = httpServer.arg("wifi_ssid");
      qpass = httpServer.arg("wifi_pass");
      qip_address = httpServer.arg("ip_address");
      qip_mask = httpServer.arg("ip_mask");
      qip_gateway = httpServer.arg("ip_gateway");
      qSupla_Server_ADR = httpServer.arg("Supla_Server_ADR");
      qSupla_Location_ID = httpServer.arg("Supla_Location_ID");
      qSupla_Location_Pass = httpServer.arg("Supla_Location_Pass");
      qapi_key_1 = httpServer.arg("api_key_1");
      qthingspeak_on_off = httpServer.arg("thingspeak_on_off");
      qds18b20_id_1 = httpServer.arg("ds18b20_id_1");
      qds18b20_id_2 = httpServer.arg("ds18b20_id_2");
      qds18b20_id_3 = httpServer.arg("ds18b20_id_3");
      qds18b20_id_4 = httpServer.arg("ds18b20_id_4");
      qds18b20_id_5 = httpServer.arg("ds18b20_id_5");
      qds18b20_id_6 = httpServer.arg("ds18b20_id_6");
      qds18b20_id_7 = httpServer.arg("ds18b20_id_7");
      qds18b20_id_8 = httpServer.arg("ds18b20_id_8");



      zmienna = qds18b20_id_1; Zamien_litery(); qds18b20_id_1 = zmienna;
      zmienna = qds18b20_id_2; Zamien_litery(); qds18b20_id_2 = zmienna;
      zmienna = qds18b20_id_3; Zamien_litery(); qds18b20_id_3 = zmienna;
      zmienna = qds18b20_id_4; Zamien_litery(); qds18b20_id_4 = zmienna;
      zmienna = qds18b20_id_5; Zamien_litery(); qds18b20_id_5 = zmienna;
      zmienna = qds18b20_id_6; Zamien_litery(); qds18b20_id_6 = zmienna;
      zmienna = qds18b20_id_7; Zamien_litery(); qds18b20_id_7 = zmienna;
      zmienna = qds18b20_id_8; Zamien_litery(); qds18b20_id_8 = zmienna;
      
      byte CardNumberByte[8];
      for(char zmienna_char = 0; zmienna_char < 8; zmienna_char++) {byte extract; char a = qds18b20_id_1[2*zmienna_char]; char b = qds18b20_id_1[2*zmienna_char + 1]; extract = convertCharToHex(a)<<4 | convertCharToHex(b); DS18b20_1[zmienna_char] = extract;}
      for(char zmienna_char = 0; zmienna_char < 8; zmienna_char++) {byte extract; char a = qds18b20_id_2[2*zmienna_char]; char b = qds18b20_id_2[2*zmienna_char + 1]; extract = convertCharToHex(a)<<4 | convertCharToHex(b); DS18b20_2[zmienna_char] = extract;}
      for(char zmienna_char = 0; zmienna_char < 8; zmienna_char++) {byte extract; char a = qds18b20_id_3[2*zmienna_char]; char b = qds18b20_id_3[2*zmienna_char + 1]; extract = convertCharToHex(a)<<4 | convertCharToHex(b); DS18b20_3[zmienna_char] = extract;}
      for(char zmienna_char = 0; zmienna_char < 8; zmienna_char++) {byte extract; char a = qds18b20_id_4[2*zmienna_char]; char b = qds18b20_id_4[2*zmienna_char + 1]; extract = convertCharToHex(a)<<4 | convertCharToHex(b); DS18b20_4[zmienna_char] = extract;}  
      for(char zmienna_char = 0; zmienna_char < 8; zmienna_char++) {byte extract; char a = qds18b20_id_5[2*zmienna_char]; char b = qds18b20_id_5[2*zmienna_char + 1]; extract = convertCharToHex(a)<<4 | convertCharToHex(b); DS18b20_5[zmienna_char] = extract;}
      for(char zmienna_char = 0; zmienna_char < 8; zmienna_char++) {byte extract; char a = qds18b20_id_6[2*zmienna_char]; char b = qds18b20_id_6[2*zmienna_char + 1]; extract = convertCharToHex(a)<<4 | convertCharToHex(b); DS18b20_6[zmienna_char] = extract;}
      for(char zmienna_char = 0; zmienna_char < 8; zmienna_char++) {byte extract; char a = qds18b20_id_7[2*zmienna_char]; char b = qds18b20_id_7[2*zmienna_char + 1]; extract = convertCharToHex(a)<<4 | convertCharToHex(b); DS18b20_7[zmienna_char] = extract;}
      for(char zmienna_char = 0; zmienna_char < 8; zmienna_char++) {byte extract; char a = qds18b20_id_8[2*zmienna_char]; char b = qds18b20_id_8[2*zmienna_char + 1]; extract = convertCharToHex(a)<<4 | convertCharToHex(b); DS18b20_8[zmienna_char] = extract;}
      EEPROM.begin(4096);
      delay(100);
      for (int zmienna_int = 0; zmienna_int < 4096; ++zmienna_int) {
        EEPROM.write(zmienna_int, 0);  //Czyszczenie pamięci EEPROM
       
      }
      EEPROM.end();
      delay(100);
      EEPROM.begin(4096);
      delay(100);
      for (zmienna_int = 0; zmienna_int < qssid.length(); ++zmienna_int)                {EEPROM.write(0+zmienna_int, qssid[zmienna_int]);delay(3);}
      for (zmienna_int = 0; zmienna_int < qpass.length(); ++zmienna_int)                {EEPROM.write(32+zmienna_int, qpass[zmienna_int]);delay(3);}
      for (zmienna_int = 0; zmienna_int < qip_address.length(); ++zmienna_int)          {EEPROM.write(4000+zmienna_int, qip_address[zmienna_int]);delay(3);}
      for (zmienna_int = 0; zmienna_int < qip_mask.length(); ++zmienna_int)             {EEPROM.write(4015+zmienna_int, qip_mask[zmienna_int]);delay(3);}
      for (zmienna_int = 0; zmienna_int < qip_gateway.length(); ++zmienna_int)          {EEPROM.write(4030+zmienna_int, qip_gateway[zmienna_int]);delay(3);}
      for (zmienna_int = 0; zmienna_int < qSupla_Server_ADR.length(); ++zmienna_int)    {EEPROM.write(300+zmienna_int, qSupla_Server_ADR[zmienna_int]);delay(3);}
      for (zmienna_int = 0; zmienna_int < qSupla_Location_ID.length(); ++zmienna_int)   {EEPROM.write(350+zmienna_int, qSupla_Location_ID[zmienna_int]);delay(3);}
      for (zmienna_int = 0; zmienna_int < qSupla_Location_Pass.length(); ++zmienna_int) {EEPROM.write(360+zmienna_int, qSupla_Location_Pass[zmienna_int]);delay(3);}
      for (zmienna_int = 0; zmienna_int < qthingspeak_on_off.length(); ++zmienna_int)   {EEPROM.write(190+zmienna_int, qthingspeak_on_off[zmienna_int]);delay(3);}
      for (zmienna_int = 0; zmienna_int < qapi_key_1.length(); ++zmienna_int)           {EEPROM.write(200+zmienna_int, qapi_key_1[zmienna_int]);delay(3);}
      EEPROM.end();       delay(200);
      EEPROM.begin(4096); delay(100);
      byte zmienna_byte6 = 100;
      for(zmienna_int = 0; zmienna_int < 8; zmienna_int++){EEPROM.write(zmienna_byte6, DS18b20_1[zmienna_int]); zmienna_byte6++; }
      for(zmienna_int = 0; zmienna_int < 8; zmienna_int++){EEPROM.write(zmienna_byte6, DS18b20_2[zmienna_int]); zmienna_byte6++; }
      for(zmienna_int = 0; zmienna_int < 8; zmienna_int++){EEPROM.write(zmienna_byte6, DS18b20_3[zmienna_int]); zmienna_byte6++; }
      for(zmienna_int = 0; zmienna_int < 8; zmienna_int++){EEPROM.write(zmienna_byte6, DS18b20_4[zmienna_int]); zmienna_byte6++; }
      for(zmienna_int = 0; zmienna_int < 8; zmienna_int++){EEPROM.write(zmienna_byte6, DS18b20_5[zmienna_int]); zmienna_byte6++; }
      for(zmienna_int = 0; zmienna_int < 8; zmienna_int++){EEPROM.write(zmienna_byte6, DS18b20_6[zmienna_int]); zmienna_byte6++; }
      for(zmienna_int = 0; zmienna_int < 8; zmienna_int++){EEPROM.write(zmienna_byte6, DS18b20_7[zmienna_int]); zmienna_byte6++; }
      for(zmienna_int = 0; zmienna_int < 8; zmienna_int++){EEPROM.write(zmienna_byte6, DS18b20_8[zmienna_int]); zmienna_byte6++; } 
      
      EEPROM.end();

          
      content = "<!DOCTYPE HTML>";
      content += "<meta http-equiv='content-type' content='text/html; charset=UTF-8'>";
      content += "<meta name='viewport' content='width=device-width,initial-scale=1,maximum-scale=1,user-scalable=no'>";
      content += "<style>body{font-size:14px;font-family:HelveticaNeue,'Helvetica Neue',HelveticaNeueRoman,HelveticaNeue-Roman,'Helvetica Neue Roman',TeXGyreHerosRegular,Helvetica,Tahoma,Geneva,Arial,sans-serif;font-weight:400;font-stretch:normal;background:#00d151;color:#fff;line-height:20px;padding:0}.s{width:460px;margin:0 auto;margin-top:calc(50vh - 340px);border:solid 3px #fff;padding:0 10px 10px;border-radius:3px}#l{display:block;max-width:150px;height:155px;margin:-80px auto 20px;background:#00d151;padding-right:5px}#l path{fill:#000}.w{margin:3px 0 16px;padding:5px 0;border-radius:3px;background:#fff;box-shadow:0 1px 3px rgba(0,0,0,.3)}h1,h3{margin:10px 8px;font-family:HelveticaNeueLight,HelveticaNeue-Light,'Helvetica Neue Light',HelveticaNeue,'Helvetica Neue',TeXGyreHerosRegular,Helvetica,Tahoma,Geneva,Arial,sans-serif;font-weight:300;font-stretch:normal;color:#000;font-size:23px}h1{margin-bottom:14px;color:#fff}span{display:block;margin:10px 7px 14px}i{display:block;font-style:normal;position:relative;border-bottom:solid 1px #00d151;height:42px}i:last-child{border:none}label{position:absolute;display:inline-block;top:0;left:8px;color:#00d151;line-height:41px;pointer-events:none}input,select{width:calc(100% - 145px);border:none;font-size:16px;line-height:40px;border-radius:0;letter-spacing:-.5px;background:#fff;color:#000;padding-left:144px;-webkit-appearance:none;-moz-appearance:none;appearance:none;outline:0!important;height:40px}select{padding:0;float:right;margin:1px 3px 1px 2px}button{width:100%;border:0;background:#000;padding:5px 10px;font-size:16px;line-height:40px;color:#fff;border-radius:3px;box-shadow:0 1px 3px rgba(0,0,0,.3);cursor:pointer}.c{background:#ffe836;position:fixed;width:100%;line-height:80px;color:#000;top:0;left:0;box-shadow:0 1px 3px rgba(0,0,0,.3);text-align:center;font-size:26px;z-index:100}@media all and (max-height:920px){.s{margin-top:80px}}@media all and (max-width:900px){.s{width:calc(100% - 20px);margin-top:40px;border:none;padding:0 8px;border-radius:0}#l{max-width:80px;height:auto;margin:10px auto 20px}h1,h3{font-size:19px}i{border:none;height:auto}label{display:block;margin:4px 0 12px;color:#00d151;font-size:13px;position:relative;line-height:18px}input,select{width:calc(100% - 10px);font-size:16px;line-height:28px;padding:0 5px;border-bottom:solid 1px #00d151}select{width:100%;float:none;margin:0}}</style>";
      content += "<div class='s'><svg version='1.1' id='l' x='0' y='0' viewBox='0 0 200 200' xml:space='preserve'><path d='M59.3,2.5c18.1,0.6,31.8,8,40.2,23.5c3.1,5.7,4.3,11.9,4.1,18.3c-0.1,3.6-0.7,7.1-1.9,10.6c-0.2,0.7-0.1,1.1,0.6,1.5c12.8,7.7,25.5,15.4,38.3,23c2.9,1.7,5.8,3.4,8.7,5.3c1,0.6,1.6,0.6,2.5-0.1c4.5-3.6,9.8-5.3,15.7-5.4c12.5-0.1,22.9,7.9,25.2,19c1.9,9.2-2.9,19.2-11.8,23.9c-8.4,4.5-16.9,4.5-25.5,0.2c-0.7-0.3-1-0.2-1.5,0.3c-4.8,4.9-9.7,9.8-14.5,14.6c-5.3,5.3-10.6,10.7-15.9,16c-1.8,1.8-3.6,3.7-5.4,5.4c-0.7,0.6-0.6,1,0,1.6c3.6,3.4,5.8,7.5,6.2,12.2c0.7,7.7-2.2,14-8.8,18.5c-12.3,8.6-30.3,3.5-35-10.4c-2.8-8.4,0.6-17.7,8.6-22.8c0.9-0.6,1.1-1,0.8-2c-2-6.2-4.4-12.4-6.6-18.6c-6.3-17.6-12.7-35.1-19-52.7c-0.2-0.7-0.5-1-1.4-0.9c-12.5,0.7-23.6-2.6-33-10.4c-8-6.6-12.9-15-14.2-25c-1.5-11.5,1.7-21.9,9.6-30.7C32.5,8.9,42.2,4.2,53.7,2.7c0.7-0.1,1.5-0.2,2.2-0.2C57,2.4,58.2,2.5,59.3,2.5z M76.5,81c0,0.1,0.1,0.3,0.1,0.6c1.6,6.3,3.2,12.6,4.7,18.9c4.5,17.7,8.9,35.5,13.3,53.2c0.2,0.9,0.6,1.1,1.6,0.9c5.4-1.2,10.7-0.8,15.7,1.6c0.8,0.4,1.2,0.3,1.7-0.4c11.2-12.9,22.5-25.7,33.4-38.7c0.5-0.6,0.4-1,0-1.6c-5.6-7.9-6.1-16.1-1.3-24.5c0.5-0.8,0.3-1.1-0.5-1.6c-9.1-4.7-18.1-9.3-27.2-14c-6.8-3.5-13.5-7-20.3-10.5c-0.7-0.4-1.1-0.3-1.6,0.4c-1.3,1.8-2.7,3.5-4.3,5.1c-4.2,4.2-9.1,7.4-14.7,9.7C76.9,80.3,76.4,80.3,76.5,81z M89,42.6c0.1-2.5-0.4-5.4-1.5-8.1C83,23.1,74.2,16.9,61.7,15.8c-10-0.9-18.6,2.4-25.3,9.7c-8.4,9-9.3,22.4-2.2,32.4c6.8,9.6,19.1,14.2,31.4,11.9C79.2,67.1,89,55.9,89,42.6z M102.1,188.6c0.6,0.1,1.5-0.1,2.4-0.2c9.5-1.4,15.3-10.9,11.6-19.2c-2.6-5.9-9.4-9.6-16.8-8.6c-8.3,1.2-14.1,8.9-12.4,16.6C88.2,183.9,94.4,188.6,102.1,188.6z M167.7,88.5c-1,0-2.1,0.1-3.1,0.3c-9,1.7-14.2,10.6-10.8,18.6c2.9,6.8,11.4,10.3,19,7.8c7.1-2.3,11.1-9.1,9.6-15.9C180.9,93,174.8,88.5,167.7,88.5z'/></svg>";
      content += "<h1><center>" + String(SuplaDevice_setName) + "</center></h1>";
      content += "<h3><center>ZAPISANO</center></h3>";
      content += "<br>";
      content += "<a href='/'><button>POWRÓT</button></a></div>";          
      content += "<br><br>";
      httpServer.send(200, "text/html", content);       
  });

//************************************************************

  httpServer.on("/firmware_up", []() {
      content = "<!DOCTYPE HTML>";
      content += "<meta http-equiv='content-type' content='text/html; charset=UTF-8'>";
      content += "<meta name='viewport' content='width=device-width,initial-scale=1,maximum-scale=1,user-scalable=no'>";
      content += "<style>body{font-size:14px;font-family:HelveticaNeue,'Helvetica Neue',HelveticaNeueRoman,HelveticaNeue-Roman,'Helvetica Neue Roman',TeXGyreHerosRegular,Helvetica,Tahoma,Geneva,Arial,sans-serif;font-weight:400;font-stretch:normal;background:#00d151;color:#fff;line-height:20px;padding:0}.s{width:460px;margin:0 auto;margin-top:calc(50vh - 340px);border:solid 3px #fff;padding:0 10px 10px;border-radius:3px}#l{display:block;max-width:150px;height:155px;margin:-80px auto 20px;background:#00d151;padding-right:5px}#l path{fill:#000}.w{margin:3px 0 16px;padding:5px 0;border-radius:3px;background:#fff;box-shadow:0 1px 3px rgba(0,0,0,.3)}h1,h3{margin:10px 8px;font-family:HelveticaNeueLight,HelveticaNeue-Light,'Helvetica Neue Light',HelveticaNeue,'Helvetica Neue',TeXGyreHerosRegular,Helvetica,Tahoma,Geneva,Arial,sans-serif;font-weight:300;font-stretch:normal;color:#000;font-size:23px}h1{margin-bottom:14px;color:#fff}span{display:block;margin:10px 7px 14px}i{display:block;font-style:normal;position:relative;border-bottom:solid 1px #00d151;height:42px}i:last-child{border:none}label{position:absolute;display:inline-block;top:0;left:8px;color:#00d151;line-height:41px;pointer-events:none}input,select{width:calc(100% - 145px);border:none;font-size:16px;line-height:40px;border-radius:0;letter-spacing:-.5px;background:#fff;color:#000;padding-left:144px;-webkit-appearance:none;-moz-appearance:none;appearance:none;outline:0!important;height:40px}select{padding:0;float:right;margin:1px 3px 1px 2px}button{width:100%;border:0;background:#000;padding:5px 10px;font-size:16px;line-height:40px;color:#fff;border-radius:3px;box-shadow:0 1px 3px rgba(0,0,0,.3);cursor:pointer}.c{background:#ffe836;position:fixed;width:100%;line-height:80px;color:#000;top:0;left:0;box-shadow:0 1px 3px rgba(0,0,0,.3);text-align:center;font-size:26px;z-index:100}@media all and (max-height:920px){.s{margin-top:80px}}@media all and (max-width:900px){.s{width:calc(100% - 20px);margin-top:40px;border:none;padding:0 8px;border-radius:0}#l{max-width:80px;height:auto;margin:10px auto 20px}h1,h3{font-size:19px}i{border:none;height:auto}label{display:block;margin:4px 0 12px;color:#00d151;font-size:13px;position:relative;line-height:18px}input,select{width:calc(100% - 10px);font-size:16px;line-height:28px;padding:0 5px;border-bottom:solid 1px #00d151}select{width:100%;float:none;margin:0}}</style>";
      content += "<div class='s'><svg version='1.1' id='l' x='0' y='0' viewBox='0 0 200 200' xml:space='preserve'><path d='M59.3,2.5c18.1,0.6,31.8,8,40.2,23.5c3.1,5.7,4.3,11.9,4.1,18.3c-0.1,3.6-0.7,7.1-1.9,10.6c-0.2,0.7-0.1,1.1,0.6,1.5c12.8,7.7,25.5,15.4,38.3,23c2.9,1.7,5.8,3.4,8.7,5.3c1,0.6,1.6,0.6,2.5-0.1c4.5-3.6,9.8-5.3,15.7-5.4c12.5-0.1,22.9,7.9,25.2,19c1.9,9.2-2.9,19.2-11.8,23.9c-8.4,4.5-16.9,4.5-25.5,0.2c-0.7-0.3-1-0.2-1.5,0.3c-4.8,4.9-9.7,9.8-14.5,14.6c-5.3,5.3-10.6,10.7-15.9,16c-1.8,1.8-3.6,3.7-5.4,5.4c-0.7,0.6-0.6,1,0,1.6c3.6,3.4,5.8,7.5,6.2,12.2c0.7,7.7-2.2,14-8.8,18.5c-12.3,8.6-30.3,3.5-35-10.4c-2.8-8.4,0.6-17.7,8.6-22.8c0.9-0.6,1.1-1,0.8-2c-2-6.2-4.4-12.4-6.6-18.6c-6.3-17.6-12.7-35.1-19-52.7c-0.2-0.7-0.5-1-1.4-0.9c-12.5,0.7-23.6-2.6-33-10.4c-8-6.6-12.9-15-14.2-25c-1.5-11.5,1.7-21.9,9.6-30.7C32.5,8.9,42.2,4.2,53.7,2.7c0.7-0.1,1.5-0.2,2.2-0.2C57,2.4,58.2,2.5,59.3,2.5z M76.5,81c0,0.1,0.1,0.3,0.1,0.6c1.6,6.3,3.2,12.6,4.7,18.9c4.5,17.7,8.9,35.5,13.3,53.2c0.2,0.9,0.6,1.1,1.6,0.9c5.4-1.2,10.7-0.8,15.7,1.6c0.8,0.4,1.2,0.3,1.7-0.4c11.2-12.9,22.5-25.7,33.4-38.7c0.5-0.6,0.4-1,0-1.6c-5.6-7.9-6.1-16.1-1.3-24.5c0.5-0.8,0.3-1.1-0.5-1.6c-9.1-4.7-18.1-9.3-27.2-14c-6.8-3.5-13.5-7-20.3-10.5c-0.7-0.4-1.1-0.3-1.6,0.4c-1.3,1.8-2.7,3.5-4.3,5.1c-4.2,4.2-9.1,7.4-14.7,9.7C76.9,80.3,76.4,80.3,76.5,81z M89,42.6c0.1-2.5-0.4-5.4-1.5-8.1C83,23.1,74.2,16.9,61.7,15.8c-10-0.9-18.6,2.4-25.3,9.7c-8.4,9-9.3,22.4-2.2,32.4c6.8,9.6,19.1,14.2,31.4,11.9C79.2,67.1,89,55.9,89,42.6z M102.1,188.6c0.6,0.1,1.5-0.1,2.4-0.2c9.5-1.4,15.3-10.9,11.6-19.2c-2.6-5.9-9.4-9.6-16.8-8.6c-8.3,1.2-14.1,8.9-12.4,16.6C88.2,183.9,94.4,188.6,102.1,188.6z M167.7,88.5c-1,0-2.1,0.1-3.1,0.3c-9,1.7-14.2,10.6-10.8,18.6c2.9,6.8,11.4,10.3,19,7.8c7.1-2.3,11.1-9.1,9.6-15.9C180.9,93,174.8,88.5,167.7,88.5z'/></svg>";
      content += "<h1><center>" + String(SuplaDevice_setName) + "</center></h1>";
      content += "<h3><center>Aktualizacja opr.</center></h3>";
      content += "<br>";
      content += "<center>";
      //content += "<div>";
      content += "<iframe src=/firmware>Twoja przeglądarka nie akceptuje ramek! width='200' height='100' frameborder='100'></iframe>";
      //content += "</div>";
      content += "</center>";
      content += "<a href='/'><button>POWRÓT</button></a></div>";          
      content += "<br><br>";
      httpServer.send(200, "text/html", content);       
  });
//************************************************************

  httpServer.on("/reboot", []() {
        content = "<!DOCTYPE HTML>";
        content += "<meta http-equiv='content-type' content='text/html; charset=UTF-8'>";
        content += "<meta name='viewport' content='width=device-width,initial-scale=1,maximum-scale=1,user-scalable=no'>";
        content += "<style>body{font-size:14px;font-family:HelveticaNeue,'Helvetica Neue',HelveticaNeueRoman,HelveticaNeue-Roman,'Helvetica Neue Roman',TeXGyreHerosRegular,Helvetica,Tahoma,Geneva,Arial,sans-serif;font-weight:400;font-stretch:normal;background:#00d151;color:#fff;line-height:20px;padding:0}.s{width:460px;margin:0 auto;margin-top:calc(50vh - 340px);border:solid 3px #fff;padding:0 10px 10px;border-radius:3px}#l{display:block;max-width:150px;height:155px;margin:-80px auto 20px;background:#00d151;padding-right:5px}#l path{fill:#000}.w{margin:3px 0 16px;padding:5px 0;border-radius:3px;background:#fff;box-shadow:0 1px 3px rgba(0,0,0,.3)}h1,h3{margin:10px 8px;font-family:HelveticaNeueLight,HelveticaNeue-Light,'Helvetica Neue Light',HelveticaNeue,'Helvetica Neue',TeXGyreHerosRegular,Helvetica,Tahoma,Geneva,Arial,sans-serif;font-weight:300;font-stretch:normal;color:#000;font-size:23px}h1{margin-bottom:14px;color:#fff}span{display:block;margin:10px 7px 14px}i{display:block;font-style:normal;position:relative;border-bottom:solid 1px #00d151;height:42px}i:last-child{border:none}label{position:absolute;display:inline-block;top:0;left:8px;color:#00d151;line-height:41px;pointer-events:none}input,select{width:calc(100% - 145px);border:none;font-size:16px;line-height:40px;border-radius:0;letter-spacing:-.5px;background:#fff;color:#000;padding-left:144px;-webkit-appearance:none;-moz-appearance:none;appearance:none;outline:0!important;height:40px}select{padding:0;float:right;margin:1px 3px 1px 2px}button{width:100%;border:0;background:#000;padding:5px 10px;font-size:16px;line-height:40px;color:#fff;border-radius:3px;box-shadow:0 1px 3px rgba(0,0,0,.3);cursor:pointer}.c{background:#ffe836;position:fixed;width:100%;line-height:80px;color:#000;top:0;left:0;box-shadow:0 1px 3px rgba(0,0,0,.3);text-align:center;font-size:26px;z-index:100}@media all and (max-height:920px){.s{margin-top:80px}}@media all and (max-width:900px){.s{width:calc(100% - 20px);margin-top:40px;border:none;padding:0 8px;border-radius:0}#l{max-width:80px;height:auto;margin:10px auto 20px}h1,h3{font-size:19px}i{border:none;height:auto}label{display:block;margin:4px 0 12px;color:#00d151;font-size:13px;position:relative;line-height:18px}input,select{width:calc(100% - 10px);font-size:16px;line-height:28px;padding:0 5px;border-bottom:solid 1px #00d151}select{width:100%;float:none;margin:0}}</style>";
        content += "<div class='s'><svg version='1.1' id='l' x='0' y='0' viewBox='0 0 200 200' xml:space='preserve'><path d='M59.3,2.5c18.1,0.6,31.8,8,40.2,23.5c3.1,5.7,4.3,11.9,4.1,18.3c-0.1,3.6-0.7,7.1-1.9,10.6c-0.2,0.7-0.1,1.1,0.6,1.5c12.8,7.7,25.5,15.4,38.3,23c2.9,1.7,5.8,3.4,8.7,5.3c1,0.6,1.6,0.6,2.5-0.1c4.5-3.6,9.8-5.3,15.7-5.4c12.5-0.1,22.9,7.9,25.2,19c1.9,9.2-2.9,19.2-11.8,23.9c-8.4,4.5-16.9,4.5-25.5,0.2c-0.7-0.3-1-0.2-1.5,0.3c-4.8,4.9-9.7,9.8-14.5,14.6c-5.3,5.3-10.6,10.7-15.9,16c-1.8,1.8-3.6,3.7-5.4,5.4c-0.7,0.6-0.6,1,0,1.6c3.6,3.4,5.8,7.5,6.2,12.2c0.7,7.7-2.2,14-8.8,18.5c-12.3,8.6-30.3,3.5-35-10.4c-2.8-8.4,0.6-17.7,8.6-22.8c0.9-0.6,1.1-1,0.8-2c-2-6.2-4.4-12.4-6.6-18.6c-6.3-17.6-12.7-35.1-19-52.7c-0.2-0.7-0.5-1-1.4-0.9c-12.5,0.7-23.6-2.6-33-10.4c-8-6.6-12.9-15-14.2-25c-1.5-11.5,1.7-21.9,9.6-30.7C32.5,8.9,42.2,4.2,53.7,2.7c0.7-0.1,1.5-0.2,2.2-0.2C57,2.4,58.2,2.5,59.3,2.5z M76.5,81c0,0.1,0.1,0.3,0.1,0.6c1.6,6.3,3.2,12.6,4.7,18.9c4.5,17.7,8.9,35.5,13.3,53.2c0.2,0.9,0.6,1.1,1.6,0.9c5.4-1.2,10.7-0.8,15.7,1.6c0.8,0.4,1.2,0.3,1.7-0.4c11.2-12.9,22.5-25.7,33.4-38.7c0.5-0.6,0.4-1,0-1.6c-5.6-7.9-6.1-16.1-1.3-24.5c0.5-0.8,0.3-1.1-0.5-1.6c-9.1-4.7-18.1-9.3-27.2-14c-6.8-3.5-13.5-7-20.3-10.5c-0.7-0.4-1.1-0.3-1.6,0.4c-1.3,1.8-2.7,3.5-4.3,5.1c-4.2,4.2-9.1,7.4-14.7,9.7C76.9,80.3,76.4,80.3,76.5,81z M89,42.6c0.1-2.5-0.4-5.4-1.5-8.1C83,23.1,74.2,16.9,61.7,15.8c-10-0.9-18.6,2.4-25.3,9.7c-8.4,9-9.3,22.4-2.2,32.4c6.8,9.6,19.1,14.2,31.4,11.9C79.2,67.1,89,55.9,89,42.6z M102.1,188.6c0.6,0.1,1.5-0.1,2.4-0.2c9.5-1.4,15.3-10.9,11.6-19.2c-2.6-5.9-9.4-9.6-16.8-8.6c-8.3,1.2-14.1,8.9-12.4,16.6C88.2,183.9,94.4,188.6,102.1,188.6z M167.7,88.5c-1,0-2.1,0.1-3.1,0.3c-9,1.7-14.2,10.6-10.8,18.6c2.9,6.8,11.4,10.3,19,7.8c7.1-2.3,11.1-9.1,9.6-15.9C180.9,93,174.8,88.5,167.7,88.5z'/></svg>";
        content += "<h1><center>" + String(SuplaDevice_setName) + "</center></h1>";
        content += "<h3><center>RESTART ESP</center></h3>";
        content += "<br>";
        content += "<a href='/'><button>POWRÓT</button></a></div>";                    
        content += "<br><br>";
        httpServer.send(200, "text/html", content);
        ESP.reset();
    }    
  );
}



//*********************************************************************************************************

// DS18B20 Sensor read implementation
double get_temperature(int channelNumber, double last_val) {
        double temp_supla;
      
          switch(channelNumber)
          {
            case 0:Serial.println("Wysylam do Supla DS0"); temp_supla = Temp_SUPLA_DS0; break;
            case 1:Serial.println("Wysylam do Supla DS1"); temp_supla = Temp_SUPLA_DS1; break;
            case 2:Serial.println("Wysylam do Supla DS2"); temp_supla = Temp_SUPLA_DS2; break;
            case 3:Serial.println("Wysylam do Supla DS3"); temp_supla = Temp_SUPLA_DS3; break;
            case 4:Serial.println("Wysylam do Supla DS4"); temp_supla = Temp_SUPLA_DS4; break;
            case 5:Serial.println("Wysylam do Supla DS5"); temp_supla = Temp_SUPLA_DS5; break;
            case 6:Serial.println("Wysylam do Supla DS6"); temp_supla = Temp_SUPLA_DS6; break;
            case 7:Serial.println("Wysylam do Supla DS7"); temp_supla = Temp_SUPLA_DS7; Nowy_pomiar = 1; break;    
          }
         
    return temp_supla;
}

//*********************************************************************************************************
void printAddress(DeviceAddress deviceAddress)
{
  for (uint8_t zmienna_unit8_t = 0; zmienna_unit8_t < 8; zmienna_unit8_t++)
  {
    if (deviceAddress[zmienna_unit8_t] < 16) Serial.print("0");
    Serial.print(deviceAddress[zmienna_unit8_t], HEX);
  }
}

//*********************************************************************************************************

void Zamien_litery()
{
  zmienna.replace('a', 'A');
  zmienna.replace('b', 'B');
  zmienna.replace('c', 'C');
  zmienna.replace('d', 'D');
  zmienna.replace('e', 'E');
  zmienna.replace('f', 'F');
}

//*********************************************************************************************************

void Pokaz_zawartosc_eeprom()
{
  EEPROM.begin(4096);
  delay(100);
Serial.println("|--------------------------------------------------------------------------------------------------------------------------------------------|");
Serial.println("|                                             HEX                                                     |                STRING                |");
                

  byte  eeprom = 0;
  String  eeprom_string = "";
  String znak = "";
  
  byte licz_wiersz = 0;  
  for (int zmienna_int2 = 0; zmienna_int2 < 4096; ++zmienna_int2)   {
    //ESP.wdtFeed(); // Reset watchdoga
    eeprom = (EEPROM.read(zmienna_int2));
    znak = char(EEPROM.read(zmienna_int2));
    if (znak == ""){eeprom_string += " ";}else{eeprom_string += znak;}znak = "";
    if (licz_wiersz == 0){Serial.print("|   ");}licz_wiersz++; 
    if (licz_wiersz >=0 && licz_wiersz < 32){;printf("%02X", eeprom);Serial.print(" ");}
    if (licz_wiersz == 32){printf("%02X", eeprom);Serial.print("   |   ");Serial.print(eeprom_string);Serial.println("   |");eeprom_string = "";licz_wiersz = 0;}}
  Serial.println("|--------------------------------------------------------------------------------------------------------------------------------------------|");   
  Serial.println("");  
  Serial.println("");

}

//*********************************************************************************************************

char convertCharToHex(char ch)
{
  char returnType;
  switch(ch)
  {
    case  '0':  returnType = 0;  break;
    case  '1':  returnType = 1;  break;
    case  '2':  returnType = 2;  break;
    case  '3':  returnType = 3;  break;
    case  '4':  returnType = 4;  break;
    case  '5':  returnType = 5;  break;
    case  '6':  returnType = 6;  break;
    case  '7':  returnType = 7;  break;
    case  '8':  returnType = 8;  break;
    case  '9':  returnType = 9;  break;
    case  'A':  returnType = 10; break;
    case  'B':  returnType = 11; break;
    case  'C':  returnType = 12; break;
    case  'D':  returnType = 13;  break;
    case  'E':  returnType = 14;  break;
    case  'F' : returnType = 15;  break;
    default:    returnType = 0;   break;
  }
  return returnType;
}

 //****************************************************************************************************************************************

 int Odczytaj_zakres_eeprom(int x1, int y1){
  EEPROM.begin(4096);
  delay(10);
  zmienna = "";   
  for (int zmienna_int_3 = x1; zmienna_int_3 < y1; ++zmienna_int_3){ 
    zmienna += char(EEPROM.read(zmienna_int_3));
  } 
 }
  //****************************************************************************************************************************************

 int Odczytaj_pojedynczy_eeprom(int x2){
  EEPROM.begin(4096);
  delay(10);
  zmienna = "";   
  zmienna += char(EEPROM.read(x2)); 
 }

  //****************************************************************************************************************************************
 void Odczytaj_ID_urzadzen_jako_string(){

  e_ds18b20_id_1 = ""; for (int zmienna_int4 = 100; zmienna_int4 < 108; ++zmienna_int4) {byte zmi_po = EEPROM.read(zmienna_int4); if (zmi_po < 16){e_ds18b20_id_1 += "0";} e_ds18b20_id_1 += String(zmi_po,HEX); zmienna = e_ds18b20_id_1; Zamien_litery(); e_ds18b20_id_1 = zmienna;}
  e_ds18b20_id_2 = ""; for (int zmienna_int4 = 108; zmienna_int4 < 116; ++zmienna_int4) {byte zmi_po = EEPROM.read(zmienna_int4); if (zmi_po < 16){e_ds18b20_id_2 += "0";} e_ds18b20_id_2 += String(zmi_po,HEX); zmienna = e_ds18b20_id_2; Zamien_litery(); e_ds18b20_id_2 = zmienna;}
  e_ds18b20_id_3 = ""; for (int zmienna_int4 = 116; zmienna_int4 < 124; ++zmienna_int4) {byte zmi_po = EEPROM.read(zmienna_int4); if (zmi_po < 16){e_ds18b20_id_3 += "0";} e_ds18b20_id_3 += String(zmi_po,HEX); zmienna = e_ds18b20_id_3; Zamien_litery(); e_ds18b20_id_3 = zmienna;}
  e_ds18b20_id_4 = ""; for (int zmienna_int4 = 124; zmienna_int4 < 132; ++zmienna_int4) {byte zmi_po = EEPROM.read(zmienna_int4); if (zmi_po < 16){e_ds18b20_id_4 += "0";} e_ds18b20_id_4 += String(zmi_po,HEX); zmienna = e_ds18b20_id_4; Zamien_litery(); e_ds18b20_id_4 = zmienna;}
  e_ds18b20_id_5 = ""; for (int zmienna_int4 = 132; zmienna_int4 < 140; ++zmienna_int4) {byte zmi_po = EEPROM.read(zmienna_int4); if (zmi_po < 16){e_ds18b20_id_5 += "0";} e_ds18b20_id_5 += String(zmi_po,HEX); zmienna = e_ds18b20_id_5; Zamien_litery(); e_ds18b20_id_5 = zmienna;}
  e_ds18b20_id_6 = ""; for (int zmienna_int4 = 140; zmienna_int4 < 148; ++zmienna_int4) {byte zmi_po = EEPROM.read(zmienna_int4); if (zmi_po < 16){e_ds18b20_id_6 += "0";} e_ds18b20_id_6 += String(zmi_po,HEX); zmienna = e_ds18b20_id_6; Zamien_litery(); e_ds18b20_id_6 = zmienna;}
  e_ds18b20_id_7 = ""; for (int zmienna_int4 = 148; zmienna_int4 < 156; ++zmienna_int4) {byte zmi_po = EEPROM.read(zmienna_int4); if (zmi_po < 16){e_ds18b20_id_7 += "0";} e_ds18b20_id_7 += String(zmi_po,HEX); zmienna = e_ds18b20_id_7; Zamien_litery(); e_ds18b20_id_7 = zmienna;}
  e_ds18b20_id_8 = ""; for (int zmienna_int4 = 156; zmienna_int4 < 164; ++zmienna_int4) {byte zmi_po = EEPROM.read(zmienna_int4); if (zmi_po < 16){e_ds18b20_id_8 += "0";} e_ds18b20_id_8 += String(zmi_po,HEX); zmienna = e_ds18b20_id_8; Zamien_litery(); e_ds18b20_id_8 = zmienna;} 
 
 }
  //****************************************************************************************************************************************
  void Odczytaj_parametry_autentykacji()
  {
      Odczytaj_zakres_eeprom(300, 350);
      custom_Supla_server = zmienna;
      Odczytaj_zakres_eeprom(350, 360);
      Location_id_string = zmienna;
      Odczytaj_zakres_eeprom(360, 410);
      custom_Location_Pass = zmienna;
  }
  //****************************************************************************************************************************************
  void Odczytaj_parametry_IP()
  {
  DHCP = 0;

  String eip_address = "";  for (int zmienna_int5 = 4000; zmienna_int5 < 4015; ++zmienna_int5)   {eip_address += char(EEPROM.read(zmienna_int5));}
  zmienna = eip_address.c_str();
  int IP[4] = {0,0,0,0};
  int Part_IP = 0;
  for ( int zmienna_int6=0; zmienna_int6<zmienna.length(); zmienna_int6++ ){char c = zmienna[zmienna_int6];if ( c == '.' ){Part_IP++;continue;}IP[Part_IP] *= 10;IP[Part_IP] += c - '0';}
 
  String eip_mask = "";  for (int zmienna_int7 = 4015; zmienna_int7 < 4030; ++zmienna_int7)   {eip_mask += char(EEPROM.read(zmienna_int7));}
  zmienna = eip_mask.c_str();
  int MASK[4] = {0,0,0,0};
      int Part_MASK = 0;
      for ( int zmienna_int8=0; zmienna_int8<zmienna.length(); zmienna_int8++ ){char c = zmienna[zmienna_int8];if ( c == '.' ){Part_MASK++;continue;}MASK[Part_MASK] *= 10;MASK[Part_MASK] += c - '0';}

  String eip_gateway = "";  for (int i = 4030; i < 4045; ++i)   {eip_gateway += char(EEPROM.read(i));}
  zmienna = eip_gateway .c_str();
  int GW[4] = {0,0,0,0};
      int Part_GATEWAY = 0;
      for ( int zmienna_int9=0; zmienna_int9<zmienna.length(); zmienna_int9++ ){char c = zmienna[zmienna_int9];if ( c == '.' ){Part_GATEWAY++;continue;}GW[Part_GATEWAY] *= 10;GW[Part_GATEWAY] += c - '0';}

  if (IP[0] == 0 && IP[1] < 1 && IP[2] == 0 && IP[3] == 0){Serial.println("Niewlasciwy adres IP");DHCP = 1;}
  if (IP[0] > 248 && IP[1] >= 0 && IP[2] >= 0 && IP[3] >= 0){Serial.println("Niewlasciwy adres IP");DHCP = 1;}

  if (MASK[0] == 0 && MASK[1] == 0 && MASK[2] == 0 && MASK[3] == 0){Serial.println("Niewlasciwy adres MASKI");DHCP = 1;}

  if (GW[0] == 0 && GW[1] < 1 && GW[2] == 0 && GW[3] == 0){Serial.println("Niewlasciwy adres BRAMY");DHCP = 1;}

  if (GW[0] >= 248 && GW[1] >= 0 && GW[2] >= 0 && GW[3] >= 0){Serial.println("Niewlasciwy adres BRAMY");DHCP = 1;}

  if (DHCP == 1){Serial.println("Pobieram adres z DHCP");}
 
  if (DHCP == 0){Serial.println("Uruchamiam statyczne IP");IPAddress staticIP(IP[0],IP[1],IP[2],IP[3]);IPAddress subnet(MASK[0],MASK[1],MASK[2],MASK[3]);IPAddress gateway(GW[0],GW[1],GW[2],GW[3]);WiFi.config(staticIP, gateway, subnet);}    
}
  //****************************************************************************************************************************************
  void Tryb_konfiguracji()
  {
    Serial.println("Tryb konfiguracji");
    WiFi.disconnect();
    delay(1000);
    WiFi.mode(WIFI_AP);
    WiFi.softAP(Config_Wifi_name,Config_Wifi_pass);
    delay(1000);
    Serial.println("Tryb AP");
    createWebServer();  
    httpServer.begin();
    Serial.println("Start Serwera"); 
    Modul_tryb_konfiguracji = 2;
    Utrzymaj_serwer:
    httpServer.handleClient();//Ta pętla nigdy się nie skończy
    if(Modul_tryb_konfiguracji == 2){
      goto Utrzymaj_serwer;
    }
  }


  void Odczytaj_temperature(){
        Serial.println("Odczytuje adresy DS18b20");
        EEPROM.begin(4096);
        delay(10);
        byte zmienna_byte3 = 100;
        for(byte zmienna_byte2 = 0; zmienna_byte2 < 8; zmienna_byte2++){DS18b20_1[zmienna_byte2] =  EEPROM.read(zmienna_byte3);zmienna_byte3++; }
        for(byte zmienna_byte2 = 0; zmienna_byte2 < 8; zmienna_byte2++){DS18b20_2[zmienna_byte2] =  EEPROM.read(zmienna_byte3);zmienna_byte3++; }
        for(byte zmienna_byte2 = 0; zmienna_byte2 < 8; zmienna_byte2++){DS18b20_3[zmienna_byte2] =  EEPROM.read(zmienna_byte3);zmienna_byte3++; }
        for(byte zmienna_byte2 = 0; zmienna_byte2 < 8; zmienna_byte2++){DS18b20_4[zmienna_byte2] =  EEPROM.read(zmienna_byte3);zmienna_byte3++; }
        for(byte zmienna_byte2 = 0; zmienna_byte2 < 8; zmienna_byte2++){DS18b20_5[zmienna_byte2] =  EEPROM.read(zmienna_byte3);zmienna_byte3++; }
        for(byte zmienna_byte2 = 0; zmienna_byte2 < 8; zmienna_byte2++){DS18b20_6[zmienna_byte2] =  EEPROM.read(zmienna_byte3);zmienna_byte3++; }
        for(byte zmienna_byte2 = 0; zmienna_byte2 < 8; zmienna_byte2++){DS18b20_7[zmienna_byte2] =  EEPROM.read(zmienna_byte3);zmienna_byte3++; }
        for(byte zmienna_byte2 = 0; zmienna_byte2 < 8; zmienna_byte2++){DS18b20_8[zmienna_byte2] =  EEPROM.read(zmienna_byte3);zmienna_byte3++; }

        sensors.begin();
        delay(100);
        
        DeviceAddress DS0 = { DS18b20_1[0], DS18b20_1[1], DS18b20_1[2], DS18b20_1[3], DS18b20_1[4], DS18b20_1[5], DS18b20_1[6], DS18b20_1[7] };
        DeviceAddress DS1 = { DS18b20_2[0], DS18b20_2[1], DS18b20_2[2], DS18b20_2[3], DS18b20_2[4], DS18b20_2[5], DS18b20_2[6], DS18b20_2[7] };
        DeviceAddress DS2 = { DS18b20_3[0], DS18b20_3[1], DS18b20_3[2], DS18b20_3[3], DS18b20_3[4], DS18b20_3[5], DS18b20_3[6], DS18b20_3[7] };
        DeviceAddress DS3 = { DS18b20_4[0], DS18b20_4[1], DS18b20_4[2], DS18b20_4[3], DS18b20_4[4], DS18b20_4[5], DS18b20_4[6], DS18b20_4[7] };
        DeviceAddress DS4 = { DS18b20_5[0], DS18b20_5[1], DS18b20_5[2], DS18b20_5[3], DS18b20_5[4], DS18b20_5[5], DS18b20_5[6], DS18b20_5[7] };
        DeviceAddress DS5 = { DS18b20_6[0], DS18b20_6[1], DS18b20_6[2], DS18b20_6[3], DS18b20_6[4], DS18b20_6[5], DS18b20_6[6], DS18b20_6[7] };
        DeviceAddress DS6 = { DS18b20_7[0], DS18b20_7[1], DS18b20_7[2], DS18b20_7[3], DS18b20_7[4], DS18b20_7[5], DS18b20_7[6], DS18b20_7[7] };
        DeviceAddress DS7 = { DS18b20_8[0], DS18b20_8[1], DS18b20_8[2], DS18b20_8[3], DS18b20_8[4], DS18b20_8[5], DS18b20_8[6], DS18b20_8[7] };

        sensors.setResolution(DS0, TEMPERATURE_PRECISION);
        sensors.setResolution(DS1, TEMPERATURE_PRECISION);
        sensors.setResolution(DS2, TEMPERATURE_PRECISION);
        sensors.setResolution(DS3, TEMPERATURE_PRECISION);
        sensors.setResolution(DS4, TEMPERATURE_PRECISION);
        sensors.setResolution(DS5, TEMPERATURE_PRECISION);
        sensors.setResolution(DS6, TEMPERATURE_PRECISION);
        sensors.setResolution(DS7, TEMPERATURE_PRECISION);     

        Odczytaj_ID_urzadzen_jako_string();
        
        
        Serial.println("Wysylam zapytanie do DS18b20");
        sensors.requestTemperatures();
        delay(100);
        ++nowe_dane_dla_thingspeak;
        for (byte zmienna_byte5 = 0; zmienna_byte5 < 8; ++zmienna_byte5) { 
        
          if( zmienna_byte5 == 0){SuplaDevice.iterate();DS_mean_co = 0;CASE_0: if(e_ds18b20_id_1 != "FFFFFFFFFFFFFFFF"){Temp_SUPLA_DS0 = sensors.getTempC(DS0);if (Temp_SUPLA_DS0== -127){++DS_mean_co;delay(10);httpServer.handleClient();if (DS_mean_co == DS_mean_co_max){Serial.print("Blad ");++DS18b20_1_err; goto Koniec_CASE_0;}sensors.requestTemperatures();delay(100);goto CASE_0;}Koniec_CASE_0:Temp_THINGSPEAK_DS0 = Temp_SUPLA_DS0; Serial.print("DS0: "); printAddress(DS0); Serial.print(" T: "); Serial.println(Temp_SUPLA_DS0);}else{Temp_SUPLA_DS0 = 255;Temp_THINGSPEAK_DS0 = Temp_SUPLA_DS0;}}
          if( zmienna_byte5 == 1){SuplaDevice.iterate();DS_mean_co = 0;CASE_1: if(e_ds18b20_id_2 != "FFFFFFFFFFFFFFFF"){Temp_SUPLA_DS1 = sensors.getTempC(DS1);if (Temp_SUPLA_DS1== -127){++DS_mean_co;delay(10);httpServer.handleClient();if (DS_mean_co == DS_mean_co_max){Serial.print("Blad ");++DS18b20_2_err; goto Koniec_CASE_1;}sensors.requestTemperatures();delay(100);goto CASE_1;}Koniec_CASE_1:Temp_THINGSPEAK_DS1 = Temp_SUPLA_DS1; Serial.print("DS1: "); printAddress(DS1); Serial.print(" T: "); Serial.println(Temp_SUPLA_DS1);}else{Temp_SUPLA_DS1 = 255;Temp_THINGSPEAK_DS1 = Temp_SUPLA_DS1;}}
          if( zmienna_byte5 == 2){SuplaDevice.iterate();DS_mean_co = 0;CASE_2: if(e_ds18b20_id_3 != "FFFFFFFFFFFFFFFF"){Temp_SUPLA_DS2 = sensors.getTempC(DS2);if (Temp_SUPLA_DS2== -127){++DS_mean_co;delay(10);httpServer.handleClient();if (DS_mean_co == DS_mean_co_max){Serial.print("Blad ");++DS18b20_3_err; goto Koniec_CASE_2;}sensors.requestTemperatures();delay(100);goto CASE_2;}Koniec_CASE_2:Temp_THINGSPEAK_DS2 = Temp_SUPLA_DS2; Serial.print("DS2: "); printAddress(DS2); Serial.print(" T: "); Serial.println(Temp_SUPLA_DS2);}else{Temp_SUPLA_DS2 = 255;Temp_THINGSPEAK_DS2 = Temp_SUPLA_DS2;}}
          if( zmienna_byte5 == 3){SuplaDevice.iterate();DS_mean_co = 0;CASE_3: if(e_ds18b20_id_4 != "FFFFFFFFFFFFFFFF"){Temp_SUPLA_DS3 = sensors.getTempC(DS3);if (Temp_SUPLA_DS3== -127){++DS_mean_co;delay(10);httpServer.handleClient();if (DS_mean_co == DS_mean_co_max){Serial.print("Blad ");++DS18b20_4_err; goto Koniec_CASE_3;}sensors.requestTemperatures();delay(100);goto CASE_3;}Koniec_CASE_3:Temp_THINGSPEAK_DS3 = Temp_SUPLA_DS3; Serial.print("DS3: "); printAddress(DS3); Serial.print(" T: "); Serial.println(Temp_SUPLA_DS3);}else{Temp_SUPLA_DS3 = 255;Temp_THINGSPEAK_DS3 = Temp_SUPLA_DS3;}}
          if( zmienna_byte5 == 4){SuplaDevice.iterate();DS_mean_co = 0;CASE_4: if(e_ds18b20_id_5 != "FFFFFFFFFFFFFFFF"){Temp_SUPLA_DS4 = sensors.getTempC(DS4);if (Temp_SUPLA_DS4== -127){++DS_mean_co;delay(10);httpServer.handleClient();if (DS_mean_co == DS_mean_co_max){Serial.print("Blad ");++DS18b20_5_err; goto Koniec_CASE_4;}sensors.requestTemperatures();delay(100);goto CASE_4;}Koniec_CASE_4:Temp_THINGSPEAK_DS4 = Temp_SUPLA_DS4; Serial.print("DS4: "); printAddress(DS4); Serial.print(" T: "); Serial.println(Temp_SUPLA_DS4);}else{Temp_SUPLA_DS4 = 255;Temp_THINGSPEAK_DS4 = Temp_SUPLA_DS4;}}
          if( zmienna_byte5 == 5){SuplaDevice.iterate();DS_mean_co = 0;CASE_5: if(e_ds18b20_id_6 != "FFFFFFFFFFFFFFFF"){Temp_SUPLA_DS5 = sensors.getTempC(DS5);if (Temp_SUPLA_DS5== -127){++DS_mean_co;delay(10);httpServer.handleClient();if (DS_mean_co == DS_mean_co_max){Serial.print("Blad ");++DS18b20_6_err; goto Koniec_CASE_5;}sensors.requestTemperatures();delay(100);goto CASE_5;}Koniec_CASE_5:Temp_THINGSPEAK_DS5 = Temp_SUPLA_DS5; Serial.print("DS5: "); printAddress(DS5); Serial.print(" T: "); Serial.println(Temp_SUPLA_DS5);}else{Temp_SUPLA_DS5 = 255;Temp_THINGSPEAK_DS5 = Temp_SUPLA_DS5;}}
          if( zmienna_byte5 == 6){SuplaDevice.iterate();DS_mean_co = 0;CASE_6: if(e_ds18b20_id_7 != "FFFFFFFFFFFFFFFF"){Temp_SUPLA_DS6 = sensors.getTempC(DS6);if (Temp_SUPLA_DS6== -127){++DS_mean_co;delay(10);httpServer.handleClient();if (DS_mean_co == DS_mean_co_max){Serial.print("Blad ");++DS18b20_7_err; goto Koniec_CASE_6;}sensors.requestTemperatures();delay(100);goto CASE_6;}Koniec_CASE_6:Temp_THINGSPEAK_DS6 = Temp_SUPLA_DS6; Serial.print("DS6: "); printAddress(DS6); Serial.print(" T: "); Serial.println(Temp_SUPLA_DS6);}else{Temp_SUPLA_DS6 = 255;Temp_THINGSPEAK_DS6 = Temp_SUPLA_DS6;}}
          if( zmienna_byte5 == 7){SuplaDevice.iterate();DS_mean_co = 0;CASE_7: if(e_ds18b20_id_8 != "FFFFFFFFFFFFFFFF"){Temp_SUPLA_DS7 = sensors.getTempC(DS7);if (Temp_SUPLA_DS7== -127){++DS_mean_co;delay(10);httpServer.handleClient();if (DS_mean_co == DS_mean_co_max){Serial.print("Blad ");++DS18b20_8_err; goto Koniec_CASE_7;}sensors.requestTemperatures();delay(100);goto CASE_7;}Koniec_CASE_7:Temp_THINGSPEAK_DS7 = Temp_SUPLA_DS7; Serial.print("DS7: "); printAddress(DS7); Serial.print(" T: "); Serial.println(Temp_SUPLA_DS7);}else{Temp_SUPLA_DS7 = 255;Temp_THINGSPEAK_DS7 = Temp_SUPLA_DS7;}}
        }

        Serial.println("Koniec pomiaru DS18b20");
        
        if(DS18b20_1_err >= 250){DS18b20_1_err = 250;}
        if(DS18b20_2_err >= 250){DS18b20_2_err = 250;}
        if(DS18b20_3_err >= 250){DS18b20_3_err = 250;}
        if(DS18b20_4_err >= 250){DS18b20_4_err = 250;}
        if(DS18b20_5_err >= 250){DS18b20_5_err = 250;}
        if(DS18b20_6_err >= 250){DS18b20_6_err = 250;}
        if(DS18b20_7_err >= 250){DS18b20_7_err = 250;}
        if(DS18b20_8_err >= 250){DS18b20_8_err = 250;}   
        
  }


  void WiFi_up(){
        WiFi.setOutputPower(20.5);
        WiFi.disconnect();
        delay(1000);
        Odczytaj_zakres_eeprom(0, 32); esid = zmienna;
        Odczytaj_zakres_eeprom(32, 96); epass = zmienna;
        Serial.println("WiFi init");
        Serial.print("SSID: ");
        Serial.print(esid.c_str());
        Serial.print("PASSWORD: ");
        Serial.print(epass.c_str());
        WiFi.mode(WIFI_STA);
        WiFi.begin(esid.c_str(), epass.c_str());

        while (WiFi.status() != WL_CONNECTED) {
            delay(500);
            Serial.print(".");
        }

        Serial.print("\nlocalIP: ");
        Serial.println(WiFi.localIP());
        Serial.print("subnetMask: ");
        Serial.println(WiFi.subnetMask());
        Serial.print("gatewayIP: ");
        Serial.println(WiFi.gatewayIP());  
  }

void status_func(int status, const char *msg) { 
    aktualny_status=status;                                  
}



      
