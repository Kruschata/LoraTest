const WebSocket = require("ws");
const { SerialPort } = require("serialport");

const wss = new WebSocket.Server({ port: 8080 });

// LoRa Serial
const port = new SerialPort({
  path: "COM44", // Ersetze dies durch den COM-Port deines ESP32
  baudRate: 115200,
  autoOpen: true
});

let serialBuffer = "";
let clients = [];

port.on("open", () => {
  console.log("Serialport offen:", port.path);
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
      port.write(text + "\n");
    }
  });

  ws.on("close", () => {
    clients = clients.filter(c => c !== ws);
  });
});

// 🔥 NUR EINMAL: LoRa → alle Clients
port.on("data", (data) => {
  serialBuffer += data.toString();
  let index;

  while ((index = serialBuffer.indexOf("\n")) !== -1) {
    const line = serialBuffer.slice(0, index).trim();
    serialBuffer = serialBuffer.slice(index + 1);

    if (!line) {
      continue;
    }

    console.log("Von LoRa:", line);

    clients.forEach(ws => {
      if (ws.readyState === 1) {
        ws.send(line);
      }
    });
  }
});
