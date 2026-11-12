

const SERVER = process.argv[2] || "ws://127.0.0.1:8787";

function open() {
  return new Promise((resolve, reject) => {
    const ws = new WebSocket(`${SERVER}/global`);
    const queue = [];
    const waiters = [];
    ws.onmessage = (e) => {
      const msg = JSON.parse(e.data);
      if (waiters.length) waiters.shift()(msg);
      else queue.push(msg);
    };
    ws.onerror = reject;
    ws.onopen = () =>
      resolve({
        send: (o) => ws.send(JSON.stringify(o)),
        next: (ms = 2000) =>
          queue.length
            ? Promise.resolve(queue.shift())
            : new Promise((res) => {
                const t = setTimeout(() => res(null), ms);
                waiters.push((m) => (clearTimeout(t), res(m)));
              }),
        close: () => ws.close(),
      });
  });
}

function check(what, ok) {
  console.log(`${ok ? "ok  " : "FAIL"} ${what}`);
  if (!ok) process.exitCode = 1;
}

async function nextOp(peer, op, matches = () => true) {
  for (let i = 0; i < 10; ++i) {
    const msg = await peer.next();
    if (!msg) return null;
    if (msg.op === op && matches(msg)) return msg;
  }
  return null;
}

function hasRoster(msg, expected) {
  if (msg?.op !== "roster" || !Array.isArray(msg.players) || msg.players.length % 2 !== 0) {
    return false;
  }
  const players = new Map();
  for (let i = 0; i < msg.players.length; i += 2) {
    players.set(String(msg.players[i]), msg.players[i + 1]);
  }
  return players.size === expected.length &&
    expected.every(([id, name]) => players.get(String(id)) === name);
}

const a = await open();
const b = await open();
const c = await open();
const wa = await a.next();
const wb = await b.next();
const wc = await c.next();
check("welcome with an id", wa.op === "welcome" && wb.id !== wa.id && wc.id !== wb.id);

a.send({ op: "hello", v: 99, name: "Alice", ep: "203.0.113.1:1001", port: 1001 });
check("hello publishes the first online name",
  hasRoster(await nextOp(a, "roster"), [[wa.id, "Alice"]]));
b.send({ op: "hello", v: 99, name: "Bob", ep: "203.0.113.2:1002", port: 1002 });
check("hello publishes the roster to existing and new clients",
  hasRoster(await nextOp(a, "roster"), [[wa.id, "Alice"], [wb.id, "Bob"]]) &&
  hasRoster(await nextOp(b, "roster"), [[wa.id, "Alice"], [wb.id, "Bob"]]));
c.send({ op: "area", stage: "F_SP104" });
await c.next();
c.send({ op: "hello", v: 99, name: "Charlie", ep: "203.0.113.3:1003", port: 1003 });
const threePlayers = [[wa.id, "Alice"], [wb.id, "Bob"], [wc.id, "Charlie"]];
check("all online names are broadcast across game areas",
  hasRoster(await nextOp(a, "roster"), threePlayers) &&
  hasRoster(await nextOp(b, "roster"), threePlayers) &&
  hasRoster(await nextOp(c, "roster"), threePlayers));
a.send({ op: "name", name: "Alice Updated" });
check("renames update every online roster",
  hasRoster(await nextOp(a, "roster"), [[wa.id, "Alice Updated"], [wb.id, "Bob"], [wc.id, "Charlie"]]) &&
  hasRoster(await nextOp(b, "roster"), [[wa.id, "Alice Updated"], [wb.id, "Bob"], [wc.id, "Charlie"]]) &&
  hasRoster(await nextOp(c, "roster"), [[wa.id, "Alice Updated"], [wb.id, "Bob"], [wc.id, "Charlie"]]));

a.send({ op: "area", stage: "F_SP103" });
await nextOp(a, "count");
b.send({ op: "area", stage: "F_SP103" });
const toB = await nextOp(b, "peer");
const toA = await nextOp(a, "peer");
check("same area: introduced to each other", toB.op === "peer" && toA.op === "peer" &&
  toB.id === wa.id && toA.id === wb.id && toA.key === toB.key && /^[0-9a-f]{64}$/.test(toA.key));
check("a home address only goes to the same network", toA.sameNet || !toA.eps.some((e) => e.startsWith("192.168.")));

c.send({ op: "area", stage: "F_SP104" });
const toC = await nextOp(c, "count");
check("other area: nobody introduced", toC.op === "count" && toC.here === 1 && toC.total === 3);

b.send({ op: "area", stage: "F_SP104" });
const goneA = await nextOp(a, "gone");
const joinedC = await nextOp(b, "peer");
check("walking out: the old area hears gone", goneA && goneA.op === "gone" && goneA.id === wb.id);
check("walking in: introduced to the new area",
  joinedC && joinedC.op === "peer" && joinedC.id === wc.id);

b.send({ op: "block", id: wc.id });
const bGone = await nextOp(b, "gone");
const cGone = await nextOp(c, "gone");
check("blocking: both hear gone", bGone && bGone.id === wc.id &&
  cGone && cGone.id === wb.id);
b.send({ op: "unblock", id: wc.id });
const unblockMsg = await nextOp(b, "peer");
check("unblocking: introduced again", unblockMsg && unblockMsg.op === "peer" && unblockMsg.id === wc.id);
await nextOp(c, "peer");

c.close();
const goneC = await nextOp(b, "gone");
check("closing: the area hears gone", goneC && goneC.op === "gone" && goneC.id === wc.id);

a.send({ op: "area", stage: "../etc" });
const bad = await nextOp(a, "count", (msg) => msg.here === 0);
check("a bad area name is no area", bad && bad.op === "count" && bad.here === 0);

a.close();
b.close();

const firstSession = await open();
const firstSessionWelcome = await nextOp(firstSession, "welcome");
firstSession.send({
  op: "hello",
  v: 99,
  name: "Robin",
  key: "0123456789abcdef0123456789abcdef",
  heartbeat: true,
  ep: "203.0.113.4:1004",
  port: 1004,
});
await nextOp(firstSession, "roster");

const replacementSession = await open();
const replacementWelcome = await nextOp(replacementSession, "welcome");
replacementSession.send({
  op: "hello",
  v: 99,
  name: "Robin",
  key: "0123456789abcdef0123456789abcdef",
  heartbeat: true,
  ep: "203.0.113.5:1005",
  port: 1005,
});
const replacementCount = await nextOp(replacementSession, "count");
const replacementRoster = await nextOp(replacementSession, "roster");
const superseded = await nextOp(firstSession, "superseded");
check("reconnecting with the same installation replaces its stale session",
  firstSessionWelcome && replacementWelcome && replacementWelcome.heartbeat === true &&
  replacementCount && replacementCount.total === 1 &&
  hasRoster(replacementRoster, [[replacementWelcome.id, "Robin"]]) &&
  superseded && superseded.op === "superseded");
replacementSession.send({ op: "ping" });
check("presence heartbeat is acknowledged",
  (await nextOp(replacementSession, "pong"))?.op === "pong");
replacementSession.close();
setTimeout(() => process.exit(), 200);
