#include <Arduino.h>
#include <ESPmDNS.h> // #include <ArduinoOTA.h>
#include <driver/ledc.h>
#include <Ticker.h>
#include <LittleFS.h>
#include <ESPAsyncWebSrv.h>
#define NEOPIXEL 8
#define SERVO_OUT 4
#define SERVO_IN 3
#define SERVO_CH 0
#define NAME "koken-key"

AsyncWebServer svr(80);
String a;
Ticker timer;
const String key=String(PRE_SHARED_KEY);
uint8_t bufl=key.length()+4;
char* buf=new char[bufl];
uint32_t crct[256];
bool isp=false;
uint8_t c;

#define servo_init(_CH) ledcSetup(_CH,320,LEDC_TIMER_14_BIT)
#define servo_start(_CH,_PIN) ledcAttachPin(_PIN,_CH)
#define servo(_CH,_X) ledcWrite(_CH,((_X)*2000+500)*5.24288) // tick/us=(hz*(2^bit))/1000000
#define servo_end(_PIN) ledcDetachPin(_PIN)

// https://www.rfc-editor.org/rfc/rfc1952
void make_crct(void){for(uint32_t c,n=0;n<256;n++){c=n;for(uint8_t k=0;k<8;k++)c=((c&1)?0xedb88320L:0)^(c>>1);crct[n]=c;}}
uint32_t crc32(char *buf,uint32_t l,uint32_t crc=0){uint32_t c=~crc;for(uint32_t n=0;n<l;n++)c=crct[(c^buf[n])&0xff]^(c>>8);return ~c;}

float clamp(float x,float a,float b){return fmin(fmax(x,a),b);}
float saturate(float x){return clamp(x,0.,1.);}
float mix(float x,float y,float a){return x*(1.-a)+y*a;}
float step(float a,float x){return x<a?0.:1.;}
float linearstep(float a,float b,float x){return saturate((x-a)/(b-a));}
void rot(float x){
	isp=true;
	neopixelWrite(NEOPIXEL,0,16,16);
	servo_start(SERVO_CH,SERVO_OUT);
	servo(SERVO_CH,x);delay(400);
	servo(SERVO_CH,.5);delay(400);
	servo_end(SERVO_OUT);
	neopixelWrite(NEOPIXEL,0,0,0);
	isp=false;
}
void expires(){a=String();}

void setup(){
	// Serial.begin(9600);
	make_crct();LittleFS.begin();servo_init(SERVO_CH);
	pinMode(SERVO_IN,INPUT);
	pinMode(SERVO_OUT,OUTPUT);
	neopixelWrite(NEOPIXEL,0,0,16);
	delay(1000);
	WiFi.begin();
	neopixelWrite(NEOPIXEL,0,16,16);
	while(WiFi.status()!=WL_CONNECTED);
	neopixelWrite(NEOPIXEL,0,16,0);
	MDNS.begin(NAME); // ArduinoOTA.setHostname(NAME).setPassword(PRE_SHARED_KEY).begin();
	svr.onNotFound([](AsyncWebServerRequest *req){req->redirect("/");});
	svr.serveStatic("/",LittleFS,"/").setDefaultFile("index.html");
	svr.on("/",HTTP_POST,[](AsyncWebServerRequest *req){
		if(a.length()&&req->hasHeader("key")&&req->getHeader("key")->value()==a){
			if(req->hasHeader("act")){
				req->send(200);
				String x=req->getHeader("act")->value();
				if(x=="open")rot(1);else if(x=="close")rot(0);
			}else req->send(202);
		}else req->send(403);
		expires();
	});
	svr.on("/hash",HTTP_GET,[](AsyncWebServerRequest *req){
		uint32_t x=esp_random();
		char str[8];
		sprintf(buf,"%s",key.c_str());for(uint8_t i=0;i<4;i++)buf[bufl-4+i]=(x>>(i<<3))&0xff;
		sprintf(str,"%08x",crc32(buf,bufl));
		a=String(str);

		sprintf(str,"%08x",x);
		String sx=String(str);
		req->send(200,"text/plane",sx);
		timer.detach();
		timer.once(5,expires);
	});
	svr.begin();
	servo_start(SERVO_CH,SERVO_OUT);
}
void loop(){
	// ArduinoOTA.handle();
	if(!isp){
		uint16_t x=analogRead(SERVO_IN);
		x=(200<x)+(500<x);
		if(c!=x&&x!=1){
			servo_start(SERVO_CH,SERVO_OUT);
			servo(SERVO_CH,x?1:0);delay(400);
			servo(SERVO_CH,.5);delay(400);
			servo_end(SERVO_OUT);
			c=x;
		}
	}
	// Serial.println(analogRead(SERVO_IN));
	delay(100);
}
