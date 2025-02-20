import chess
import numpy as np
import json
import paho.mqtt.client as mqtt
import threading

#State Diagram:
#https://drive.google.com/file/d/1h9whv9Un0wUjQ-3JaLobKUYU050ZlDQ4/view

#board = chess.Board()
physicalBoard = []
uciMove = ['xx', 'xx']        #https://en.wikipedia.org/wiki/Universal_Chess_Interface
client = mqtt.Client()

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
        "host_board": [1,1,1,1,1,1,1,1,
                       1,1,1,1,1,1,1,1,
                       0,0,0,0,0,0,0,0,
                       0,0,0,0,0,0,0,0,
                       0,0,0,0,0,0,0,0,
                       0,0,0,0,0,0,0,0,
                       1,1,1,1,1,1,1,1,
                       1,1,1,1,1,1,1,1],
        "guest_board": [1,1,1,1,1,1,1,1,
                       1,1,1,1,1,1,1,1,
                       0,0,0,0,0,0,0,0,
                       0,0,0,0,0,0,0,0,
                       0,0,0,0,0,0,0,0,
                       0,0,0,0,0,0,0,0,
                       1,1,1,1,1,1,1,1,
                       1,1,1,1,1,1,1,1],
        "hostID": hostID,
        "guestID": guestID,
        "picked_up": False,
    }
    sessions[gameID] = session
    board_to_session[hostID] = gameID
    board_to_session[guestID] = gameID
    sendMessage(sessions[gameID], -1, "chess_start")
    print(f"Initialized session '{gameID}' with host '{hostID}' and guest '{guestID}'")

def on_connect(client, userdata, flags, rc):
    print("Connected to MQTT with rc " + str(rc))
    client.subscribe("server/to/chessServer")
    client.subscribe("boards/to/chess")

# Convert index 0-63 to uci
def index_to_chess_square(index, is_guest=False):
    if is_guest:
        index = 63 - index
    file = chr(ord('a') + (index % 8))  # 'a' to 'h'
    rank = str(8 - (index // 8))  # '1' to '8'
    return file + rank


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
        boardID = payload.get("playerID")
        boardState = payload.get("boardState")
        confirmButton = payload.get("confirmButton")
        if boardID is None or boardState is None or len(boardState) != 64:
            print("Invalid board update message received")
            return

        # Find game session
        gameID = board_to_session.get(str(boardID))
        if not gameID:
            print(f"ERROR: BoardID '{boardID}' not associated with any active session")
            return
        
        session = sessions.get(gameID)
        if not session:
            print(f"Session '{gameID}' not found")
            return

        if session['game_board'].turn == chess.WHITE and str(boardID) == session['guestID']:
            print("WARNING: Guest message received on host turn")
            return

        if session['game_board'].turn == chess.BLACK and str(boardID) == session['hostID']:
            print("WARNING: Host message received on guest turn")
            return


        # Find if board is host or guest
        if str(boardID) == session["hostID"]:
            print(f"Host board '{boardID}' message received.")
            print(np.reshape(boardState, (8, 8))[::-1])
            previous_board = session['host_board']
            board_changed = 'host'
        elif str(boardID) == session["guestID"]:
            print(f"Guest board '{boardID}' message received.")
            # flip host board
            boardState = np.array(boardState).reshape(8, 8)[::-1, ::-1].flatten().tolist()
            print(np.reshape(boardState, (8, 8))[::-1])
            previous_board = session['guest_board']
            board_changed = 'guest'
        else:
            print("BoardID not associated with host or guest in session")
            return

        # Detect board changes
        changed_positions = []
        picked_up_square = None
        placed_square = None

        if confirmButton:
            if board_changed == 'host':
                action(session["hostID"], 0, session["picked_up"], True)
            else:
                action(session["guestID"], 0, session["picked_up"], True)
            return

        for i in range(64):
            if previous_board[i] != boardState[i]:
                changed_positions.append((i, boardState[i]))
                is_guest = (board_changed == 'guest')
                uci_square = index_to_chess_square(i, is_guest=is_guest)

                if previous_board[i] == 1 and boardState[i] == 0:
                    # Piece removed
                    picked_up_square = uci_square
                    session["picked_up"] = True
                elif previous_board[i] == 0 and boardState[i] == 1:
                    # Piece placed
                    placed_square = uci_square
                    session["picked_up"] = False

        if changed_positions:
            print("Board changes detected:")
            for idx, val in changed_positions:
                print(f" - Index {idx}: New value {val} ({index_to_chess_square(idx)})")

            if picked_up_square:
                print(f"Piece picked up from: {picked_up_square}")

            if placed_square:
                print(f"Piece placed on: {placed_square}")

            # Update session board state
            if board_changed == 'host':
                print("Updating board state of HOST board")
                session["host_board"] = boardState
                board_id = session['hostID']
            else:
                print("Updating board state of GUEST board")
                session["guest_board"] = boardState
                board_id = session['guestID']

            if picked_up_square:
                action(board_id, chess.parse_square(picked_up_square), session["picked_up"])
                return

            if placed_square:
                action(board_id, chess.parse_square(placed_square), session["picked_up"])
                return
            


def mqtt_init():
    client.on_connect = on_connect
    client.on_message = on_message
    client.connect("150.136.93.68", 1883, 60)
    
    #mqtt_thread = threading.Thread(target=client.loop_start)
    #mqtt_thread.daemon = True
    #mqtt_thread.start()

    client.loop_forever()

#state = 'WaitOnMove'
    #ShowOppFrom, ShowOppTo, InvalidOppFrom
    #InvalidOppPickup, ShowValidMoves, InvalidMove,
    #InvalidSecondPickup, ValidCapture, WaitingConfirm,
    #CastlingProgress, EnPassantProgress(not implimented yet)

def main():
    mqtt_init()
    #init_session('111111-111112', 111111, 111112)
    #input("Run test1()...")
    #test1()
    #test2()
    #for i in range(12):
    #    debugBoard(i)

#Inputs MQTT message, 
# compares it to last board update,
# sends to action() if valid change,
# sends message to player if other 
# player's turn.
def mqttReciever():
    #if no changes found return
    # debug if 2 or more changes
    print('default')
    # update physicalBoard from mqtt

def sendMessage(session, ledArray, message):
    if session['state'] == 'Initial' and ledArray == -1 and message == 'chess_start':
        print('Initializing host and guest board via MQTT')
        # Send initialization message to host and guest
        message = 'chess_start_host'
        topic = f"board/{session['hostID']}"
        payload = json.dumps({
            "message": message
        })
        client.publish(topic, payload)

        message = 'chess_start_guest'
        topic = f"board/{session['guestID']}"
        payload = json.dumps({
            "message": message
        })
        client.publish(topic, payload)
        session['state'] = 'WaitOnMove'
        return

    #Validate ledArray
    if len(ledArray) != 64:
        print('ERROR: invalid ledArray length ' + len(ledArray))
        return

    print('Message: ' + str(message))
    print(np.reshape(ledArray, (8, 8))[::-1])

    if session['game_board'].turn == chess.WHITE:
        #TODO: send message to host board
        print("Host board modified")
        topic = f"board/{session['hostID']}"
        payload = json.dumps({
            "ledArray": ledArray,
            "message": message
        })
        client.publish(topic, payload)
    elif session['game_board'].turn == chess.BLACK:
        #TODO: send message to guest board
        print("Guest board modified")
        topic = f"board/{session['guestID']}"
        payload = json.dumps({
            "ledArray": ledArray,
            "message": message
        })
        client.publish(topic, payload)
    else:
        print('ERROR: invalid player turn color when sending message')


#Cycle through each board piece to
# make sure user has board correct.
#return True if debug mode completed
def debugBoard(boardID, stage: int):
    session = sessions[board_to_session[boardID]]
    if not session:
        print(f"ERROR: Session with boardID '{boardID}' not found")
        return
    else:
        print(f"Session found with hostID '{session['hostID']}'")

    #Validate board state
    if session['game_board'].is_valid() == False:
        print('ERROR: board not valid')
        return True

    leds = ['0']*8*8
    squares = []
    name = ''
    #White Pawns
    if stage == 0:
        squares = session['game_board'].pieces(chess.PAWN, chess.WHITE)
        name = 'White Pawn'
    elif stage == 1:
        squares = session['game_board'].pieces(chess.ROOK, chess.WHITE)
        name = 'White Rook'
    elif stage == 2:
        squares = session['game_board'].pieces(chess.KNIGHT, chess.WHITE)
        name = 'White Knight'
    elif stage == 3:
        squares = session['game_board'].pieces(chess.BISHOP, chess.WHITE)
        name = 'White Bishop'
    elif stage == 4:
        squares = session['game_board'].pieces(chess.KING, chess.WHITE)
        name = 'White King'
    elif stage == 5:
        squares = session['game_board'].pieces(chess.QUEEN, chess.WHITE)
        name = 'White Queen'
    elif stage == 6:
        squares = session['game_board'].pieces(chess.PAWN, chess.BLACK)
        name = 'Black Pawn'
    elif stage == 7:
        squares = session['game_board'].pieces(chess.ROOK, chess.BLACK)
        name = 'Black Rook'
    elif stage == 8:
        squares = session['game_board'].pieces(chess.KNIGHT, chess.BLACK)
        name = 'Black Knight'
    elif stage == 9:
        squares = session['game_board'].pieces(chess.BISHOP, chess.BLACK)
        name = 'Black Bishop'
    elif stage == 10:
        squares = session['game_board'].pieces(chess.KING, chess.BLACK)
        name = 'Black King'
    elif stage == 11:
        squares = session['game_board'].pieces(chess.QUEEN, chess.BLACK)
        name = 'Black Queen'
    else:
        print('ERROR: Stage not found -> ' + str(stage))
        return True

    if len(squares) == 0:
        name += ' (none)'
    for square in squares:
        leds[square] = 'b'
        #print(chess.square_name(square))
    sendMessage(leds, name)


def action(boardID, square: int, isPickedUp: bool, confirmButton = False):
    global uciMove

    session = sessions[board_to_session[boardID]]
    if not session:
        print(f"ERROR: Session with boardID '{boardID}' not found")
        return
    else:
        print(f"Session found with hostID '{session['hostID']}'")

    #Square validation
    if square < 0 or square >= 64:
        print('ERROR: Invalid square index -> ' + str(square))
        return
    
    #Validate board state
    if session['game_board'].is_valid() == False:
        print('ERROR: board not valid')
        return

    print()
    print('Action: square(' + chess.square_name(square) + 
            '), isPickedUp(' + str(isPickedUp) + 
            '), confirmButton(' + str(confirmButton) + ')')

    leds = ['0']*8*8
    message = ''
    RED = 'r'
    BLINK_RED = 'R'
    BLUE = 'b'
    GREEN = 'g'

    print('Entering state: ' + session['state'])
    #Control Logic for states
    if session['state'] == 'WaitOnMove':
        if isPickedUp == True:
            #Make sure player is picking up their piece first
            if session['game_board'].color_at(square) == session['game_board'].turn:
                session['state'] = 'ShowValidMoves'
            elif str(session['game_board'].color_at(square)) != 'None':
                session['state'] = 'InvalidOppPickup'
            else:
                print('Warning: Player should not be able to pick piece from an empty square')
        else:
            print('Warning: Player should not be able to place a piece down WaitOnMove state')
    elif session['state'] == 'ShowOppFrom':
        move = session['game_board'].peek()
        #Correct opp piece pickup
        if move.uci()[0:2] == chess.square_name(square):
            session['state'] = 'ShowOppTo'
        else:
            session['state'] = 'InvalidOppFrom'
            print('Warning: player picked up wrong piece for opp turn')
            print(f"Correct opp pickup piece: {move.uci()[0:2]}")
            print(f"Attempted move: {chess.square_name(square)}")
    elif session['state'] == 'ShowOppTo':
        move = session['game_board'].peek()
        if isPickedUp == False:
            #Correct opp piece place down
            if move.uci()[2:4] == chess.square_name(square):
                session['state'] = 'WaitOnMove'
            else:
                print('Warning: player placed the wrong piece for opp turn')
    elif session['state'] == 'InvalidOppFrom':
        move = session['game_board'].peek()
        #Correct opp piece pickup
        if move.uci()[0:2] == chess.square_name(square):
            session['state'] = 'ShowOppTo'
        else:
            print('Warning: player picked up wrong piece for opp turn')
    elif session['state'] == 'InvalidOppPickup':
        if isPickedUp == False:
            if uciMove[1] == chess.square_name(square):
                session['state'] = 'WaitOnMove'
            else:
                print('Warning: player did not place piece back on the correct square')
    elif session['state'] == 'ShowValidMoves':
        uciMove[1] = chess.square_name(square)
        move = chess.Move.from_uci(uciMove[0] + uciMove[1])
        if isPickedUp == True:
            if session['game_board'].color_at(square) != session['game_board'].turn:
                move = chess.Move.from_uci(uciMove[0] + uciMove[1])
                if move in session['game_board'].legal_moves:
                    session['state'] = 'ValidCapture'
                else:
                    session['state'] = 'InvalidSecondPickup'
            else:
                #TODO: Is castling possible? -branch
                session['state'] = 'InvalidSecondPickup'
        else:
            if uciMove[0] == chess.square_name(square):
                session['state'] = 'WaitOnMove'
            elif move in session['game_board'].legal_moves:
                session['state'] = 'WaitingConfirm'
            else:
                session['state'] = 'InvalidMove'
    elif session['state'] == 'InvalidMove':
        if isPickedUp == True:
            session['state'] = 'ShowValidMoves'
    elif session['state'] == 'InvalidSecondPickup':
        if isPickedUp == True:
            session['state'] = 'ShowValidMoves'
    elif session['state'] == 'ValidCapture':
        if isPickedUp == False and chess.square_name(square) == uciMove[1]:
            move = chess.Move.from_uci(uciMove[0] + uciMove[1])
            if move in session['game_board'].legal_moves:
                session['state'] = 'WaitingConfirm'
            else:
                session['state'] = 'InvalidMove'
        else:
            print('Warning: user messed something up at ValidCapture')
    elif session['state'] == 'WaitingConfirm':
        if confirmButton == True:
            move = chess.Move.from_uci(uciMove[0] + uciMove[1])
            if move in session['game_board'].legal_moves:
                #Turn completed
                print('Pushing move')
                session['game_board'].push(move)
                uciMove = ['xx', 'xx']
                print(session['game_board'])
                if session['game_board'].is_game_over() == True:
                    print(session['game_board'].outcome())
                session['state'] = 'ShowOppFrom'
            else:
                print('ERROR: ' + uciMove + ' is not a valid move')
        elif isPickedUp == True and chess.square_name(square) == uciMove[1]:
            session['state'] = 'ShowValidMoves'
    elif session['state'] == 'CastlingProgress':
        pass
    elif session['state'] == 'EnPassantProgress':
        pass
    else:
        print('ERROR: State \"' + session['state'] + '\" not found')

    print('Executing state: ' + session['state'])
    #Execution Logic for states
    if session['state'] == 'WaitOnMove':
        if session['game_board'].is_check():
            king = chess.square(session['game_board'].king(session['game_board'].turn))
            leds[king] = RED
        message = 'Make move'
    elif session['state'] == 'ShowOppFrom':
        leds[session['game_board'].peek().from_square] = BLUE
        message = 'Pick up opp piece'
    elif session['state'] == 'ShowOppTo':
        leds[session['game_board'].peek().from_square] = BLUE
        if session['game_board'].is_capture(session['game_board'].peek()):
            leds[session['game_board'].peek().to_square] = RED
            message = 'Capture opp\'s piece'
        else:
            leds[session['game_board'].peek().to_square] = GREEN
            message = 'Move opp\'s piece'
    elif session['state'] == 'InvalidOppFrom':
        leds[square] = RED
        message = 'Pick up opp piece'
    elif session['state'] == 'InvalidOppPickup':
        leds[square] = BLINK_RED
        message = 'Place down Opp piece'
        if isPickedUp == True:
            uciMove[1] = chess.square_name(square)
    elif session['state'] == 'ShowValidMoves':
        for move in session['game_board'].legal_moves:
            if move.from_square == square:
                if session['game_board'].is_capture(move):
                    #print('Capture: ' + str(move))
                    leds[move.to_square] = RED
                else:
                    #print('Move: ' + str(move))
                    leds[move.to_square] = GREEN
        uciMove[0] = str(chess.square_name(square))
        leds[square] = BLUE
        message = 'Make move'
    elif session['state'] == 'InvalidMove':
        message = 'Invalid move, pick up piece'
        leds[square] = BLINK_RED
    elif session['state'] == 'InvalidSecondPickup':
        message = 'Invalid pick up, place piece down'
        leds[square] = BLINK_RED
    elif session['state'] == 'ValidCapture':
        message = 'Place down your piece over capture'
        leds[square] = GREEN
    elif session['state'] == 'WaitingConfirm':
        message = 'Press confirm button'
        pass
    elif session['state'] == 'CastlingProgress':
        pass
    elif session['state'] == 'EnPassantProgress':
        pass
    else:
        print('ERROR: State \"' + session['state'] + '\" not found')

    #Send to board
    sendMessage(session, leds, message)
    

def test1(): #Scholar's mate
    input("test1 action: 1")
    action(111111, chess.parse_square('e2'), True)  #Pick up e2 pawn
    input("test1 action: 2")
    action(111111, chess.parse_square('e4'), False) #Place pawn on e4
    input("test1 action: 3")
    action(111111, 0, False, True)                  #Pressed confirm button

    input("test1 action: 4")
    action(111112, chess.parse_square('e2'), True)  #Pick up opp piece
    input("test1 action: 5")
    action(111112, chess.parse_square('e4'), False) #Place opp piece on e4
    input("test1 action: 6")
    action(111112, chess.parse_square('e7'), True)  #Pick up e7 pawn
    input("test1 action: 7")
    action(111112, chess.parse_square('e5'), False) #Place pawn on e5
    input("test1 action: 8")
    action(111112, 0, False, True)                  #Pressed confirm button

    input("test1 action: 9")
    action(111111, chess.parse_square('e7'), True)
    input("test1 action: 10")
    action(111111, chess.parse_square('e5'), False) #opp e5
    input("test1 action: 11")
    action(111111, chess.parse_square('d1'), True)
    input("test1 action: 12")
    action(111111, chess.parse_square('h5'), False) #Qh5
    input("test1 action: 13")
    action(111111, 0, False, True)

    input("test1 action: 14")
    action(111112, chess.parse_square('d1'), True)
    input("test1 action: 15")
    action(111112, chess.parse_square('h5'), False) #opp Qh5
    input("test1 action: 16")
    action(111112, chess.parse_square('b8'), True)
    input("test1 action: 17")
    action(111112, chess.parse_square('c6'), False) #Nc6
    input("test1 action: 18")
    action(111112, 0, False, True)

    input("test1 action: 19")
    action(111111, chess.parse_square('b8'), True)
    input("test1 action: 20")
    action(111111, chess.parse_square('c6'), False) #opp Nc6
    input("test1 action: 21")
    action(111111, chess.parse_square('f1'), True)
    input("test1 action: 22")
    action(111111, chess.parse_square('c4'), False) #Bc4
    input("test1 action: 23")
    action(111111, 0, False, True)

    input("test1 action: 24")
    action(111112, chess.parse_square('f1'), True)
    input("test1 action: 25")
    action(111112, chess.parse_square('c4'), False) #opp Bc4
    input("test1 action: 26")
    action(111112, chess.parse_square('g8'), True)
    input("test1 action: 27")
    action(111112, chess.parse_square('f6'), False) #Nf6
    input("test1 action: 28")
    action(111112, 0, False, True)

    input("test1 action: 29")
    action(111111, chess.parse_square('g8'), True)
    input("test1 action: 30")
    action(111111, chess.parse_square('f6'), False) #opp Nf6
    input("test1 action: 31")
    action(111111, chess.parse_square('h5'), True)  #Lift up queen
    input("test1 action: 32")
    action(111111, chess.parse_square('f7'), True)  #lift up pawn
    input("test1 action: 33")
    action(111111, chess.parse_square('f7'), False) #Qxf7#
    input("test1 action: 34")
    action(111111, 0, False, True)

def test2():
    action(-1, True) #Invalid square
    action(64, True) #Invalid square

    action(chess.parse_square('e4'), True)  #Invalid pickup (no piece on square)
    action(chess.parse_square('e4'), False) #Invalid placement (no piece selected)
    
    action(chess.parse_square('e7'), True)  #InvalidOppPickup
    action(chess.parse_square('e2'), True)  #Pick up your piece (before placing back Opp piece)
    action(chess.parse_square('e2'), False) #Place your piece back down
    action(chess.parse_square('e7'), False) #Place original Opp piece down

main()
