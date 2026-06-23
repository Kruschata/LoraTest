R"JS(
// DOM-Elemente
const box = document.getElementById('messages');
const form = document.getElementById('form');
const input = document.getElementById('text');
let last = '';

// HTML-Escaping für Sicherheit
function esc(s) {
  return String(s).replace(/[&<>"']/g, c => ({
    '&': '&amp;',
    '<': '&lt;',
    '>': '&gt;',
    '"': '&quot;',
    "'": '&#39;'
  }[c]));
}

// Nachrichten laden und anzeigen
async function load() {
  const text = await fetch('/api/messages', { cache: 'no-store' })
    .then(r => r.text())
    .catch(() => null);

  if (text === null || text === last) return;
  last = text;

  box.innerHTML = text
    .split('\n')
    .filter(Boolean)
    .map(line => {
      const tx = line.startsWith('TX|');
      const parts = line.split('|');
      const msg = parts.slice(tx ? 3 : 5).join('|');
      
      const meta = tx
        ? `Gesendet von LilyGO ${parts[1]}, Paket ${parts[2]}`
        : `LilyGO ${parts[1]}, RSSI ${parts[3]}, SNR ${parts[4]}`;

      return `<div class="msg ${tx ? 'tx' : 'rx'}"><span class="meta">${esc(meta)}</span>${esc(msg)}</div>`;
    })
    .join('') || '<p>Noch keine Nachrichten.</p>';

  box.scrollTop = box.scrollHeight;
}

// Formular absenden
form.addEventListener('submit', async e => {
  e.preventDefault();
  const text = input.value.trim();
  if (!text) return;

  input.value = '';
  await fetch('/api/send', {
    method: 'POST',
    headers: { 'Content-Type': 'text/plain' },
    body: text
  }).catch(() => {});

  load();
});

// Initial laden und Auto-Refresh
load();
setInterval(load, 1000);
)JS"
