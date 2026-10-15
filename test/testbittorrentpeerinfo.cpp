/*
 * Bittorrent Client using Qt and libtorrent.
 * Copyright (C) 2026 The qBittorrent project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include "base/bittorrent/peerinfo.h"

#include <QtTest>

class TestBitTorrentPeerInfo final : public QObject
{
    Q_OBJECT

private slots:
    void identifiesLocalEndpointWhenRelayIsActive();
    void doesNotIdentifyLocalEndpointWithoutRelay();
    void doesNotIdentifyRemoteEndpointAsRelayed();
};

void TestBitTorrentPeerInfo::identifiesLocalEndpointWhenRelayIsActive()
{
    QVERIFY(BitTorrent::PeerInfo::isRelayedEndpoint(QHostAddress::LocalHost, QStringLiteral("pi3")));
}

void TestBitTorrentPeerInfo::doesNotIdentifyLocalEndpointWithoutRelay()
{
    QVERIFY(!BitTorrent::PeerInfo::isRelayedEndpoint(QHostAddress::LocalHost, {}));
}

void TestBitTorrentPeerInfo::doesNotIdentifyRemoteEndpointAsRelayed()
{
    QVERIFY(!BitTorrent::PeerInfo::isRelayedEndpoint(QHostAddress {QStringLiteral("203.0.113.8")}, QStringLiteral("pi3")));
}

QTEST_APPLESS_MAIN(TestBitTorrentPeerInfo)

#include "testbittorrentpeerinfo.moc"
