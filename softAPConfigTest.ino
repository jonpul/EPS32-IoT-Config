/*
  added support for hardware config reset button connected to pin D2 on the Nano. (buttonResetConfig)
  this could be replaced by some touch screen affordance - like maybe long tap or double tap on the "connecting and getting" boot up screens.

  hardware button has D2 connected as shown
        
         |   |
        [     ]
        [  o  ]
        [     ]
         |   |
        GND  D2 pin
*/


#include <Preferences.h>
#include <WiFi.h>   
#include <HTTPClient.h>

Preferences preferences;

const int buttonResetConfig = 2;

// ssid and pwd for the AP on this device
const char* configSSID= "GlanceDisplayConfig";
const char* configPwd = "123456789";

// vars for the ssid and pwd for you home wifi
String wifiSSID ="";
String wifiPwd = "";

// Set web server port number to 80
WiFiServer server(80);
// Variable to store the HTTP request
String header;

// various flags for state tracking
bool alreadyMessaged = false;
bool connectionFail = false;
bool credentialsStored = false;

void setup() 
{
  Serial.begin(9600);
  // put your setup code here, to run once:
  delay(500);
  pinMode(buttonResetConfig, INPUT_PULLUP);

  // flash storage on chip
  preferences.begin("ConfigTest", false);
  // Remove all preferences under the opened namespace 
  // preferences.clear();
  // pull settings from Flash if they exist
  wifiSSID = preferences.getString("SSID", "");
  wifiPwd = preferences.getString("PWD", "");

  // we have an SSID, so we know we got it from flash and no need to store it again
  // we don't check pwd because the pwd could be blank and that's ok
  if(wifiSSID!="")
  {
    credentialsStored = true;
  }

  if(wifiSSID=="")
  {
    Serial.println("SSID not set up. Entering config mode.");
    // start soft AP and do the config through a web page
    WiFi.softAP(configSSID, configPwd);
    delay(2000);
    IPAddress IP = WiFi.softAPIP();
    // IRL would show this address on the device screen formatted as a URL and instructions to connect to this device AP
    Serial.print("AP IP address for phone browser address: ");
    Serial.println(IP);
    
    server.begin();
  }
}


void loop() 
{

  if(digitalRead(buttonResetConfig) == LOW)
  {
    Serial.println("Reset button pressed");
    wifiSSID = "";
    preferences.clear();
  }
  // if this conditional loop is uncommented here and at the bottom, the loop is too tight apparently for the phone browser to get to the page. 
  // the intent is to have the config page run if you need to configure and connect to home wifi if not. 
  if(wifiSSID == "")
  {
    Serial.println("SSID not set up. Entering config mode.");
    // start soft AP and do the config through a web page
    WiFi.softAP(configSSID, configPwd);
    delay(2000);
    IPAddress IP = WiFi.softAPIP();
    // IRL would show this address on the device screen formatted as a URL and instructions to connect to this device AP
    Serial.print("AP IP address for phone browser address: ");
    Serial.println(IP);
    
    server.begin();
    WiFiClient client = server.available();   // Listen for incoming clients
    handleWebServer(client);
   
  }  // end of web server config condition
  else
  {
    if(!alreadyMessaged)
    {
      Serial.println("done with config");
      alreadyMessaged = true;
      server.close();
      server.end();
      WiFi.softAPdisconnect();
      delay(500);
      // connect to wifi
      Serial.println("Connecting home wifi");
      Serial.println(wifiSSID);
      Serial.println(wifiPwd);

      WiFi.begin(wifiSSID, wifiPwd);
      delay(1000);
      int retryCount = 0;
      while(!WiFi.isConnected() && retryCount < 5)
      {
        retryCount++;
        delay(1000);
        Serial.println("Waiting for wifi...");
      }
      if(!WiFi.isConnected())
      {
        // if still not connected, they probably have wrong Wifi creds
        Serial.println("didn't connect so clearing cred");
        wifiSSID = "";
        wifiPwd = "";
        connectionFail = true;
      }
      else
      {
        // store SSID & pwd to flash
        if(!credentialsStored)
        {
          preferences.putString("SSID", wifiSSID);
          preferences.putString("PWD", wifiPwd);
          credentialsStored = true;
          Serial.println("Creds stored in flash");
        }
        // make a test web call
        testGetTime();
      }
    }
  }
}

void handleWebServer(WiFiClient client)
{
      if (client) {                             // If a new client connects,
      Serial.println("New Client.");          // print a message out in the serial port
      String currentLine = "";                // make a String to hold incoming data from the client
      while (client.connected()) {            // loop while the client's connected
        if (client.available()) {             // if there's bytes to read from the client,
          char c = client.read();             // read a byte, then
          Serial.write(c);                    // print it out the serial monitor
          header += c;
          if (c == '\n') {                    // if the byte is a newline character
            // if the current line is blank, you got two newline characters in a row.
            // that's the end of the client HTTP request, so send a response:
            if (currentLine.length() == 0) {
              // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
              // and a content-type so the client knows what's coming, then a blank line:
              client.println("HTTP/1.1 200 OK");
              client.println("Content-type:text/html");
              client.println("Connection: close");
              client.println();

              if (header.indexOf("GET /?ssid=") >= 0) 
              {
                //extract the ssid from the "=" in "ssid="" to " " or "&"
                int startSSID=header.indexOf("ssid=");
                startSSID+=5; // get to the actual start of the SSID
                int endSSID = header.indexOf("&pwd=");
                wifiSSID=header.substring(startSSID,endSSID);
                // A "+" appended in the GET happens if there is a space at the end of the text input box which can be added by autocorrect
                if(wifiSSID.endsWith("+"))
                {
                  wifiSSID.remove(wifiSSID.length()-1);
                }
                wifiSSID.trim();

                int startPwd=header.indexOf("pwd=");
                startPwd+=4; // get to the actual start of the pwd
                int endPwd = header.indexOf(" ", startPwd);
                wifiPwd=header.substring(startPwd,endPwd);
                // A "+" appended in the GET happens if there is a space at the end of the text input box which can be added by autocorrect
                if(wifiPwd.endsWith("+"))
                {
                  wifiPwd.remove(wifiPwd.length()-1);
                }
                wifiPwd.trim();
               // replace common URL esc characters
                wifiPwd.replace("%24","$");
                wifiPwd.replace("%23","#");
                wifiPwd.replace("%25","%");
                wifiPwd.replace("%40","@");
                wifiPwd.replace("%3f","?");
                wifiPwd.replace("%3F","?");
                wifiPwd.replace("%3d","=");
                wifiPwd.replace("%3D","=");
                wifiPwd.replace("%26","&");
                
                // Display the HTML web page
                client.println("<!DOCTYPE html><html>");
                client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
                client.println("<link rel=\"icon\" href=\"data:,\">");
                // show what was entered
                client.println("<body><h1>ESP32 Web Server</h1>");
                client.println("<p>Stored your wifi credentials for SSID: "+wifiSSID);
                client.println("<p>You can close this page.");
                credentialsStored = false;
              }
              else
              {
                // Display the HTML web page
                client.println("<!DOCTYPE html><html>");
                client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
                client.println("<link rel=\"icon\" href=\"data:,\">");
                
                // Web Page Heading
                client.println("<body><h1>ESP32 Web Server</h1>");
                if(connectionFail)
                {
                  client.println("<p><b>Previous entered credentials did not connect correctly. Please re-enter.</b><br><br>");
                }
                client.println("<p>Reminder: WiFi SSID and Password are case-sensitive.");
                client.println("<form>");
                // woudl be better to get a list of SSIDs in a drop down, but then you'd still have to allow "other" with a text field for non-broadcast SSIDs
                client.println("<label for=\"ssid\">SSID:<\label><input type=\"text\" id=\"ssid\" name=\"ssid\"><br><br>");
                client.println("<label for=\"pwd\">Password:<\label><input type=\"password\" id=\"pwd\" name=\"pwd\"><br><br>");
                client.println("<input type=\"submit\" value=\"Submit\">");
                
                client.println("</form>");            
              }             
              client.println("</body></html>");
              
              // The HTTP response ends with another blank line
              client.println();
              // Break out of the while loop
              break;
            } else { // if you got a newline, then clear currentLine
              currentLine = "";
            }
          } else if (c != '\r') {  // if you got anything else but a carriage return character,
            currentLine += c;      // add it to the end of the currentLine
          }
        }
      }
      // Clear the header variable
      header = "";
      // Close the connection
      client.stop();
      Serial.println("Client disconnected.");
    }
}


void testGetTime()
{
  Serial.println("testGetTime");
  if(WiFi.isConnected())
  {
    // Initialize the HTTPClient object
    HTTPClient http;
    
    // Construct the URL using token from secrets.h  
    //this is WorldTimeAPI.org time request for current public IP
    String url = "https://worldtimeapi.org/api/ip";
    // Make the HTTP GET request 
    http.begin(url);
    int httpCode = http.GET();

    String payload = "";
    // Check the return code
    if (httpCode == HTTP_CODE_OK) {
      // If the server responds with 200, return the payload
      payload = http.getString();
    } else if (httpCode == HTTP_CODE_UNAUTHORIZED) {
      // If the server responds with 401, print an error message
    } else {
      // For any other HTTP response code, print it
    }
    // End the HTTP connection
    http.end();
    Serial.println(url);
    Serial.println(payload);
  }
  else 
  {
    Serial.println("wifi not connected");
  }
}