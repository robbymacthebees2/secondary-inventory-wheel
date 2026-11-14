# Wheel Co-op rendezvous and chat server

This MIT-licensed Worker is the Crests of Courage-compatible service source vendored for the
combined Secondary Inventory Wheel. It supports room codes and the `/global` discovery/chat
protocol. It only introduces peers: gameplay snapshots, actors, and voice continue over direct
encrypted UDP. There is no gameplay or voice relay.

If two players' networks are too strict to connect directly, the mod tells them to use Tailscale.
There is no relay.

## Put it online (free, about five minutes)

You need [Node.js](https://nodejs.org) and a free [Cloudflare](https://dash.cloudflare.com/sign-up)
account. No credit card is needed.

```bash
cd server
npm install
npx wrangler login
npx wrangler deploy
```

`wrangler deploy` prints the address, such as
`https://secondary-inventory-wheel-rooms.<your-subdomain>.workers.dev`. The combined Wheel
currently defaults to `wss://secondary-inventory-wheel-rooms.scp173musicsalt.workers.dev`; for
another deployment, change `https` to `wss` and set the mod's `room_server` value to that URL.
This routes room-code rendezvous and Hyrule Online peer discovery through your deployment.

Hyrule Online chat uses the same `/global` WebSocket as player discovery. Messages and renames are
sent only to connected players in the same game stage and combined-mod version; blocked peers do
not receive them. Blocks use a stable per-install tag rather than a display name or temporary
session ID, so renaming or restarting the game does not clear a block. This is not an account ban:
clearing or replacing the installation key creates a new identity. The server validates and filters
text, limits messages to 100 characters and one per two seconds per connection, and does not store
or log chat history. Hyrule Online sessions refresh their presence periodically and send an explicit
leave when they shut down; inactive sessions expire, and reconnects replace an older session with
the same installation identity. The player roster contains each online session's configured display
name. Room-code co-op chat continues over the existing encrypted direct peer connection. The
combined Wheel no longer connects to Remia's Co-op chat relay; it uses this Worker's `/global`
endpoint instead.

## Try it without deploying anything

```bash
node dev-server.mjs
node test-flow.mjs
node test-global-blocks.mjs
```

`dev-server.mjs` runs the same code on your own machine at `ws://127.0.0.1:8787`. Plain `ws://` is
only allowed for localhost. `test-flow.mjs` plays a host and a joiner against it and checks they
connect. `test-global-blocks.mjs` exercises stable Hyrule Online blocks, including rename and
restart cases, without starting a server. To use it from the game, set `room_server` to
`ws://127.0.0.1:8787`. Run
`node dev-server.mjs --code TEST22` to give every host the same code, which is handy for scripted
tests.

## What it costs

On the free plan a join is about half a dozen small messages. A host waiting for players holds a
sleeping WebSocket, which costs next to nothing. If you ever run past the free daily limit, new joins fail
until midnight UTC. Nothing is charged, and players can still join by address.
