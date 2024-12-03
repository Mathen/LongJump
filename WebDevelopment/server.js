const fs = require('fs');
const mqtt = require('mqtt');
const express = require('express');
const app = express();
const http = require('http');
const server = http.createServer(app);

const client = mqtt.connect('mqtt://150.136.93.68:1883');
const BOARD_DATA_FILE = './boardData.json';
let boardData = {};
/*
// Load board data from file
try {
    if (fs.existsSync(BOARD_DATA_FILE)) {
        boardData = JSON.parse(fs.readFileSync(BOARD_DATA_FILE));
    }
} catch (error) {
    console.error('Error loading board data:', error);
}
*/

// Save board data to file
function saveBoardData() {
    fs.writeFileSync(BOARD_DATA_FILE, JSON.stringify(boardData, null, 2));
}

client.on('connect', () => {
    console.log('Connected to MQTT broker');
    client.subscribe('boards/to/server');
    client.subscribe('frontend/to/server'); // Subscribe to frontend messages
    client.subscribe('frontend/to/server/host_response');
});

client.on('message', (topic, message) => {
    const data = message.toString().trim();
    console.log(`Received message on topic "${topic}": ${data}`);

    if (topic === 'boards/to/server') {
        if (/^\d{6}$/.test(data)) {
            if (!boardData[data]) {
                boardData[data] = { role: "", state: {} };
                console.log(`Board ID ${data} added.`);
                saveBoardData();
            } else {
                console.log(`Board ID ${data} already exists.`);
            }
        } else {
            console.error(`Invalid board ID received from board: ${data}`);
        }
    } else if (topic === 'frontend/to/server') {
        try {
            const { boardID, role, hostID } = JSON.parse(data);

            if (role === 'guest') {
                // Step 1: Validate guest board ID
                if (!boardData[boardID]) {
                    client.publish(`server/to/frontend/${boardID}`, JSON.stringify({ error: "Invalid guest ID" }));
                    console.log(`Invalid guest ID received: ${boardID}`);
                    return;
                }

                // Notify the frontend that the guest ID is valid
                client.publish(`server/to/frontend/${boardID}`, JSON.stringify({ message: "Guest ID validated. Please enter host ID." }));
                console.log(`Guest ID ${boardID} validated.`);

                return;
            }

            // Step 2: Host state handling (only proceed if host ID is provided after guest validation)
            if (role === 'connect' && hostID) {
                // Ensure guest ID is valid (redundant double-check)
                if (!boardData[boardID]) {
                    client.publish(`server/to/frontend/${boardID}`, JSON.stringify({ error: "Invalid guest ID. Please restart the process." }));
                    console.log(`Invalid guest ID during connection attempt: ${boardID}`);
                    return;
                }

                // Validate host ID and ensure it's in a waiting state
                if (!boardData[hostID] || boardData[hostID].role !== 'host' || boardData[hostID].state.message !== "Waiting for guest...") {
                    client.publish(`server/to/frontend/${boardID}`, JSON.stringify({ error: "Invalid or unavailable host ID" }));
                    console.log(`Invalid or unavailable host ID: ${hostID}`);
                    return;
                }

                // Notify host of the guest's request
                client.publish(`server/to/host/${hostID}`, JSON.stringify({ guestID: boardID }));
                client.publish(`server/to/frontend/${boardID}`, JSON.stringify({ message: "Request sent to host, awaiting response" }));
                console.log(`Guest ${boardID} requested to join Host ${hostID}`);
                return;
            }

            if (role === 'host') {
                // Validate host board ID
                if (!boardData[boardID]) {
                    client.publish(`server/to/frontend/${boardID}`, JSON.stringify({ error: "Invalid host ID" }));
                    console.log(`Invalid host ID received: ${boardID}`);
                    return;
                }

                // Update host state to waiting
                boardData[boardID].role = 'host';
                boardData[boardID].state.message = "Waiting for guest...";
                saveBoardData();

                client.publish(`server/to/frontend/${boardID}`, JSON.stringify({ message: "Waiting for guest..." }));
                console.log(`Host ${boardID} is now waiting for a guest`);
            }
        } catch (error) {
            console.error("Error parsing frontend message:", error);
        }
    } else if (topic === 'frontend/to/server/host_response') {
    try {
        const { hostID, decision, guestID } = JSON.parse(data);

        // Validate host and guest IDs
        if (!boardData[hostID] || !boardData[guestID]) {
            console.log(`Invalid IDs in host response: hostID=${hostID}, guestID=${guestID}`);
            return;
        }

        if (decision === 'accept') {
            client.publish(`server/to/frontend/${guestID}`, JSON.stringify({ message: "Request accepted by host. Game starting!" }));
            boardData[hostID].state.message = "Host occupied";
            saveBoardData();

            console.log(`Host ${hostID} accepted Guest ${guestID}`);

            // Notify the game server to initialize a new game
            const gameID = `${hostID}-${guestID}`; // Example game ID generation
            const payload = JSON.stringify({
                gameID,
                hostID,
                guestID
            });
            client.publish('server/to/gameserver', payload);

            console.log(`Game initialized with gameID: ${gameID}, hostID: ${hostID}, guestID: ${guestID}`);
        } else if (decision === 'reject') {
            client.publish(`server/to/frontend/${guestID}`, JSON.stringify({ message: "Request rejected by host" }));
            console.log(`Host ${hostID} rejected Guest ${guestID}`);
        } else {
            console.log(`Unknown decision from host: ${decision}`);
        }
    } catch (error) {
        console.error("Error parsing host response:", error);
    }
}
});

server.listen(3000, "127.0.0.1", () => {
    console.log('Clearing JSON');
    fs.writeFileSync(BOARD_DATA_FILE, JSON.stringify({}));
    console.log('Server running on port 3000');
});