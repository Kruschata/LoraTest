const socket = new WebSocket("ws://localhost:8080");

// Verbindung geöffnet
socket.onopen = () => {
    console.log("WebSocket verbunden");
};

// Nachricht vom Server (LoRa / Backend)
socket.onmessage = (event) => {
    addReceivedMessage("LoRa Gerät", event.data);
};

// Fehler
socket.onerror = (error) => {
    console.error("WebSocket Fehler:", error);
};

// Verbindung geschlossen
socket.onclose = () => {
    console.log("WebSocket getrennt");
};


function submitText() {
    const inputText = document.getElementById("textInput").value;

    if (!inputText) return;

    // an Server (LoRa Backend) senden
    if (socket.readyState === WebSocket.OPEN) {
        socket.send(inputText);
    } else {
        console.warn("WebSocket nicht verbunden");
    }

    document.getElementById("output").textContent = "Gesendet: " + inputText;

    addReceivedMessage("Website", inputText);

    document.getElementById("textInput").value = "";
}

function addReceivedMessage(sender, message) {
    const container = document.getElementById("messageContainer");

    if (container.innerText.includes("Noch keine Nachrichten")) {
        container.innerHTML = "";
    }

    const now = new Date().toLocaleTimeString();

    const messageElement = document.createElement("div");
    messageElement.className = "card mb-2 border-success";

    messageElement.innerHTML = `
        <div class="card-body py-2">
            <div class="d-flex justify-content-between">
                <strong>${sender}</strong>
                <small class="text-muted">${now}</small>
            </div>
            <div>${message}</div>
        </div>
    `;

    container.appendChild(messageElement);

    container.scrollTop = container.scrollHeight;
}