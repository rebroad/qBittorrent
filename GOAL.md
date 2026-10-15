# Current Goal

Make qBittorrent automatically keep downloading on this computer when direct BitTorrent connectivity is insufficient, using exactly one relay at a time: pi3 is the current primary because its inbound path is verified; bigquiet is the failover when it is reachable and configured for the same routing.

## Requirements

- Keep qBittorrent as the only torrent client for the download. Torrent data and downloaded files stay on this computer; pi3 and bigquiet must not download a second copy as part of the relay.
- Keep normal local qBittorrent downloads and file handling on this computer.
- Integrate relay startup, health monitoring, failover, and shutdown with qBittorrent's normal lifecycle.
- Select either pi3 or bigquiet, never route through both. On relay change, move the router's UPnP port mapping to the selected relay and update qBittorrent's advertised address.
- Provide the network path qBittorrent needs when direct connectivity fails, including TCP and UDP BitTorrent traffic needed for tracker announces, DHT, uTP, and peer connections. The current SSH SOCKS tunnel handles TCP only; use or build a UDP-capable tunnel/relay path as needed.
- Keep the existing TCP incoming-peer reverse tunnel where useful, and use outbound peer proxying when it improves reachability.
- Automatically handle UDP-only magnets and stalled existing torrents, including adding/reannouncing TCP trackers when those are the available fallback.
- Verify the selected relay, router mapping, local qBittorrent listener, outbound tracker/peer route, relay failover, and local download behavior without running a second torrent client.

## Current Evidence and Open Work

- The pi3 TCP reverse-tunnel path currently passes the tunnel verifier: the router maps TCP 54716 to pi3 (192.168.0.11), qBittorrent advertises the router's public IP, and a TCP probe through pi3 reaches the local qBittorrent listener.
- The stalled-download monitor automatically switched qBittorrent to pi3's OpenVPN interface after 120 seconds. The VPN egress IP is `82.1.79.142`, while ordinary host egress remains `213.122.174.139`.
- The SOCKS fallback fetched an HTTP tracker list through pi3 and automatically added its TCP trackers to the two stalled magnets. qBittorrent's UDP tracker announces now also succeed through the VPN; several trackers return peer lists for both torrents.
- Both local VisionQuest downloads are progressing through the pi3 VPN (last observed at 50% and 64%, each above 650 KB/s). qBittorrent reports DHT enabled, and no second torrent client was found running on pi3.
- The fallback monitor now stores its original network/proxy settings in `~/.local/state/qBittorrent/network-fallback.json`, reloads them after a monitor restart, and keeps the relay active while incomplete torrents are still progressing.
- The old system-level `qbittorrent-tunnel.service` that was hard-wired to bigquiet has been disabled because it competed with the pi3/bigquiet selector.
- pi3 is reachable and runs an OpenVPN server on TCP 443. A dedicated client certificate was created for this host, and its authenticated OpenVPN client is tied to qBittorrent's system service lifecycle.
- pi3 already forwards and SNATs the VPN subnet to the internet. qBittorrent has live TCP and UDP sockets bound to `tun-qbt` at `10.8.0.2`; VPN byte counters increase during downloads.
- Incoming UDP/uTP is still unverified and not configured at the router: TCP 54716 maps to pi3 but UDP 54716 has no UPnP mapping. A DHT ping from pi3 to qBittorrent's UDP port timed out. The installed root firewall helper currently has port rules for `wlo1`, not `tun-qbt`, so inbound UDP may also need a firewall rule. Root firewall changes require an authenticated sudo command.
- bigquiet is currently unreachable from this host (`ssh bigquiet` reports “No route to host”), so relay failover and its inbound permission remain unverified.
