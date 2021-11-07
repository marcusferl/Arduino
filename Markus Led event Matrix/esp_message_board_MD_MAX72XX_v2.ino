// Use the MD_MAX72XX library to scroll text on the display
// received through the ESP8266 WiFi interface.
//
// Demonstrates the use of the callback function to control what
// is scrolled on the display text. User can enter text through
// a web browser and this will display as a scrolling message on
// the display.
//
// IP address for the ESP8266 is displayed on the scrolling display
// after startup initialization and connected to the WiFi network.
//
// Connections for ESP8266 hardware SPI are:
// Vcc       3v3     LED matrices seem to work at 3.3V
// GND       GND     GND
// DIN        D7     HSPID or HMOSI
// CS or LD   D8     HSPICS or HCS
// CLK        D5     CLK or HCLK
//

// TODO: Rewrite to handle common get request, too

#include <ESP8266WiFi.h>
#include <MD_MAX72xx.h>
#include <SPI.h>

#define PRINT_CALLBACK  0
#define DEBUG 0
#define HTTPDEBUG 0
#define LED_HEARTBEAT 0

#if DEBUG
#define PRINT(s, v) { Serial.print(F(s)); Serial.print(v); }
#define PRINTS(s)   { Serial.print(F(s)); }
#else
#define PRINT(s, v)
#define PRINTS(s)
#endif


#if LED_HEARTBEAT
#define HB_LED  D4
#define HB_LED_TIME 1000 // in milliseconds
#endif

// Define the number of devices we have in the chain and the hardware interface
// NOTE: These pin numbers will probably not work with your hardware and may
// need to be adapted
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 4

#define CLK_PIN   D5 // or SCK
#define DATA_PIN  D7 // or MOSI
#define CS_PIN    D8 // D8 // or SS

// SPI hardware interface
MD_MAX72XX mx = MD_MAX72XX(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);
// Arbitrary pins
//MD_MAX72XX mx = MD_MAX72XX(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);

// WiFi login parameters - network name and password
const char *ssid     = "***your-ssid***";
const char *password = "***your-pass***";

// WiFi Server object and parameters
WiFiServer server(80);

// Global message buffers shared by Wifi and Scrolling functions
const uint8_t MESG_SIZE = 255;
const uint8_t CHAR_SPACING = 1;

uint8_t scrollDelay = 50;

char curMessage[MESG_SIZE];
char newMessage[MESG_SIZE];
char requestMessage[MESG_SIZE*2];
char decodedMessage[MESG_SIZE];
bool newMessageAvailable = false;

int8_t intensity = MAX_INTENSITY/3;
int messageCount=0;
int timeMessage=5;

static byte c1;  // Last character buffer for UTF8-string to extended ASCII conversion

const char WebResponse[] = "HTTP/1.1 200 OK\nContent-Type: text/html\n\n";
const char WebPageStart[] =
"<!DOCTYPE html>" \
"<html>" \
"<head>" \
"<title>Message Board</title>" \
"<meta charset='UTF-8'>" \
"<script>" \
"strLine = \"\";" \
"function SendText()" \
"{" \
"  nocache = \"&nocache=\" + Math.random() * 1000000;" \
"  var request = new XMLHttpRequest();" \
"  strLine = \"/?message=\" + document.getElementById(\"txt_form\").message.value;" \
"  request.open(\"GET\", strLine + nocache, false);" \
"  request.send(null);" \
"}" \
"</script>" \
"</head>" \
"<body>" \
"<div><p><b>Message Board</b></p></div>" ;

const char WebPageForm[] =
"<div>" \
"<form id=\"txt_form\" name=\"frmText\">" \
"<label>Message: <input type=\"text\" name=\"message\" maxlength=\"255\"></label><br><br>" \
"<input type=\"hidden\" name=\"nocache\" value=\"12\"></label><br><br>" \
"</form>" \
"<input type=\"submit\" value=\"Send Text\" onclick=\"SendText()\">" \
"</div>";

const char WebPageEnd[] =
"<div>"  \
"<hr />" \
"</div>" \
"<div><strong>Command messages</strong></div>" \
"<div class='hint'>{w-} faster or {w+} slower message scroll, {w} restore default speed</div>" \
"<div class='hint'>{b+} brighter or {b-} darker display, {b++} brightest or {b--} darkest display, {b} restore default brightness</div>" \
"<div class='hint'>{c+} show messages more often or {c-} fewer and a blank or {time} message shows date time string only, {c} restore default value to 5</div>" \
"</body>" \
"</html>";

const char *err2Str(wl_status_t code)
{
  switch (code)
  {
  case WL_IDLE_STATUS:    return("IDLE");           break; // WiFi is in process of changing between statuses
  case WL_NO_SSID_AVAIL:  return("NO_SSID_AVAIL");  break; // case configured SSID cannot be reached
  case WL_CONNECTED:      return("CONNECTED");      break; // successful connection is established
  case WL_CONNECT_FAILED: return("CONNECT_FAILED"); break; // password is incorrect
  case WL_DISCONNECTED:   return("CONNECT_FAILED"); break; // module is not configured in station mode
  default: return("??");
  }
}

uint8_t htoi(char c)
{
  c = toupper(c);
  if ((c >= '0') && (c <= '9')) return(c - '0');
  if ((c >= 'A') && (c <= 'F')) return(c - 'A' + 0xa);
  return(0);
}

boolean getText(char *szMesg, char *psz, uint8_t len)
{
  boolean isValid = false;  // text received flag
  char *pStart, *pEnd;      // pointer to start and end of text

  // get pointer to the beginning of the text
  pStart = strstr(szMesg, "/?message=");

  if (pStart != NULL)
  {
    pStart += 10;  // skip to start of data
    pEnd = strstr(pStart, "&nocache");
    if (pEnd != NULL)
    {
      while (pStart != pEnd)
      {
        *psz++ = *pStart++;
      }
      *psz = '\0'; // terminate the string
      isValid = true;
    }
  }

  return(isValid);
}

void handleWiFi(void)
{
  static enum { S_IDLE, S_WAIT_CONN, S_READ, S_EXTRACT, S_RESPONSE, S_DISCONN } state = S_IDLE;
  static char szBuf[1024];
  static uint16_t idxBuf = 0;
  static WiFiClient client;
  static uint32_t timeStart;

  switch (state)
  {
  case S_IDLE:   // initialize
    PRINTS("\nS_IDLE");
    idxBuf = 0;
    state = S_WAIT_CONN;
    break;

  case S_WAIT_CONN:   // waiting for connection
    {
      client = server.available();
      if (!client) break;
      if (!client.connected()) break;

#if DEBUG
      char szTxt[20];
      sprintf(szTxt, "%03d:%03d:%03d:%03d", client.remoteIP()[0], client.remoteIP()[1], client.remoteIP()[2], client.remoteIP()[3]);
      PRINT("\nNew client @ ", szTxt);
#endif

      timeStart = millis();
      state = S_READ;
    }
    break;

  case S_READ: // get the first line of data
    PRINTS("\nS_READ");
    while (client.available())
    {
      char c = client.read();
      if ((c == '\r') || (c == '\n'))
      {
        szBuf[idxBuf] = '\0';
        client.flush();
        PRINT("\nRecv: ", szBuf);
        state = S_EXTRACT;
      }
      else
        szBuf[idxBuf++] = (char)c;
    }
    if (millis() - timeStart > 1000)
    {
      PRINTS("\nWait timeout");
      state = S_DISCONN;
    }
#if HTTPDEBUG
    Serial.print("\n Request Raw: ");
    Serial.println(szBuf);
#endif    
    break;

  case S_EXTRACT: // extract data
    PRINTS("\nS_EXTRACT");
    // Extract the string from the message if there is one
    newMessageAvailable = getText(szBuf, requestMessage, MESG_SIZE);
#if HTTPDEBUG
    Serial.print("\n Request: ");
    Serial.println(szBuf);
#endif    
    PRINT("\nRequest Msg: ", requestMessage);
    state = S_RESPONSE;
    break;

  case S_RESPONSE: // send the response to the client
    PRINTS("\nS_RESPONSE");
    // Return the response to the client (web page)
    client.print(WebResponse);
    client.print(WebPageStart);
    client.print(WebPageForm);
    client.print(WebPageEnd);
    state = S_DISCONN;
    break;

  case S_DISCONN: // disconnect client
    PRINTS("\nS_DISCONN");
    client.flush();
    client.stop();
    state = S_IDLE;
    break;

  default:  state = S_IDLE;
  }
}

void setTime() {
      time_t now;
      struct tm * timeinfo;
      char buf[80]; // dd.mm.yyyy - hh:mm:ss
      const char * weekday[] = {"So.", "Mo.", "Di.", "Mi.", "Do.", "Fr.", "Sa." };

      // get current time
      time( &now );
      
      // fill timeinfo
      timeinfo = localtime (&now);
      strcpy(buf, weekday[timeinfo->tm_wday]);
      strftime( buf+sizeof(weekday[timeinfo->tm_wday])-1, sizeof(buf)-sizeof(weekday[timeinfo->tm_wday])-1, " %d.%m.%Y - %H:%M:%S", timeinfo );
      // format date time string, see http://www.cplusplus.com/reference/ctime/strftime/
      strcpy(curMessage, buf);
}

// ****** UTF8-Decoder: convert UTF8-string to extended ASCII *******

// Convert a single Character from UTF8 to Extended ASCII
// Return "0" if a byte has to be ignored
byte utf8ascii(byte ascii) {
    if ( ascii<128 )   // Standard ASCII-set 0..0x7F handling  
    {   c1=0;
        return( ascii );
    }

    // get previous input
    byte last = c1;   // get last char
    c1=ascii;         // remember actual character

    switch (last)     // conversion depending on first UTF8-character
    {   case 0xC2: return  (ascii);  break;
        case 0xC3: return  (ascii | 0xC0);  break;
        case 0x82: if(ascii==0xAC) return(0x80);       // special case Euro-symbol
    }

    return  (0);                                     // otherwise: return zero, if character has to be ignored
}

// convert String object from UTF8 String to Extended ASCII
String utf8ascii(String s)
{      
        String r="";
        char c;
        for (int i=0; i<s.length(); i++)
        {
                c = utf8ascii(s.charAt(i));
                if (c!=0) r+=c;
        }
        return r;
}

// In Place conversion UTF8-string to Extended ASCII (ASCII is shorter!)
void utf8ascii(char* s)
{      
        int k=0;
        char c;
        for (int i=0; i<strlen(s); i++)
        {
                c = utf8ascii(s[i]);
                if (c!=0)
                        s[k++]=c;
        }
        s[k]=0;
}


void urldecode(char *dst, const char *src)
{
  char a, b,c;
  if (dst==NULL) return;
  while (*src) {
    if ((*src == '%') &&
      ((a = src[1]) && (b = src[2])) &&
      (isxdigit(a) && isxdigit(b))) {
      if (a >= 'a')
        a -= 'a'-'A';
      if (a >= 'A')
        a -= ('A' - 10);
      else
        a -= '0';
      if (b >= 'a')
        b -= 'a'-'A';
      if (b >= 'A')
        b -= ('A' - 10);
      else
        b -= '0';
      *dst++ = 16*a+b;
      src+=3;
    } 
    else {
        c = *src++;
        if(c=='+')c=' ';
      *dst++ = c;
    }
  }
  *dst++ = '\0';
}

void checkMessage() {
      // decode url string
      urldecode(decodedMessage, requestMessage);

#if HTTPDEBUG
      Serial.print("\n requestMessage: ");
      Serial.println(requestMessage);
      
      Serial.print("\n decodedMessage (utf-8): ");
      Serial.println(decodedMessage);
#endif
      
      // convert utf8 to extended ascii 
      utf8ascii(decodedMessage);

#if HTTPDEBUG
      Serial.print("\n decodedMessage (ascii): ");
      Serial.println(decodedMessage);
#endif

      // modify delay
      if (strcmp(decodedMessage, "{w}") == 0) {
          scrollDelay=75;
      }
      else if (strcmp(decodedMessage, "{w+}") == 0)  { 
          if (scrollDelay<250)
            scrollDelay=scrollDelay+5;
      }
      else if (strcmp(decodedMessage, "{w-}") == 0) {
          if (scrollDelay>5)
            scrollDelay=scrollDelay-5;
      }
          // modify message counter to show datetime info
      else if (strcmp(decodedMessage, "{c}") == 0) {
          timeMessage=5;
      }
      else if (strcmp(decodedMessage, "{c+}") == 0) { 
          if (messageCount<100)
            timeMessage++;
      }
      else if (strcmp(decodedMessage, "{c-}") == 0) {
          if (messageCount>1)
            timeMessage--;
      }
   
      // modify brightness of didplay
      else if (strcmp(decodedMessage, "{b}") == 0) {
          intensity = MAX_INTENSITY/3;
          mx.control(MD_MAX72XX::INTENSITY, intensity);
      }
      else if (strcmp(decodedMessage, "{b+}") == 0)  {
          if (intensity<MAX_INTENSITY) {
            intensity++;
            mx.control(MD_MAX72XX::INTENSITY, intensity);
          }
      }
      else if (strcmp(decodedMessage, "{b++}") == 0)  {
          intensity=MAX_INTENSITY;
          mx.control(MD_MAX72XX::INTENSITY, intensity);
      }
      else if (strcmp(decodedMessage, "{b-}") == 0) {
          if (intensity>=1) {
            intensity--;
            mx.control(MD_MAX72XX::INTENSITY, intensity);
          }
      }
      else if (strcmp(decodedMessage, "{b--}") == 0) {
          intensity=0;
          mx.control(MD_MAX72XX::INTENSITY, intensity);
      }
      else if (strcmp(decodedMessage, "{time}") == 0) 
      {
        setTime();
      }
      else {
        strcpy(newMessage, decodedMessage); // copy it in  
      }
#if HTTPDEBUG
      Serial.print("\n Intensity: ");
      Serial.print(intensity);
      Serial.print("\n Scroll delay ");
      Serial.println(scrollDelay);
#endif      
}

void scrollDataSink(uint8_t dev, MD_MAX72XX::transformType_t t, uint8_t col)
// Callback function for data that is being scrolled off the display
{
#if PRINT_CALLBACK
  Serial.print("\n cb ");
  Serial.print(dev);
  Serial.print(' ');
  Serial.print(t);
  Serial.print(' ');
  Serial.println(col);
#endif
}

uint8_t scrollDataSource(uint8_t dev, MD_MAX72XX::transformType_t t)
// Callback function for data that is required for scrolling into the display
{
  static enum { S_IDLE, S_NEXT_CHAR, S_SHOW_CHAR, S_SHOW_SPACE } state = S_IDLE;
  static char *p;
  static uint16_t curLen, showLen;
  static uint8_t  cBuf[8];
  uint8_t colData = 0;

  // finite state machine to control what we do on the callback
  switch (state)
  {
  case S_IDLE: // reset the message pointer and check for new message to load
    PRINTS("\nS_IDLE");
    p = curMessage;      // reset the pointer to start of message
    if (newMessageAvailable)  // there is a new message waiting
    {
      checkMessage();
      strcpy(curMessage, newMessage); // copy it in  
      newMessageAvailable = false;
    } else if (strcmp(decodedMessage, "{time}") == 0) {
      setTime();
    } else if ( timeMessage > 0 ) {
      messageCount++;
#if DEBUG
      Serial.print("\n count: ");
      Serial.print(messageCount);
      Serial.print("\n timeMessage ");
      Serial.println(timeMessage);
#endif          
      if (messageCount > timeMessage) {
        setTime();
        messageCount=0;
      } else {
        strcpy(curMessage, newMessage); // copy it in  again
      }
    }
    state = S_NEXT_CHAR;
    break;

  case S_NEXT_CHAR: // Load the next character from the font table
    PRINTS("\nS_NEXT_CHAR");
    if (*p == '\0')
      state = S_IDLE;
    else
    {
      showLen = mx.getChar(*p++, sizeof(cBuf) / sizeof(cBuf[0]), cBuf);
      curLen = 0;
      state = S_SHOW_CHAR;
    }
    break;

  case S_SHOW_CHAR: // display the next part of the character
    PRINTS("\nS_SHOW_CHAR");
    colData = cBuf[curLen++];
    if (curLen < showLen)
      break;

    // set up the inter character spacing
    showLen = (*p != '\0' ? CHAR_SPACING : (MAX_DEVICES*COL_SIZE)/2);
    curLen = 0;
    state = S_SHOW_SPACE;
    // fall through

  case S_SHOW_SPACE:  // display inter-character spacing (blank column)
    PRINT("\nS_ICSPACE: ", curLen);
    PRINT("/", showLen);
    curLen++;
    if (curLen == showLen)
      state = S_NEXT_CHAR;
    break;

  default:
    state = S_IDLE;
  }

  return(colData);
}

void scrollText(void)
{
  static uint32_t	prevTime = 0;

  // Is it time to scroll the text?
  if (millis() - prevTime >= scrollDelay)
  {
    mx.transform(MD_MAX72XX::TSL);  // scroll along - the callback will load all the data
    prevTime = millis();      // starting point for next time
  }
}

void setup()
{
#if DEBUG
  Serial.begin(115200);
  PRINTS("\n[MD_MAX72XX WiFi Message Display]\nType a message for the scrolling display from your internet browser");
#endif
#if HTTPDEBUG
  Serial.begin(115200);
  PRINTS("\n[MD_MAX72XX WiFi Message Display]\nType a message for the scrolling display from your internet browser");
#endif

#if LED_HEARTBEAT
  pinMode(HB_LED, OUTPUT);
  digitalWrite(HB_LED, LOW);
#endif

  curMessage[0] = newMessage[0] = '\0';

  // Connect to and initialize WiFi network
  PRINT("\nConnecting to ", ssid);

  // set hostname of device
  WiFi.hostname("messageboard"); // devboard");
  WiFi.begin(ssid, password);
  // set time zone to europe/berlin
  configTime(0 * 3600, 0, "pool.ntp.org", "time.nist.gov");
  setenv("TZ", "CET-1CEST,M3.5.0/01,M10.5.0/02",1);
  tzset();
  // set locales
  setlocale(LC_ALL, "de_DE");
  setlocale(LC_TIME, "de_DE");
  setlocale(LC_NUMERIC, "de_DE");
  
  // Wait for connection
  int i = 0;
  while (WiFi.status() != WL_CONNECTED && i++ <= 100) //wait 10 seconds
  {
    PRINT("\n", err2Str(WiFi.status()));
    delay(500);
    i++;
  }
  if(i > 100)
  {
    PRINTS("\nWiFi not connected");
    while(1) delay(500);
  }
  else 
  {  
    PRINTS("\nWiFi connected");
  }

  // Start the server
  server.begin();
  PRINTS("\nServer started");

  // Display initialization
  mx.begin();
  mx.setShiftDataInCallback(scrollDataSource);
  mx.setShiftDataOutCallback(scrollDataSink);

  // Set up first message as the IP address
  sprintf(curMessage, "Willkommen (%03d:%03d:%03d:%03d)...", WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], WiFi.localIP()[3]);
  // copy first message to all message variables
  strcpy(newMessage, curMessage);
  strcpy(decodedMessage, curMessage);
  PRINT("\nAssigned IP ", curMessage);
  
}

void loop()
{
#if LED_HEARTBEAT
  static uint32_t timeLast = 0;
  if (millis() - timeLast >= HB_LED_TIME)
  {
    digitalWrite(HB_LED, digitalRead(HB_LED) == LOW ? HIGH : LOW);
    timeLast = millis();
  }
#endif
  handleWiFi();
  scrollText();
}
