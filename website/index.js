const protocol = location.protocol === "https:" ? "wss:" : "ws:";
const socket = new WebSocket(`${protocol}//${location.host}`);

const form = document.getElementById("chatForm");
const input = document.getElementById("textInput");
const sendButton = document.getElementById("sendButton");
const container = document.getElementById("messageContainer");
const statusBadge = document.getElementById("connectionStatus");

function setStatus(text, level = "warn") {
    statusBadge.textContent = text;
    statusBadge.className = `status-badge status-${level}`;
}

function clearEmptyState() {
    const empty = container.querySelector(".empty-state");
    if (empty) {
        empty.remove();
    }
}

function addMessage({ direction, sender, text, meta }) {
    clearEmptyState();

    const row = document.createElement("article");
    row.className = `message-row message-${direction}`;

    const bubble = document.createElement("div");
    bubble.className = "message-bubble";

    const heading = document.createElement("div");
    heading.className = "message-heading";

    const senderEl = document.createElement("strong");
    senderEl.textContent = sender;

    const timeEl = document.createElement("time");
    timeEl.textContent = new Date().toLocaleTimeString([], { hour: "2-digit", minute: "2-digit", second: "2-digit" });

    const body = document.createElement("div");
    body.className = "message-text";
    body.textContent = text;

    heading.append(senderEl, timeEl);
    bubble.append(heading, body);

    if (meta) {
        const metaEl = document.createElement("div");
        metaEl.className = "message-meta";
        metaEl.textContent = meta;
        bubble.append(metaEl);
    }

    row.appendChild(bubble);
    container.appendChild(row);
    container.scrollTop = container.scrollHeight;
}

function addSystemMessage(text, level = "info") {
    clearEmptyState();

    const item = document.createElement("div");
    item.className = `system-message system-${level.toLowerCase()}`;
    item.textContent = text;
    container.appendChild(item);
    container.scrollTop = container.scrollHeight;
}

socket.addEventListener("open", () => {
    setStatus("WebSocket verbunden", "ok");
    sendButton.disabled = false;
});

socket.addEventListener("message", (event) => {
    let data;
    try {
        data = JSON.parse(event.data);
    } catch (_error) {
        data = { type: "raw", text: event.data };
    }

    if (data.type === "rx") {
        addMessage({
            direction: "incoming",
            sender: `LilyGO ${data.from}`,
            text: data.text,
            meta: `RSSI ${data.rssi} dBm, SNR ${data.snr}`
        });
        return;
    }

    if (data.type === "tx") {
        addMessage({
            direction: "outgoing",
            sender: `LilyGO ${data.from}`,
            text: data.text,
            meta: `Paket ${data.counter} gesendet`
        });
        return;
    }

    if (data.type === "status") {
        const level = data.level === "OK" ? "ok" : data.level === "ERROR" ? "error" : "warn";
        setStatus(data.text, level);
        addSystemMessage(data.text, level);
        return;
    }

    addSystemMessage(data.text || event.data, "info");
});

socket.addEventListener("error", () => {
    setStatus("WebSocket Fehler", "error");
});

socket.addEventListener("close", () => {
    setStatus("Verbindung getrennt", "error");
    sendButton.disabled = true;
});

form.addEventListener("submit", (event) => {
    event.preventDefault();

    const text = input.value.trim();
    if (!text || socket.readyState !== WebSocket.OPEN) {
        return;
    }

    socket.send(JSON.stringify({ type: "chat", text }));
    input.value = "";
    input.focus();
});
