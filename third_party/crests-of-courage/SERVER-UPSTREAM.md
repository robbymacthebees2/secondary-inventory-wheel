The self-hostable Worker under `server/` is sourced from `remiafterdark/crests-of-courage`
revision `bb9922eb4ea52f36ca21c40edb6d2e2cf9a7ee61` and retains its MIT-licensed source files,
tests, deployment configuration, and `LICENSE`. It contains no CoC model or voice assets.

The Worker provides the existing room-code rendezvous and `/global` presence/chat protocol. It
does not relay gameplay or voice traffic; snapshots and actor synchronization remain encrypted
peer-to-peer UDP. See `server/README.md` for deployment and test instructions.
