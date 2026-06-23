#ifndef INDEX_H
#define INDEX_H

#include "style.h"
#include "script.h"

const char INDEX_HTML[] PROGMEM = R"HTML(
<!doctype html>
<html lang="de">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>LoRa Chat</title>
  <style>
)HTML"
#include "style.inc"
R"HTML(
  </style>
</head>
<body>
  <main>
    <section class="panel">
      <header>
        <h1>LoRa Chat</h1>
        <p>Verbunden mit 192.168.4.1</p>
      </header>
      <div id="messages"></div>
      <form id="form">
        <input 
          id="text" 
          maxlength="180" 
          placeholder="Nachricht eingeben..." 
          autocomplete="off"
        >
        <button>Senden</button>
      </form>
    </section>
  </main>

  <script>
)HTML"
#include "script.inc"
R"HTML(
  </script>
</body>
</html>
)HTML";

#endif
