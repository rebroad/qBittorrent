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

## Remaining work

- Add a qBittorrent `Proxied` status and clear proxy icon that reflect successfully working outbound peer proxying through the selected relay. Do not show it merely because tracker proxying or a proxy setting is enabled, and clear it when peer proxying fails. Verify the UI state and icon against real peer traffic and a proxy-path failure. The latest live API check showed `proxy_peer_connections=false`, `connection_status=firewalled`, and zero DHT nodes, so this requirement is not yet met.
