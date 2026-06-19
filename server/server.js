const express = require("express");
const http = require("http");
const path = require("path");
const WebSocket = require("ws");
const { SerialPort } = require("serialport");

const HTTP_PORT = process.env.PORT ? parseInt(process.env.PORT, 10) : 5500;
const SERIAL_PATH = process.env.SERIAL_PORT || "COM11";
const SERIAL_BAUD = process.env.SERIAL_BAUD ? parseInt(process.env.SERIAL_BAUD, 10) : 115200;

const app = express();
app.use(express.static(path.join(__dirname, "..", "website")));

app.get("/api/serial-ports", async (_req, res) => {
  try {
    res.json(await SerialPort.list());
  } catch (error) {
    res.status(500).json({ error: error.message });
  }
});

const server = http.createServer(app);
const wss = new WebSocket.Server({ server });

let serialBuffer = "";
let serialReady = false;

function unescapeField(value = "") {
  let out = "";
  let escaped = false;

  for (const char of value) {
    if (escaped) {
      out += char;
      escaped = false;
    } else if (char === "\\") {
      escaped = true;
    } else {
      out += char;
    }
  }

  return out;
}

function sendToClients(event) {
  const payload = JSON.stringify(event);
  for (const client of wss.clients) {
    if (client.readyState === WebSocket.OPEN) {
      client.send(payload);
    }
  }
}

function parseSerialLine(line) {
  const parts = line.split("|");
  const type = parts[0];

  if (type === "RX" && parts.length >= 6) {
    return {
      type: "rx",
      from: parts[1],
      counter: parts[2],
      rssi: Number(parts[3]),
      snr: Number(parts[4]),
      text: unescapeField(parts.slice(5).join("|"))
    };
  }

  if (type === "TX" && parts.length >= 4) {
    return {
      type: "tx",
      from: parts[1],
      counter: parts[2],
      text: unescapeField(parts.slice(3).join("|"))
    };
  }

  if (type === "STATUS") {
    return {
      type: "status",
      level: parts[1] || "INFO",
      text: parts.slice(2).join(" | ") || line
    };
  }

  if (type === "RAW" && parts.length >= 4) {
    return {
      type: "raw",
      rssi: Number(parts[1]),
      snr: Number(parts[2]),
      text: unescapeField(parts.slice(3).join("|"))
    };
  }

  return { type: "raw", text: line };
}

const port = new SerialPort({
  path: SERIAL_PATH,
  baudRate: SERIAL_BAUD,
  autoOpen: false
});

port.open((error) => {
  if (error) {
    console.error(`Serialport konnte nicht geoeffnet werden (${SERIAL_PATH}): ${error.message}`);
    console.error("Tipp: PowerShell: node -e \"require('serialport').SerialPort.list().then(console.log)\"");
    sendToClients({ type: "status", level: "ERROR", text: error.message });
    return;
  }
});

port.on("open", () => {
  serialReady = true;
  console.log("Serialport offen:", port.path, "@", SERIAL_BAUD);
  sendToClients({ type: "status", level: "OK", text: `Serial verbunden: ${port.path}` });
});

port.on("close", () => {
  serialReady = false;
  console.log("Serialport geschlossen");
  sendToClients({ type: "status", level: "ERROR", text: "Serial getrennt" });
});

port.on("error", (error) => {
  serialReady = false;
  console.error("Serialport Fehler:", error.message);
  sendToClients({ type: "status", level: "ERROR", text: error.message });
});

port.on("data", (data) => {
  serialBuffer += data.toString("utf8");

  let index;
  while ((index = serialBuffer.indexOf("\n")) !== -1) {
    const line = serialBuffer.slice(0, index).trim();
    serialBuffer = serialBuffer.slice(index + 1);

    if (!line) {
      continue;
    }

    const event = parseSerialLine(line);
    console.log("Serial:", line);
    sendToClients(event);
  }
});

wss.on("connection", (ws) => {
  console.log("Client verbunden");
  ws.send(JSON.stringify({
    type: "status",
    level: serialReady ? "OK" : "WARN",
    text: serialReady ? `Serial verbunden: ${SERIAL_PATH}` : `Warte auf Serial: ${SERIAL_PATH}`
  }));

  ws.on("message", (message) => {
    let event;
    try {
      event = JSON.parse(message.toString());
    } catch (_error) {
      event = { type: "chat", text: message.toString() };
    }

    const text = String(event.text || "").trim();
    if (!text) {
      return;
    }

    if (!serialReady) {
      ws.send(JSON.stringify({ type: "status", level: "ERROR", text: "Serial ist nicht verbunden." }));
      return;
    }

    port.write(`CHAT|${text}\n`, (error) => {
      if (error) {
        ws.send(JSON.stringify({ type: "status", level: "ERROR", text: error.message }));
      }
    });
  });
});

server.listen(HTTP_PORT, () => {
  console.log(`HTTP & WebSocket Server laeuft auf http://localhost:${HTTP_PORT}`);
  console.log(`Benutze Serial Port: ${SERIAL_PATH} @ ${SERIAL_BAUD}`);
});
