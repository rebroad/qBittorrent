# Current Goals

## Fix the qBittorrent crash

Diagnose and fix the qBittorrent SIGSEGV observed at 19:08:01 on 2026-10-15 after deploying the Proxied-status build. Preserve the prior working binary until the crash is understood. Keep core-dump or GDB capture reliable, use the captured crash evidence to identify the root cause, fix it in source, build and verify the fix, then redeploy only after validation.

## Show relayed peer connections accurately

When a peer connection is carried through the selected pi3 or bigquiet SSH relay, show it in the peer list and WebUI as `Relayed via pi3` or `Relayed via bigquiet` instead of showing this computer's local SSH-forward socket address as if it were the other peer's address. Show an original peer IP or port only when the transport provides it reliably.

Report `Proxied` while a real peer connection is verified to use the SSH-forwarded relay path or an enabled outbound peer proxy. Report `Firewalled` when no proxied peer connection is active and direct incoming connectivity is unavailable. An idle relay or tracker-only proxy must not set `Proxied`; clear the state when the peer path disconnects or fails.

## Preserve the relay operating model

Keep qBittorrent as the only torrent client for downloads. Use exactly one relay at a time: pi3 is primary and bigquiet is failover. Torrent data and downloaded files stay on this computer. Keep direct local downloads, relay lifecycle integration, router mapping failover, and the TCP/UDP peer and tracker routing requirements already verified in [GOAL_PROGRESS.md](GOAL_PROGRESS.md).

## Completion evidence

- Capture and inspect the original crash with symbols, identify its concrete root cause, and add focused regression coverage.
- Build and run the focused tests; verify qBittorrent remains stable with the fix before replacing the production binary.
- Verify that an active incoming SSH-forwarded peer is shown as relayed rather than as this host's IP, and that the connection indicator reports `Proxied` only while peer traffic is actually using the relay or peer proxy.
- Verify the status clears after the relayed/proxied peer disconnects or the path fails, and remains clear for tracker-only proxying.
- Recheck the relay routing and single-relay behavior recorded in [GOAL_PROGRESS.md](GOAL_PROGRESS.md).

See [GOAL_PROGRESS.md](GOAL_PROGRESS.md) for implementation status, evidence, and remaining work.
