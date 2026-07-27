// se-webserial.js -- talk to the physical Sword of Secrets over WebSerial.
// This replaces client.py's SETransportSerial. The sword is the Secure Element:
// it holds the SE private key and is the crypto root of trust. Its private key
// NEVER leaves the device -- the browser only ever asks it to sign.
//
// Wire facts (verified against the firmware):
//   * 115200 8N1, newline-delimited JSON, one object per line.
//   * Request line MUST start with '{' and contain NO whitespace after colons
//     (the device extracts fields by naive substring match). So we build lines
//     by hand / JSON.stringify (no spaces), and send `params` already-canonical
//     (sorted keys) because the device echoes params verbatim into the body it
//     signs -- if params isn't canonical, the server's integrity re-check fails.
//   * On port.open() WebSerial asserts DTR/RTS, which RESETS the device into a
//     ~1.7s OTA-bootloader listen window. If we send during that window the
//     device drops into the AES-CBC bootloader and answers with single chars
//     from "VMCSE" instead of JSON. So we stay silent ~1.8s after opening.
//   * SIGN_CHALLENGE runs two software ECDSA signs + a 200ms blink -> budget
//     seconds per call.

import { canonicalJSON } from "./protocol.js";

const sleep = (ms) => new Promise((r) => setTimeout(r, ms));

export const WEBSERIAL_SUPPORTED = typeof navigator !== "undefined" && "serial" in navigator;

export class SwordSerial {
  constructor(port) {
    this.port = port;
    this.reader = null;
    this.writer = null;
    this._buf = "";
    this._lines = [];      // parsed JSON lines waiting to be consumed
    this._waiters = [];     // pending recv() resolvers
    this._readLoop = null;
    this._closed = false;
    this.sawOtaChars = false;
    this.onDebug = null;    // optional (dir, obj) hook for the DEBUG view
  }

  // Prompt the user to pick the sword, then open + wait out the OTA window.
  static async request() {
    if (!WEBSERIAL_SUPPORTED) throw new Error("WebSerial not supported (use Chrome or Edge)");
    const port = await navigator.serial.requestPort();
    const s = new SwordSerial(port);
    await s.connect();
    return s;
  }

  async connect() {
    await this.port.open({ baudRate: 115200 });
    const dec = new TextDecoderStream();
    this.port.readable.pipeTo(dec.writable).catch(() => {});
    this.reader = dec.readable.getReader();
    this.writer = this.port.writable.getWriter();
    this._startReader();
    // Stay silent through the ~1.7s OTA listen window after the DTR reset.
    await sleep(1800);
  }

  _startReader() {
    this._readLoop = (async () => {
      try {
        while (!this._closed) {
          const { value, done } = await this.reader.read();
          if (done) break;
          if (value) this._ingest(value);
        }
      } catch (_) {
        /* reader torn down */
      }
    })();
  }

  _ingest(chunk) {
    this._buf += chunk;
    let idx;
    while ((idx = this._buf.search(/[\r\n]/)) >= 0) {
      const line = this._buf.slice(0, idx).trim();
      this._buf = this._buf.slice(idx + 1);
      if (!line) continue;
      if (!line.startsWith("{")) {
        // Single-char "VMCSE" replies => device is in OTA mode.
        if (/^[VMCSE]+$/.test(line)) this.sawOtaChars = true;
        continue;
      }
      let obj;
      try { obj = JSON.parse(line); } catch { continue; }
      if (this.onDebug) this.onDebug("sword->client", obj);
      const w = this._waiters.shift();
      if (w) { clearTimeout(w.timer); w.resolve(obj); }
      else this._lines.push(obj);
    }
  }

  _recv(timeoutMs) {
    if (this._lines.length) return Promise.resolve(this._lines.shift());
    return new Promise((resolve, reject) => {
      const timer = setTimeout(() => {
        const i = this._waiters.findIndex((w) => w.timer === timer);
        if (i >= 0) this._waiters.splice(i, 1);
        reject(new Error(
          this.sawOtaChars
            ? "Sword is in OTA/bootloader mode (got 'VMCSE' chars). Unplug, replug, wait ~2s, and reconnect."
            : "Sword serial timeout",
        ));
      }, timeoutMs);
      this._waiters.push({ resolve, reject, timer });
    });
  }

  async _sendLine(line) {
    await this.writer.write(new TextEncoder().encode(line + "\n"));
  }

  async _rpc(line, obj, timeoutMs) {
    if (this.onDebug) this.onDebug("client->sword", obj);
    await this._sendLine(line);
    return this._recv(timeoutMs);
  }

  // ---- SE commands (mirror client.py se_* helpers) ------------------------

  async getCertificate() {
    const resp = await this._rpc('{"command":"GET_CERTIFICATE"}', { command: "GET_CERTIFICATE" }, 8000);
    if (resp.status !== "OK") throw new Error(`SE error: ${resp.error || "unknown"}`);
    return resp.certificate;
  }

  async signHello(version, counter) {
    const obj = { command: "SIGN_HELLO", version, counter };
    const line = `{"command":"SIGN_HELLO","version":${int(version)},"counter":${int(counter)}}`;
    const resp = await this._rpc(line, obj, 10000);
    if (resp.status !== "OK") throw new Error(`SE error: ${resp.error || "unknown"}`);
    return resp;
  }

  // Returns the raw response (status may be OK or ERROR) so callers can show
  // the sword's refusal (e.g. READ in v2) rather than throwing.
  async signChallenge(version, nonce, action, params, counter) {
    const obj = { command: "SIGN_CHALLENGE", version, counter, nonce, action, params };
    // params MUST be canonical (sorted, no spaces) -- the device echoes it verbatim.
    const line =
      `{"command":"SIGN_CHALLENGE","version":${int(version)},"counter":${int(counter)},` +
      `"nonce":"${String(nonce)}","action":"${String(action)}","params":${canonicalJSON(params ?? {})}}`;
    return this._rpc(line, obj, 20000);
  }

  async close() {
    this._closed = true;
    try { await this.reader?.cancel(); } catch {}
    try { this.reader?.releaseLock(); } catch {}
    try { await this.writer?.close(); } catch {}
    try { this.writer?.releaseLock(); } catch {}
    try { await this.port.close(); } catch {}
  }
}

function int(n) {
  if (!Number.isInteger(n)) throw new Error("expected integer, got " + n);
  return String(n);
}
