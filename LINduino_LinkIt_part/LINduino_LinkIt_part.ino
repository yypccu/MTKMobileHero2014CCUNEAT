#include <LGPRS.h>
#include <LGPRSClient.h>
#include <LGPRSServer.h>
#include <LGPRSUdp.h>

#include <LFlash.h>
#include <LSD.h>
#include <LStorage.h>

LGPRSClient client;

String header = "1000000221 ,, 5 ,";

String inputString = "";         // a string to hold incoming data
boolean stringComplete = false;  // whether the string is complete
boolean flag = false;

void setup() {
  // initialize Serial1:
  Serial.begin(9600);
  Serial1.begin(9600);
  
  LSD.begin();
  checkSim();
  // reserve 200 bytes for the inputString:
  inputString.reserve(200);
}


void loop() {
  // print the string when a newline arrives:
  if (stringComplete) {
    Serial.println(inputString);
    
    String command = inputString.substring(0, 8);
    String data = inputString.substring(10, inputString.length());
    
    if(command == "AT+SAVSD"){
      LFile f = LSD.open("data.txt",FILE_WRITE);
      f.println(data);
      f.close();
      Serial1.println("ok");
    }else if(command == "AT+CLOUD"){
      
      // Analysis the string
//      for(int i = 0; i < 12; i++) {
//        index_f = data.indexOf(':', index_b) + 1;
//        index_b = data.indexOf(' ', index_f);
//        dataToCloud[i] = data.substring(index_f, index_b);
//      }
      // Upload to mediatek cloud sandbox
      checkSim();
      postToCloud(data);
    }
    // clear the string:
    inputString = "";
    stringComplete = false;
  }
  
  char v;
    while(client.available())
    {
       v = (char)client.read();
       if (v < 0)
         break;
       Serial1.print(v);
    }
}


void serialEvent1() {
  while (Serial1.available()) {
    // get the new byte:
    char inChar = (char)Serial1.read();
    // add it to the inputString: 
    // if the incoming character is a newline, set a flag
    // so the main loop can do something about it:
    
    if(inChar == 'A'){
      flag = true;
    }
    if (inChar == '\n') {
      stringComplete = true;
      flag = false;
    }else if(flag){
      inputString += inChar;
    }
  }
}

void postToCloud(String data) {
  
  Serial.println("Sending POST request...");
  client.println("POST /api/v1.0/devices/100000089/datapoints.csv HTTP/1.1");
  client.println("Host: api.mediatek.com");
  client.println("apiKey: d3aed1408560d5b29232bed12724bde1$1605000028");
  client.print("Content-Length: ");
  client.println(header.length()+data.length());
  client.println("Content-Type: text/csv");
  client.println("Connection: close");
  client.println();
  client.print(header);
  client.println(data);
  
  Serial1.println("ok");
}

void checkSim(){
  while(!LGPRS.attachGPRS())
   {
     Serial.println("wait for SIM card ready");
     delay(1000);
   }
   Serial.print("Connecting to : api.mediatek.com...");
   if(!client.connect("api.mediatek.com", 80))
   {
     Serial.println("FAIL!");
     return; 
   }
   Serial.println("done");
  
}

