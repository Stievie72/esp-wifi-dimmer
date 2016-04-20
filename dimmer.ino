#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>

const char WiFiSSID[] = "Depeche Modem (2.4GHz)";
const char WiFiPSK[] = "powerhouseofthecell";
const int LED_PIN = 5; // Thing's onboard, green LED

unsigned char IN_PIN = 12;
unsigned char OUT_PIN = 13;
float brightness = 1.0;  // Dimming level (0-100)

WiFiServer server(80);

void setup() 
{
  initHardware();
  connectWiFi();
  server.begin();
  setupMDNS();

  pinMode(OUT_PIN, OUTPUT);
  attachInterrupt(IN_PIN, zero_crosss_int, RISING);
}

void initHardware()
{
  Serial.begin(9600);
  pinMode(LED_PIN, OUTPUT);
}

void connectWiFi()
{
  byte ledStatus = LOW;
  Serial.println();
  Serial.println("Connecting to: '" + String(WiFiSSID) + "'");
  // Set WiFi mode to station (as opposed to AP or AP_STA)
  WiFi.mode(WIFI_STA);

  // WiFI.begin([ssid], [passkey]) initiates a WiFI connection
  // to the stated [ssid], using the [passkey] as a WPA, WPA2,
  // or WEP passphrase.
  WiFi.begin(WiFiSSID, WiFiPSK);

  // Use the WiFi.status() function to check if the ESP8266
  // is connected to a WiFi network.
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("current status: '" + String(WiFi.status()) + "'");
    // Blink the LED
    digitalWrite(LED_PIN, ledStatus); // Write LED high/low
    ledStatus = (ledStatus == HIGH) ? LOW : HIGH;

    // Delays allow the ESP8266 to perform critical tasks
    // defined outside of the sketch. These tasks include
    // setting up, and maintaining, a WiFi connection.
    delay(100);
    // Potentially infinite loops are generally dangerous.
    // Add delays -- allowing the processor to perform other
    // tasks -- wherever possible.
  }
  Serial.println("WiFi connected");  
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void setupMDNS()
{
  // Call MDNS.begin(<domain>) to set up mDNS to point to
  // "<domain>.local"
  if (!MDNS.begin("thing")) 
  {
    Serial.println("Error setting up MDNS responder!");
    while(1) { 
      delay(1000);
    }
  }
  Serial.println("mDNS responder started");

}

void loop() 
{
  // Check if a client has connected
  WiFiClient client = server.available();
  if (!client) {
    return;
  }

  // seek forward to the body:
  bool hasBody = client.find("\r\n\r\n");
  if (!hasBody) {
    return;
  }

  brightness = client.parseFloat();

  client.flush();

  // Prepare the response. Start with the common header:
  String s = "HTTP/1.1 200 OK\r\n";
  s += "Content-Type: text/plain\r\n\r\n";
  s += "good jorb\n";

  // Send the response to the client
  client.print(s);
  delay(1);
  Serial.println("Client disonnected");

  // The client will actually be disconnected 
  // when the function returns and 'client' object is detroyed
}

void zero_crosss_int()  // function to be fired at the zero crossing to dim the light
{
  // Firing angle calculation : 1 full 50Hz wave =1/50=20ms 
  // Every zerocrossing : (50Hz)-> 10ms (1/2 Cycle) For 60Hz (1/2 Cycle) => 8.33ms 

  if (brightness == 0) {
    digitalWrite(OUT_PIN, LOW);
    return;
  }
  if (brightness == 1) {
    digitalWrite(OUT_PIN, HIGH);   // triac firing
    return;
  }

  int dimming = (1.0 - brightness) * 100;
  int dimtime = (70 * dimming) + 280;

  delayMicroseconds(dimtime);    // Off cycle
  triggerTriac();
}

static inline void triggerTriac()
{
  digitalWrite(OUT_PIN, HIGH);   // triac firing
  delayMicroseconds(50);         // triac On propogation delay
  digitalWrite(OUT_PIN, LOW);    // triac Off
}

