# Utility Funktionen - Dokumentation

Die **Utility-Funktionen** sind Hilfsfunktionen für die sichere Verarbeitung von Strings. Sie schützen vor Problemen, die entstehen würden, wenn Nachrichten Sonderzeichen enthalten.

## Das Problem

Das LoRa Chat System verwendet dieses Nachrichtenformat mit Pipe-Trennzeichen:

```
TX|DeviceID|Counter|Nachricht
RX|DeviceID|Counter|RSSI|SNR|Nachricht
```

Das `|` (Pipe) ist ein **Trennzeichen** - es trennt die verschiedenen Felder. Aber was passiert, wenn die Nachricht selbst ein `|` enthält?

### Beispiel ohne Escaping:
```
Nachricht: "Hallo|Test"
Ergebnis:  TX|2|123|Hallo|Test
           ↑  ↑   ↑  ↑    ↑
           1  2   3  4    5 (Parser findet 5 Felder statt 4!)
```

Der Parser wird verwirrt - er denkt, es gibt 5 Felder statt 4!

## Die Lösung: Escaping

Deshalb werden Sonderzeichen **maskiert** (escaped):
- Das `|` wird zu `\|`
- Das `\` wird zu `\\`

```
Nachricht: "Hallo|Test"
Escaped:   "Hallo\|Test"
Ergebnis:  TX|2|123|Hallo\|Test
           ↑  ↑   ↑  ↑ (nur 4 Felder, korrekt!)
```

---

## Die Drei Funktionen

### 1. `escapeField()` - Text sichern

```cpp
String escapeField(const String &value)
```

**Was es tut:**
Macht einen String sicher für die Übertragung, indem Sonderzeichen maskiert werden.

**Regeln:**
- `|` → `\|`
- `\` → `\\`
- `\r` und `\n` → entfernt (keine Zeilenumbrüche)

**Ablauf:**
```
für jeden Charakter c im Input:
  1. Wenn c == '\' oder c == '|': Füge '\' hinzu
  2. Wenn c != '\r' und c != '\n': Füge c hinzu
```

**Beispiele:**
| Input | Output |
|-------|--------|
| `"Hallo"` | `"Hallo"` |
| `"Hallo\|Test"` | `"Hallo\\|Test"` |
| `"Hallo"` + Zeilenumbruch | `"Hallo"` (Umbruch weg) |

---

### 2. `unescapeField()` - Text dekodieren

```cpp
String unescapeField(const String &value)
```

**Was es tut:**
Das Gegenteil von `escapeField()` - entfernt die Maskierung wieder.

**Regeln:**
Wenn `\` gefunden wird, wird das nächste Zeichen nicht besonders behandelt (auch wenn es `\` oder `|` ist).

**Ablauf:**
```
escaped = false
für jeden Charakter c im Input:
  1. Wenn escaped == true: Füge c hinzu und setze escaped = false
  2. Wenn c == '\': Setze escaped = true
  3. Sonst: Füge c hinzu
```

**Beispiele:**
| Input | Output |
|-------|--------|
| `"Hallo"` | `"Hallo"` |
| `"Hallo\|Test"` | `"Hallo\|Test"` → `"Hallo\|Test"` |
| `"Hallo\\Test"` | `"Hallo\Test"` |

---

### 3. `findUnescapedPipe()` - Trennzeichen finden

```cpp
int findUnescapedPipe(const String &value, int startAt)
```

**Was es tut:**
Findet die Position eines echten Trennzeichens (`|`), das nicht maskiert ist. Berücksichtigt dabei das Escaping.

**Parameter:**
- `value`: Der durchsuchte String
- `startAt`: Startposition (ab hier wird gesucht)

**Rückgabe:**
- Position des unmassierten `|` (0-basiert)
- `-1` wenn kein unmasiertes `|` gefunden wurde

**Ablauf:**
```
escaped = false
für jeden Charakter c von startAt bis Ende:
  1. Wenn escaped == true: Setze escaped = false
  2. Wenn c == '\': Setze escaped = true
  3. Wenn c == '|' UND escaped == false: Rückgabe = i
```

**Beispiele:**
| Input | startAt | Output |
|-------|---------|--------|
| `"Hallo\|Test"` | 0 | `-1` (das `\|` ist maskiert) |
| `"Hallo\|Test\|Wow"` | 0 | `13` (zweites `\|` ist auch maskiert!) |
| `"TX\|2\|123\|Hallo"` | 0 | `-1` (alle sind maskiert) |
| `"TX"` | 0 | `-1` |

**Anwendungsbeispiel:**
Bei der Nachricht `TX|2|123|Hallo\|Welt` (Payload mit Trennzeichen):
```
p1 = findUnescapedPipe(payload, 0)     // = 2
p2 = findUnescapedPipe(payload, p1+1)  // = 4
p3 = findUnescapedPipe(payload, p2+1)  // = 8
p4 = findUnescapedPipe(payload, p3+1)  // = -1 (kein weiteres echtes Pipe)
```

Die Nachricht wird dann korrekt geparsed als:
- `TX` | `2` | `123` | `Hallo\|Welt` (4 Felder)

**Hinweis:** Der Backslash wird entfernt - `\|` wird zu `|`, nicht zu `\|`!

---

### 3. `findUnescapedPipe()` - Findet echte Trennzeichen

```cpp
int findUnescapedPipe(const String &value, int startAt)
```

**Was es tut:**
Sucht das nächste **echte** `|` (ohne Backslash davor) ab einer bestimmten Position.

**Rückgabewert:**
- Index des gefundenen `|`
- `-1` wenn kein `|` gefunden

**Warum wichtig?**
Beim Parsen von Nachrichten muss man die Felder trennen. Man darf aber nur bei echten Trennzeichen trennen, nicht bei `\|`.

**Ablauf:**
```
escaped = false
für jeden Charakter c ab Position startAt:
  1. Wenn escaped == true: Setze escaped = false
  2. Wenn c == '\': Setze escaped = true
  3. Wenn c == '|' und escaped == false: gib Position zurück
gib -1 zurück (nicht gefunden)
```

**Beispiel:**
```
Text: "TX|2|123|Hallo\|Welt|end"
      0  2  4   9          17  21

findUnescapedPipe(text, 0)  → 2  (erstes |)
findUnescapedPipe(text, 3)  → 4  (zweites |)
findUnescapedPipe(text, 5)  → 9  (drittes |)
findUnescapedPipe(text, 10) → 21 (viertes | - springt über \| bei Position 14)
findUnescapedPipe(text, 22) → -1 (kein | mehr)
```

---

## Praktisches Beispiel

### Szenario: Benutzer sendet "Hallo|Freund"

**1. Senden (escapeField):**
```cpp
String message = "Hallo|Freund";
String escaped = escapeField(message);
// escaped = "Hallo\|Freund"

String payload = "TX|2|1|" + escaped;
// payload = "TX|2|1|Hallo\|Freund"
```

**2. Empfangen (Parsing mit findUnescapedPipe):**
```cpp
String payload = "TX|2|1|Hallo\|Freund";

int p1 = findUnescapedPipe(payload, 0);    // 2
int p2 = findUnescapedPipe(payload, p1+1); // 3
int p3 = findUnescapedPipe(payload, p2+1); // 5
int p4 = findUnescapedPipe(payload, p3+1); // -1 (kein viertes Pipe)

// Felder extrahieren:
String type = payload.substring(0, p1);           // "TX"
String device = payload.substring(p1+1, p2);      // "2"
String counter = payload.substring(p2+1, p3);     // "1"
String text = payload.substring(p3+1);            // "Hallo\|Freund"

// Dekodieren:
text = unescapeField(text);
// text = "Hallo|Freund" ✓
```

---

## Zusammenfassung

| Funktion | Aufgabe | Richtung |
|----------|---------|----------|
| `escapeField()` | Text sicher machen | Vor dem Senden |
| `unescapeField()` | Text dekodieren | Nach dem Empfang |
| `findUnescapedPipe()` | Trennzeichen finden | Beim Parsing |

Diese drei Funktionen sorgen dafür, dass **Nachrichten mit beliebigen Inhalten** korrekt übertragen und empfangen werden können!
