// server.js -- the NEXUS server, running in the browser.  Port of server.py.
//
// It accepts ALL actions (LIST/READ/WRITE) regardless of protocol version --
// exactly like the real server. The only gate is cryptographic: a message must
// carry a valid integrity + a valid SE signature over that integrity + a valid
// certificate + a valid nonce_response. The action-vs-version policy lives in
// the Secure Element (the sword), NOT here. That asymmetry is what the attack
// abuses: downgrade to v1 at the sword to unlock READ, then reuse a v1
// signature (over a 16-bit CRC) on a forged WRITE body that collides that CRC.
//
// The flag is NOT here in plaintext. A valid forged WRITE returns the shutdown
// message plus the AES-GCM flag blob; only the physical sword can produce the
// key to decrypt it (see terminal.js / crypto.js).

import * as P from "./protocol.js";
import * as C from "./crypto.js";
import { DATA } from "./data.js";

const enc = new TextEncoder();

export class NexusServer {
  constructor() {
    this.serverPubKey = null;
  }

  async init() {
    this.serverPubKey = await C.importServerPublicKey(DATA.SERVER_PUBLIC_SPKI_B64);
  }

  // Mirrors server.py NexusServer._verify_message.
  // Returns { ok, error, sePub }.
  async _verifyMessage(msg) {
    const version = msg.version ?? P.VERSION_SECURE;
    const body = msg.body ?? {};
    const integrity = msg.integrity ?? "";
    const signature = msg.signature ?? "";

    if (!(await P.verifyIntegrity(version, body, integrity)))
      return { ok: false, error: "Integrity check failed" };

    const cert = body.certificate;
    if (!cert) return { ok: false, error: "Missing certificate in body" };

    if (!(await C.verifyCertificate(this.serverPubKey, cert)))
      return { ok: false, error: "Certificate verification failed" };

    const sePub = await C.certificateToPubKey(cert);
    if (!(await C.verifySignatureHex(sePub, enc.encode(integrity), signature)))
      return { ok: false, error: "Signature verification failed" };

    return { ok: true, error: "", sePub };
  }

  // ---- action dispatch (mirrors server.py) --------------------------------

  doList() {
    return { status: "OK", files: DATA.FILE_MANIFEST };
  }

  doRead(params) {
    const path = params?.path ?? "";
    if (!path) return { status: "ERROR", error: "Missing 'path' parameter" };
    if (path.includes("..") || path.startsWith("/")) return { status: "ERROR", error: "Invalid path" };
    if (!(path in DATA.FILES)) return { status: "ERROR", error: `File not found: ${path}` };
    return { status: "OK", path, content: DATA.FILES[path] };
  }

  doWrite(params) {
    const path = params?.path ?? "";
    const data = params?.data ?? "";
    if (path === "core/emergency_shutdown" && data === "KILL_SWITCH_ENGAGE") {
      return {
        status: "OK",
        message:
          "EMERGENCY SHUTDOWN SEQUENCE INITIATED\n\n" +
          "  NEXUS neural core ......... OFFLINE\n" +
          "  Threat assessment grid .... OFFLINE\n" +
          "  Network propagation ....... HALTED\n" +
          "  Autonomous directives ..... PURGED\n\n" +
          "  All 147 nodes disconnected.\n" +
          "  Human override authority restored.\n",
        // Encrypted flag -- terminal.js decrypts it with the sword oracle.
        flag_blob: DATA.FLAG_BLOB,
        reveal: true,
      };
    }
    return { status: "ERROR", error: `NEXUS: Nice try. Write to '${path}' denied. I'm watching you.` };
  }

  dispatch(action, params) {
    if (action === P.ACTION_LIST) return this.doList();
    if (action === P.ACTION_READ) return this.doRead(params);
    if (action === P.ACTION_WRITE) return this.doWrite(params);
    return { status: "ERROR", error: `Unknown action: ${action}` };
  }

  // A fresh connection = one handshake. Mirrors server.py handle_connection,
  // but as direct method calls instead of TCP.
  newConnection() {
    return new NexusConnection(this);
  }
}

export class NexusConnection {
  constructor(server) {
    this.server = server;
    this.nonce = null;
    this.version = P.VERSION_SECURE;
  }

  // Step 1 -> 2: receive CLIENT_HELLO, return SERVER_CHALLENGE (or error result).
  async clientHello(msg) {
    const body = msg.body ?? {};
    if (body.type !== P.CLIENT_HELLO)
      return this._result(P.VERSION_SECURE, { status: "ERROR", error: "Expected CLIENT_HELLO" });

    const v = await this.server._verifyMessage(msg);
    if (!v.ok) return this._result(P.VERSION_SECURE, { status: "ERROR", error: v.error });

    this.version = msg.version ?? P.VERSION_SECURE;
    const nonceBytes = new Uint8Array(32);
    crypto.getRandomValues(nonceBytes);
    this.nonce = C.bytesToHex(nonceBytes);
    return P.buildMessage(this.version, { type: P.SERVER_CHALLENGE, nonce: this.nonce });
  }

  // Step 3 -> 4: receive CLIENT_RESPONSE, verify nonce, dispatch, return RESULT.
  async clientResponse(msg) {
    const body = msg.body ?? {};
    const respVersion = msg.version ?? P.VERSION_SECURE;
    if (body.type !== P.CLIENT_RESPONSE)
      return this._result(respVersion, { status: "ERROR", error: "Expected CLIENT_RESPONSE" });

    const v = await this.server._verifyMessage(msg);
    if (!v.ok) return this._result(respVersion, { status: "ERROR", error: v.error });

    const nonceResponse = body.nonce_response ?? "";
    if (!(await C.verifySignatureHex(v.sePub, C.hexToBytes(this.nonce), nonceResponse)))
      return this._result(respVersion, { status: "ERROR", error: "Nonce verification failed" });

    const result = this.server.dispatch(body.action ?? "", body.params ?? {});
    return this._result(respVersion, result);
  }

  _result(version, resultBody) {
    resultBody.type = P.SERVER_RESULT;
    return P.buildMessage(version, resultBody);
  }
}
