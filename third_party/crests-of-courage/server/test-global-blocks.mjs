import { Global } from "./src/index.js";

class TestSocket {
  constructor(info) {
    this.info = info;
    this.messages = [];
  }

  deserializeAttachment() {
    return structuredClone(this.info);
  }

  serializeAttachment(info) {
    this.info = structuredClone(info);
  }

  send(text) {
    this.messages.push(JSON.parse(text));
  }

  close() {
    this.closed = true;
  }

  take(op) {
    const index = this.messages.findIndex((message) => message.op === op);
    if (index < 0) return null;
    return this.messages.splice(index, 1)[0];
  }
}

const sockets = [];
const storage = new Map();
const ctx = {
  getWebSockets: () => sockets,
  storage: {
    get: async (keys) => Array.isArray(keys)
      ? new Map(keys.filter((key) => storage.has(key)).map((key) => [key, storage.get(key)]))
      : storage.get(keys),
    put: async (key, value) => void storage.set(key, value),
    delete: async (key) => storage.delete(key),
    deleteAlarm: async () => {},
    setAlarm: async () => {},
  },
};
let server = new Global(ctx, {});
let failures = 0;

function check(description, passed) {
  console.log(`${passed ? "ok  " : "FAIL"} ${description}`);
  if (!passed) ++failures;
}

function connect(id) {
  const socket = new TestSocket({
    id,
    ip: "",
    hello: false,
    v: 0,
    ep: "",
    port: 0,
    lan: "",
    area: "",
    peers: [],
    window: Date.now(),
    changes: 0,
    lastSeen: Date.now(),
    heartbeat: false,
  });
  sockets.push(socket);
  return socket;
}

async function message(socket, value) {
  await server.webSocketMessage(socket, JSON.stringify(value));
}

function hasNoOp(socket, op) {
  return !socket.messages.some((message) => message.op === op);
}

function hasMessage(socket, op, matches) {
  return socket.messages.some((message) => message.op === op && matches(message));
}

function hasRoster(socket, expected) {
  const players = socket.take("roster")?.players;
  if (!Array.isArray(players) || players.length % 2 !== 0) return false;
  const roster = new Map();
  for (let i = 0; i < players.length; i += 2) {
    roster.set(String(players[i]), players[i + 1]);
  }
  return roster.size === expected.length &&
    expected.every(([id, name]) => roster.get(String(id)) === name);
}

async function tagFor(key) {
  const digest = await crypto.subtle.digest("SHA-256", new TextEncoder().encode(key));
  return [...new Uint8Array(digest)].slice(0, 8)
    .map((byte) => byte.toString(16).padStart(2, "0")).join("");
}

const blockerKey = "abcdef0123456789abcdef0123456789";
const targetKey = "0123456789abcdef0123456789abcdef";
const targetTag = await tagFor(targetKey);
const blocker = connect(1);
const target = connect(2);

await message(blocker, {
  op: "hello",
  v: 99,
  name: "Blocker",
  key: blockerKey,
});
await message(target, {
  op: "hello",
  v: 99,
  name: "Target",
  key: targetKey,
});
await message(blocker, { op: "area", stage: "F_SP105" });
await message(target, { op: "area", stage: "F_SP105" });
check("peer carries the stable installation tag", blocker.take("peer")?.tag === targetTag);
target.take("peer");
blocker.messages.length = 0;
target.messages.length = 0;

await message(blocker, { op: "block", id: 2, tag: targetTag });
check("blocking immediately unpairs both players",
  blocker.take("gone")?.id === 2 && target.take("gone")?.id === 1);
check("blocked tags are stored outside the size-limited WebSocket attachment",
  storage.has("global-blocked:1") && !("blockedTags" in blocker.info));
check("blocked player is removed from the roster",
  hasRoster(blocker, [[1, "Blocker"]]));
server = new Global(ctx, {});
await message(target, { op: "name", name: "Renamed" });
check("renaming does not restore roster visibility",
  hasRoster(blocker, [[1, "Blocker"]]));
await message(target, { op: "chat", text: "blocked message" });
check("renamed blocked player cannot send chat",
  !hasMessage(blocker, "chat", (message) => message.id === 2 && message.text === "blocked message"));
await message(blocker, { op: "chat", text: "message to blocked player" });
check("blocker cannot send chat to the blocked player",
  !hasMessage(target, "chat", (message) => message.id === 1 && message.text === "message to blocked player"));

await message(target, { op: "bye" });
sockets.splice(sockets.indexOf(target), 1);
const restartedTarget = connect(3);
await message(restartedTarget, {
  op: "hello",
  v: 99,
  name: "Target Restarted",
  key: targetKey,
});
check("target restart with a new session ID remains hidden",
  hasRoster(blocker, [[1, "Blocker"]]) &&
  hasNoOp(blocker, "peer"));
await message(restartedTarget, { op: "area", stage: "F_SP105" });
await message(restartedTarget, { op: "chat", text: "after target restart" });
check("restarted blocked player cannot send chat",
  !hasMessage(blocker, "chat", (message) => message.id === 3 && message.text === "after target restart"));

await message(blocker, { op: "bye" });
sockets.splice(sockets.indexOf(blocker), 1);
const restartedBlocker = connect(4);
await message(restartedBlocker, {
  op: "hello",
  v: 99,
  name: "Blocker",
  key: blockerKey,
  blocked: [targetTag],
});
check("blocker restart restores its saved tag list",
  hasRoster(restartedBlocker, [[4, "Blocker"]]));
await message(restartedBlocker, { op: "area", stage: "F_SP105" });
await message(restartedTarget, { op: "area", stage: "F_SP105" });
check("blocked connection is not reintroduced after blocker restart",
  hasNoOp(restartedBlocker, "peer") && hasNoOp(restartedTarget, "peer"));
await message(restartedTarget, { op: "chat", text: "after blocker restart" });
check("server continues filtering chat after blocker restart",
  hasNoOp(restartedBlocker, "chat"));

await message(restartedBlocker, { op: "unblock", id: 0, tag: targetTag });
check("unblocking by tag allows the player to return",
  restartedBlocker.take("peer")?.tag === targetTag &&
  hasRoster(restartedBlocker, [[3, "Target Restarted"], [4, "Blocker"]]));

process.exitCode = failures === 0 ? 0 : 1;
