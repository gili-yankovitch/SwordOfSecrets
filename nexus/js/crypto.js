// crypto.js -- the verification half of crypto_utils.py, on WebCrypto.
//
// The browser NEXUS server only ever VERIFIES (never signs) -- signing is the
// hardware sword's job. So this module needs only:
//   * ECDSA-P256 signature verification (raw r||s, SHA-256), matching Python
//     verify_signature_hex()
//   * certificate verification against the server public key
//
// The flag is NOT decrypted here. A winning WRITE returns it AES-256-CBC-
// encrypted under SHA-256 of the previous challenge's final flag; the operator
// decrypts it offline once they have that flag.
//
// secp256r1 raw signatures are r||s (64 bytes) which is exactly the IEEE-P1363
// form WebCrypto expects -- no DER conversion needed.

const enc = new TextEncoder();

export function hexToBytes(hex) {
  if (hex.length % 2) throw new Error("odd hex length");
  const out = new Uint8Array(hex.length / 2);
  for (let i = 0; i < out.length; i++) out[i] = parseInt(hex.substr(i * 2, 2), 16);
  return out;
}

export function bytesToHex(bytes) {
  return [...bytes].map((b) => b.toString(16).padStart(2, "0")).join("");
}

export function b64ToBytes(b64) {
  const bin = atob(b64);
  const out = new Uint8Array(bin.length);
  for (let i = 0; i < bin.length; i++) out[i] = bin.charCodeAt(i);
  return out;
}

// Server public key (P-256) from base64 SPKI DER -> verify-only CryptoKey.
export async function importServerPublicKey(spkiB64) {
  return crypto.subtle.importKey(
    "spki",
    b64ToBytes(spkiB64),
    { name: "ECDSA", namedCurve: "P-256" },
    false,
    ["verify"],
  );
}

// SE public key from 64-byte hex (X||Y). WebCrypto 'raw' wants the uncompressed
// point, i.e. 0x04 || X || Y.
export async function importSEPublicKey(pubHex) {
  const raw = hexToBytes("04" + pubHex);
  return crypto.subtle.importKey(
    "raw",
    raw,
    { name: "ECDSA", namedCurve: "P-256" },
    false,
    ["verify"],
  );
}

// Verify a raw r||s hex signature over `dataBytes` (WebCrypto applies SHA-256).
export async function verifySignatureHex(pubKey, dataBytes, sigHex) {
  const sig = hexToBytes(sigHex);
  if (sig.length !== 64) return false;
  return crypto.subtle.verify({ name: "ECDSA", hash: "SHA-256" }, pubKey, sig, dataBytes);
}

// Deterministic bytes the server signed when issuing the certificate.
function certSigningPayload(pubkeyHex, subject) {
  return enc.encode(pubkeyHex + "|" + subject);
}

export async function verifyCertificate(serverPubKey, cert) {
  return verifySignatureHex(serverPubKey, certSigningPayload(cert.public_key, cert.subject), cert.signature);
}

export async function certificateToPubKey(cert) {
  return importSEPublicKey(cert.public_key);
}

// Flag decryption intentionally omitted -- the flag stays encrypted in the
// browser (AES-128-CBC under the previous challenge's key) and is decrypted
// offline by the operator, so no key or decrypt routine ships here.
