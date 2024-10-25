
# Project LongJump:

## Project Overview


## Team Members
1. Nathan Thompson
2. Walter Stahll
3. Jack Ditzel
4. Drew Reisner


## Pre-Alpha

### Current Features
- LCD screen displays website URL to connect to.
- Once a piece is placed on the board (currently being simulated on hardware), the MicroProcessor stores an array of the board state.
- Corresponding LED on the grid lights up based on if a piece is being pressed or not from the array in the MicroProcessor.
- Board state is being sent to the server through Mqtt and displayed on the server terminal.

### Bugs/Know issues
- Piece placement
  - Connections are being read through the analog ports on the arduino, so the data comes in as anolog values (and needs to be converted) instead of digital 0/1.
  - Piece placement is being simulated by using a wire to connect the circuit instead of having physical pieces hover over and being detected.
- Confirm button on the board not currently implimented in code. Currently board state is sent at around 10Hz to server. 
- LED array is physical set up as a 1-D strip instead of being layed out on the 2-D grid board array.
- Wifi/Network credentials are being hard coded for a team members personal hotspot. So, network testing can't be preformed without their phone.
  - Also, the setup doesn't complete until there is a network connection, so other testing can't be done on the board.
- If Wi-Fi is disconnected during active MQTT transmission, no error message is displayed via serial and the Arduino still attempts to send messages
