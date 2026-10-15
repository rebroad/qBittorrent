# Current Goal

Make qBittorrent automatically keep downloading on this computer when direct BitTorrent connectivity is insufficient, using exactly one relay at a time: pi3 is the current primary because its inbound path is verified; bigquiet is the failover when it is reachable and configured for the same routing.

## Requirements

- Keep qBittorrent as the only torrent client for the download. Torrent data and downloaded files stay on this computer; pi3 and bigquiet must not download a second copy as part of the relay.
- Keep normal local qBittorrent downloads and file handling on this computer.
- Integrate relay startup, health monitoring, failover, and shutdown with qBittorrent's normal lifecycle.
- Select either pi3 or bigquiet, never route through both. On relay change, move the router's UPnP port mapping to the selected relay and update qBittorrent's advertised address.
- Provide the network path qBittorrent needs when direct connectivity fails, including TCP and UDP tracker, DHT, uTP, and peer traffic. On pi3, route UDP through the VPN when qBittorrent is bound to `tun-qbt`; do not advertise a UDP mapping for a relay mode that cannot forward it.
- Keep the existing TCP incoming-peer reverse tunnel where useful, and use outbound peer proxying when it improves reachability.
- Automatically handle UDP-only magnets and stalled existing torrents, including adding/reannouncing TCP trackers when those are the available fallback.
- Verify the selected relay, router mapping, local qBittorrent listener, outbound tracker/peer route, relay failover, and local download behavior without running a second torrent client.

## Current Evidence and Open Work

- `qbittorrent-relay-tunnel.service` is enabled at the user manager's default target and starts the SOCKS fallback monitor. The relay script waits for the system qBittorrent service and never starts a second torrent client; it restarts after qBittorrent exits and cleans up the active router mapping.
- The pi3 TCP reverse-tunnel verifier passes: TCP 54716 maps to pi3 (`192.168.0.11`), qBittorrent advertises `82.1.79.142`, the remote TCP probe reaches the local listener, and the router hairpin probe passes.
- The relay's public TCP listener now stays alive across local service restarts and forwards through a stable reverse-tunnel port. This avoids a restart failure caused by a stale half-closed TCP connection holding the public port.
- When qBittorrent is bound to `tun-qbt` at `10.8.0.2`, the relay automation adds the router's UDP 54716 mapping and pi3 DNAT/FORWARD rules to that VPN address. The host firewall helper allows TCP and UDP 54716 on `tun-qbt`. When qBittorrent returns to `wlo1`, the automation removes the UDP mapping and pi3 forwarding rules.
- A controlled VPN-binding check confirmed qBittorrent's TCP and UDP sockets moved to `10.8.0.2`; VPN-bound HTTPS and UDP DNS egress worked. A packet capture on `tun-qbt` saw packets from public peer addresses reach qBittorrent and UDP packets leave its socket. The original `wlo1` binding was restored, UDP mapping and DNAT rules were removed, and TCP verification passed again.
- An automatic monitor test using a random, non-downloading magnet triggered the fallback without manual intervention: qBittorrent switched to `tun-qbt`, disabled its BitTorrent proxy so UDP could flow, and the relay installed the UDP map and pi3 DNAT/FORWARD rules. While in fallback, qBittorrent reported several UDP trackers as working. Removing the test magnet returned qBittorrent to its saved direct-mode proxy/binding settings, and the UDP relay rules disappeared. The test threshold was temporary; the monitor is back on its production 120-second default.
- The SOCKS fallback still adds HTTP tracker-list entries to stalled torrents. UDP tracker success is now demonstrated for some trackers while routed through the VPN, but DHT response behavior remains unverified: a synthetic DHT ping did not get a reply. UDP-only magnet behavior still needs a full download-path verification.
- Earlier runs showed the stall monitor switching to pi3's VPN after 120 seconds, with VPN egress `82.1.79.142` and normal host egress `213.122.174.139`. The fallback state file preserves the original qBittorrent network and proxy settings across monitor restarts.
- No content was downloaded during the synthetic automatic-fallback test. It verified monitor transition and UDP route setup/cleanup, but not a complete download or peer exchange. qBittorrent remains the only torrent client; no second torrent client runs on pi3.
- bigquiet is currently unreachable (`ssh bigquiet` reports “No route to host”), so its failover, UDP path, and inbound permissions remain unverified.
