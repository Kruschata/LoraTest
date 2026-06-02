function submitText() {
            const inputText = document.getElementById("textInput").value;
            document.getElementById("output").textContent = "Output: " + inputText;
        }

function addReceivedMessage(sender, message) {
    const container = document.getElementById("messageContainer");

    // Platzhalter entfernen
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

    // automatisch nach unten scrollen
    container.scrollTop = container.scrollHeight;