const mqtt = require('mqtt');

// MQTT Server Connection
const MQTT_BROKER = 'mqtt://150.136.93.68:1883';
const client = mqtt.connect(MQTT_BROKER);

// Internal mapping of games and boards
const gameConnections = {};
const gameStates = {}; // Stores board states for each game

// MQTT Event Listeners
client.on('connect', () => {
    console.log('Connected to MQTT broker');

    // Subscribe to game start channel
    client.subscribe('server/to/gameserver', (err) => {
        if (err) console.error('Failed to subscribe to server/to/gameserver');
        else console.log('Subscribed to server/to/gameserver');
    });

    // Subscribe to messages from boards
    client.subscribe('server/from/board', (err) => {
        if (err) console.error('Failed to subscribe to server/from/board');
        else console.log('Subscribed to server/from/board');
    });
});

client.on('message', (topic, message) => {
    console.log(`Received message on topic "${topic}": ${message.toString()}`);
    try {
        const payload = JSON.parse(message.toString());

        if (topic === 'server/to/gameserver') {
            // Handling new game start
            console.log('Initializing new game with payload:', payload);
            const { gameID, hostID, guestID } = payload;
            console.log('Game ID:', gameID);
            gameConnections[gameID] = { hostID, guestID };
            gameStates[gameID] = {
                sessionBoard: Array(9).fill(0),
                boards: {
                    [hostID]: Array(9).fill(0),
                    [guestID]: Array(9).fill(0)
                }
            };
            console.log(`Game ${gameID} initialized with Host: ${hostID}, Guest: ${guestID}`);

            // Notify boards to start
            client.publish(`board/${hostID}`, JSON.stringify({ command: 'start_host' }));
            client.publish(`board/${guestID}`, JSON.stringify({ command: 'start_guest' }));
        } else if (topic === 'server/from/board') {
            // Handling board updates
            console.log('Processing move from board:', payload);
            const { playerID, boardState } = payload;

            // Find the game session
            // Find the gameID
            const playerIDString = String(playerID);
            console.log(playerID);

            const gameID = Object.keys(gameConnections).find(id => gameConnections[id].hostID === playerIDString || gameConnections[id].guestID === playerIDString);
            console.log(gameID);

            if (!gameID) return console.warn('Game session not found for player', playerID);
            console.log(`Player ${playerID} is in game ${gameID}`);

            const { hostID, guestID } = gameConnections[gameID];
            console.log("Found hostID: ", hostID);
            console.log("Found guestID: ", guestID);
            const gameState = gameStates[gameID];
            isHost = false;
            if (playerID == hostID) {
                isHost = true;
            }
            console.log('isHost? ', isHost);
            const marker = isHost ? 1 : 2;

            // Find the first difference
            const previousBoardState = gameState.boards[playerID];
            let moveIndex = -1;
            for (let i = 0; i < 9; i++) {
                if (previousBoardState[i] !== boardState[i] && gameState.sessionBoard[i] === 0) {
                    moveIndex = i;
                    break;
                }
            }

            if (moveIndex === -1) return console.warn('No valid move detected for player', playerID);
            console.log(`Player ${playerID} placed marker at index ${moveIndex}`);

            // Update the individual board state and session board
            gameState.boards[playerID] = [...boardState];
            gameState.sessionBoard[moveIndex] = marker;
            console.log(`Updated session board for game ${gameID}:`, gameState.sessionBoard);

            // Check for a win condition
            if (checkWin(gameState.sessionBoard)) {
                console.log(`Player ${marker} wins in game ${gameID}`);
                if (marker == 1) {
                    client.publish(`board/${hostID}`, JSON.stringify({ command: 'win', BoardState: gameState.sessionBoard }));
                    client.publish(`board/${guestID}`, JSON.stringify({ command: 'lose', BoardState: gameState.sessionBoard }));
                } else if (marker == 2) {
                    client.publish(`board/${hostID}`, JSON.stringify({ command: 'lose', BoardState: gameState.sessionBoard }));
                    client.publish(`board/${guestID}`, JSON.stringify({ command: 'win', BoardState: gameState.sessionBoard }));
                } else {
                    console.warn('Invalid marker declared winner.');
                }
            } else {
                turn_counter = 0;
                for (let i = 0; i < 9; i++) {
                    if (boardState[i] == 1 || boardState[i] == 2) {
                        turn_counter++;
                    }
                }
                console.log("# of turns", turn_counter);
                if (turn_counter == 9) {
                    console.log("Game resulted in draw");
                    client.publish(`board/${hostID}`, JSON.stringify({ command: 'draw', BoardState: gameState.sessionBoard }));
                    client.publish(`board/${guestID}`, JSON.stringify({ command: 'draw', BoardState: gameState.sessionBoard }));
                } else {
                    // Send move info to other board
                    if (marker == 1) {
                        // Send move info to guest
                        client.publish(`board/${guestID}`, JSON.stringify({ command: 'move', BoardState: gameState.sessionBoard }));
                    } else if (marker == 2) {
                        // Send move info to host
                        client.publish(`board/${hostID}`, JSON.stringify({ command: 'move', BoardState: gameState.sessionBoard }));
                    } else {
                        console.warn('Invalid marker detected.');
                    }
                }
            }
        } else {
            console.warn(`Unexpected topic "${topic}" received.`);
        }
    } catch (error) {
        console.error('Error parsing or handling message:', error);
    }
});

// Function to check if a player has won
function checkWin(board) {
    const winPatterns = [
        [0, 1, 2], [3, 4, 5], [6, 7, 8],
        [0, 3, 6], [1, 4, 7], [2, 5, 8],
        [0, 4, 8], [2, 4, 6]
    ];
    return winPatterns.some(pattern => {
        const [a, b, c] = pattern;
        return board[a] !== 0 && board[a] === board[b] && board[a] === board[c];
    });
}

client.on('error', (err) => {
    console.error('MQTT Error:', err);
});
