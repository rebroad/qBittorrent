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

## Open verification and gaps

- Verify an actual small, lawful download reaches peers and transfers data locally through automatic fallback, without starting a second torrent client.
- Verify DHT and uTP behavior end to end. A synthetic DHT ping did not receive a reply.
- Verify an automatically added TCP tracker allows a UDP-only magnet to discover peers or metadata when its original UDP trackers are inaccessible.
- Verify an actual pi3 network failure automatically triggers the already-tested bigquiet selection branch. The current test supplied the existing failure marker; it did not interrupt pi3 connectivity.
- Verify bigquiet-mode UDP tracker and DHT behavior. Bigquiet currently provides the verified inbound TCP relay; the relay deliberately removes its UDP mapping because it has no source-preserving UDP path.
- Recheck live qBittorrent and relay state before relying on the dated evidence above.
