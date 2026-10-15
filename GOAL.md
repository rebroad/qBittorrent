# Current Goal

Make qBittorrent automatically keep downloading on this computer when direct BitTorrent connectivity is insufficient, using exactly one relay at a time: pi3 is the current primary because its inbound path is verified; bigquiet is the failover when it is reachable and configured for the same routing.

## Requirements

- Keep qBittorrent as the only torrent client for the download. Torrent data and downloaded files stay on this computer; pi3 and bigquiet must not download a second copy as part of the relay.
- Keep normal local qBittorrent downloads and file handling on this computer.
- Integrate relay startup, health monitoring, failover, and shutdown with qBittorrent's normal lifecycle.
- Select either pi3 or bigquiet, never route through both. On relay change, move the router's UPnP port mapping to the selected relay and update qBittorrent's advertised address.
- Provide the network path qBittorrent needs when direct connectivity fails, including TCP and UDP tracker, DHT, uTP, and peer traffic. On pi3, route UDP through the VPN when qBittorrent is bound to `tun-qbt`; do not advertise a UDP mapping for a relay mode that cannot forward it.
- Keep the existing TCP incoming-peer reverse tunnel where useful, and use outbound peer proxying when it improves reachability.
- When outbound peer traffic is actively and successfully routed through the selected relay, have qBittorrent report `Proxied` with a clear proxy icon instead of reporting only `Firewalled`. Show that state only while real peer proxying is verified healthy; clear it when the path fails or only tracker proxying is active. If qBittorrent has no suitable native indicator, add one to its UI.
- Automatically handle UDP-only magnets and stalled existing torrents, including adding/reannouncing TCP trackers when those are the available fallback.
- Verify the selected relay, router mapping, local qBittorrent listener, outbound tracker/peer route, relay failover, local download behavior, and the `Proxied` text/icon during successful peer proxying (including that it clears on proxy failure), without running a second torrent client.

See [GOAL_PROGRESS.md](GOAL_PROGRESS.md) for current implementation status, verification evidence, and remaining work.
