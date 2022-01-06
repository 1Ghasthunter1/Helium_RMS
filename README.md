# Helium Off-Grid RMS
A system to communicate with Renogy solar charge controllers often used in **Off-Grid Helium Hotspots**.

*Note: this ReadMe intentionally does not share all necessary information for an individual to set up their own Helium Off-Grid RMS setup. This is an education ReadMe to demonstrate the purpose of the device and foreshadow a future complete release*

### Features
- Runs for 3 days after a station has died via internal backup battery and LoRaWAN transmitting
- Helium network integration
- Battery, solar, and load statistics
- Error code monitoring
- Remote load control
- Can be integrated into any dashboard via Helium Console

### Tested Devices
|Device                |Tested                          |Predicted                         |
|----------------|-------------------------------|-----------------------------|
|Renogy Wanderer|`Yes`            |Yes            |
|Renogy Rover          |`No`            |Yes            |


### Hardware
1x HTCC-AB01 CubeCell Dev Board
1x JST-SH1.25 LiPo battery
1x RS232 to TTL converter

### How it works
1. The controller sends out data requests via the built-in RS232 RJ12 port on the Renogy controller and receives information like solar charge, load status, and error messages
2. The controller transmits the data over the LoRaWAN Helium Network 
3. The controller checks for incoming messages (downlinks) from the Helium network, like a request to turn load on or off
4. The device sleeps, and repeats the cycle approximately every thirty seconds.

### Additional Information
To route the data out of the Helium network, an integration and decoder function must be in place. For the time being, those are being kept internal, however running data through an MQTT integration with a Node-RED dashboard has worked well and is shown in pictures below.

### Setup

Within Miner_RMS.ino, change any line with `<REPLACE_ME>`to the requested information. There are only three items that need configuring at the top of the script, which are the `dev EUI`, `app EUI`, and `app key` from the Helium Console.

After uploading the code to your Arduino, your device should register within the helium network and begin transmitting data every thirty seconds.
