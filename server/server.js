const express = require("express");
const http = require("http");
const WebSocket = require("ws");
const { SerialPort } = require("serialport");

const HTTP_PORT = process.env.PORT ? parseInt(process.env.PORT, 10) : 5500;
const SERIAL_PATH = process.env.SERIAL_PORT || "COM11"; // Passe bei Bedarf an
const SERIAL_BAUD = process.env.SERIAL_BAUD ? parseInt(process.env.SERIAL_BAUD, 10) : 115200;

const app = express();
app.use(express.static(require('path').join(__dirname, '..', 'website')));

const server = http.createServer(app);
const wss = new WebSocket.Server({ server });

// LoRa Serial
const port = new SerialPort({
  path: SERIAL_PATH,
  baudRate: SERIAL_BAUD,
  autoOpen: true
});

let serialBuffer = "";
let clients = [];

port.on("open", () => {
  console.log("Serialport offen:", port.path, "@", SERIAL_BAUD);
});

port.on("error", (err) => {
  console.error("Serialport Fehler:", err.message);
});

// WebSocket Verbindung
wss.on("connection", (ws) => {
  console.log("Client verbunden");
  clients.push(ws);

  ws.on("message", (msg) => {
    const text = msg.toString().trim();
    console.log("Von Website:", text);

    if (text.length > 0) {
      // an das über USB angeschlossene Lilygo-Gerät senden
      port.write(text + "\n");
    }
  });

  ws.on("close", () => {
    clients = clients.filter(c => c !== ws);
  });
});

// LoRa → alle WebSocket-Clients
port.on("data", (data) => {
  serialBuffer += data.toString();
  let index;

  while ((index = serialBuffer.indexOf("\n")) !== -1) {
    const line = serialBuffer.slice(0, index).trim();
    serialBuffer = serialBuffer.slice(index + 1);

    if (!line) continue;

    console.log("Von LoRa:", line);

    clients.forEach(ws => {
      if (ws.readyState === WebSocket.OPEN) {
        ws.send(line);
      }
    });
  }
});

server.listen(HTTP_PORT, () => {
  console.log(`HTTP & WebSocket Server läuft auf http://localhost:${HTTP_PORT}`);
  console.log(`Benutze Serial Port: ${SERIAL_PATH} @ ${SERIAL_BAUD}`);
});
