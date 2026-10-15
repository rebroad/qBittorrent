# Torrent Relay Goal Progress

This file records implementation state and verification evidence for [GOAL.md](GOAL.md). Keep the goal statement focused on the requested end state.

## Current implementation

- The qBittorrent relay lifecycle service owns relay selection, the router mapping, the incoming TCP tunnel, and qBittorrent's advertised relay address. The fallback monitor watches stalled downloads and starts no second torrent client.
- The selector prefers pi3 and uses bigquiet when pi3's relay path fails. It selects one relay at a time and moves the UPnP mapping with it.
- The incoming TCP path uses SSH reverse forwarding and a socat listener. The relay script searches for a free internal SSH port and repoints an existing socat listener when necessary, avoiding collisions with other SSH forwards.
- When fallback activates, qBittorrent binds to pi3's OpenVPN interface or bigquiet's WireGuard interface with proxying disabled, so TCP and UDP traffic use the selected relay directly. The selected relay forwards incoming UDP only when that VPN route is active. SOCKS remains available as a TCP-only fallback.
- The production stall threshold is 120 seconds. The monitor adds and reannounces HTTP trackers for stalled UDP-only magnets.

## Verified

- The Pi3 verifier passes: TCP 54716 maps to pi3 at 192.168.0.11, qBittorrent advertises the router public IP, TCP reaches the local listener through the reverse tunnel, and the router hairpin probe passes.
- Pi3 VPN binding checks verified outbound HTTPS and UDP DNS, inbound UDP peer traffic, and the UDP DNAT/FORWARD rules while qBittorrent was bound to tun-qbt at 10.8.0.2. Direct mode removes the UDP mapping and relay forwarding rules.
- Earlier Pi3 fallback tests verified HTTP tracker additions, TCP peer connections, DHT, and uTP. The official Debian 13.7 netinst torrent downloaded payload while bound to the Pi3 VPN. The test torrents and files were removed, and qBittorrent returned to its saved settings.
- The current UPnP helper is installed at the same user-level path on pi3 and bigquiet and has a matching SHA-256. No compatibility branch for an older helper remains.
- Bigquiet WireGuard egress was verified for TCP and UDP. An official Debian netinst test downloaded 29,594,046 bytes locally with qBittorrent bound to wg-qbt at 10.89.0.2. DHT reached 203 nodes. A UDP-only synthetic magnet contacted tracker.opentrackr.org successfully and reported one peer; a second UDP tracker timed out, while the monitor-added HTTP trackers reported working status and peers.
- A controlled Pi3 relay SSH outage was tested with a temporary output firewall rule to Pi3's ZeroTier address, excluding the active Codex SSH connection. The SSH keepalive timed out, systemd restarted the relay selector, and it automatically selected bigquiet and moved TCP/UDP UPnP mappings to 192.168.0.10. The monitor switched qBittorrent to bigquiet WireGuard.
- During that automatically selected Bigquiet fallback, the official Debian torrent downloaded 22,639,980 bytes locally. DHT reached 169 nodes and qBittorrent reported a connected state. The Bigquiet verifier passed for the router mapping, advertised address, incoming TCP tunnel, hairpin probe, and active lifecycle service.
- The outage exposed a Pi3 port collision: an existing SSH app-server session held remote port 54717. The relay script now finds a free internal port and updates socat's target; it used port 54718 without disrupting that session. The Pi3 verifier passed afterward.
- The test torrents and temporary files were deleted. qBittorrent returned to wlo1 with its saved proxy settings, its original 13 torrents remained, no torrent client was running on pi3 or bigquiet, and the temporary firewall rule and Bigquiet route were removed. The selector returned to pi3 and the Pi3 verifier passed again. The monitor was restarted with no temporary environment override, so its 120-second production threshold is active.

## Final audit (2026-10-15)

- Published the six goal-related implementation files to the configured `/home/rebroad` remote in commit `5189609d` (`feat: route stalled torrents through relay VPNs`). The route helper installed at `/usr/local/libexec/qbittorrent-vpn-route-helper` matches the committed source. The Bigquiet relay firewall helper also matches its committed source, and both relays have the same UPnP helper hash.
- The live read-only verifier passed: router TCP 54716 maps to pi3 (`192.168.0.11`), the local API reports listen port 54716 and advertised public IP `82.1.79.142`, TCP reaches qBittorrent through the reverse tunnel, router hairpin works, and the relay lifecycle service is active.
- Both lifecycle services are active with no temporary environment overrides. The active relay state is pi3. qBittorrent is in normal `wlo1` mode; its local TCP and UDP listeners are present. The fallback monitor was restarted after the final script edit and is running the current source.
- qBittorrent has 13 torrents and none are incomplete. No qBittorrent or Transmission client is running on pi3 or bigquiet. With no incomplete download to recover, the monitor has no active fallback state; the previously recorded controlled pi3 outage and Bigquiet local-download test verify automatic failover behavior.
- The previous implementation changes are pushed. The live audit above predates the newly added `Proxied` status/icon requirement.

## Proxied status implementation and verification (2026-10-15)

- Added a shared connection-status mapping with `Proxied` precedence when an outbound peer connection has completed successfully through libtorrent's SOCKS5 or HTTP proxy socket. Track peers from successful outgoing `peer_connect_alert`s, remove them on disconnect/error, and clear tracking when proxy settings change or libtorrent reports dropped alerts. Tracker replies and the proxy-enabled setting alone do not set the status.
- The desktop status bar and both WebUI API status paths now report `proxied`; the desktop tooltip and WebUI accessible label/title say `Connection status: Proxied`. Added a proxy-specific icon to the desktop theme and WebUI assets.
- Built the application in `/mnt/kingston/builds/rebroad/src/qBittorrent.build/qb-build`. The `qbt_app` target and `testsessionstatus` built successfully; the focused CTest passed.
- Verified the built client in an isolated local profile with the existing SOCKS endpoint (`127.0.0.1:1089`) and an official Debian 13.7 netinst test torrent. With no proxy peers connected, both APIs reported `firewalled`. After enabling peer proxying and observing real peer connections, `sync/maindata` reported `proxied` and the WebUI showed the Proxied label/icon. Changing only the test profile's SOCKS port to an unused port cleared the status back to `firewalled` in both API and WebUI. No torrent payload was downloaded; the test process, profile, and torrent file were removed afterward.
- Installed the built application atomically at `/usr/local/bin/qbittorrent` and restarted `qbittorrent.service`. The service is active with the new executable (SHA-256 `c7e3b8a2bacc20bd0ba38e955cfbafb621b74c6de9e8871d876ab159ee294ecc`); the local Web API responds as qBittorrent v5.2.3 and still reports all 13 existing torrents with none incomplete.
- Production currently reports `connection_status=firewalled`. This is expected because the existing live setting `proxy_peer_connections=false` remains unchanged, and the status only becomes `proxied` after a successful outbound peer connection through an enabled proxy. The new behavior has been verified in the isolated client with real peer connections; production has not yet exercised that path.

## Production audit (2026-10-15)

- Re-ran `bittorrent-tunnel-verify.sh 54716 pi3`: the router maps TCP 54716 to pi3 (`192.168.0.11`), the public address and qBittorrent announce address match, the reverse tunnel reaches the local listener, the hairpin probe passes, and the relay service is active.
- Confirmed both user services are running: the relay tunnel selector and its SOCKS fallback monitor. They agree on pi3 as the only selected relay. The local SOCKS endpoint on `127.0.0.1:1089` successfully exits as the router public IP (`82.1.79.142`). No qBittorrent relay path is using bigquiet concurrently.
- The live API has 13 torrents and none incomplete. The monitor's SOCKS peer fallback is therefore inactive, and `proxy_peer_connections=false` is the saved baseline. This is the expected production state; the monitor enables peer proxying only after a stalled download and a failed UDP-capable VPN route, then restores the saved preference when fallback ends. The production UI/API should say `Proxied` only during a successful SOCKS peer connection, not merely because the SOCKS tunnel or tracker proxy is available.
- Source build, focused status test, isolated real-peer WebUI success case, and isolated proxy-failure clearing case are recorded above. The built binary is installed and running in production. No incomplete production torrent currently exists to exercise the conditional SOCKS peer fallback without creating a new download.
