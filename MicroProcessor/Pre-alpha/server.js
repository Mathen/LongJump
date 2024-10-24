const fs = require('fs');
const https = require('https');
const mqtt = require('mqtt');
const express = require('express');
const app = express();

const server = https.createServer({
    key: fs.readFileSync('/etc/letsencrypt/live/longjump.ip-dynamic.org/privkey.pem'),
    cert: fs.readFileSync('/etc/letsencrypt/live/longjump.ip-dynamic.org/fullchain.pem')
}, app);

const io = require('socket.io')(server, {
    path: '/socket.io/',
    transports: ['websocket', 'polling']
});

const client = mqtt.connect('mqtt://150.136.93.68:1883');

client.on('connect', () => {
    console.log('Connected to MQTT broker');
    client.subscribe(['board1/to/board2', 'board2/to/board1']);
});

client.on('message', (topic, message) => {
    // Input sanitization
    if (!topic || typeof topic !== 'string') {
        console.error('Invalid topic:', topic);
        return;
    }

    const data = message ? message.toString() : null;
    if (!data) {
        console.error('Empty or invalid message:', message);
        return;
    }

    console.log('Topic:', topic, 'Message:', data);
    io.emit('mqtt-message', { topic, data });
});

io.on('connection', (socket) => {
    console.log('Client connected:', socket.id);
});

app.get('/', (req, res) => {
    res.sendFile(__dirname + '/index.html');
});

server.listen(3000, '127.0.0.1', () => {
    console.log('Server running on port 3000');
});
