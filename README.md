
# Project LongJump:

## Team Members
1. Nathan Thompson
2. Walter Stahll
3. Jack Ditzel
4. Drew Reisner

## Project Overview
LongJump is an electronic board that connects to other LongJump board to play classic boardgames such as checkers, chess, and connect 4.
Currently we have relased our prototype build with features listed below. To start a match or view board internet connection instructions visit: https://longjump.ip-dynamic.org/

## Prototype

### Build instructions
- Install the Aruino IDE at version 2.3.3 or later.
- Install the following libraries from the Arduino library manager:
  - ArduinoMqttCliet
  - LiquidCrystal
  - Adafruit NeoPixel
  - ArduinoJson
  - WiFiProvisioner
- Select the ESP32 Dev Module as the board type.
- Make sure the ESP32 is plugged in and build the application.
- Once build visit the link displayed on the screen or https://longjump.ip-dynamic.org/ to connect the board to the internet and website.

### Board Hardware:
- Each board is a 8x8 grid of tiles and contains the following features:
  - LCD screen displays website URL to connect to. When game is initialized, LCD updates to display useful game information such as turn info or if an invalid move was made.
  - MicroProcessor to connect to the internet and operate the circitry.
  - Level shifters to enhance the brightness of the LEDs and detection range of the hall effect sensor.
  - Each board is scanned like a keyboard where the processor cycles powering each row of the grid and recording the outputs of each column.
- Tiles:
  - 16x16 grid of RGB LEDs used to create 8x8 board (2x2 LEDs represents 1 tile).
  - For the output response to the user, each tile has a programmable RGB LED. The LEDs are programmed as a 1d array and weaved in a snake like pattern on the board and software is used to interface with the spesific index.
  - For the input response from the user, each tile consists of a hall effect sensor which measures magnetic fields (to detect the pieces) and a diode to make sure only the correct connections are reported.
- Pieces:
  - Each piece is 3d printed with a hole in the bottom that has a magnet in it with the polarity of the magnet being in the direction that is detectable by the hall effect sensor.

### Software MicroProcessor
- When a user attempts to connect the board to the internet, the board appears as reciever. So when the user uses one of their devices to select the board as the network to connect to, a pop up appears (sent from the Microprocessor) to connect to the actual network. Once all the credentials are filled in, the board connects to the local network.
- Once a piece is placed on the board over the hall effect sensor, the MicroProcessor scans the board and stores an array of the board state as a boolean whenever or not there is a piece over the tile.
- The board transmits the board state to the server via MQTT (to be sent later sent to the guest board).
- When the board recieves the board state (of the host board) from the server via MQTT, it lights up the the correspinging LEDs to the board state.

### Oracle Backend
- Hosts listener to add newly connected board IDs to JSON database.
- Facilitates initial board-to-board connection process.
- Provides host-to-guest board state transmission over MQTT.

### Website Frontend
- Provides instructions to complete WiFi provisioning process.
- Allows users to initialize game by selecting "Host" or "Guest" role and entering respective board IDs.

### Bugs/Known issues
- The board mirroring the other board is only one directional currently. So the recieving board doesn't transmit it's state to the other board.
- Piece placement
  - Connections are being read through the analog ports on the arduino, so the data comes in as anolog values (and needs to be converted) instead of digital 0/1.
  - Because there is no shell/case above the circitry yet, pieces tend to fall off the sensor when placed on the board so they must be held for the sensor to detect the piece.
- Confirm button on the board not currently implimented in code. Currently board state is sent at around 10Hz to server. 
- The board doesn't complete setup until there is a network connection, so other board functionality can't be tested on the board without an established connecting.
- If Wi-Fi is disconnected during active MQTT transmission, no error message is displayed via serial and the Arduino still attempts to send messages.
- Website must be reloaded to start a new game, visual bugs result if trying to start a new session after one has already been started without refreshing.
