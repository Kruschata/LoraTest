#ifndef STYLE_H
#define STYLE_H

#include <Arduino.h>

const char STYLE_CSS[] PROGMEM = R"CSS(
/* Farben und Variablen */
:root {
  --bg: #eef2f5;
  --panel: #fff;
  --line: #d7dee5;
  --text: #17212b;
  --muted: #657282;
  --out: #d8ecff;
  --in: #f4f7f9;
  --accent: #1769aa;
}

/* Basis-Styles */
* {
  box-sizing: border-box;
}

body {
  margin: 0;
  min-height: 100vh;
  background: var(--bg);
  font-family: system-ui, -apple-system, Segoe UI, sans-serif;
  color: var(--text);
}

/* Layout */
main {
  min-height: 100vh;
  display: grid;
  place-items: center;
  padding: 14px;
}

.panel {
  width: min(760px, 100%);
  height: min(760px, calc(100vh - 28px));
  display: grid;
  grid-template-rows: auto 1fr auto;
  background: var(--panel);
  border: 1px solid var(--line);
  border-radius: 8px;
  overflow: hidden;
}

/* Header */
header {
  padding: 16px;
  border-bottom: 1px solid var(--line);
}

h1 {
  font-size: 1.25rem;
  margin: 0;
}

p {
  margin: 4px 0 0;
  color: var(--muted);
}

/* Nachrichten-Bereich */
#messages {
  padding: 14px;
  overflow: auto;
  background: #fbfcfd;
}

.msg {
  max-width: 86%;
  margin: 0 0 10px;
  padding: 9px 11px;
  border: 1px solid var(--line);
  border-radius: 8px;
  background: var(--in);
  overflow-wrap: anywhere;
  white-space: pre-wrap;
}

/* Sendete Nachrichten (rechts, blau) */
.tx {
  margin-left: auto;
  background: var(--out);
  border-color: #b9d8f2;
}

/* Metadaten (Absender, Signalstärke) */
.meta {
  display: block;
  margin-bottom: 4px;
  color: var(--muted);
  font-size: 0.78rem;
}

/* Eingabeformular */
form {
  display: grid;
  grid-template-columns: 1fr auto;
  gap: 8px;
  padding: 12px;
  border-top: 1px solid var(--line);
}

input, button {
  font: inherit;
  padding: 10px;
  border-radius: 8px;
  border: 1px solid var(--line);
}

button {
  min-width: 92px;
  border-color: var(--accent);
  background: var(--accent);
  color: white;
  cursor: pointer;
}

button:hover {
  opacity: 0.9;
}

/* Mobil-optimiert */
@media (max-width: 560px) {
  main {
    padding: 0;
  }

  .panel {
    height: 100vh;
    border: 0;
    border-radius: 0;
  }

  form {
    grid-template-columns: 1fr;
  }

  button {
    width: 100%;
  }
}
)
)CSS";

#endif
