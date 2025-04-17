import chess
import numpy as np
import json
import paho.mqtt.client as mqtt

#State Diagram:
#https://drive.google.com/file/d/1h9whv9Un0wUjQ-3JaLobKUYU050ZlDQ4/view

board = chess.Board()
physicalBoard = []
uciMove = ['xx', 'xx']        #https://en.wikipedia.org/wiki/Universal_Chess_Interface

# Sessions dictionary
sessions = {}

# Map board IDs to respective sessions
board_to_session = {}

# Initialized new game session
def init_session(gameID, hostID, guestID):
    session = {
        "game_board": chess.Board(),
        "state": "Initial",
        "uciMove": ['xx', 'xx'],
        "host_board": [0] * 64,
        "guest_board": [0] * 64,
        "hostID": hostID,
        "guestID": guestID,
    }
    sessions[gameID] = session
    board_to_session[hostID] = gameID
    board_to_session[guestID] = gameID
    print(f"Initialized session '{gameID}' with host '{hostID}' and guest '{guestID}'")

def on_connect(client, userdata, flags, rc):
    print("Connected to MQTT with rc " + str(rc))
    client.subscribe("server/to/chessServer")
    client.subscribe("boards/to/chess")

def on_message(client, userdata, msg):
    topic = msg.topic
    try:
        payload = json.loads(msg.payload.decode())
    except Exception as e:
        print("Failed to decode message:", e)
        return
    
    # Check if message is from matchmaking server
    if topic == "server/to/chessServer":
        gameID = payload.get("gameID")
        hostID = payload.get("hostID")
        guestID = payload.get("guestID")
        if gameID and hostID and guestID:
            init_session(gameID, hostID, guestID)
        else:
            print("Invalid matchmaking server message received")
    # Board update
    elif topic == "boards/to/chess":
        boardID = payload.get("boardID")
        boardState = payload.get("boardState")
        if boardID is None or boardState is None or len(boardState) != 64:
            print("Invalid board update message received")
            return
        
        # Find game session
        gameID = board_to_session.get(boardID)
        if not gameID:
            print(f"BoardID '{boardID}' not associated with any active session")
            return
        
        session = sessions.get(gameID)
        if not session:
            print(f"Session '{gameID}' not found")
            return
        
        # Find if board is host or guest
        if boardID == session["hostID"]:
            prev_state = session["host_board"]
            board_key = "host_board"
            print(f"Host board '{boardID}' message received")
        elif boardID == session["guestID"]:
            prev_state = session["guest_board"]
            board_key = "guest_board"
            print(f"Guest board '{boardID}' message received")
        else:
            print("BoardID not associated with host or guest in session")
            return
        
        # TODO: process board state change, convert to UCI format



def mqtt_init():
    client = mqtt.Client()
    client.on_connect = on_connect
    client.on_message = on_message
    client.connect("150.136.93.68", 1883, 60)
    client.loop_forever()

state = 'WaitOnMove'
    #ShowOppFrom, ShowOppTo,
    #InvalidOppPickup, ShowValidMoves, InvalidMove,
    #InvalidSecondPickup, ValidCapture, WaitingConfirm,
    #CastlingProgress, EnPassantProgress(not implimented yet)

def main():
    mqtt_init()

    action(chess.parse_square('e2'), True)  #Pick up e2 pawn
    action(chess.parse_square('e4'), False) #Place pawn on e4
    action(0, False, True)                  #Pressed confirm button
    #Other board
    action()
    action(chess.parse_square('e7'), True)  #Pick up e7 pawn
    action(chess.parse_square('e5'), False) #Place pawn on e5
    action(0, False, True)                  #Pressed confirm button


#Inputs MQTT message, 
# compares it to last board update,
# sends to action() if valid change,
# sends message to player if other player's turn
def mqttReciever():
    #if no changes found return
    # debug if 2 or more changes
    print('default')
    # update physicalBoard from mqtt


def action(square: int, isPickedUp: bool, confirmButton = False):
    global state
    global uciMove

    print()
    print('Action: square(' + chess.square_name(square) + 
            '), isPickedUp(' + str(isPickedUp) + 
            '), confirmButton(' + str(confirmButton) + ')')

    #Square validation
    if square < 0 or square >= 64:
        print('ERROR: Invalid square format -> ' + str(square) + ' ('+ chess.square_name(square) + ')')
        return
    
    #Validate board state
    if board.is_valid() == False:
        print('ERROR: board not valid')
        return

    leds = [0]*8*8
    RED = 'r'
    BLINK_RED = 'R'
    BLUE = 'b'
    GREEN = 'g'

    print('Entering state: ' + state)
    #Control Logic for states
    if state == 'WaitOnMove':
        if isPickedUp == True:
            #Make sure player is picking up their piece first
            if board.color_at(square) == board.turn:
                state = 'ShowValidMoves'
            else:
                state = 'InvalidOppPickup'
        else:
            print('ERROR: Player should not be able to place a piece down WaitOnMove state')
    elif state == 'ShowOppFrom':
        move = board.peek()
        #Correct opp piece pickup
        if move.uci()[0:2] == chess.square_name(square):
            state = 'ShowOppTo'
        else:
            print('Warning: player picked up wrong piece for opp turn')
    elif state == 'ShowOppTo':
        move = board.peek()
        if isPickedUp == False:
            #Correct opp piece place down
            if move.uci()[2:4] == chess.square_name(square):
                state = 'WaitOnMove'
            else:
                print('Warning: player placed the wrong piece for opp turn')
    elif state == 'InvalidOppPickup':
        if isPickedUp == False:
            if uciMove[1] == chess.square_name(square):
                state = 'WaitOnMove'
            else:
                print('Warning: player did not place piece back on the correct square')
    elif state == 'ShowValidMoves':
        if isPickedUp == True:
            if board.color_at(square) != board.turn:
                move = chess.Move.from_uci(uciMove[0] + uciMove[1])
                if move in board.legal_moves:
                    state = 'ValidCapture'
                else:
                    state = 'InvalidSecondPickup'
            else:
                #TODO: Is castling possible? -branch
                state = 'InvalidSecondPickup'
        else:
            uciMove[1] = chess.square_name(square)
            move = chess.Move.from_uci(uciMove[0] + uciMove[1])
            if uciMove[0] == chess.square_name(square):
                state = 'WaitOnMove'
            elif move in board.legal_moves:
                state = 'WaitingConfirm'
            else:
                state = 'InvalidMove'
    elif state == 'InvalidMove':
        if isPickedUp == True:
            state = 'ShowValidMoves'
    elif state == 'InvalidSecondPickup':
        if isPickedUp == True:
            state = 'ShowValidMoves'
    elif state == 'ValidCapture':
        if isPickedUp == False:
            move = chess.Move.from_uci(uciMove[0] + chess.square_name(square))
            if move in board.legal_moves:
                state == 'WaitingConfirm'
            else:
                state == 'InvalidMove'
    elif state == 'WaitingConfirm':
        if confirmButton == True:
            state = 'ShowOppFrom'
        elif isPickedUp == True and chess.square_name(square) == uciMove[1]:
            state = 'ShowValidMoves'
    elif state == 'CastlingProgress':
        pass
    elif state == 'EnPassantProgress':
        pass
    else:
        print('ERROR: State \"' + state + '\" not found')

    print('Executing state: ' + state)
    #Execution Logic for states
    if state == 'Initial':
        #TODO: Record previous move and reset
        move = chess.Move.from_uci(uciMove[0] + uciMove[1])
        if move in board.legal_moves:
            board.push(move)
        else:
            print('ERROR: ' + uciMove + ' is not a valid move')
        uciMove = ['xx', 'xx']
        #TODO: Verify physicalBoard matches board
    elif state == 'ShowOppFrom':
        leds[board.peek().from_square] = BLUE
        #TODO: Send message to 'Pick up opp piece'
    elif state == 'ShowOppTo':
        leds[board.peek().from_square] = BLUE
        if board.is_capture(board.peek()):
            leds[board.peek().to_square] = RED
        else:
            leds[board.peek().to_square] = GREEN
        #TODO: Send message to 'Move/Capture opp\'s piece'
    elif state == 'WaitOnMove':
        if board.is_check():
            king = chess.square(board.king(board.turn))
            leds[king] = RED
        #TODO: Send message to 'Make move'
    elif state == 'InvalidOppPickup':
        leds[square] = BLINK_RED
        #TODO: Send message to 'Place down Opp piece'
        if isPickedUp == True:
            uciMove[1] = chess.square_name(square)
    elif state == 'ShowValidMoves':
        for move in board.legal_moves:
            if move.from_square == square:
                if board.is_capture(move):
                    #print('Capture: ' + str(move))
                    leds[move.to_square] = RED
                else:
                    #print('Move: ' + str(move))
                    leds[move.to_square] = GREEN
        leds[move.from_square] = BLUE
        uciMove[0] = str(chess.square_name(square))
    elif state == 'InvalidMove':
        #TODO: Send message to 'Invalid move, pick up piece'
        leds[square] = BLINK_RED
    elif state == 'InvalidSecondPickup':
        #TODO: Send message to 'Invalid pick up, place piece down'
        leds[square] = BLINK_RED
    elif state == 'ValidCapture':
        #TODO: Send message to 'Place down your piece over capture'
        leds[square] = GREEN
    elif state == 'WaitingConfirm':
        #TODO: Send message to 'Press confirm button'
        pass
    elif state == 'CastlingProgress':
        pass
    elif state == 'EnPassantProgress':
        pass
    else:
        print('ERROR: State \"' + state + '\" not found')

    print(leds)
        

main()
