# Torrent Relay Goal Progress

This file records implementation state and verification evidence for [GOAL.md](GOAL.md). Update it as the implementation changes; keep the goal statement itself focused on the requested end state.

## Current implementation

- `qbittorrent-relay-tunnel.service` runs the relay lifecycle script and the SOCKS fallback monitor. The relay script starts no second torrent client and waits for the system qBittorrent service.
- The relay selector uses one relay at a time. pi3 is preferred while reachable; bigquiet is the alternate. The selected relay owns the router TCP port mapping and qBittorrent's advertised relay address.
- TCP peer connections use the stable reverse SSH tunnel and relay listener. On pi3, when qBittorrent switches to `tun-qbt` (`10.8.0.2`), the relay also configures UDP UPnP and VPN DNAT/FORWARD rules. Direct mode removes those UDP forwarding rules.
- The stall monitor's production threshold is 120 seconds. The SOCKS relay carries TCP, while the pi3 VPN mode carries UDP as well.

## Verified

- The pi3 TCP verifier passed: TCP 54716 maps to pi3 (`192.168.0.11`), qBittorrent advertises the relay address, the remote TCP probe reaches the local listener, and the router hairpin probe passes.
- A controlled VPN binding check moved qBittorrent sockets to `10.8.0.2`. VPN-bound HTTPS and UDP DNS worked, and packet capture on `tun-qbt` showed inbound peer-address UDP packets and outgoing UDP. Returning to the original binding removed UDP mapping and pi3 forwarding rules; TCP verification passed afterward.
- A synthetic random-hash magnet triggered the automatic stall monitor. qBittorrent switched to `tun-qbt`, disabled BitTorrent proxying, and pi3's UDP mapping and NAT rules appeared. Several UDP trackers reported working while fallback was active. Removing the magnet restored saved settings and removed UDP forwarding. No torrent content was downloaded during this test.
- The test-only 8-second stall threshold was cleared after the test. The script default remains 120 seconds.
- Bigquiet became reachable. The current UPnP helper was copied to `/home/rebroad/bin/upnp_port_setup.py` there; its SHA-256 matches the local source. Relay selection now uses that same helper path on pi3 and bigquiet, with no old-helper compatibility branch.
- With the existing pi3-failure marker supplied to the automatic selector, it chose bigquiet, moved TCP 54716's router mapping to `192.168.0.10`, installed the bigquiet firewall rule, and removed the stale UDP mapping. `bittorrent-tunnel-verify.sh 54716 bigquiet` passed: the advertised address and qBittorrent settings matched, inbound TCP reached the local qBittorrent listener, router hairpin worked, and the relay service stayed active.
- After the test, the marker was cleared and the service returned to pi3. `bittorrent-tunnel-verify.sh 54716 pi3` passed with TCP 54716 mapped to `192.168.0.11` and inbound TCP and hairpin checks passing.
- Rechecked 2026-10-15: bigquiet's `/home/rebroad/bin/upnp_port_setup.py` still has the same SHA-256 as the local helper (`3634df2d…bc564`). The relay script contains only that user-level helper path; there is no old-version compatibility branch. The active relay remains pi3, and `bittorrent-tunnel-verify.sh 54716 pi3` passes for mapping, advertised address, inbound TCP, hairpin, and active lifecycle service.
- 2026-10-15 end-to-end download: a no-content synthetic magnet stalled long enough for the production monitor to switch qBittorrent to `tun-qbt` via pi3. The official [Debian netinst torrent](https://www.debian.org/CD/index) was then added with only a UDP tracker in its metadata and a 64 KiB/s per-torrent cap. qBittorrent reported `downloading` and 6,303,383 downloaded bytes on `tun-qbt`; both UDP and automatically added HTTP trackers were present. The trigger and test torrent were removed, the save directory was deleted, and the original `wlo1`/proxy settings were restored. qBittorrent had 13 pre-existing torrents before and after; post-test inspection found no torrent-client process running on pi3 or bigquiet.
- 2026-10-15 TCP tracker attribution check: with DHT temporarily disabled and the test torrent's only UDP tracker set to unreachable `127.0.0.1:1`, the UDP tracker timed out. The automatic fallback added HTTP trackers; several reported a working status and nonzero peer counts (up to 67). qBittorrent remained stalled with zero payload bytes in this DHT-disabled check, so tracker-based peer discovery is verified, but TCP peer data transfer is not. DHT was restored and test torrents were removed afterward.

## Open verification and gaps

- Verify TCP peer data transfer after automatic HTTP tracker fallback. HTTP trackers returned peers while the UDP tracker was unreachable, but the DHT-disabled test did not transfer payload; determine whether the returned peers were seeds and whether outgoing TCP peer connections need the SOCKS path.
- Verify DHT and uTP behavior end to end. A synthetic DHT ping did not receive a reply.
- Verify an actual pi3 network failure automatically triggers the already-tested bigquiet selection branch. The current test supplied the existing failure marker; it did not interrupt pi3 connectivity.
- Verify bigquiet-mode UDP tracker and DHT behavior. Bigquiet currently provides the verified inbound TCP relay; the relay deliberately removes its UDP mapping because it has no source-preserving UDP path.
- Rechecked after the test: qBittorrent is back on `wlo1`, SOCKS and DHT preferences match their saved values, no temporary test torrents remain, relay services are active, and the pi3 inbound TCP and hairpin verifier passes.
