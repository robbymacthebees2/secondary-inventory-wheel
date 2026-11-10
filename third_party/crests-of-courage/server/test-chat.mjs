

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
        next: (op, ms = 1500) =>
          new Promise((res) => {
            let waiter = null;

            const t = setTimeout(() => {
              const i = waiters.indexOf(waiter);
              if (i >= 0) waiters.splice(i, 1);
              res(null);
            }, ms);
            const look = () => {
              const i = queue.findIndex((m) => m.op === op);
              if (i >= 0) {
                clearTimeout(t);
                return res(queue.splice(i, 1)[0]);
              }
              waiter = (m) => {
                queue.push(m);
                look();
              };
              waiters.push(waiter);
            };
            look();
          }),
        close: () => ws.close(),
      });
  });
}

function check(what, ok) {
  console.log(`${ok ? "ok  " : "FAIL"} ${what}`);
  if (!ok) process.exitCode = 1;
}

const a = await open();
const b = await open();
const c = await open();
const d = await open();
const peers = [a, b, c, d];
const welcomes = await Promise.all(peers.map((p) => p.next("welcome")));
for (const [i, p] of peers.entries()) {
  p.send({
    op: "hello",
    v: i === 3 ? 100 : 99,
    ep: "",
    port: i + 1,
    name: i === 1 ? "FuckFace" : `Player${i}`,
    key: `${String(i + 1).padStart(2, "0")}`.repeat(16),
  });
  await p.next("count");
}
peers[2].send({ op: "chat", text: "not in an area yet" });
check("chat is rejected until the client has a stage", (await peers[2].next("chat_unavailable")) !== null);
for (const [i, p] of peers.entries()) {
  p.send({ op: "area", stage: i === 2 ? "F_SP104" : "F_SP103" });
  await p.next("count");
}

a.send({ op: "chat", text: "what the fuck is this" });
const toB = await b.next("chat");
const toA = await a.next("chat");
check("chat reaches peers in the same area and version", toB && toA && toB.text === toA.text);
check("the server names the sender", toB && toB.name === "Player0");
check("and tags them from their key", toB && /^[0-9a-f]{16}$/.test(toB.tag));
check("chat text is filtered", toB && toB.text === "what the **** is this");
check("other areas do not receive chat", (await c.next("chat", 100)) === null);
check("other mod versions do not receive chat", (await d.next("chat", 100)) === null);

a.send({ op: "name", name: "Zelda" });
const renamed = await b.next("rename");
check("renames stay within the same area and version", renamed && renamed.name === "Zelda");
check("renames do not leak across areas", (await c.next("rename", 100)) === null);
check("renames do not leak across mod versions", (await d.next("rename", 100)) === null);

a.send({ op: "chat", text: "again" });
check("two seconds between messages", (await a.next("chat_slow")) !== null);

b.send({ op: "chat", text: "hi" });
const fromB = await a.next("chat");
check("a bad name is replaced", fromB && fromB.name === "Player");

await new Promise((r) => setTimeout(r, 2100));
b.send({ op: "chat", text: "x".repeat(300) });
const long = await a.next("chat");
check("chat is capped at 100 characters", long && long.text.length === 100);

const late = await open();
late.send({
  op: "hello",
  v: 99,
  ep: "",
  port: 5,
  name: "Late",
  key: "05050505050505050505050505050505",
});
await late.next("count");
late.send({ op: "area", stage: "F_SP103" });
await late.next("count");
check("new joiners do not receive chat history", (await late.next("chat", 100)) === null);

a.send({ op: "block", id: welcomes[1].id });
await new Promise((r) => setTimeout(r, 100));
await a.next("gone");
b.send({ op: "chat", text: "blocked message" });
check("blocked peers do not receive chat", (await a.next("chat", 100)) === null);

for (const p of [...peers, late]) p.close();
setTimeout(() => process.exit(), 200);
