const mqtt = require('mqtt');

// MQTT Server Connection
const MQTT_BROKER = 'mqtt://150.136.93.68:1883';
const client = mqtt.connect(MQTT_BROKER);

// Internal mapping of games and boards
const gameConnections = {};

// Helper function to forward messages from host to guest
function forwardHostMessage(hostID, message) {
    console.log('hostID:', hostID);
    console.log('gameConnections:', gameConnections);

    // Ensure `hostID` is a string for comparison
    const hostIDString = String(hostID);

    // Find the gameID by matching `hostID` correctly
    const gameID = Object.keys(gameConnections).find(id => gameConnections[id].hostID === hostIDString);

    if (!gameID) {
        console.error(`Game for host ID ${hostID} not found.`);
        return;
    }

    const guestID = gameConnections[gameID].guestID;

    console.log('Forwarding message from host to guest:', message);

    // Forward the message to the guest
    client.publish(`board/${guestID}`, message);
}


// MQTT Event Listeners
client.on('connect', () => {
    console.log('Connected to MQTT broker');

    // Subscribe to game start channel
    client.subscribe('server/to/gameserver', (err) => {
        if (err) console.error('Failed to subscribe to server/to/gameserver');
    });

    // Subscribe to messages from boards
    client.subscribe('server/from/board', (err) => {
        if (err) console.error('Failed to subscribe to server/from/board');
    });
});

client.on('message', (topic, message) => {
    console.log(`Received message on topic "${topic}": ${message.toString()}`);
    try {
        const payload = JSON.parse(message.toString());

        if (topic === 'server/to/gameserver') {
            console.log('Handling new game start:', payload);
            const { gameID, hostID, guestID } = payload;

            // Save the game connections
            gameConnections[gameID] = { hostID, guestID };

            // Notify boards to start
            client.publish(`board/${hostID}`, JSON.stringify({ command: 'start_host' }));
            client.publish(`board/${guestID}`, JSON.stringify({ command: 'start_guest' }));
        } else if (topic === 'server/from/board') {
            const { playerID } = payload;

            // Check if the sender is a host and forward the message to the guest
            console.log('Forwarding message from host to guest:', payload);
            forwardHostMessage(playerID, message.toString());
        } else {
            console.warn(`Unexpected topic "${topic}" received.`);
        }
    } catch (error) {
        console.error('Error parsing or handling message:', error);
    }
});

client.on('error', (err) => {
    console.error('MQTT Error:', err);
});

