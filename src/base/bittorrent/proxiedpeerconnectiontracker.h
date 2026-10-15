/*
 * Bittorrent Client using Qt and libtorrent.
 * Copyright (C) 2026  qBittorrent contributors
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#pragma once

#include <QHash>
#include <QSet>
#include <QString>

namespace BitTorrent
{
    template <typename TorrentHandle>
    class ProxiedPeerConnectionTracker
    {
    public:
        void add(const TorrentHandle &torrent, const QString &peer)
        {
            m_connections[torrent].insert(peer);
        }

        void remove(const TorrentHandle &torrent, const QString &peer)
        {
            auto torrentIt = m_connections.find(torrent);
            if (torrentIt == m_connections.end())
                return;

            torrentIt->remove(peer);
            if (torrentIt->isEmpty())
                m_connections.erase(torrentIt);
        }

        void removeTorrent(const TorrentHandle &torrent)
        {
            m_connections.remove(torrent);
        }

        void reconcile(const QHash<TorrentHandle, QSet<QString>> &activeConnections,
            const QHash<TorrentHandle, QSet<QString>> &relayedConnections = {})
        {
            for (auto it = relayedConnections.cbegin(); it != relayedConnections.cend(); ++it)
            {
                const auto activeIt = activeConnections.constFind(it.key());
                if (activeIt == activeConnections.cend())
                    continue;

                for (const QString &peer : it.value())
                {
                    if (activeIt->contains(peer))
                        m_connections[it.key()].insert(peer);
                }
            }

            auto it = m_connections.begin();
            while (it != m_connections.end())
            {
                const auto activeIt = activeConnections.constFind(it.key());
                if (activeIt == activeConnections.cend())
                {
                    it = m_connections.erase(it);
                    continue;
                }

                it.value() &= activeIt.value();
                if (it->isEmpty())
                    it = m_connections.erase(it);
                else
                    ++it;
            }
        }

        void clear()
        {
            m_connections.clear();
        }

        bool isEmpty() const
        {
            return m_connections.isEmpty();
        }

    private:
        QHash<TorrentHandle, QSet<QString>> m_connections;
    };
}
