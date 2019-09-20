#include <SPI.h>
#include <RF22.h>
#include <RF22Router.h>
#include <string.h>
#include <NewPing.h>

//Sonar Constants
#define TRIGGER_PIN_SONAR 6
#define ECHO_PIN_SONAR 7
#define MAX_DISTANCE_SONAR 400
NewPing sonar(TRIGGER_PIN_SONAR, ECHO_PIN_SONAR, MAX_DISTANCE_SONAR); // NewPing setup of pins and

#define SOURCE_ADDRESS 0
#define SOURCE_ADDRESS1 1
#define SOURCE_ADDRESS2 2
#define DESTINATION_ADDRESS 0
#define DESTINATION_ADDRESS1 1
#define DESTINATION_ADDRESS2 2

RF22Router rf22(SOURCE_ADDRESS);


float throughput = 0;
int flag_measurement = 0;
int max_counter = 2;
int number_of_bytes = 0;
int current_number_of_bytes = 0;
int flag_ALOHA_SUCCESS = 0;
int aloha_delay = 0;
int initial_time = 0;
const int hygrometer = A0;
int hum_value;


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


  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);


  randomSeed(millis());
  initial_time = millis();
}

void loop() {
  int counter = 0;
  int final_time = 0;
  number_of_bytes = 0;


  //Send
  for (counter = 0; counter < max_counter; counter++)
  {
    flag_ALOHA_SUCCESS = 0;
    int color = 1;
    int current_time = millis();

    while (flag_ALOHA_SUCCESS == 0)
    {
      //Sensors Acquiring Data
      unsigned int sonarResult = sonar.ping_cm();
      //humidity
      hum_value = analogRead(hygrometer);
      hum_value = constrain(hum_value, 300, 1023);
      hum_value = map(hum_value, 300, 1023, 100, 0);

      char data_read[RF22_ROUTER_MAX_MESSAGE_LEN];
      uint8_t data_send[RF22_ROUTER_MAX_MESSAGE_LEN];
      memset(data_read, '\0', RF22_ROUTER_MAX_MESSAGE_LEN);
      memset(data_send, '\0', RF22_ROUTER_MAX_MESSAGE_LEN);
      sprintf(data_read, "ID %d d %d c %d h %d", SOURCE_ADDRESS, sonarResult, color, hum_value);
      data_read[RF22_ROUTER_MAX_MESSAGE_LEN - 1] = '\0';
      memcpy(data_send, data_read, RF22_ROUTER_MAX_MESSAGE_LEN);
      current_number_of_bytes = sizeof(data_send);

      int current_time = millis();
      if ((abs(current_time) - initial_time) % 17000 <= 10000) {
        color = 1;
      }
      else if ((abs(current_time) - initial_time) % 17000 > 10000) {
        color = 0;
      }
      Serial.println(current_time);

      if (color == 0) {
        digitalWrite(4, HIGH);
        digitalWrite(5, LOW);
      }
      else if (color == 1) {
        digitalWrite(5, HIGH);
        digitalWrite(4, LOW);
      }
      if (((rf22.sendtoWait(data_send, sizeof(data_send), DESTINATION_ADDRESS1) != RF22_ROUTER_ERROR_NONE) || (rf22.sendtoWait(data_send, sizeof(data_send), DESTINATION_ADDRESS2) != RF22_ROUTER_ERROR_NONE)) && ((rf22.sendtoWait(data_send, sizeof(data_send), DESTINATION_ADDRESS2) != RF22_ROUTER_ERROR_NONE) || (rf22.sendtoWait(data_send, sizeof(data_send), DESTINATION_ADDRESS1) != RF22_ROUTER_ERROR_NONE)) )
        //if ((rf22.sendtoWait(data_send, sizeof(data_send), DESTINATION_ADDRESS1) != RF22_ROUTER_ERROR_NONE) || (rf22.sendtoWait(data_send, sizeof(data_send), DESTINATION_ADDRESS2) != RF22_ROUTER_ERROR_NONE))
      {
        Serial.println("sendtoWait failed");
      }
      else
      {
        Serial.println("sendtoWait Successful");
        flag_ALOHA_SUCCESS == 1;
      }
      if (flag_ALOHA_SUCCESS == 0)
      {
        aloha_delay = random(400);
        delay(aloha_delay);
      }
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

