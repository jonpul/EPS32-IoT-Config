# EPS32-IoT-Config
A prototype showing how to have an IoT device that starts its own wifi access point and web server so a user can use another device to configure the IoT with home wifi creds

Shows how to allow a limited device to be set up for local WiFi credentials without hardcoding it in, loading it from a file or needing a keyboard. 
Tested on Arduino Nano ESP32 and Xseeed ESP32 C3. 
This prototype supports a physical button to clear the configuration and stores the user credentials in ESP32 preferences flash.


