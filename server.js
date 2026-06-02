const WebSocket = require("ws");
const { SerialPort } = require("serialport");

const wss = new WebSocket.Server({ port: 8080 });

// LoRa Serial
const port = new SerialPort({
  path: "COM44",
  baudRate: 115200
});

let clients = [];

// WebSocket Verbindung
wss.on("connection", (ws) => {
  console.log("Client verbunden");
  clients.push(ws);

  ws.on("message", (msg) => {
    console.log("Von Website:", msg.toString());

    port.write(msg.toString() + "\n");
  });

  ws.on("close", () => {
    clients = clients.filter(c => c !== ws);
  });
});

// 🔥 NUR EINMAL: LoRa → alle Clients
port.on("data", (data) => {
  const message = data.toString();

  console.log("Von LoRa:", message);

  clients.forEach(ws => {
    if (ws.readyState === 1) {
      ws.send(message);
    }
  });
});