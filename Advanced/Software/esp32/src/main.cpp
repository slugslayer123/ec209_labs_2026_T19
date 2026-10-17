#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* CHANGE TO 0 after the standard board is physically connected and tested. */
#define USE_DEMO_DATA 1
#define ESP32_UART_RX_GPIO 15  // Standard board schematic ESP_RX -> GPIO15; physically verify!
#define UART_BAUD 9600

static const char *AP_SSID = "EE209-Monitor";
static const char *AP_PASSWORD = "ee209demo123"; /* >=8 chars; change for deployment */
WebServer server(80);
HardwareSerial meterSerial(2);

struct Reading {
  uint32_t voltage_mV=0, current_mA=0, energy_mWh=0;
  int32_t power_mW=0;
  uint16_t seq=0, v_bias=0, i_bias=0, offset_boot=0;
  uint8_t flags=0;
  unsigned long received_ms=0;
  bool received=false;
};
Reading reading;
uint32_t accepted_frames=0, invalid_frames=0, overlong_frames=0;
char rx_line[128];
size_t rx_size=0;
bool rx_discard=false;

static bool parse_number(char *p, long &out, bool unsig=false) {
  if (!p || !*p) return false;
  char *end=nullptr;
  if (unsig) { unsigned long n=strtoul(p,&end,10); out=(long)n; }
  else out=strtol(p,&end,10);
  return end!=p && *end=='\0';
}

static bool accept_frame(char *frame) {
  if(strncmp(frame,"SEM1,",5)!=0) return false;
  char *star=strchr(frame,'*');
  if(!star || strlen(star)!=3) return false;
  unsigned checksum=0;
  for(char *p=frame;p<star;++p) checksum^=(uint8_t)*p;
  char hex[3]={star[1],star[2],0};
  char *end=nullptr;
  unsigned long expected=strtoul(hex,&end,16);
  if(*end!='\0' || expected>255 || checksum!=expected) return false;
  *star='\0';
  char *save=nullptr;
  char *tok=strtok_r(frame,",",&save);
  if(!tok || strcmp(tok,"SEM1")!=0) return false;
  long numbers[9];
  for(int i=0;i<9;i++) {
    tok=strtok_r(nullptr,",",&save);
    if(!parse_number(tok,numbers[i],i!=3)) return false;
  }
  if(strtok_r(nullptr,",",&save)!=nullptr) return false;
  if(numbers[0]<0 || numbers[0]>65535 || numbers[5]<0 || numbers[5]>1023 ||
     numbers[6]<0 || numbers[6]>1023 || numbers[7]<0 || numbers[7]>65535 ||
     numbers[8]<0 || numbers[8]>255) return false;
  reading.seq=(uint16_t)numbers[0];
  reading.voltage_mV=(uint32_t)(unsigned long)numbers[1];
  reading.current_mA=(uint32_t)(unsigned long)numbers[2];
  reading.power_mW=(int32_t)numbers[3];
  reading.energy_mWh=(uint32_t)(unsigned long)numbers[4];
  reading.v_bias=(uint16_t)numbers[5];
  reading.i_bias=(uint16_t)numbers[6];
  reading.offset_boot=(uint16_t)numbers[7];
  reading.flags=(uint8_t)numbers[8];
  reading.received_ms=millis();
  reading.received=true;
  return true;
}

static void handle_serial() {
  while(meterSerial.available()) {
    const char c=(char)meterSerial.read();
    if(c=='\r') continue;
    if(c=='\n') {
      if(!rx_discard && rx_size) {
        rx_line[rx_size]='\0';
        if(accept_frame(rx_line)) accepted_frames++;
        else invalid_frames++;
      }
      rx_size=0;
      rx_discard=false;
    } else if(!rx_discard) {
      if(rx_size<sizeof(rx_line)-1) rx_line[rx_size++]=c;
      else { rx_discard=true; overlong_frames++; rx_size=0; }
    }
  }
}

static void make_demo() {
  static unsigned long last=0;
  static float demo_energy_mWh=0;
  static uint16_t demo_seq=0;
  if((uint32_t)(millis()-last)<250) return;
  last=millis();
  const float t=(float)millis()/1000.0f;
  const float vrms=14.0f+0.4f*sinf(t*0.3f);
  const float peak=520.0f+90.0f*sinf(t*0.12f);
  const float pf=0.86f;
  const uint32_t mv=(uint32_t)(vrms*1000.0f);
  const uint32_t ma=(uint32_t)peak;
  const int32_t mw=(int32_t)(vrms*(peak/1000.0f)/1.41421356f*pf*1000.0f);
  demo_energy_mWh+=mw*0.25f/3600.0f;
  const uint32_t mwh=(uint32_t)(demo_energy_mWh+0.5f);
  /* EXERCISE THE SAME CSV/checksum parser used by hardware UART. */
  char frame[128];
  int n=snprintf(frame,sizeof frame,"SEM1,%u,%lu,%lu,%ld,%lu,430,430,430,2",
      (unsigned)demo_seq++, (unsigned long)mv, (unsigned long)ma,
      (long)mw, (unsigned long)mwh);
  if(n<=0 || n>=(int)sizeof frame-4) return;
  uint8_t crc=0;
  for(int k=0;k<n;k++) crc^=(uint8_t)frame[k];
  snprintf(frame+n,sizeof frame-n,"*%02X",crc);
  if(accept_frame(frame)) accepted_frames++;
  else invalid_frames++;
}

static void handle_measurements() {
  const bool online=reading.received && ((uint32_t)(millis()-reading.received_ms)<2000u);
  const uint32_t age=reading.received ? (uint32_t)(millis()-reading.received_ms) : 0u;
  String s;
  s.reserve(460);
  s += "{\"schema\":\"ee209.v1\",\"source\":\"";
  s += (USE_DEMO_DATA ? "demo" : "atmega328p");
  s += "\",\"demo\":"; s += (USE_DEMO_DATA ? "true" : "false");
  s += ",\"online\":"; s+=(online?"true":"false");
  s += ",\"calibrated\":"; s+=((reading.flags&1)?"true":"false");
  s += ",\"measurementComplete\":"; s+=((reading.flags&2)?"true":"false");
  s += ",\"sampleLoss\":"; s+=((reading.flags&4)?"true":"false");
  s += ",\"adcClipping\":"; s+=((reading.flags&8)?"true":"false");
  s += ",\"sequence\":"; s+=String(reading.seq);
  s += ",\"ageMs\":"; s+=String(age);
  s += ",\"voltageVrms\":"; s+=String(reading.voltage_mV/1000.0f,3);
  s += ",\"peakCurrentMa\":"; s+=String(reading.current_mA);
  s += ",\"realPowerW\":"; s+=String(reading.power_mW/1000.0f,3);
  s += ",\"energyWh\":"; s+=String(reading.energy_mWh/1000.0f,3);
  s += ",\"adc0Bias\":"; s+=String(reading.v_bias);
  s += ",\"adc1Bias\":"; s+=String(reading.i_bias);
  s += ",\"adc2Boot\":"; s+=String(reading.offset_boot);
  s += ",\"validFrames\":"; s+=String(accepted_frames);
  s += ",\"invalidFrames\":"; s+=String(invalid_frames);
  s += ",\"overlongFrames\":"; s+=String(overlong_frames);
  s += "}";
  server.sendHeader("Access-Control-Allow-Origin","*");
  server.sendHeader("Cache-Control","no-store");
  server.send(200,"application/json",s);
}

static void handle_home() {
  server.send(200,"text/html",
    "<!doctype html><meta name='viewport' content='width=device-width,initial-scale=1'>"
    "<title>EE209 diagnostics</title><style>body{font:18px system-ui;background:#101727;color:white;max-width:650px;margin:4rem auto;padding:1rem}pre{white-space:pre-wrap}</style>"
    "<h2>EE209 ESP32 Diagnostics</h2><p>Flutter app: connect to this ESP32 Wi-Fi network, then open the app.</p>"
    "<p>Live API: <a style='color:#9dc' href='/api/v1/state'>/api/v1/state</a></p><pre id='data'>Connecting…</pre>"
    "<script>setInterval(async()=>{try{document.getElementById('data').textContent=JSON.stringify(await(await fetch('/api/v1/state')).json(),null,2)}catch(e){document.getElementById('data').textContent=e}},500)</script>");
}
void setup() {
  Serial.begin(115200);
  meterSerial.begin(UART_BAUD,SERIAL_8N1,ESP32_UART_RX_GPIO,-1);
  WiFi.mode(WIFI_AP);
  if(!WiFi.softAP(AP_SSID,AP_PASSWORD)) Serial.println("ERROR: softAP failed");
  server.on("/",HTTP_GET,handle_home);
  server.on("/api/v1/state",HTTP_GET,handle_measurements);
  server.on("/health",HTTP_GET,[]{server.send(200,"text/plain","EE209 ESP32 OK");});
  server.begin();
  Serial.println("EE209 ESP32 starting");
  Serial.printf("SSID: %s  Password: %s  IP: %s\n",AP_SSID,AP_PASSWORD,WiFi.softAPIP().toString().c_str());
  Serial.printf("UART on GPIO%d, %d 8N1. Demo mode = %s\n",ESP32_UART_RX_GPIO,UART_BAUD,USE_DEMO_DATA?"ON":"OFF");
}
void loop() {
#if USE_DEMO_DATA
  make_demo();
#else
  handle_serial();
#endif
  server.handleClient();
  delay(1);
}
