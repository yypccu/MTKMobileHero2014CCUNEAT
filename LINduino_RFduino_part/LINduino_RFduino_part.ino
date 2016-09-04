// UNO board p1 -> RFduino p1
// UNO board p0 -> RFduino p0

#include <DHT.h>
#include <Wire.h>
#include <RFduinoBLE.h>

#define DHTTYPE  DHT22
#define DHTPIN1  4
#define DHTPIN2  5
#define DHTPIN3  6

#define SCLPIN  3
#define SDAPIN  2

#define ADXLSAMPLETIMES 1000
#define SCALE 10

#define ADXL345_ADDR 0x53
#define BYTES_TO_READ 6
#define Z_REG_ADDRESS 0x32
#define PWR_CTL_REG 0x2D
#define MAXBOUND 20

DHT dht1(DHTPIN1, DHTTYPE);
DHT dht2(DHTPIN2, DHTTYPE);
DHT dht3(DHTPIN3, DHTTYPE);

byte buffer[BYTES_TO_READ];
char keyString[] = {'o','k'};
boolean flag = false;
boolean upload = false;

void setup(){
  Serial.begin(9600);
  dht1.begin();
  dht2.begin();
  dht3.begin();  
  Wire.beginOnPins(SCLPIN, SDAPIN);     // Begin 1-wire communication with GPIO2(SCL) and GPIO3(SDA)
  
  initADXL345();
  
  RFduinoBLE.deviceName = "Linduino"; 
  RFduinoBLE.txPowerLevel = +4; 
  RFduinoBLE.advertisementInterval = 200;
  RFduinoBLE.begin();
}

void loop() {
  
  
  
  // Read the data of DHT22
  float humidityArray[3];
  float temperatureArray[3];

  humidityArray[0] = dht1.readHumidity();
  temperatureArray[0] = dht1.readTemperature();
  humidityArray[1] = dht2.readHumidity();
  temperatureArray[1] = dht2.readTemperature();
  humidityArray[2] = dht3.readHumidity();
  temperatureArray[2] = dht3.readTemperature();
  
//  for(int i = 0; i < 3; i++) {
//    Serial.print("DHT");
//    Serial.print(i+1);
//    Serial.print("\tHumidity: "); 
//    Serial.print(humidityArray[i]);
//    Serial.print(" %\tTemperature: "); 
//    Serial.println(temperatureArray[i]);
//  }   
  
  // Read data from ADXL345 and print it out
  int16_t x_valueArray[ADXLSAMPLETIMES/SCALE];
  int16_t y_valueArray[ADXLSAMPLETIMES/SCALE];
  int16_t z_valueArray[ADXLSAMPLETIMES/SCALE];
  int avrgX = 0;
  int avrgY = 0;
  int avrgZ = 0;
  unsigned long varX = 0;
  unsigned long varY = 0;
  unsigned long varZ = 0;
  
  for(int j = 0; j < SCALE; j++) {
    int sumOfX = 0;
    int sumOfY = 0;
    int sumOfZ = 0;
    unsigned long sumOfSqrX = 0;
    unsigned long sumOfSqrY = 0;
    unsigned long sumOfSqrZ = 0;
    
    for(int i = 0; i < ADXLSAMPLETIMES/SCALE; i++) {
      readReg(ADXL345_ADDR, Z_REG_ADDRESS, BYTES_TO_READ);
      x_valueArray[i] = (buffer[1]<<8) | buffer[0];
      y_valueArray[i] = (buffer[3]<<8) | buffer[2];
      z_valueArray[i] = (buffer[5]<<8) | buffer[4];
      
      sumOfX += x_valueArray[i];
      sumOfY += y_valueArray[i];
      sumOfZ += z_valueArray[i];
      delay(10);
    }
    sumOfX = sumOfX / (ADXLSAMPLETIMES/SCALE);
    sumOfY = sumOfY / (ADXLSAMPLETIMES/SCALE);
    sumOfZ = sumOfZ / (ADXLSAMPLETIMES/SCALE);
    avrgX += sumOfX;
    avrgY += sumOfY;
    avrgZ += sumOfZ;
    
    for(int i = 0; i < ADXLSAMPLETIMES/SCALE; i++) {
      sumOfSqrX += (x_valueArray[i] - sumOfX)*(x_valueArray[i] - sumOfX);
      sumOfSqrY += (y_valueArray[i] - sumOfY)*(y_valueArray[i] - sumOfY);
      sumOfSqrZ += (z_valueArray[i] - sumOfZ)*(z_valueArray[i] - sumOfZ);
    }
    varX += (sumOfSqrX/(ADXLSAMPLETIMES/SCALE));
    varY += (sumOfSqrY/(ADXLSAMPLETIMES/SCALE));
    varZ += (sumOfSqrZ/(ADXLSAMPLETIMES/SCALE));
  }
  avrgX /= SCALE;
  avrgY /= SCALE;
  avrgZ /= SCALE;
//  Serial.print("avrgX: ");
//  Serial.print(avrgX);
//  Serial.print("  avrgY: ");
//  Serial.print(avrgY);
//  Serial.print("  avrgZ: ");
//  Serial.println(avrgZ); 
//  Serial.print("varX: ");
//  Serial.print(varX);
//  Serial.print("  varY: ");
//  Serial.print(varY);
//  Serial.print("  varZ: ");
//  Serial.println(varZ);

  if(flag == true){
    sendValue(humidityArray[0],humidityArray[1],humidityArray[2] ,temperatureArray[0],temperatureArray[1],temperatureArray[2],varX,varY,varZ, avrgX,avrgY,avrgZ);
  }
  
  String dataOfTenSec = String(millis()/1000) + " DHT1 H:" + f2S(humidityArray[0]) + " T:" + f2S(temperatureArray[0]) + "  DHT2 H:" + f2S(humidityArray[1]) + " T:" + f2S(temperatureArray[1]) + "  DHT3 H:" + f2S(humidityArray[2]) + " T:" + f2S(temperatureArray[2]) + "  avrgX:" + avrgX + "  avrgY:" + avrgY + "  avrgZ:" + avrgZ + "  varX:" + varX + "  varY:" + varY + "  varZ:" + varZ;
  Serial.println(dataOfTenSec);
//  saveSD(dataOfTenSec);
  delay(10);
//  if(upload){
//    uploadCloud(dataOfTenSec);
//    upload = false;
//  }
  
}

///////////////////////////////////////////////////////
//         The following codes are callbacks.        //
///////////////////////////////////////////////////////

void RFduinoBLE_onConnect() {
  flag = true; 
}

void RFduinoBLE_onDisconnect(){
  flag = false; 
}

void RFduinoBLE_onReceive(char *data, int len){
  uint8_t myByte = data[0]; 
  if(myByte== 66){
    upload = true;
  }
}

///////////////////////////////////////////////////////
//         The following codes are functions.        //
///////////////////////////////////////////////////////

void initADXL345() {
  //Set ADXL345 to measurement mode.
  Wire.beginTransmission(ADXL345_ADDR);
  Wire.write(PWR_CTL_REG);
  Wire.write(8);
  Wire.endTransmission();
  
  //Set ADXL345 to measurement mode.
  Wire.beginTransmission(ADXL345_ADDR);
  Wire.write(0x31);
  Wire.write(8);
  Wire.endTransmission();
}

void readReg(int device_addr, byte reg_addr, int num_byte) {
  Wire.beginTransmission(device_addr);
  Wire.write(reg_addr);
  Wire.endTransmission();
  
  Wire.beginTransmission(device_addr);
  Wire.requestFrom(device_addr, num_byte);

  int i = 0;
  while(Wire.available()){
    buffer[i] = Wire.read();
    i++;  
  }
  Wire.endTransmission();
}

void saveSD(String data){
  Serial.print("AT+SAVSD ");
  Serial.println(data);
  waitOK();
}

void uploadCloud(String data){
  Serial.print("AT+CLOUD ");
  Serial.println(data);
  waitOK();
}

boolean waitOK(){
  char c = 0;
  int ind=0;
  while (true) {
    
    if( Serial.available() ){
      c = (char)Serial.read();
      //Serial.print(c);
      if(c == keyString[ind]){
	ind++;
      }else{
	ind = 0;
      }    
      if( ind == 1 ){
        delay(100);
        return true;
      }
    }
  }
}

// Turn a float number into String 
String f2S(float f) {
  String string;
  string = String(int(100*f));
  string = string.substring(0,2) + '.' + string.substring(2,4);
  return string;
}


void sendValue(float humi1,float humi2,float humi3,float temp1,float temp2,float temp3,int vX,int vY,int vZ,int aX,int aY,int aZ ){
  char buf[15];
  buf[0] = 1;
  
  buf[1]=(int)humi1;
  buf[2]=humi1*100 - ((int)humi1)*100;
  
  buf[3]=(int)humi2;
  buf[4]=humi2*100 - ((int)humi2)*100;
  
  buf[5]=(int)humi3;
  buf[6]=humi3*100 - ((int)humi3)*100;
  
  buf[7]=(int)temp1;
  buf[8]=temp1*100 - ((int)temp1)*100;
  
  buf[9]=(int)temp2;
  buf[10]=temp2*100 - ((int)temp2)*100;
  
  buf[11]=(int)temp3;
  buf[12]=temp3*100 - ((int)temp3)*100;
  
  RFduinoBLE.send(buf, 13);
  
  buf[0] = 2;
  
  buf[1] = (int)(vX/1000);
  buf[2] = (int)(vX/10) - buf[1]*100;
  buf[3] = (int)vX - buf[1]*1000 - buf[2]*10;
  
  buf[4] = (int)(vY/1000);
  buf[5] = (int)(vY/10) - buf[4]*100;
  buf[6] = (int)vY - buf[4]*1000 - buf[5]*10;

  buf[7] = (int)(vZ/1000);
  buf[8] = (int)(vZ/10) - buf[7]*100;
  buf[9] = (int)vZ - buf[7]*1000 - buf[8]*10;
 
  buf[10] = (int)(abs(aX)/100);
  buf[11] = (int)abs(aX) - buf[10]*100;
  if(aX<0){
    buf[10]+=10;}
    
  buf[12] = (int)(abs(aY)/100);
  buf[13] = (int)abs(aY) - buf[12]*100;
  if(aY<0){
    buf[12]+=10;}
    
  buf[14] = (int)(abs(aZ)/100);
  buf[15] = (int)abs(aZ) - buf[14]*100;
  if(aZ<0){
    buf[14]+=10;}
  RFduinoBLE.send(buf, 16);
}
