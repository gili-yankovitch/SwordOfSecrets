// tool.js -- the operator's exploit toolkit, exposed as `window.nexus`.
//
// Open DevTools (F12) and type `nexus.help()`. These are the raw protocol
// primitives -- the same ones client.js uses -- but with NO version or action
// restrictions imposed by the UI. Assembling them into the kill switch is the
// challenge. Nothing here spoils the solution; it just hands you the tools.

import * as P from "./protocol.js";
import * as C from "./crypto.js";
import { DATA } from "./data.js";

export function installTool({ client, sword, server, echo }) {
  const enc = new TextEncoder();

  const nexus = {
    // live objects
    client, sword, server,

    // public constants
    SE_CERTIFICATE: DATA.SE_CERTIFICATE,
    FIXED_NONCE: DATA.FIXED_NONCE,
    protocol: P,

    // ---- helpers ----------------------------------------------------------

    // canonical JSON exactly as the server/sword compute integrity over.
    canonical: (obj) => P.canonicalJSON(obj),

    // CRC-16-CCITT (v1 integrity). Accepts a string or an object (canonicalized).
    crc16: (x) => {
      const bytes = typeof x === "string" ? enc.encode(x) : P.canonicalBytes(x);
      return P.crc16(bytes).toString(16).padStart(4, "0");
    },

    // SHA-256 hex (v2 integrity). Accepts a string or an object.
    sha256: (x) => {
      const bytes = typeof x === "string" ? enc.encode(x) : P.canonicalBytes(x);
      return P.sha256hex(bytes);
    },

    buildMessage: P.buildMessage,

    // ---- Secure Element (the sword) --------------------------------------

    // Ask the sword to sign a challenge. version/action are yours to choose --
    // the sword enforces its own policy and may refuse. Returns the raw
    // {status, body, integrity, signature} (status may be "ERROR").
    signChallenge: (version, nonce, action, params = {}, counter = 1) =>
      sword.signChallenge(version, nonce, action, params, counter),

    signHello: (version = 2, counter = 0) => sword.signHello(version, counter),
    getCertificate: () => sword.getCertificate(),

    // ---- Server -----------------------------------------------------------

    // Open a handshake; returns { conn, nonce }. `version` is what you tell the
    // server in CLIENT_HELLO (it lives outside the signed body...).
    beginHandshake: (version = 2) => client.beginHandshake(version),

    // Send a CLIENT_RESPONSE you built yourself. Returns the server result body.
    // You choose every field: the wire version, the body, the integrity string,
    // and the signature. The server only checks that they are consistent.
    submit: (conn, version, body, integrity, signature) =>
      client.submitResponse(conn, version, body, integrity, signature),

    // print into the on-page terminal
    print: (text) => echo?.(String(text)),

    help() {
      const msg = `
NEXUS operator toolkit  --  window.nexus
========================================
Everything is async; use await.

State / constants:
  nexus.SE_CERTIFICATE     the sword's certificate (public)
  nexus.FIXED_NONCE        (used internally to unlock the flag; ignore for the attack)

Crypto helpers:
  nexus.canonical(obj)     -> canonical JSON string (what integrity is computed over)
  nexus.crc16(str|obj)     -> 4-hex CRC-16 (v1 integrity)
  await nexus.sha256(str|obj) -> 64-hex SHA-256 (v2 integrity)

Secure Element (the physical sword):
  await nexus.getCertificate()
  await nexus.signHello(version, counter)
  await nexus.signChallenge(version, nonce, action, params={}, counter=1)
      -> { status, body, integrity, signature }
      The sword enforces which actions it will sign per version.

Server:
  const { conn, nonce } = await nexus.beginHandshake(version)
  await nexus.submit(conn, version, body, integrity, signature)
      -> server result body. You control every field you send.

Tips:
  * Turn on DEBUG in the terminal to watch every message on the wire.
  * Read the source: docs/js/client.js and docs/js/protocol.js.
  * The mission target is in classified/project_2501.txt.
  * Stuck? See HINTS.md in the repo.
`;
      echo ? echo(msg) : console.log(msg);
      return undefined;
    },
  };

  if (typeof window !== "undefined") window.nexus = nexus;
  return nexus;
}
