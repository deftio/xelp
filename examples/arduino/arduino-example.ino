/****
 * Arduino XELP example
 *
 ****/


#include <Adafruit_NeoPixel.h>

#define NUMPIXELS        1


XELP cli;

XELPRESULT cmdHelp (const char* args_str, int maxlen) {
    return XELPHelp(&cli);
}

XELPRESULT cmdBanner (const char* args_str, int maxlen) {
    Serial.println(XELP_BANNER_STR);
    return XELP_S_OK;
}

int gLEDColor;
Adafruit_NeoPixel pixels(NUMPIXELS, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);

void writeChar (char c) {
  Serial.write(c);
}

//some sample functions 


XELPRESULT cmdListToks (const char* args_str, int maxlen)
{
    
    XelpBuf b,tok;
    int n,i;
    XELPRESULT r;
    char *a;
    int aa = (int)(args_str);
    a = (char *)(0+aa);
    XELP_XBInit(b, a,maxlen);
    XelpNumToks(&b,&n);
    XELP_XBTOP(b);
    Serial.print("[");
    Serial.print(n);
    Serial.print("]");
    for (i=0; i< n; i++) {
        XELP_XBTOP(b);
        r = XELPTokN( &b,i,&tok);
        XELPOut(&cli,"<",-1);
        Serial.print(i);
        XELPOut(&cli,":",-1);
        XELPOut(&cli,tok.s,tok.p-tok.s);
        XELPOut(&cli,"> ",-1);
    }
    Serial.print("\n");
  
  return XELP_S_OK;
};


XELPRESULT setColorLED (const char* args_str, int maxlen)
{
    XelpBuf b,tok;
    int n;
    XELPRESULT r;
    char *a;
    int aa = (int)(args_str);
    a = (char *)(0+aa);
    XELP_XBInit(b, a,maxlen);
    XelpNumToks(&b,&n);

    XELP_XBTOP(b);
    XELPTokN(&b,1,&tok);
    //gLEDColor = XELPStr2Int(tok.s,tok.p-tok.s);
    XelpParseNum(tok.s,tok.p-tok.s,&gLEDColor );

    Serial.print("LED set to: ");
    Serial.println(gLEDColor);
    return XELP_S_OK;
};


//create map of functions, with  {function, "command" , "help string"}
XELPCLIFuncMapEntry gMyCLICommands[] =
{
    {&cmdHelp           , "help"    ,   "help"                  },
    {&cmdBanner         , "banner"  ,   "print banner"          },
    {&cmdListToks       , "lt"      ,   "list tokens"           },
    {&setColorLED       , "LED"     ,   "Set LED Brighness (number)"           },
    
    XELP_FUNC_ENTRY_LAST
};

// the setup routine runs once when you press reset:
void setup() {
  Serial.begin(115200);
  while (!Serial) ;
#if defined(NEOPIXEL_POWER)
  // If this board has a power control pin, we must set it to output and high
  // in order to enable the NeoPixels. We put this in an #if defined so it can
  // be reused for other boards without compilation errors
  pinMode(NEOPIXEL_POWER, OUTPUT);
  digitalWrite(NEOPIXEL_POWER, HIGH);
#endif

  pixels.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)
  pixels.setBrightness(20); // not so bright
  
  
  XELPInit(&cli,   "ESP32 Xelp test. 1.0.0\n"); // set the about string for the interpreter and initialize internal state  

  XELP_SET_FN_OUT(cli,&writeChar);  
  XELP_SET_FN_CLI(cli,gMyCLICommands);         // map the cli commands  
  XELP_SET_VAL_CLI_PROMPT(cli,"myprompt>");    // if using per-instance prompt...
  Serial.println(XELP_BANNER_STR);
   
}

// the loop routine runs over and over again forever:
void loop() {
  int time = millis();

  if ((time % 500) < 250) {
    // set color
    pixels.fill(0x222222); // low white
    pixels.show();
  }
  else {
    // different color
    pixels.fill(gLEDColor);
    pixels.show();
  }
  
  if (Serial.available() > 0) {
            char c = Serial.read();
            XELPParseKey(&cli, c);  
  }
}
