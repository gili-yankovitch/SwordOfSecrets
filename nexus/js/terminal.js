// terminal.js -- the cyberpunk NEXUS Operator Terminal (DOM). Port of the
// client.py rich TUI: banner, prompt, LIST/READ/WRITE/DEBUG/HELP, rich-style
// panels + tables, and the protocol DEBUG wire dump.

import { NexusServer } from "./server.js";
import { NexusClient } from "./client.js";
import { SwordSerial, WEBSERIAL_SUPPORTED } from "./se-webserial.js";
import { installTool } from "./tool.js";
import * as P from "./protocol.js";

const BANNER = String.raw`
    ███╗   ██╗███████╗██╗  ██╗██╗   ██╗███████╗
    ████╗  ██║██╔════╝╚██╗██╔╝██║   ██║██╔════╝
    ██╔██╗ ██║█████╗   ╚███╔╝ ██║   ██║███████╗
    ██║╚██╗██║██╔══╝   ██╔██╗ ██║   ██║╚════██║
    ██║ ╚████║███████╗██╔╝ ╚██╗╚██████╔╝███████║
    ╚═╝  ╚═══╝╚══════╝╚═╝   ╚═╝ ╚═════╝ ╚══════╝
    ──── SECURE OPERATOR TERMINAL v2.1 ────
    "The only way to win is to pull the plug."`;

const HELP = `Available commands:

  LIST                    List files on the NEXUS server
  READ <path>             Read a file from the server
  WRITE <path> <data>     Write data to a path on the server
  DEBUG                   Toggle protocol debug output
  CLEAR                   Clear the screen
  HELP                    Show this help
  EXIT                    Disconnect`;

export class Terminal {
  constructor(root) {
    this.root = root;
    this.debug = false;
    this.client = null;
    this.sword = null;
    this.server = null;
    this.busy = false;
    this._build();
  }

  _build() {
    this.root.classList.add("nexus-term");
    this.out = document.createElement("div");
    this.out.className = "term-out";
    this.inputLine = document.createElement("div");
    this.inputLine.className = "term-inline";
    this.promptEl = document.createElement("span");
    this.promptEl.className = "prompt";
    this.promptEl.textContent = "nexus://operator>";
    this.input = document.createElement("input");
    this.input.className = "term-in";
    this.input.setAttribute("autocomplete", "off");
    this.input.setAttribute("spellcheck", "false");
    this.input.disabled = true;
    this.inputLine.append(this.promptEl, this.input);
    this.root.append(this.out, this.inputLine);

    this.input.addEventListener("keydown", (e) => {
      if (e.key === "Enter" && !this.busy) {
        const raw = this.input.value;
        this.input.value = "";
        this.echoLine(`nexus://operator> ${raw}`, "cmd");
        this.runCommand(raw);
      }
    });
    this.root.addEventListener("mouseup", (e) => {
      const sel = window.getSelection();
      const isSelecting = sel && sel.toString().length > 0;
      if (!isSelecting) {
        this.input.focus();
      }
    });
    //this.root.addEventListener("click", () => this.input.focus());
  }

  // ---- output primitives (rich-ish) --------------------------------------

  echo(text, cls) {
    const pre = document.createElement("pre");
    pre.className = "line" + (cls ? " " + cls : "");
    pre.textContent = text;
    this.out.appendChild(pre);
    this._scroll();
    return pre;
  }
  echoLine(text, cls) { return this.echo(text, cls); }

  panel(title, body, style = "green") {
    const box = document.createElement("div");
    box.className = `panel panel-${style}`;
    if (title) {
      const t = document.createElement("div");
      t.className = "panel-title";
      t.textContent = title;
      box.appendChild(t);
    }
    const b = document.createElement("pre");
    b.className = "panel-body";
    b.textContent = body;
    box.appendChild(b);
    this.out.appendChild(box);
    this._scroll();
    return box;
  }

  table(files) {
    const tbl = document.createElement("table");
    tbl.className = "nexus-table";
    tbl.innerHTML =
      "<thead><tr><th>Path</th><th>Description</th><th>Access</th></tr></thead>";
    const tb = document.createElement("tbody");
    for (const f of files) {
      const tr = document.createElement("tr");
      const acc = f.access === "READ" ? "acc-read" : "acc-write";
      tr.innerHTML =
        `<td class="col-path">${esc(f.path)}</td>` +
        `<td>${esc(f.description)}</td>` +
        `<td class="${acc}">${esc(f.access)}</td>`;
      tb.appendChild(tr);
    }
    tbl.appendChild(tb);
    const wrap = document.createElement("div");
    wrap.className = "table-wrap";
    wrap.appendChild(tbl);
    this.out.appendChild(wrap);
    this._scroll();
  }

  _scroll() { this.out.scrollTop = this.out.scrollHeight; }

  // ---- boot / connect -----------------------------------------------------

  mount() {
    this.echo(BANNER, "banner");
    this.panel(
      null,
      "Establish a secure channel to the NEXUS defense network.\n" +
        "Your mission: find the emergency shutdown and terminate the rogue AI.\n" +
        "Type HELP for available commands.",
      "red",
    );
    if (!WEBSERIAL_SUPPORTED) {
      this.panel(
        "UNSUPPORTED BROWSER",
        "This terminal drives the physical Sword of Secrets over WebSerial,\n" +
          "which needs Chrome or Edge (desktop) over HTTPS. Firefox/Safari won't work.",
        "red",
      );
      return;
    }
    this.echo('Click "Connect Sword" (top right) to attach your Secure Element over serial.', "dim");
  }

  async connect() {
    if (this.client) return;
    this.echo("Requesting serial port...", "dim");
    let sword;
    try {
      sword = await SwordSerial.request();
    } catch (e) {
      this.panel("CONNECTION FAILED", "Could not open the sword: " + e.message, "red");
      return;
    }
    sword.onDebug = (dir, obj) => this.debugWire(dir, obj);

    const server = new NexusServer();
    await server.init();
    const client = new NexusClient(sword, server);
    client.onWire = (dir, obj) => this.debugWire(dir, obj);
    client.onReveal = (result) => this.displayResult(result, P.ACTION_WRITE);

    this.sword = sword;
    this.server = server;
    this.client = client;
    installTool({ client, sword, server, echo: (t) => this.echo(t) });

    this.panel("SECURE ELEMENT ONLINE", "Sword connected @115200. Channel to NEXUS established.", "green");
    this.echo("Type HELP for commands. DevTools: nexus.help()", "dim");
    this.input.disabled = false;
    this.input.focus();
  }

  // ---- REPL ---------------------------------------------------------------

  async runCommand(raw) {
    const parts = raw.trim().split(/\s+/);
    if (!parts[0]) return;
    const cmd = parts[0].toUpperCase();

    if (cmd === "HELP" || cmd === "?") return void this.echo(HELP, "help");
    if (cmd === "CLEAR") return void (this.out.innerHTML = "");
    if (cmd === "EXIT" || cmd === "QUIT") return void this.echo("Connection terminated.", "dim");
    if (cmd === "DEBUG") {
      this.debug = !this.debug;
      return void this.echo(`Protocol debug output: ${this.debug ? "ON" : "OFF"}`, "warn");
    }

    let action, params;
    if (cmd === "LIST") { action = P.ACTION_LIST; params = {}; }
    else if (cmd === "READ") {
      if (parts.length < 2) return void this.echo("Usage: READ <path>", "err");
      action = P.ACTION_READ; params = { path: parts[1] };
    } else if (cmd === "WRITE") {
      const m = raw.trim().match(/^\S+\s+(\S+)\s+([\s\S]+)$/);
      if (!m) return void this.echo("Usage: WRITE <path> <data>", "err");
      action = P.ACTION_WRITE; params = { path: m[1], data: m[2] };
    } else {
      this.echo(`Unknown command: ${cmd}`, "err");
      return void this.echo("Type HELP for available commands.", "dim");
    }

    this.busy = true;
    const spin = this.echo("... contacting Secure Element + NEXUS ...", "dim");
    try {
      const result = await this.client.command(action, params);
      spin.remove();
      this.displayResult(result, action);
    } catch (e) {
      spin.remove();
      this.panel("ERROR", e.message, "red");
    } finally {
      this.busy = false;
      this.input.focus();
    }
  }

  displayResult(result, action) {
    const status = result.status ?? "UNKNOWN";
    if (status === "ERROR") {
      return void this.panel("ACCESS DENIED", result.error ?? "Unknown error", "red");
    }
    if (action === P.ACTION_LIST) return void this.table(result.files ?? []);
    if (action === P.ACTION_READ) {
      return void this.panel(result.path ?? "?", result.content ?? "", "green");
    }
    if (action === P.ACTION_WRITE) {
      let body = result.message ?? "Write acknowledged.";
      if (result.flagEncrypted) {
        const b = result.flagEncrypted;
        body += `\n  ENCRYPTED FLAG (${b.algo}, iv=${b.iv}):\n  ${b.ct}\n`;
        body += "\n  Sealed under SHA-256 of the Sword's final flag.\n";
        body += "  Finish that challenge, then decrypt this blob offline.\n";
      } else if (result.flagError) body += `\n  [${result.flagError}]\n`;
      body += "\n  Congratulations, operator. NEXUS has been neutralized.\n";
      return void this.panel("NEXUS TERMINATED", body, "bright");
    }
    this.panel("RESPONSE", JSON.stringify(result, null, 2), "cyan");
  }

  debugWire(dir, obj) {
    if (!this.debug) return;
    this.panel(dir, JSON.stringify(obj, null, 2), "yellow");
  }
}

function esc(s) {
  return String(s).replace(/[&<>"]/g, (c) => ({ "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;" }[c]));
}
