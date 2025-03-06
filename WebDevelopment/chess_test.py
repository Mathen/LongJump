import paho.mqtt.client as mqtt
import json
import time
import numpy as np

HOST_ID = 239104

# Initialize MQTT client
client = mqtt.Client()

# Initial board state
INITIAL_BOARD = [1,1,1,1,1,1,1,1,
                1,1,1,1,1,1,1,1,
                0,0,0,0,0,0,0,0,
                0,0,0,0,0,0,0,0,
                0,0,0,0,0,0,0,0,
                0,0,0,0,0,0,0,0,
                1,1,1,1,1,1,1,1,
                1,1,1,1,1,1,1,1]

host_board = INITIAL_BOARD.copy()
guest_board = INITIAL_BOARD.copy()

def chess_square_to_index(square):
    file = ord(square[0]) - ord('a')
    rank = 8 - int(square[1])
    return rank * 8 + file

def send_board_state(player_id, board_state, confirm_button=False):
    # input(f"Press Enter to continue: {player_id}")
    payload = {
        "playerID": player_id,
        "boardState": board_state,
        "confirmButton": 1 if confirm_button else 0
    }
    print(f"\nSending board state from {'Host' if player_id == HOST_ID else 'Guest'} board:")
    print(f"Player ID: {player_id}")
    print("Board state:")
    print(np.array(board_state).reshape(8, 8)[::-1])
    print(f"Confirm button: {confirm_button}")

    client.publish("boards/to/chess", json.dumps(payload))
    print('-'*25)
    time.sleep(0.01)

def simulate_scholars_mate():
    global host_board, guest_board

    print("Connecting to MQTT broker...")
    client.connect("150.136.93.68", 1883, 60)
    client.loop_start()
    time.sleep(1)

    '''
    payload = json.dumps({
        "gameID": 'HOST_ID-111112',
        "hostID": 'HOST_ID',
        "guestID": '111112',
    })
    client.publish('server/to/chessServer', payload)'
    '''

    input("Press Enter to start Scholar's Mate simulation")
    
    moves = [
        (HOST_ID, "e2", False, False), (HOST_ID, "e4", False, False), (HOST_ID, None, True, False),
        (111112, "e2", False, False),

        #(HOST_ID, "c6", False, True), # dummy pick up
        #(HOST_ID, "f7", False, True), # dummy pick up
        #(HOST_ID, "c6", False, True), # dummy place
        #(HOST_ID, "f7", False, True), # dummy place
        
        (111112, "e4", False, False), (111112, "e7", False, False),
        (111112, "e5", False, False), (111112, None, True, False), (HOST_ID, "e7", False, False),
        (HOST_ID, "e5", False, False), (HOST_ID, "d1", False, False), (HOST_ID, "h5", False, False),
        (HOST_ID, None, True, False), (111112, "d1", False, False), (111112, "h5", False, False),
        (111112, "b8", False, False), (111112, "c6", False, False), (111112, None, True, False),
        (HOST_ID, "b8", False, False), # pickup opp

        #(111112, "d1", False, True), # dummy pickup

        (HOST_ID, "c6", False, False), (HOST_ID, "f1", False, False), # place opp, pickup
        
        #(111112, "d1", False, True), # dummy place
        
        (HOST_ID, "c4", False, False), (HOST_ID, None, True, False), (111112, "f1", False, False), # place, confirm, pickup opp
        (111112, "c4", False, False), (111112, "g8", False, False), (111112, "f6", False, False),
        (111112, None, True, False), (HOST_ID, "g8", False, False), (HOST_ID, "f6", False, False),
        (HOST_ID, "h5", False, False), (HOST_ID, "f7", False, False), (HOST_ID, "f7", False, False),
        (HOST_ID, None, True, False)
    ]

    for action in moves:
        player, square, confirm, isDummy = action
        if square:
            input(f"----------Press Enter to move {square} for player {player}----------")
            index = chess_square_to_index(square)
            if isDummy:
                print(f'----------DUMMY MOVE: Move {square} for player {player}----------')
            if player == HOST_ID:
                host_board[index] = 0 if host_board[index] else 1
                send_board_state(player, host_board)
            else:
                guest_board[index] = 0 if guest_board[index] else 1
                send_board_state(player, guest_board)
        if confirm:
            input(f"Press Enter to confirm move for player {player}")
            send_board_state(player, host_board if player == HOST_ID else guest_board, confirm_button=True)

if __name__ == "__main__":
    simulate_scholars_mate()
