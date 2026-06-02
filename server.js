const express = require("express");

const app = express();

app.use(express.json());

app.post("/webhook", (req, res) => {
    console.log("Neue TTN Nachricht:");
    console.log(JSON.stringify(req.body, null, 2));

    res.status(200).send("OK");
});

app.listen(8080, () => {
    console.log("Server läuft auf Port 8080");
});