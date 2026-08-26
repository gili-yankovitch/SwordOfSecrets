// client.js -- the NEXUS Operator Terminal client. Port of client.py.
//
// READ ME. This file is the teaching artifact of the challenge. It is the
// honest operator client: it proxies your commands between you, the NEXUS
// server (server.js, running in this browser), and the Secure Element -- the
// physical Sword of Secrets, reached over WebSerial (se-webserial.js).
//
// The full handshake for every command is:
//   1. CLIENT_HELLO   -- the sword signs a hello; we send it to the server.
//   2. SERVER_CHALLENGE -- the server replies with a random nonce.
//   3. CLIENT_RESPONSE -- the sword signs (nonce + action + params); we send it.
//   4. SERVER_RESULT   -- the server verifies everything and runs the action.
//
// Notice how a message looks on the wire (turn on DEBUG to watch it):
//   { "version": 2, "body": {...}, "integrity": "...", "signature": "..." }
// The "version" sits OUTSIDE "body". The integrity + signature only cover
// "body". Who decides the version, and what changes if it were 1 instead of 2?
// (The Secure Element only signs LIST in v2. In v1 it also signs READ. It
// never signs WRITE. The server, though, happily runs any action whose
// message verifies. Hmm.)
//
// This honest client always speaks v2 -- so LIST works, but READ and WRITE are
// refused by the sword. To go further you drive the protocol yourself through
// the exposed `window.nexus` tool (see tool.js). Everything you need is there.

import * as P from "./protocol.js";

export class NexusClient {
  constructor(sword, server) {
    this.sword = sword;   // SwordSerial (the physical Secure Element)
    this.server = server; // NexusServer (in-browser)
    this.certificate = null;
    this.onWire = null;   // optional (dir, msg) hook for the DEBUG view
    this.onReveal = null; // optional (resultBody) hook fired on a winning WRITE
  }

  _wire(dir, msg) { this.onWire?.(dir, msg); }

  async getCertificate() {
    if (!this.certificate) this.certificate = await this.sword.getCertificate();
    return this.certificate;
  }

  // --- low-level handshake building blocks (reused by the exploit tool) ----

  // Open a connection and get through steps 1-2. Returns { conn, nonce }.
  // `version` is the version we tell the SERVER in CLIENT_HELLO.
  async beginHandshake(version = P.VERSION_SECURE) {
    await this.getCertificate();
    const hello = await this.sword.signHello(version, 0);
    const conn = this.server.newConnection();
    const helloMsg = P.buildMessage(version, hello.body, hello.integrity, hello.signature);
    this._wire("client->server", helloMsg);
    const challenge = await conn.clientHello(helloMsg);
    this._wire("server->client", challenge);
    const body = challenge.body ?? {};
    if (body.type === P.SERVER_RESULT) return { conn, nonce: null, error: body };
    return { conn, nonce: body.nonce };
  }

  // Send a fully-formed CLIENT_RESPONSE to the server. Returns the result body.
  //
  // A valid emergency-shutdown WRITE makes the server signal `reveal` and hand
  // back the flag STILL ENCRYPTED (AES-256-CBC). It is sealed under SHA-256 of
  // the PREVIOUS challenge's final flag -- the grand prize a solver carries over
  // from the hardware Sword. We deliberately do NOT decrypt it here: pulling off
  // the protocol attack gets you the ciphertext; finishing the earlier challenge
  // gets you the key, and you decrypt the blob offline.
  async submitResponse(conn, version, body, integrity, signature) {
    const responseMsg = P.buildMessage(version, body, integrity, signature);
    this._wire("client->server", responseMsg);
    const result = await conn.clientResponse(responseMsg);
    this._wire("server->client", result);
    const out = result.body ?? result;

    if (out.reveal && out.flag_blob) {
      out.flagEncrypted = out.flag_blob; // surfaced for offline decryption
    } else if (out.reveal && !out.flag_blob) {
      out.flagError = "Flag blob not provisioned.";
    }
    if (out.reveal) this.onReveal?.(out);
    return out;
  }

  // --- the honest, v2-only command path (LIST/READ/WRITE) ------------------

  async command(action, params) {
    const version = P.VERSION_SECURE; // honest client: always the highest version

    const { conn, nonce, error } = await this.beginHandshake(version);
    if (error) return error;

    // Ask the sword to sign the challenge for this action.
    const se = await this.sword.signChallenge(version, nonce, action, params ?? {}, 1);
    if (se.status !== "OK") {
      // e.g. "action not permitted in v2" for READ/WRITE -- the restriction.
      return { status: "ERROR", error: se.error || "SE signing failed" };
    }

    return this.submitResponse(conn, version, se.body, se.integrity, se.signature);
  }
}
