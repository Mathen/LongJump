import chess
import numpy as np
import json
import paho.mqtt.client as mqtt

#State Diagram:
#https://drive.google.com/file/d/1h9whv9Un0wUjQ-3JaLobKUYU050ZlDQ4/view

#board = chess.Board()
physicalBoard = []
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
        "uciMove": ['xx', 'xx'],        #https://en.wikipedia.org/wiki/Universal_Chess_Interface
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
        "gameID": gameID,
        "picked_up_host": False,
        "picked_up_guest": False,
        "winner": "",
        "debug": 0
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
    print("-"*90)
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
        print("Message received from board")
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

        # Find if board is host or guest
        if str(boardID) == session["hostID"]:
            print(f"Host board '{boardID}' message received.")
            print(np.reshape(boardState, (8, 8))[::-1])
            board_changed = 'host'
        elif str(boardID) == session["guestID"]:
            print(f"Guest board '{boardID}' message received.")
            board_changed = 'guest'
        else:
            print("BoardID not associated with host or guest in session")
            return

        expected_turn = "host" if session['game_board'].turn == chess.WHITE else "guest"

        if board_changed != expected_turn:
            print(f"WARNING: {board_changed} sent update out of turn. Ignoring move.")
            return

        if session['game_board'].turn == chess.WHITE and str(boardID) == session['guestID']:
            print("WARNING: Guest message received on host turn")
            return

        if session['game_board'].turn == chess.BLACK and str(boardID) == session['hostID']:
            print("WARNING: Host message received on guest turn")
            return

        if str(boardID) == session['hostID']:
            previous_board = session['host_board']
        elif str(boardID) == session['guestID']:
            previous_board = session['guest_board']
            # flip guest board
            boardState = np.array(boardState).reshape(8, 8)[::-1, ::-1].flatten().tolist()
            print(np.reshape(boardState, (8, 8))[::-1])
        else:
            print("ERROR: boardID does not match host or guest")
            return

        # Detect board changes
        changed_positions = []
        picked_up_square = None
        placed_square = None

        for i in range(64):
            if previous_board[i] != boardState[i]:
                changed_positions.append((i, boardState[i]))
                is_guest = (board_changed == 'guest')
                uci_square = index_to_chess_square(i, is_guest=is_guest)

                if previous_board[i] == 1 and boardState[i] == 0:
                    # Piece removed
                    picked_up_square = uci_square
                    if (not is_guest):
                        session["picked_up_host"] = True
                    elif is_guest:
                        session["picked_up_guest"] = True
                elif previous_board[i] == 0 and boardState[i] == 1:
                    # Piece placed
                    placed_square = uci_square
                    if (not is_guest):
                        session["picked_up_host"] = False
                    elif is_guest:
                        session["picked_up_guest"] = False

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
                print("MESSAGE: Updating board state of HOST board")
                session["host_board"] = boardState
                board_id = session['hostID']
            else:
                print("MESSAGE: Updating board state of GUEST board")
                session["guest_board"] = boardState
                board_id = session['guestID']

        if confirmButton:
            action(boardID, 0, session["picked_up_host"] or session["picked_up_guest"], True)
        elif picked_up_square:
            if board_changed == 'host':
                print(f'MESSAGE: Host picked up square {picked_up_square}')
                print(f'MESSAGE: Host picked up status: {session["picked_up_host"]}')
                action(boardID, chess.parse_square(picked_up_square), session["picked_up_host"])
            elif board_changed == 'guest':
                print(f'MESSAGE: Guest picked up square {picked_up_square}')
                print(f'MESSAGE: Guest picked up status: {session["picked_up_guest"]}')
                action(boardID, chess.parse_square(picked_up_square), session["picked_up_guest"])
            else:
                return
        elif placed_square:
            if board_changed == 'host':
                print(f'MESSAGE: Host placed square {placed_square}')
                print(f'MESSAGE: Host picked up status: {session["picked_up_host"]}')
                action(boardID, chess.parse_square(placed_square), session["picked_up_host"])
            elif board_changed == 'guest':
                print(f'MESSAGE: Guest placed square {placed_square}')
                print(f'MESSAGE: Guest picked up status: {session["picked_up_guest"]}')
                action(boardID, chess.parse_square(placed_square), session["picked_up_guest"])
            else:
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
    #CastleKing, CastleRook, EnPassantMove, 
    #ShowCastleKing, ShowCastleRook, ShowEnPassant,
    #GameOver, Debug

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
        leds = ['0']*8*8
        message = ''
        RED = 'r'
        BLINK_RED = 'R'
        BLUE = 'b'
        GREEN = 'g'
        ORANGE = 'o'
                
        for i in range(len(leds)):
            row = i // 8
            col = i % 8

            if (row + col) % 2 == 1:
                leds[i] = ORANGE


        print('Initializing host and guest board via MQTT')
        # Send initialization message to host and guest
        message = 'chess_start_host'
        topic = f"board/{session['hostID']}"
        payload = json.dumps({
            "ledArray": leds,
            "message": message
        })
        client.publish(topic, payload)

        message = 'chess_start_guest'
        topic = f"board/{session['guestID']}"
        payload = json.dumps({
            "ledArray": leds,
            "message": message
        })
        client.publish(topic, payload)
        session['state'] = 'WaitOnMove'
        sendMessageFrontend(session['gameID'], session['game_board'])
        return
    
    if message == 'GameOver HOST':
        message = 'win'
        topic = f"board/{session['hostID']}"
        payload = json.dumps({
            "message": message
        })
        client.publish(topic, payload)

        message = 'lose'
        topic = f"board/{session['guestID']}"
        payload = json.dumps({
            "message": message
        })
        client.publish(topic, payload)
        return
    elif message == 'GameOver GUEST':
        message = 'win'
        topic = f"board/{session['guestID']}"
        payload = json.dumps({
            "message": message
        })
        client.publish(topic, payload)

        message = 'lose'
        topic = f"board/{session['hostID']}"
        payload = json.dumps({
            "message": message
        })
        client.publish(topic, payload)
        return
    elif message == 'GameOver DRAW':
        message = 'draw'
        topic = f"board/{session['hostID']}"
        payload = json.dumps({
            "message": message
        })
        client.publish(topic, payload)

        topic = f"board/{session['guestID']}"
        payload = json.dumps({
            "message": message
        })
        client.publish(topic, payload)
        return

    #Validate ledArray
    if len(ledArray) != 64:
        print('ERROR: invalid ledArray length ' + len(ledArray))
        return

    print('Message: ' + str(message))
    print(np.reshape(ledArray, (8, 8))[::-1])

    if session['game_board'].turn == chess.WHITE:
        #TODO: send message to host board
        print("Sending HOST message")
        topic = f"board/{session['hostID']}"
        payload = json.dumps({
            "ledArray": ledArray,
            "message": message
        })
        client.publish(topic, payload)
    elif session['game_board'].turn == chess.BLACK:
        #TODO: send message to guest board
        print("Sending GUEST message")
        topic = f"board/{session['guestID']}"
        payload = json.dumps({
            "ledArray": ledArray,
            "message": message
        })
        client.publish(topic, payload)
    else:
        print('ERROR: invalid player turn color when sending message')

def sendMessageFrontend(gameID, boardState):
    topic = f"chess/to/frontend/{gameID}"
    payload = json.dumps({
        "boardState": str(boardState)
    })
    client.publish(topic, payload)

    print(f'Sending board state to frontend on topic: {topic}')
    print(boardState)


def action(boardID, square: int, isPickedUp: bool, confirmButton = False):
    session = sessions[board_to_session[str(boardID)]]
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
    ORANGE = 'o'
            
    for i in range(len(leds)):
        row = i // 8
        col = i % 8

        if (row + col) % 2 == 1:
            leds[i] = ORANGE

    print('Entering state: ' + session['state'])
    #Control Logic for states
    if confirmButton == True and session['state'] != 'WaitingConfirm' and session['state'] != 'Debug':
        session['state'] = 'Debug'
    elif session['state'] == 'WaitOnMove':
        if isPickedUp == True:
            #Make sure player is picking up their piece first
            if session['game_board'].color_at(square) == session['game_board'].turn:
                session['state'] = 'ShowValidMoves'
                session['uciMove'][0] = str(chess.square_name(square))
            elif str(session['game_board'].color_at(square)) != 'None':
                session['state'] = 'InvalidOppPickup'
            else:
                print('Warning: Player should not be able to pick piece from an empty square')
        else:
            print('Warning: Player should not be able to place a piece down WaitOnMove state')
    elif session['state'] == 'ShowOppFrom':
        move = session['game_board'].peek()
        #Correct opp piece pickup
        if move.from_square == square:
            session['state'] = 'ShowOppTo'
        else:
            session['state'] = 'InvalidOppFrom'
            print('Warning: player picked up wrong piece for opp turn')
            print(f"Correct opp pickup piece: {move.from_square}")
            print(f"Attempted move: {chess.square_name(square)}")
    elif session['state'] == 'ShowOppTo':
        move = session['game_board'].peek()
        if isPickedUp == False:
            #Correct opp piece place down
            if move.to_square == square:
                if session['game_board'].is_castling(move):
                    session['state'] = 'ShowCastleKing'
                elif (session['game_board'].is_en_passant(move)):
                    session['state'] = 'EnPassantMove'
                else:
                    session['state'] = 'WaitOnMove'
            else:
                print('Warning: player placed the wrong piece for opp turn')
                print(f"Correct opp place location: {move.from_square}")
                print(f"Attempted move: {chess.square_name(square)}")
    elif session['state'] == 'InvalidOppFrom':
        move = session['game_board'].peek()
        #Correct opp piece pickup
        if True: #move.from_square == square:
            session['state'] = 'ShowOppFrom'
        else:
            print('Warning: player picked up wrong piece for opp turn')
            print(f"Correct opp pickup piece: {move.from_square}")
            print(f"Attempted move: {chess.square_name(square)}")
    elif session['state'] == 'InvalidOppPickup':
        if isPickedUp == False:
            if session['uciMove'][1] == chess.square_name(square):
                session['state'] = 'WaitOnMove'
            else:
                print('Warning: player did not place piece back on the correct square')
    elif session['state'] == 'ShowValidMoves':
        session['uciMove'][1] = chess.square_name(square)
        move = chess.Move.null()
        if session['uciMove'][0] != session['uciMove'][1]:
            move = chess.Move.from_uci(session['uciMove'][0] + session['uciMove'][1])
        if isPickedUp == True:
            if session['game_board'].color_at(square) != session['game_board'].turn:
                if move in session['game_board'].legal_moves:
                    session['state'] = 'ValidCapture'
                else:
                    session['state'] = 'InvalidSecondPickup'
            else:
                session['state'] = 'InvalidSecondPickup'
        else:
            if session['uciMove'][0] == chess.square_name(square):
                session['state'] = 'WaitOnMove'
            elif (session['game_board'].is_castling(move)):
                session['state'] = 'CastleKing'
            elif (session['game_board'].is_en_passant(move)):
                session['state'] = 'EnPassantMove'
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
        if isPickedUp == False and chess.square_name(square) == session['uciMove'][1]:
            move = chess.Move.from_uci(session['uciMove'][0] + session['uciMove'][1])
            if move in session['game_board'].legal_moves:
                session['state'] = 'WaitingConfirm'
            else:
                session['state'] = 'InvalidMove'
        else:
            print('Warning: user messed something up at ValidCapture')
    elif session['state'] == 'WaitingConfirm':
        if confirmButton == True:
            move = chess.Move.from_uci(session['uciMove'][0] + session['uciMove'][1])
            if move in session['game_board'].legal_moves:
                message = 'Confirmed! Waiting for opp.'
                sendMessage(session, leds, message)

                #Turn completed
                print('Pushing move')
                session['game_board'].push(move)
                session['uciMove'] = ['xx', 'xx']

                print(session['game_board'])
                sendMessageFrontend(session['gameID'], session['game_board'])

                if session['game_board'].is_game_over() == True:
                    print(session['game_board'].outcome())
                    outcome = session['game_board'].outcome()
                    if outcome:
                        if outcome.winner == chess.WHITE:
                            print("WHITE (HOST) WINS")
                            session['winner'] = "HOST"
                        elif outcome.winner == chess.BLACK:
                            print("BLACK (GUEST) WINS")
                            session['winner'] = "GUEST"
                        else:
                            print("GAME DRAW")
                            session['winner'] = "DRAW"
                    session['state'] = 'GameOver'

                if session['state'] != 'GameOver':
                    session['state'] = 'ShowOppFrom'
            else:
                print('ERROR: ' + session['uciMove'] + ' is not a valid move')
        elif isPickedUp == True and chess.square_name(square) == session['uciMove'][1]:
            session['state'] = 'ShowValidMoves'
    elif session['state'] == 'CastleKing':
        if isPickedUp == True:
            if session['uciMove'][1] == 'c1' and chess.square_name(square) == 'a1':
                session['state'] = 'CastleRook'
            elif session['uciMove'][1] == 'g1' and chess.square_name(square) == 'h1':
                session['state'] = 'CastleRook'
            elif session['uciMove'][1] == 'c8' and chess.square_name(square) == 'a8':
                session['state'] = 'CastleRook'
            elif session['uciMove'][1] == 'g8' and chess.square_name(square) == 'h8':
                session['state'] = 'CastleRook'
            else:
                print('ERROR: Trying to castle in an invalid castle')
        else:
            session['state'] = 'InvalidMove'
    elif session['state'] == 'CastleRook':
        if isPickedUp == False:
            if session['uciMove'][1] == 'c1' and chess.square_name(square) == 'd1':
                session['state'] = 'WaitingConfirm'
            elif session['uciMove'][1] == 'g1' and chess.square_name(square) == 'f1':
                session['state'] = 'WaitingConfirm'
            elif session['uciMove'][1] == 'c8' and chess.square_name(square) == 'd8':
                session['state'] = 'WaitingConfirm'
            elif session['uciMove'][1] == 'g8' and chess.square_name(square) == 'f8':
                session['state'] = 'WaitingConfirm'
            else:
                session['state'] = 'InvalidMove'
    elif session['state'] == 'EnPassantMove':
        if isPickedUp == True:
            if session['uciMove'][1] == chess.square_name(square):
                session['state'] = 'ShowValidMoves'
            elif session['uciMove'][1][1] == '6' and chess.square_name(square) == session['uciMove'][1][0] + '5':
                session['state'] = 'WaitingConfirm'
            elif session['uciMove'][1][1] == '3' and chess.square_name(square) == session['uciMove'][1][0] + '4':
                session['state'] = 'WaitingConfirm'
            else:
                session['state'] = 'InvalidPickup'
    elif session['state'] == 'ShowCastleKing':
        move = session['game_board'].peek()
        if isPickedUp == True:
            if move.uci[2:4] == 'c1' and chess.square_name(square) == 'a1':
                session['state'] = 'ShowCastleRook'
            elif move.uci[2:4] == 'g1' and chess.square_name(square) == 'h1':
                session['state'] = 'ShowCastleRook'
            elif move.uci[2:4] == 'c8' and chess.square_name(square) == 'a8':
                session['state'] = 'ShowCastleRook'
            elif move.uci[2:4] == 'g8' and chess.square_name(square) == 'h8':
                session['state'] = 'ShowCastleRook'
            else:
                print('ERROR: Trying to castle in an invalid castle')
                session['state'] = 'InvalidOppFrom'
        else:
            session['state'] = 'InvalidOppFrom'
    elif session['state'] == 'ShowCastleRook':
        move = session['game_board'].peek()
        if isPickedUp == False:
            if move.uci[2:4] == 'c1' and chess.square_name(square) == 'd1':
                session['state'] = 'WaitingConfirm'
            elif move.uci[2:4] == 'g1' and chess.square_name(square) == 'f1':
                session['state'] = 'WaitingConfirm'
            elif move.uci[2:4] == 'c8' and chess.square_name(square) == 'd8':
                session['state'] = 'WaitingConfirm'
            elif move.uci[2:4] == 'g8' and chess.square_name(square) == 'f8':
                session['state'] = 'WaitingConfirm'
            else:
                session['state'] = 'InvalidMove'
    elif session['state'] == 'ShowEnPassant':
        move = session['game_board'].peek()
        if isPickedUp == True:
            if move.uci[2:4] == chess.square_name(square):
                session['state'] = 'ShowOppTo'
            elif move.uci[3] == '6' and chess.square_name(square) == move.uci[2] + '5':
                session['state'] = 'WaitingConfirm'
            elif move.uci[3] == '3' and chess.square_name(square) == move.uci[2] + '4':
                session['state'] = 'WaitingConfirm'
            else:
                session['state'] = 'InvalidPickup'
    elif session['state'] == 'GameOver':
        print('Game Over')
    elif session['state'] == 'Debug':
        if confirmButton == True:
            session['debug'] += 1
            if session['debug'] >= 12:
                session['debug'] = 0
                session['state'] = 'WaitOnMove'
    else:
        print('ERROR: State \"' + session['state'] + '\" not found')

    #-----Execution Logic for states-----
    print('Executing state: ' + session['state'])
    if session['state'] == 'WaitOnMove':
        if session['game_board'].is_check():
            king = chess.square(session['game_board'].king(session['game_board'].turn))
            leds[king] = RED
        message = 'Your turn! Make move'
    elif session['state'] == 'ShowOppFrom':
        leds[session['game_board'].peek().from_square] = BLUE
        message = 'Pick up opp piece'
    elif session['state'] == 'ShowOppTo':
        leds[session['game_board'].peek().from_square] = BLUE
        if session['game_board'].is_capture(session['game_board'].peek()):
            leds[session['game_board'].peek().to_square] = GREEN
            message = 'Capture opp\'s piece'
        else:
            leds[session['game_board'].peek().to_square] = GREEN
            message = 'Move opp\'s piece'
    elif session['state'] == 'InvalidOppFrom':
        leds[square] = RED
        message = 'Pick up wrong opp piece'
    elif session['state'] == 'InvalidOppPickup':
        leds[square] = BLINK_RED
        message = 'Place down opp piece'
        if isPickedUp == True:
            session['uciMove'][1] = chess.square_name(square)
    elif session['state'] == 'ShowValidMoves':
        print('uciMove: ' + session['uciMove'][0])
        if session['uciMove'][0] == 'xx':
            session['uciMove'][0] = chess.square_name(square)
        for move in session['game_board'].legal_moves:
            if move.from_square == chess.parse_square(session['uciMove'][0]):
                if session['game_board'].is_capture(move):
                    #print('Capture: ' + str(move))
                    leds[move.to_square] = RED
                else:
                    #print('Move: ' + str(move))
                    leds[move.to_square] = GREEN
        leds[chess.parse_square(session['uciMove'][0])] = BLUE
        message = 'Make move'
    elif session['state'] == 'InvalidMove':
        message = 'Invalid move, pick up piece'
        leds[square] = BLINK_RED
    elif session['state'] == 'InvalidSecondPickup':
        message = 'Invalid pick up, replace piece'
        leds[square] = BLINK_RED
    elif session['state'] == 'ValidCapture':
        message = 'Place down piece over capture'
        leds[chess.parse_square(session['uciMove'][0])] = BLUE
        leds[square] = RED
    elif session['state'] == 'WaitingConfirm':
        leds[chess.parse_square(session['uciMove'][0])] = BLUE
        leds[chess.parse_square(session['uciMove'][1])] = GREEN
        message = 'Press confirm button'
    elif session['state'] == 'CastleKing':
        message = 'Move Rook Over'
        if session['uciMove'][1] == 'c1':
            leds[chess.parse_square('a1')] = GREEN
        elif session['uciMove'][1] == 'g1':
            leds[chess.parse_square('h1')] = GREEN
        elif session['uciMove'][1] == 'c8':
            leds[chess.parse_square('a8')] = GREEN
        elif session['uciMove'][1] == 'g8':
            leds[chess.parse_square('h8')] = GREEN
        else:
            print('ERROR: Trying to castle in an invalid castle')
    elif session['state'] == 'CastleRook':
        message = 'Place Rook Down'
        if session['uciMove'][1] == 'c1':
            leds[chess.parse_square('d1')] = GREEN
        elif session['uciMove'][1] == 'g1':
            leds[chess.parse_square('f1')] = GREEN
        elif session['uciMove'][1] == 'c8':
            leds[chess.parse_square('d8')] = GREEN
        elif session['uciMove'][1] == 'g8':
            leds[chess.parse_square('f8')] = GREEN
        else:
            print('ERROR: Trying to castle in an invalid castle')
    elif session['state'] == 'EnPassantMove':
        message = 'Remove En Passant\'ed pawn'
        if session['uciMove'][1][1] == '6':
            leds[chess.parse_square(session['uciMove'][1][0] + '5')] = RED
        elif session['uciMove'][1][1] == '3':
            leds[chess.parse_square(session['uciMove'][1][0] + '4')] = RED
        else:
            print('ERROR: Trying to en passant in an invalid en passant')
    elif session['state'] == 'GameOver':
        if session['winner'] == "HOST":
            message = 'GameOver HOST'
        elif session['winner'] == "GUEST":
            message = 'GameOver GUEST'
        elif session['winner'] == "DRAW":
            message = 'GameOver DRAW'
    elif session['state'] == 'ShowCastleKing':
        move = session['game_board'].peek()
        message = 'Move Rook Over'
        if move.uci()[2:4] == 'c1':
            leds[chess.parse_square('a1')] = GREEN
        elif move.uci()[2:4] == 'g1':
            leds[chess.parse_square('h1')] = GREEN
        elif move.uci()[2:4] == 'c8':
            leds[chess.parse_square('a8')] = GREEN
        elif move.uci()[2:4] == 'g8':
            leds[chess.parse_square('h8')] = GREEN
        else:
            print('ERROR: Trying to castle in an invalid castle')
    elif session['state'] == 'ShowCastleRook':
        move = session['game_board'].peek()
        message = 'Place Rook Down'
        if move.uci()[2:4] == 'c1':
            leds[chess.parse_square('d1')] = GREEN
        elif move.uci()[2:4] == 'g1':
            leds[chess.parse_square('f1')] = GREEN
        elif move.uci()[2:4] == 'c8':
            leds[chess.parse_square('d8')] = GREEN
        elif move.uci()[2:4] == 'g8':
            leds[chess.parse_square('f8')] = GREEN
        else:
            print('ERROR: Trying to castle in an invalid castle')
    elif session['state'] == 'ShowEnPassant':
        move = session['game_board'].peek()
        message = 'Remove En Passant\'ed pawn'
        if move.uci()[3] == '6':
            leds[chess.parse_square(move.uci()[2] + '5')] = RED
        elif move.uci()[3] == '3':
            leds[chess.parse_square(move.uci()[2] + '4')] = RED
        else:
            print('ERROR: Trying to en passant in an invalid en passant')
    elif session['state'] == 'Debug':
        squares = []
        if session['debug'] == 0:
            squares = session['game_board'].pieces(chess.PAWN, chess.WHITE)
            message = 'White Pawn'
        elif session['debug'] == 1:
            squares = session['game_board'].pieces(chess.ROOK, chess.WHITE)
            message = 'White Rook'
        elif session['debug'] == 2:
            squares = session['game_board'].pieces(chess.KNIGHT, chess.WHITE)
            message = 'White Knight'
        elif session['debug'] == 3:
            squares = session['game_board'].pieces(chess.BISHOP, chess.WHITE)
            message = 'White Bishop'
        elif session['debug'] == 4:
            squares = session['game_board'].pieces(chess.KING, chess.WHITE)
            message = 'White King'
        elif session['debug'] == 5:
            squares = session['game_board'].pieces(chess.QUEEN, chess.WHITE)
            message = 'White Queen'
        elif session['debug'] == 6:
            squares = session['game_board'].pieces(chess.PAWN, chess.BLACK)
            message = 'Black Pawn'
        elif session['debug'] == 7:
            squares = session['game_board'].pieces(chess.ROOK, chess.BLACK)
            message = 'Black Rook'
        elif session['debug'] == 8:
            squares = session['game_board'].pieces(chess.KNIGHT, chess.BLACK)
            message = 'Black Knight'
        elif session['debug'] == 9:
            squares = session['game_board'].pieces(chess.BISHOP, chess.BLACK)
            message = 'Black Bishop'
        elif session['debug'] == 10:
            squares = session['game_board'].pieces(chess.KING, chess.BLACK)
            message = 'Black King'
        elif session['debug'] == 11:
            squares = session['game_board'].pieces(chess.QUEEN, chess.BLACK)
            message = 'Black Queen'

        if len(squares) == 0:
            message += ' (none)'
        for square in squares:
            leds[square] = 'b'
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
