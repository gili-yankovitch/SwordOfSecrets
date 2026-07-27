// protocol.js -- browser port of protocol.py
//
// Message envelope on the wire:
//   { "version": 2, "body": {...}, "integrity": "...", "signature": "..." }
//
// The "version" field lives OUTSIDE the signed "body" -- that is the first
// vulnerability. Integrity is computed over the CANONICAL JSON of the body:
//   v2 -> SHA-256(body)               (64 hex chars)
//   v1 -> CRC-16-CCITT/XModem(body)   (4 hex chars, zero-padded) -- only 65536
//         possible values, trivially collided.
//
// canonicalJSON() must be byte-identical to Python
//   json.dumps(obj, sort_keys=True, separators=(",",":"), ensure_ascii=True)
// and to the sword firmware's hardcoded (already alphabetical, compact) output.

export const CLIENT_HELLO = "CLIENT_HELLO";
export const SERVER_CHALLENGE = "SERVER_CHALLENGE";
export const CLIENT_RESPONSE = "CLIENT_RESPONSE";
export const SERVER_RESULT = "SERVER_RESULT";

export const ACTION_LIST = "LIST";
export const ACTION_READ = "READ";
export const ACTION_WRITE = "WRITE";

export const VERSION_SECURE = 2;
export const VERSION_LEGACY = 1;

// --- canonical JSON --------------------------------------------------------

function escapeString(s) {
  let out = '"';
  for (let i = 0; i < s.length; i++) {
    const c = s.charCodeAt(i);
    if (c === 0x22) out += '\\"';
    else if (c === 0x5c) out += "\\\\";
    else if (c === 0x08) out += "\\b";
    else if (c === 0x09) out += "\\t";
    else if (c === 0x0a) out += "\\n";
    else if (c === 0x0c) out += "\\f";
    else if (c === 0x0d) out += "\\r";
    else if (c < 0x20 || c >= 0x80) out += "\\u" + c.toString(16).padStart(4, "0");
    else out += s[i];
  }
  return out + '"';
}

export function canonicalJSON(obj) {
  if (obj === null) return "null";
  const t = typeof obj;
  if (t === "number") {
    if (!Number.isFinite(obj)) throw new Error("non-finite number");
    // integers only in this protocol; String() gives plain decimal
    return String(obj);
  }
  if (t === "boolean") return obj ? "true" : "false";
  if (t === "string") return escapeString(obj);
  if (Array.isArray(obj)) return "[" + obj.map(canonicalJSON).join(",") + "]";
  if (t === "object") {
    const keys = Object.keys(obj).sort();
    return "{" + keys.map((k) => escapeString(k) + ":" + canonicalJSON(obj[k])).join(",") + "}";
  }
  throw new Error("cannot canonicalize type " + t);
}

const enc = new TextEncoder();

export function canonicalBytes(obj) {
  return enc.encode(canonicalJSON(obj));
}

// --- integrity -------------------------------------------------------------

// CRC-16-CCITT (XModem): poly 0x1021, init 0x0000, no reflection.
// Identical to Python binascii.crc_hqx(data, 0).
export function crc16(bytes) {
  let crc = 0x0000;
  for (let i = 0; i < bytes.length; i++) {
    crc ^= bytes[i] << 8;
    for (let b = 0; b < 8; b++) {
      crc = crc & 0x8000 ? ((crc << 1) ^ 0x1021) & 0xffff : (crc << 1) & 0xffff;
    }
  }
  return crc & 0xffff;
}

export async function sha256hex(bytes) {
  const digest = await crypto.subtle.digest("SHA-256", bytes);
  return [...new Uint8Array(digest)].map((b) => b.toString(16).padStart(2, "0")).join("");
}

// integrity string for a body dict, per version
export async function computeIntegrity(version, body) {
  const bytes = canonicalBytes(body);
  if (version === VERSION_SECURE) return await sha256hex(bytes);
  return crc16(bytes).toString(16).padStart(4, "0");
}

export async function verifyIntegrity(version, body, expected) {
  return (await computeIntegrity(version, body)) === expected;
}

// --- message helpers -------------------------------------------------------

export function buildMessage(version, body, integrity = "", signature = "") {
  const msg = { version, body };
  if (integrity) msg.integrity = integrity;
  if (signature) msg.signature = signature;
  return msg;
}
