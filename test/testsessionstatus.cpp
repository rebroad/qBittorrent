/*
 * Bittorrent Client using Qt and libtorrent.
 * Copyright (C) 2026  qBittorrent contributors
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * In addition, as a special exception, the copyright holders give permission to
 * link this program with the OpenSSL project's "OpenSSL" library (or with
 * modified versions of it that use the same license as the "OpenSSL" library),
 * and distribute the linked executables. You must obey the GNU General Public
 * License in all respects for all of the code used other than "OpenSSL".  If you
 * modify file(s), you may extend this exception to your version of the file(s),
 * but you are not obligated to do so. If you do not wish to do so, delete this
 * exception statement from your version.
 */

#include <QObject>
#include <QTest>

#include "base/bittorrent/sessionstatus.h"
#include "base/bittorrent/proxiedpeerconnectiontracker.h"

using namespace Qt::Literals::StringLiterals;

class TestSessionStatus final : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(TestSessionStatus)

public:
    TestSessionStatus() = default;

private slots:
    void connectionStatus() const
    {
        BitTorrent::SessionStatus status;
        QCOMPARE(status.connectionStatus(false), BitTorrent::ConnectionStatus::Disconnected);
        QCOMPARE(status.connectionStatus(true), BitTorrent::ConnectionStatus::Firewalled);

        status.hasIncomingConnections = true;
        QCOMPARE(status.connectionStatus(true), BitTorrent::ConnectionStatus::Connected);

        status.hasProxiedPeerConnections = true;
        QCOMPARE(status.connectionStatus(true), BitTorrent::ConnectionStatus::Proxied);
        QCOMPARE(status.connectionStatus(false), BitTorrent::ConnectionStatus::Disconnected);
    }

    void proxiedPeerDisconnectClearsStatus() const
    {
        BitTorrent::ProxiedPeerConnectionTracker<QString> connections;
        BitTorrent::SessionStatus status;

        connections.add(u"torrent-1"_s, u"peer-1"_s);
        status.hasProxiedPeerConnections = !connections.isEmpty();
        QCOMPARE(status.connectionStatus(true), BitTorrent::ConnectionStatus::Proxied);

        connections.remove(u"torrent-1"_s, u"peer-1"_s);
        status.hasProxiedPeerConnections = !connections.isEmpty();
        QCOMPARE(status.connectionStatus(true), BitTorrent::ConnectionStatus::Firewalled);
    }

    void remainingProxiedPeersKeepStatus() const
    {
        BitTorrent::ProxiedPeerConnectionTracker<QString> connections;
        BitTorrent::SessionStatus status;

        connections.add(u"torrent-1"_s, u"peer-1"_s);
        connections.add(u"torrent-2"_s, u"peer-2"_s);
        connections.remove(u"torrent-1"_s, u"peer-1"_s);
        status.hasProxiedPeerConnections = !connections.isEmpty();
        QCOMPARE(status.connectionStatus(true), BitTorrent::ConnectionStatus::Proxied);

        connections.removeTorrent(u"torrent-2"_s);
        status.hasProxiedPeerConnections = !connections.isEmpty();
        QCOMPARE(status.connectionStatus(true), BitTorrent::ConnectionStatus::Firewalled);
    }

    void peerSnapshotReconcilesDisconnectedPeer() const
    {
        BitTorrent::ProxiedPeerConnectionTracker<QString> connections;
        BitTorrent::SessionStatus status;
        const QString torrent = u"torrent-1"_s;
        const QString peer = u"peer-1"_s;

        connections.add(torrent, peer);
        status.hasProxiedPeerConnections = !connections.isEmpty();
        QCOMPARE(status.connectionStatus(true), BitTorrent::ConnectionStatus::Proxied);

        QHash<QString, QSet<QString>> activeConnections;
        activeConnections.insert(torrent, {});
        connections.reconcile(activeConnections);
        status.hasProxiedPeerConnections = !connections.isEmpty();
        QCOMPARE(status.connectionStatus(true), BitTorrent::ConnectionStatus::Firewalled);
    }

    void peerSnapshotDiscoversRelayedPeer() const
    {
        BitTorrent::ProxiedPeerConnectionTracker<QString> connections;
        BitTorrent::SessionStatus status;
        const QString torrent = u"torrent-1"_s;
        const QString peer = u"peer-1"_s;
        QHash<QString, QSet<QString>> activeConnections {{torrent, {peer}}};
        QHash<QString, QSet<QString>> relayedConnections {{torrent, {peer}}};

        connections.reconcile(activeConnections, relayedConnections);
        status.hasProxiedPeerConnections = !connections.isEmpty();
        QCOMPARE(status.connectionStatus(true), BitTorrent::ConnectionStatus::Proxied);
    }
};

QTEST_APPLESS_MAIN(TestSessionStatus)

#include "testsessionstatus.moc"
