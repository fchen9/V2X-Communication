#include <SPI.h>
#include <RF22.h>
#include <RF22Router.h>
#include <string.h>
#include <NewPing.h>

//Sonar Constants
#define TRIGGER_PIN_SONAR 6
#define ECHO_PIN_SONAR 5
#define MAX_DISTANCE_SONAR 400
NewPing sonar(TRIGGER_PIN_SONAR, ECHO_PIN_SONAR, MAX_DISTANCE_SONAR); // NewPing setup of pins and

#define SOURCE_ADDRESS 1
#define SOURCE_ADDRESS1 2
#define SOURCE_ADDRESS2 0
#define DESTINATION_ADDRESS 1
#define DESTINATION_ADDRESS1 2
#define DESTINATION_ADDRESS2 0

RF22Router rf22(SOURCE_ADDRESS);


float throughput = 0;
int flag_measurement = 0;
int max_counter = 2;
int number_of_bytes = 0;
int current_number_of_bytes = 0;

long x, y, z, G;
float x1, g, y1, z1, x2, y2, z2;



void setup() {
  Serial.begin(9600);
  //Transmitter Router
  if (!rf22.init())
    Serial.println("RF22T init failed");
  // Defaults after init are 434.0MHz, 0.05MHz AFC pull-in, modulation FSK_Rb2_4Fd36
  if (!rf22.setFrequency(431.0))
    Serial.println("setFrequency Fail");
  rf22.setTxPower(RF22_TXPOW_20DBM);
  //1,2,5,8,11,14,17,20 DBM
  rf22.setModemConfig(RF22::OOK_Rb40Bw335  );
  //modulation

  // Manually define the routes for this network
  rf22.addRouteTo(DESTINATION_ADDRESS1, DESTINATION_ADDRESS1);
  rf22.addRouteTo(DESTINATION_ADDRESS2, DESTINATION_ADDRESS2);
  rf22.addRouteTo(SOURCE_ADDRESS1, SOURCE_ADDRESS1);
  rf22.addRouteTo(SOURCE_ADDRESS2, SOURCE_ADDRESS2);


  pinMode(7, OUTPUT);
  pinMode(4, OUTPUT);

  x = analogRead(0);       // read analog input pin 0
  y = analogRead(1);       // read analog input pin 1
  z = analogRead(2); // read analog input pin 3
  G = sq(x) + sq(y) + sq(z);
  g = sqrt(G);

  randomSeed(millis());
}

void loop() {
  int counter = 0;
  int initial_time = millis();
  int final_time = 0;
  number_of_bytes = 0;


  //Receive & Send

  //Receive
  int timeReceive = random(1000, 2000);
  int current_time = millis();
  while ( current_time - initial_time <= timeReceive) {
    char* received_value = 0;
    // Should be a message for us now
    uint8_t buf[RF22_ROUTER_MAX_MESSAGE_LEN];
    char incoming[RF22_ROUTER_MAX_MESSAGE_LEN];
    memset(buf, '\0', RF22_ROUTER_MAX_MESSAGE_LEN);
    memset(incoming, '\0', RF22_ROUTER_MAX_MESSAGE_LEN);
    uint8_t len = sizeof(buf);
    uint8_t from;
    if (rf22.recvfromAck(buf, &len, &from))
    {
      buf[RF22_ROUTER_MAX_MESSAGE_LEN - 1] = '\0';
      memcpy(incoming, buf, RF22_ROUTER_MAX_MESSAGE_LEN);
      //  Serial.print("got request from : ");
      //  Serial.println(from, DEC);
      received_value = ((char*)incoming);
      String receivedString = String(received_value);
      Serial.println(receivedString);
      String first_string = getValue(receivedString, ' ', 1);
      String second_string = getValue(receivedString, ' ', 3);
      String third_string = getValue(receivedString, ' ', 5);
      String forth_string = getValue(receivedString, ' ', 7);
      int id = first_string.toInt();
      int dist = second_string.toInt();
      int acc_x = 0;
      int acc_y = 0;
      int color = 0;
      int hum = 0;
      if (id >= 1) {
        acc_x = third_string.toInt() / 100;
        acc_y = forth_string.toInt() / 100;
      }
      if (id == 0) {
        color = third_string.toInt();
        hum = forth_string.toInt();
        if (color==1) {
          digitalWrite(7, HIGH);
          digitalWrite(4, LOW);
        }
        else {
          digitalWrite(7, LOW);
          digitalWrite(4, HIGH);
        }
        if (hum > 20) {
          Serial.println("Warning");
        }
      }


    }
    current_time = millis();
  }
  //Send
  for (counter = 0; counter < max_counter; counter++)
  {
    //Sensors Acquiring Data
    unsigned int sonarResult = sonar.ping_cm();
    x1 = analogRead(0);       // read analog input pin 0
    y1 = analogRead(1);       // read analog input pin 1
    z1 = analogRead(2);       // read analog input pin 2
    x2 = 100 * x1 / g;
    y2 = 100 * y1 / g;
    z2 = 100 * z1 / g;
   
    char data_read[RF22_ROUTER_MAX_MESSAGE_LEN];
    uint8_t data_send[RF22_ROUTER_MAX_MESSAGE_LEN];
    memset(data_read, '\0', RF22_ROUTER_MAX_MESSAGE_LEN);
    memset(data_send, '\0', RF22_ROUTER_MAX_MESSAGE_LEN);
    sprintf(data_read, "ID %d d %d x %d y %d", SOURCE_ADDRESS, sonarResult, (int)x2, (int)y2);
    data_read[RF22_ROUTER_MAX_MESSAGE_LEN - 1] = '\0';
    memcpy(data_send, data_read, RF22_ROUTER_MAX_MESSAGE_LEN);
    current_number_of_bytes = sizeof(data_send);
    //if (rf22.sendtoWait(data_send, sizeof(data_send), DESTINATION_ADDRESS1) != RF22_ROUTER_ERROR_NONE) || rf22.sendtoWait(data_send, sizeof(data_send), DESTINATION_ADDRESS2) != RF22_ROUTER_ERROR_NONE) )
        if (((rf22.sendtoWait(data_send, sizeof(data_send), DESTINATION_ADDRESS1) != RF22_ROUTER_ERROR_NONE) || (rf22.sendtoWait(data_send, sizeof(data_send), DESTINATION_ADDRESS2) != RF22_ROUTER_ERROR_NONE)))
    {
      Serial.println("sendtoWait failed");
    }
    else
    {
      Serial.println("sendtoWait Successful");

    }
  }



}

/////////////////////Other Functions////////////////////
String getValue(String data, char separator, int index)
{
  int found = 0;
  int strIndex[] = { 0, -1 };
  int maxIndex = data.length() - 1;

  for (int i = 0; i <= maxIndex && found <= index; i++) {
    if (data.charAt(i) == separator || i == maxIndex) {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }
  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}



char* string2char(String command) {
  if (command.length() != 0) {
    char *p = const_cast<char*>(command.c_str());
    return p;
  }
}

////////////////Sensor Functions//////////////////////////

