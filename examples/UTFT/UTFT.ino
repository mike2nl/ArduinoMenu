/********************
Sept. 2014 ~ Oct 2016 Rui Azevedo - ruihfazevedo(@rrob@)gmail.com
creative commons license 4.0: Attribution-ShareAlike CC BY-SA
This software is furnished "as is", without technical support, and with no
warranty, express or implied, as to its usefulness for any purpose.

Thread Safe: No
Extensible: Yes

menu with UTFT
output: 3.2″ TFT LCD Module Display 240X320
input: Serial + Touch Panel
www.r-site.net
***/

#include <Arduino.h>
#include <UTFT.h>
#include <UTouch.h>
#include <menu.h>
#include <dev/utftOut.h>
#include <dev/serialOut.h>

using namespace Menu;

UTFT tft(CTE28,25,26,27,28);
extern uint8_t SmallFont[];
//extern uint8_t BigFont[];
//extern uint8_t SevenSegNumFont[];

UTouch  myTouch( 6, 5, 4, 3, 2);
//menuUTouch menuTouch(myTouch,gfx);

#define LEDPIN 13

result showEvent(eventMask e,navNode& nav,prompt& item) {
  Serial<<e<<" on "<<item<<endl;
  return proceed;
}

int test=55;

result action1(eventMask e) {
  Serial<<e<<" action1 executed, proceed menu"<<endl;Serial.flush();
  return proceed;
}

result action2(eventMask e, navNode& nav, prompt &item, Stream &in, menuOut &out) {
  Serial<<item<<" "<<e<<" action2 executed, quiting menu"<<endl;
  return quit;
}

int ledCtrl=LOW;

result ledOn() {
  ledCtrl=HIGH;
  return proceed;
}
result ledOff() {
  ledCtrl=LOW;
  return proceed;
}

TOGGLE(ledCtrl,setLed,"Led: ",doNothing,noEvent,noStyle//,doExit,enterEvent,noStyle
  ,VALUE("On",HIGH,doNothing,noEvent)
  ,VALUE("Off",LOW,doNothing,noEvent)
);

int selTest=0;
SELECT(selTest,selMenu,"Select",doNothing,noEvent,noStyle
  ,VALUE("Zero",0,doNothing,noEvent)
  ,VALUE("One",1,doNothing,noEvent)
  ,VALUE("Two",2,doNothing,noEvent)
);

int chooseTest=-1;
CHOOSE(chooseTest,chooseMenu,"Choose",doNothing,noEvent,noStyle
  ,VALUE("First",1,doNothing,noEvent)
  ,VALUE("Second",2,doNothing,noEvent)
  ,VALUE("Third",3,doNothing,noEvent)
  ,VALUE("Last",-1,doNothing,noEvent)
);

//customizing a prompt look!
//by extending the prompt class
class altPrompt:public prompt {
public:
  altPrompt(const promptShadow& p):prompt(p) {}
  idx_t printTo(navRoot &root,bool sel,menuOut& out,idx_t len) override {
    return out.printRaw("special prompt!",len);;
  }
};

MENU(subMenu,"Sub-Menu",showEvent,anyEvent,noStyle
  ,OP("Sub1",showEvent,anyEvent)
  ,OP("Sub2",showEvent,anyEvent)
  ,OP("Sub3",showEvent,anyEvent)
  ,altOP(altPrompt,"",showEvent,anyEvent)
  ,EXIT("<Back")
);

result alert(menuOut& o,idleEvent e) {
  if (e==idling) {
    o.setColor(fgColor);
    o<<"alert test"<<endl<<"press [select]"<<endl<<"to continue..."<<endl;
  }
  return proceed;
}

result doAlert(eventMask e, navNode& nav, prompt &item, Stream &in, menuOut &out) {
  nav.root->idleOn(alert);
  return proceed;
}

MENU(mainMenu,"Main menu",doNothing,noEvent,wrapStyle
  ,OP("Op1",action1,anyEvent)
  ,OP("Op2",action2,enterEvent)
  ,FIELD(test,"Test","%",0,100,10,1,doNothing,noEvent,wrapStyle)
  ,SUBMENU(subMenu)
  ,SUBMENU(setLed)
  ,OP("LED On",ledOn,enterEvent)
  ,OP("LED Off",ledOff,enterEvent)
  ,SUBMENU(selMenu)
  ,SUBMENU(chooseMenu)
  ,OP("Alert test",doAlert,enterEvent)
  ,EXIT("<Back")
);

// define menu colors --------------------------------------------------------
//  {{disabled normal,disabled selected},{enabled normal,enabled selected, enabled editing}}
//monochromatic color table

const colorDef<uint16_t> colors[] MEMMODE={
  {{VGA_BLACK,VGA_BLACK},{VGA_BLACK,VGA_BLUE,VGA_BLUE}},//bgColor
  {{VGA_GRAY,VGA_GRAY},{VGA_WHITE,VGA_WHITE,VGA_WHITE}},//fgColor
  {{VGA_WHITE,VGA_BLACK},{VGA_YELLOW,VGA_YELLOW,VGA_RED}},//valColor
  {{VGA_WHITE,VGA_BLACK},{VGA_WHITE,VGA_YELLOW,VGA_YELLOW}},//unitColor
  {{VGA_WHITE,VGA_GRAY},{VGA_BLACK,VGA_BLUE,VGA_WHITE}},//cursorColor
  {{VGA_WHITE,VGA_YELLOW},{VGA_BLUE,VGA_RED,VGA_RED}},//titleColor
};

//PANELS(serial_panels,{0,0,40,10});//or use default
serialOut outSerial(Serial);//,serial_panels);//the output device (just the serial port)

PANELS(gfx_panels,{0,0,14,8},{14,0,13,8});
menuUTFT outGfx(tft,colors,gfx_panels,8,12);//output device, latter set resolution from font measure

MENU_OUTPUTS(out,&outGfx,&outSerial);

#define MAX_DEPTH 2
NAVROOT(nav,mainMenu,MAX_DEPTH,Serial,out);

//when menu is suspended
result idle(menuOut& o,idleEvent e) {
  if (e==idling) {
    o.setColor(fgColor);
    o<<"suspended..."<<endl<<"press [select]"<<endl<<"to continue"<<endl<<(millis()%1000);
  }
  return proceed;
}

void setup() {
  pinMode(LEDPIN,OUTPUT);
  Serial.begin(115200);
  while(!Serial);
  Serial<<"menu 3.0 test"<<endl;Serial.flush();
  nav.idleTask=idle;//point a function to be used when menu is suspended
  mainMenu[1].enabled=disabledStatus;

  tft.InitLCD();
  tft.setBrightness(4);
  tft.clrScr();

  myTouch.InitTouch();
  myTouch.setPrecision(PREC_MEDIUM);

  tft.setFont(SmallFont);
  tft.setColor(0, 255, 0);
  tft.setBackColor(0, 0, 0);

  outGfx.resX=tft.getFontXsize();
  outGfx.resY=tft.getFontYsize();
  outGfx<<"Menu 3.x test on UTFT"<<endl;
  delay(1000);
  tft.fillScr(VGA_GREEN);
  outGfx.clear(1);
  delay(5000);
}

void loop() {
  nav.poll();//this device only draws when needed
  digitalWrite(LEDPIN, ledCtrl);
  delay(300);//simulate a delay when other tasks are done
}
