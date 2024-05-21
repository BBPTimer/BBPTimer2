#define D7 (13)
#define PUMP_PIN D7
#define varReadFreq 55000 //Warmup, PreInfuse, Cooldown variable read frequency, in ms
#define sleepReadFreq 55000 //DisplaySleep variable read frequency, in ms

//Display libraries
#include <Adafruit_SSD1306.h> //https://github.com/adafruit/Adafruit_SSD1306
#include <Adafruit_GFX.h> //https://github.com/adafruit/Adafruit-GFX-Library
#include <Wire.h>
//Timer library
#include <Timer.h> //https://github.com/JChristensen/Timer
//WiFi libraries
#include <ESP8266WiFi.h>
#define WEBSERVER_H "fix conflict" //https://github.com/tzapu/WiFiManager/issues/1530#issuecomment-1902673009
#include <WiFiManager.h> //https://github.com/tzapu/WiFiManager
//Web server libraries
#include <ESPAsyncTCP.h> //https://github.com/me-no-dev/ESPAsyncTCP
#include <ESPAsyncWebServer.h> //https://github.com/me-no-dev/ESPAsyncWebServer
//SPIFFS libraries
#include <Hash.h>
#include <FS.h>

//Initialize display
Adafruit_SSD1306 display(128, 64, &Wire, -1);

//Initialize timer
Timer t;

//Initialize WiFiManager
WiFiManager wifiManager;

//Initialize web server
AsyncWebServer server(80);

bool reedOpenSensor = true; // set to true/false when using another type of reed sensor

bool displayOn = true;

int timerCount = 0;
int prevTimerCount = 0;
bool timerStarted = false;
long timerStartMillis = 0;
long timerStopMillis = 0;
long timerDisplayOffMillis = 0;
int pumpInValue = 0;
long varInputMillis = 0;
long sleepInputMillis = 0;

bool wifiCredSaved = false;

//Initialize user variables
int yourWarmup = 0;
int yourPreInfuse = 0;
int yourCooldown = 0;
int yourDisplaySleep = 1;

const char* PARAM_WU = "Warmup";
const char* PARAM_PI = "PreInfuse";
const char* PARAM_CD = "Cooldown";
const char* PARAM_DS = "DisplaySleep";
const char* PARAM_FW = "ForgetWiFi";

//HTML page
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>
<head>
  <style>
  * {
    font-family: Arial, sans-serif;
  }
  body {
    text-align: center;
  }
  input[type=number] {
    width: 50px;
    padding: 4px 4px;
    box-sizing: border-box;
    border: 1px solid lightgray;
    border-radius: 4px;
  }
  input[type=submit] {
    background-color: gray;
    border: 1px solid black;
    border-radius: 4px;
    color: white;
    padding: 4px 4px;
    font-weight: bold;
    cursor: pointer;
  }
  .left {
    display:inline-block;
    text-align: left;
  }
  </style>
  <title>BBPTimer</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <script>
    function submitMessage() {
      alert("Thank you! Settings will reload in 5 seconds.");
      setTimeout(function(){ document.location.reload(false); }, 5000); 
    }
  </script>
    <script>
    function wiFiInstructions() {
      alert("Connect to BBPTimer WiFi network to enter new WiFi credentials.");
    }
  </script>
</head>
<body>
  <h1>BBPTimer Settings</h1>
  <form action="/get" target="hidden-form">
    <b>Warmup</b>, in seconds (between 0 and 10); currently %Warmup% seconds: <input type="number" name="Warmup" min="0" max="10" step="1">
    <input type="submit" value="Save" onclick="submitMessage()">
  </form><br>
  <form action="/get" target="hidden-form">
    <b>PreInfuse</b>, in seconds (between 0 and 10); currently %PreInfuse% seconds: <input type="number" name="PreInfuse" min="0" max="10" step="1">
    <input type="submit" value="Save" onclick="submitMessage()">
  </form><br>
  <form action="/get" target="hidden-form">
    <b>Cooldown</b>, in seconds (between 0 and 15); currently %Cooldown% seconds: <input type="number" name="Cooldown" min="0" max="15" step="1">
    <input type="submit" value="Save" onclick="submitMessage()">
  </form><br>
  <form action="/get" target="hidden-form">
    <b>DisplaySleep</b>, in minutes (between 1 and 99); currently %DisplaySleep% minutes: <input type="number" name="DisplaySleep" min="1" max="99" step="1">
    <input type="submit" value="Save" onclick="submitMessage()">
  </form><br>
  <form action="/get" target="hidden-form">
    <input type="submit" name="ForgetWiFi" value="Forget WiFi Credentials" onclick="wiFiInstructions()"> ; currently <b>%WiFiCred%</b>
  </form>
  <iframe style="display:none" name="hidden-form"></iframe>
  <h1>Calibration Instructions</h1>
  <div class="left">
  Pull a blank shot. Record these 3 timestamps:
  <ol>
    <li>Timer value when the pump starts vibrating
    <li>Timer value when the pump stops vibrating
    <li>Final timer value
  </ol>
  #1 is your <b>Warmup</b> time.<br>
  #3 minus #2 is your <b>Cooldown</b> time.
  </div>
</body>
</html>
)rawliteral";

void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}

//SPIFFS stuff
String readFile(fs::FS &fs, const char * path){
  Serial.printf("Reading file: %s\r\n", path);
  File file = fs.open(path, "r");
  if(!file || file.isDirectory()){
    Serial.println("- empty file or failed to open file");
    return String();
  }
  Serial.println("- read from file:");
  String fileContent;
  while(file.available()){
    fileContent+=String((char)file.read());
  }
  file.close();
  Serial.println(fileContent);
  return fileContent;
}

void writeFile(fs::FS &fs, const char * path, const char * message){
  Serial.printf("Writing file: %s\r\n", path);
  File file = fs.open(path, "w");
  if(!file){
    Serial.println("- failed to open file for writing");
    return;
  }
  if(file.print(message)){
    Serial.println("- file written");
  } else {
    Serial.println("- write failed");
  }
  file.close();
}

// Replaces placeholder with stored values
String processor(const String& var){
  //Serial.println(var);
  if(var == "Warmup"){
    return readFile(SPIFFS, "/Warmup.txt");
  }
  else if(var == "PreInfuse"){
    return readFile(SPIFFS, "/PreInfuse.txt");
  }
  else if(var == "Cooldown"){
    return readFile(SPIFFS, "/Cooldown.txt");
  }
  else if(var == "DisplaySleep"){
    return readFile(SPIFFS, "/DisplaySleep.txt");
  }
  else if(var == "WiFiCred"){
    return WiFi.SSID();
  }
  return String();
}

//Called when WiFiManager saves WiFi credentials
void saveConfigCallback () {
  Serial.println("WiFi credentials saved");
  wifiCredSaved = true;
}

//Reset function
void(* resetFunc) (void) = 0;//declare reset function at address 0

void setup() {
  Serial.begin(115200);

  pinMode(PUMP_PIN, INPUT_PULLUP);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  t.every(100, updateDisplay);

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.display();

  varInputMillis = millis();
  sleepInputMillis = millis();

  // Initialize SPIFFS
  if(!SPIFFS.begin()){
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  //WiFi stuff
  WiFi.setHostname("bbptimer");

  std::vector<const char *> menu = {"wifi","exit"};
  wifiManager.setMenu(menu);
  wifiManager.setSaveConfigCallback(saveConfigCallback);
  wifiManager.autoConnect("BBPTimer", "3$Pr3$$0");
  if (wifiCredSaved) {
    Serial.println("Resetting");
    resetFunc();
  }

  //Web server stuff
  // Send web page with input fields to client
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });
  server.on("/get", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String inputMessage;
    // GET Warmup value on <ESP_IP>/get?Warmup=<inputMessage>
    if (request->hasParam(PARAM_WU)) {
      inputMessage = request->getParam(PARAM_WU)->value();
      writeFile(SPIFFS, "/Warmup.txt", inputMessage.c_str());
    }
    // GET PreInfuse value on <ESP_IP>/get?PreInfuse=<inputMessage>
    else if (request->hasParam(PARAM_PI)) {
      inputMessage = request->getParam(PARAM_PI)->value();
      writeFile(SPIFFS, "/PreInfuse.txt", inputMessage.c_str());
    }
    // GET Cooldown value on <ESP_IP>/get?Cooldown=<inputMessage>
    else if (request->hasParam(PARAM_CD)) {
      inputMessage = request->getParam(PARAM_CD)->value();
      writeFile(SPIFFS, "/Cooldown.txt", inputMessage.c_str());
    }
    // GET DisplaySleep value on <ESP_IP>/get?DisplaySleep=<inputMessage>
    else if (request->hasParam(PARAM_DS)) {
      inputMessage = request->getParam(PARAM_DS)->value();
      writeFile(SPIFFS, "/DisplaySleep.txt", inputMessage.c_str());
    }
    else if (request->hasParam(PARAM_FW)) {
      wifiManager.resetSettings();
    }
    else {
      inputMessage = "No message sent";
    }
    Serial.println(inputMessage);
    request->send(200, "text/text", inputMessage);
  });
  server.onNotFound(notFound);
  server.begin();

  //Display IP on boot
  displayIP();
}

void loop() {
  t.update();
  detectChanges();
}

void detectChanges() {
  //Access sleep variable
  if ((millis() - sleepInputMillis) > sleepReadFreq) {
  yourDisplaySleep = readFile(SPIFFS, "/DisplaySleep.txt").toInt();
  Serial.print("*** Your DisplaySleep: ");
  Serial.println(yourDisplaySleep);

  sleepInputMillis = millis();
  }

  digitalWrite(LED_BUILTIN, digitalRead(PUMP_PIN));
  if(reedOpenSensor) {
    pumpInValue = digitalRead(PUMP_PIN);
  } else {
    pumpInValue = !digitalRead(PUMP_PIN);
  }
  if (!timerStarted && !pumpInValue) {
    timerStartMillis = millis();
    timerStarted = true;
    displayOn = true;
    Serial.println("Start pump");
  }
  if (timerStarted && pumpInValue) {
    if (timerStopMillis == 0) {
      timerStopMillis = millis();
    }
    if (millis() - timerStopMillis > 1500) {
      timerStarted = false;
      timerStopMillis = 0;
      timerDisplayOffMillis = millis();
      display.invertDisplay(false);
      Serial.println("Stop pump");
    }
  } else {
    timerStopMillis = 0;
  }
  if (!timerStarted && displayOn && timerDisplayOffMillis >= 0 && (millis() - timerDisplayOffMillis > 1000 * 60 * yourDisplaySleep)) {
    timerDisplayOffMillis = 0;
    timerCount = 0;
    prevTimerCount = 0;
    displayOn = false;
    Serial.println("Sleep");
  }
}

String getTimer() {
  //Access variables
  if ((millis() - varInputMillis) > varReadFreq) {
  yourWarmup = readFile(SPIFFS, "/Warmup.txt").toInt();
  Serial.print("*** Your Warmup: ");
  Serial.println(yourWarmup);

  yourPreInfuse = readFile(SPIFFS, "/PreInfuse.txt").toInt();
  Serial.print("*** Your PreInfuse: ");
  Serial.println(yourPreInfuse);

  yourCooldown = readFile(SPIFFS, "/Cooldown.txt").toInt();
  Serial.print("*** Your Cooldown: ");
  Serial.println(yourCooldown);

  varInputMillis = millis();
  }

  char outMin[2];
  if (timerStarted) {
    timerCount = ((millis() - timerStartMillis ) / 1000) - yourWarmup;
    if (yourPreInfuse > 0 && timerCount < yourPreInfuse) {
      display.invertDisplay(true);
    } else {
      display.invertDisplay(false);
    }
      prevTimerCount = timerCount;
  } else {
    timerCount = prevTimerCount - yourCooldown;
    display.invertDisplay(false);
  }
  if (timerCount < 0) {
    return "00";
  }
  if (timerCount > 99) {
    return "99";
  }
  sprintf( outMin, "%02i", timerCount);
  return outMin;
}

void updateDisplay() {
  display.clearDisplay();
  if (displayOn) {
    display.setTextSize(7);
    display.setCursor(25, 8);
    display.print(getTimer());
  }
  display.display();
}

void displayIP() {
  display.clearDisplay();
  display.setTextSize(1);
  display.println("Access settings at\n");
  display.setTextSize(2);
  display.println(WiFi.localIP());
  display.display();
  delay(30000);
}