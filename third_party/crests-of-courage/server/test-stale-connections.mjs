import assert from "node:assert/strict";
import { Global, Room } from "./src/index.js";

class Socket {
  constructor(tags, attachment) {
    this.tags = tags;
    this.attachment = structuredClone(attachment);
    this.messages = [];
    this.closed = false;
  }

  send(text) {
    if (this.closed) throw new Error("socket closed");
    this.messages.push(JSON.parse(text));
  }

  close(code = 1000, reason = "") {
    this.closed = true;
    this.closeCode = code;
    this.closeReason = reason;
  }

  serializeAttachment(value) {
    this.attachment = structuredClone(value);
  }

  deserializeAttachment() {
    return structuredClone(this.attachment);
  }
}

function context() {
  const sockets = new Set();
  const values = new Map();
  let alarmAt = null;
  return {
    acceptWebSocket(ws, tags) {
      ws.tags = tags;
      sockets.add(ws);
    },
    getWebSockets(tag) {
      return [...sockets].filter((ws) => !ws.closed && (!tag || ws.tags.includes(tag)));
    },
    storage: {
      async get(key) {
        return Array.isArray(key)
          ? new Map(key.filter((item) => values.has(item)).map((item) => [item, values.get(item)]))
          : values.get(key);
      },
      async put(key, value) {
        values.set(key, value);
      },
      async delete(key) {
        return values.delete(key);
      },
      async setAlarm(at) {
        alarmAt = at;
      },
      async deleteAlarm() {
        alarmAt = null;
      },
    },
    alarmAt: () => alarmAt,
  };
}

const realNow = Date.now;
let now = realNow();
Date.now = () => now;

try {
  const roomContext = context();
  const room = new Room(roomContext);
  const host = new Socket(["host"], {
    role: "host",
    hello: null,
    lastSeen: now,
    heartbeat: false,
    expired: false,
  });
  roomContext.acceptWebSocket(host, ["host"]);

  await room.webSocketMessage(host, JSON.stringify({
    op: "hello",
    v: 1,
    ep: "",
    port: 1,
    heartbeat: true,
  }));
  await room.webSocketMessage(host, JSON.stringify({ op: "ping" }));
  assert.equal(host.messages.at(-1).op, "pong");
  assert.equal(roomContext.alarmAt(), now + 60_000);

  now += 60_001;
  assert.equal(room.host(), null, "expired host must not reserve its room");
  await room.alarm();
  assert.equal(host.closed, true, "room alarm must close a missed heartbeat");
  assert.equal(roomContext.alarmAt(), null, "empty room must cancel its alarm");

  const failedRoomContext = context();
  const failedRoom = new Room(failedRoomContext);
  const failedHost = new Socket(["host"], {
    role: "host",
    hello: null,
    lastSeen: now,
    heartbeat: false,
    expired: true,
  });
  failedRoomContext.acceptWebSocket(failedHost, ["host"]);
  await failedRoom.scheduleCleanup();
  assert.equal(failedRoomContext.alarmAt(), now + 1, "failed sockets must get cleanup alarms");
  await failedRoom.alarm();
  assert.equal(failedHost.attachment.cleaned, true, "failed room sockets must be reaped");

  now += 9 * 60_000;
  const legacyContext = context();
  const legacyRoom = new Room(legacyContext);
  const legacyHost = new Socket(["host"], {
    role: "host",
    hello: { v: 1 },
    lastSeen: now,
    heartbeat: false,
    expired: false,
  });
  legacyContext.acceptWebSocket(legacyHost, ["host"]);
  now += 9 * 60_000;
  await legacyRoom.alarm();
  assert.equal(legacyHost.closed, false, "legacy sessions receive the longer grace period");
  now += 60_001;
  await legacyRoom.alarm();
  assert.equal(legacyHost.closed, true, "idle legacy rooms must eventually be reclaimed");

  const legacyGlobalContext = context();
  const legacyLobby = new Global(legacyGlobalContext, {});
  const legacyPlayer = new Socket(["global"], {
    id: 3,
    hello: true,
    heartbeat: false,
    lastSeen: now,
    peers: [],
    area: "",
    expired: false,
    removed: false,
  });
  legacyGlobalContext.acceptWebSocket(legacyPlayer, ["global"]);
  await legacyLobby.scheduleCleanup();
  assert.equal(legacyGlobalContext.alarmAt(), now + 10 * 60_000);
  now += 9 * 60_000;
  await legacyLobby.alarm();
  assert.equal(legacyPlayer.closed, false, "legacy global sessions receive the longer grace period");
  now += 60_001;
  await legacyLobby.alarm();
  assert.equal(legacyPlayer.closed, true, "idle legacy global sessions must eventually be reclaimed");

  const globalContext = context();
  const lobby = new Global(globalContext, {});
  const player = new Socket(["global"], {
    id: 1,
    hello: true,
    heartbeat: true,
    lastSeen: now,
    peers: [],
    area: "",
    expired: false,
  });
  globalContext.acceptWebSocket(player, ["global"]);
  now += 60_001;
  assert.equal(lobby.all().length, 0, "expired presence must not count toward capacity");
  await lobby.alarm();
  assert.equal(player.closed, true, "global alarm must close missed heartbeats");
  assert.equal(player.attachment.removed, true, "global close must remove the stale session");
  assert.equal(globalContext.alarmAt(), null, "empty lobby must cancel its alarm");

  const failedGlobalContext = context();
  const failedLobby = new Global(failedGlobalContext, {});
  const failedPlayer = new Socket(["global"], {
    id: 2,
    hello: true,
    heartbeat: true,
    lastSeen: now,
    peers: [],
    area: "",
    expired: true,
    removed: false,
  });
  failedGlobalContext.acceptWebSocket(failedPlayer, ["global"]);
  await failedLobby.scheduleCleanup();
  assert.equal(failedGlobalContext.alarmAt(), now + 1, "failed presence must get cleanup alarms");
  await failedLobby.alarm();
  assert.equal(failedPlayer.attachment.removed, true, "failed presence must be reaped");

  console.log("stale connection checks passed");
} finally {
  Date.now = realNow;
}
