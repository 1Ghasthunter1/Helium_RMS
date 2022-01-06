#include "LoRaWan_APP.h"
#include "Arduino.h"
#include "softSerial.h"


//================ LORAWAN SETTINGS ================
uint8_t devEui[] = { <FILLMEIN> };
uint8_t appEui[] = { <FILLMEIN> };
uint8_t appKey[] = { <FILLMEIN> };

/* ABP para*/
uint8_t nwkSKey[] = { 0x15, 0xb1, 0xd0, 0xef, 0xa4, 0x63, 0xdf, 0xbe, 0x3d, 0x11, 0x18, 0x1e, 0x1e, 0xc7, 0xda,0x85 };
uint8_t appSKey[] = { 0xd7, 0x2c, 0x78, 0x75, 0x8c, 0xdc, 0xca, 0xbf, 0x55, 0xee, 0x4a, 0x77, 0x8d, 0x16, 0xef,0x67 };
uint32_t devAddr =  ( uint32_t )0x007e6ae1;

/*LoraWan channelsmask, default channels 0-7*/ 
uint16_t userChannelsMask[6]={ 0xFF00,0x0000,0x0000,0x0000,0x0000,0x0000 };

/*LoraWan region, select in arduino IDE tools*/
LoRaMacRegion_t loraWanRegion = ACTIVE_REGION;

/*LoraWan Class, Class A and Class C are supported*/
DeviceClass_t  loraWanClass = LORAWAN_CLASS;

/*the application data transmission duty cycle.  value in [ms].*/
uint32_t appTxDutyCycle = 30000;

/*OTAA or ABP*/
bool overTheAirActivation = LORAWAN_NETMODE;

/*ADR enable*/
bool loraWanAdr = LORAWAN_ADR;

/* set LORAWAN_Net_Reserve ON, the node could save the network info to flash, when node reset not need to join again */
bool keepNet = LORAWAN_NET_RESERVE;

/* Indicates if the node is sending confirmed or unconfirmed messages */
bool isTxConfirmed = LORAWAN_UPLINKMODE;

/* Application port */
uint8_t appPort = 2;
/*!
* Number of trials to transmit the frame, if the LoRaMAC layer did not
* receive an acknowledgment. The MAC performs a datarate adaptation,
* according to the LoRaWAN Specification V1.0.2, chapter 18.4, according
* to the following table:
*
* Transmission nb | Data Rate
* ----------------|-----------
* 1 (first)       | DR
* 2               | DR
* 3               | max(DR-1,0)
* 4               | max(DR-1,0)
* 5               | max(DR-2,0)
* 6               | max(DR-2,0)
* 7               | max(DR-3,0)
* 8               | max(DR-3,0)
*
* Note, that if NbTrials is set to 1 or 2, the MAC will not decrease
* the datarate, in case the LoRaMAC layer did not receive an acknowledgment
*/
uint8_t confirmedNbTrials = 4;

//================ REGULAR SETTINGS ================

static byte panel_voltage_msg[8] = {0xFF, 0x03, 0x01, 0x07, 0x00, 0x01};
static byte panel_current_msg[8] = {0xFF, 0x03, 0x01, 0x08, 0x00, 0x01};
static byte panel_power_msg[8] = {0xFF, 0x03, 0x01, 0x09, 0x00, 0x01};
static byte batt_voltage_msg[8] = {0xFF, 0x03, 0x01, 0x01, 0x00, 0x01}; 
static byte batt_current_msg[8] = {0xFF, 0x03, 0x01, 0x02, 0x00, 0x01};
static byte batt_percent_msg[8] = {0xFF, 0x03, 0x01, 0x00, 0x00, 0x01}; 
static byte load_voltage_msg[8] = {0xFF, 0x03, 0x01, 0x04, 0x00, 0x01}; 
static byte load_current_msg[8] = {0xFF, 0x03, 0x01, 0x05, 0x00, 0x01}; 
static byte load_power_msg[8] = {0xFF, 0x03, 0x01, 0x06, 0x00, 0x01};
static byte load_status_msg[8] = {0xFF, 0x03, 0x01, 0x20, 0x00, 0x01}; 
static byte error_status_msg[8] = {0xFF, 0x03, 0x01, 0x21, 0x00, 0x02}; 

static byte load_on[8] = {0xFF, 0x06, 0x01, 0x0A, 0x00, 0x01}; 
static byte load_off[8] = {0xFF, 0x06, 0x01, 0x0A, 0x00, 0x00}; 

static byte* func_out;
static byte bitmask_chargestatus = 0b01111111;
String temp_str;
char temp[50];

softSerial renogyWnd(GPIO1, GPIO2);

static unsigned long lastMsg = 0;

long loops = 0;
long lastMillis = 0;

uint8_t recvd_downlink = 0;

uint16_t ModRTU_CRC( byte message[], int sizeOfArray)
{
  uint16_t crc = 0xFFFF;
    
  for (int pos = 0; pos < sizeOfArray-2; pos++) {
    crc ^= (uint16_t)message[pos];          // XOR byte into least sig. byte of crc
  
    for (int i = 8; i != 0; i--) {    // Loop over each bit
      if ((crc & 0x0001) != 0) {      // If the LSB is set
        crc >>= 1;                    // Shift right and XOR 0xA001
        crc ^= 0xA001;
      }
      else                            // Else LSB is not set
        crc >>= 1;                    // Just shift right
    }
  }

  byte MSB = (crc >> 8) & 0xFF;
  byte LSB = crc & 0xFF;
  
  // Note, this number has low and high bytes swapped, so use it accordingly (or swap bytes)
  return crc;  
}

byte * querySlave( byte b[], int sizeOfArray ) {
  uint16_t crc = ModRTU_CRC(b, sizeOfArray);
  byte LSB = crc & 0xFF; //comes first
  byte MSB = (crc >> 8) & 0xFF; //then second
  b[sizeOfArray-2] = LSB;
  b[sizeOfArray-1] = MSB;
  
  static byte recvd_data[16] = {};
  
  renogyWnd.write(b, sizeOfArray);
  int counter = 0;
  while ((renogyWnd.available() < 1) && (counter < 10)){
    counter += 1;
    delay(5);
  }
  

  counter = 0;
  while (renogyWnd.available() > 0) {
    recvd_data[counter] = renogyWnd.read();
    counter++;

  }

  return recvd_data;

}

void downLinkDataHandle(McpsIndication_t *mcpsIndication)
{
  for(uint8_t i=0;i<mcpsIndication->BufferSize;i++)
  {
    //printhexbyte(mcpsIndication->Buffer[i]);
  }
  if(mcpsIndication->Buffer[0] == 0x00 && mcpsIndication->Buffer[1] == 0x69){ //Turns on load
    querySlave(load_on, sizeof(load_on));
  }
  else if(mcpsIndication->Buffer[0] == 0x00 && mcpsIndication->Buffer[1] == 0x70){ //Turns on load
    querySlave(load_off, sizeof(load_off));
  }
  recvd_downlink = 1;

  
  uint32_t color=mcpsIndication->Buffer[0]<<16|mcpsIndication->Buffer[1]<<8|mcpsIndication->Buffer[2];
#if(LoraWan_RGB==1)
  turnOnRGB(color,250);
  turnOffRGB();
#endif
}

static void prepareTxFrame( uint8_t port )
{
  /*appData size is LORAWAN_APP_DATA_MAX_SIZE which is defined in "commissioning.h".
  *appDataSize max value is LORAWAN_APP_DATA_MAX_SIZE.0
  *if enabled AT, don't modify LORAWAN_APP_DATA_MAX_SIZE, it may cause system hanging or failure.
  *if disabled AT, LORAWAN_APP_DATA_MAX_SIZE can be modified, the max value is reference to lorawan region and SF.
  *for example, if use REGION_CN470, 
  *the max value for different DR can be found in MaxPayloadOfDatarateCN470 refer to DataratesCN470 and BandwidthsCN470 in "RegionCN470.h".
  */
  appDataSize = 23;
  
  //Sends 9 basic metrics out for each of the nine below functions
  func_out = querySlave(panel_voltage_msg, sizeof(panel_voltage_msg));
  appData[0] = func_out[3];
  appData[1] = func_out[4];

  func_out = querySlave(panel_current_msg, sizeof(panel_current_msg));
  appData[2] = func_out[3];
  appData[3] = func_out[4];

  func_out = querySlave(panel_power_msg, sizeof(panel_power_msg));
  appData[4] = func_out[3];
  appData[5] = func_out[4];

  func_out = querySlave(batt_voltage_msg, sizeof(batt_voltage_msg));
  appData[6] = func_out[3];
  appData[7] = func_out[4];

  func_out = querySlave(batt_current_msg, sizeof(batt_current_msg));
  appData[8] = func_out[3];
  appData[9] = func_out[4];

  func_out = querySlave(batt_percent_msg, sizeof(batt_percent_msg));
  appData[10] = func_out[3];
  appData[11] = func_out[4];

  func_out = querySlave(load_voltage_msg, sizeof(load_voltage_msg));
  appData[12] = func_out[3];
  appData[13] = func_out[4];

  func_out = querySlave(load_current_msg, sizeof(load_current_msg));
  appData[14] = func_out[3];
  appData[15] = func_out[4];
  
  func_out = querySlave(load_power_msg, sizeof(load_power_msg));
  appData[16] = func_out[3];
  appData[17] = func_out[4];


  //gathers information on load status as well as battery charging mode
  func_out = querySlave(load_status_msg, sizeof(load_status_msg));    
  appData[18] = func_out[3];
  appData[19] = func_out[4];


  func_out = querySlave(error_status_msg, sizeof(error_status_msg));
  appData[20] = func_out[3];
  appData[21] = func_out[4];

  appData[22] = recvd_downlink;
  recvd_downlink = 0;
}

void setup()
{
  boardInitMcu();
  //Critical for serial communication
  //pinMode( 14, INPUT );
  // Open USB serial communications
  renogyWnd.begin( 9600 );

#if(AT_SUPPORT)
  enableAt();
#endif
  deviceState = DEVICE_STATE_INIT;
  LoRaWAN.ifskipjoin();



}
void loop() {

  switch( deviceState )
  {
    case DEVICE_STATE_INIT:
    {
#if(AT_SUPPORT)
      getDevParam();
#endif
      printDevParam();
      LoRaWAN.init(loraWanClass,loraWanRegion);
      deviceState = DEVICE_STATE_JOIN;
      break;
    }
    case DEVICE_STATE_JOIN:
    {
      LoRaWAN.join();
      break;
    }
    case DEVICE_STATE_SEND:
    {
      prepareTxFrame( appPort );
      LoRaWAN.send();
      deviceState = DEVICE_STATE_CYCLE;
      break;
    }
    case DEVICE_STATE_CYCLE:
    {
      // Schedule next packet transmission
      txDutyCycleTime = appTxDutyCycle + randr( 0, APP_TX_DUTYCYCLE_RND );
      LoRaWAN.cycle(txDutyCycleTime);
      deviceState = DEVICE_STATE_SLEEP;
      break;
    }
    case DEVICE_STATE_SLEEP:
    {
      LoRaWAN.sleep();
      break;
    }
    default:
    {
      deviceState = DEVICE_STATE_INIT;
      break;
    }
  }
}