/*
 * Bittorrent Client using Qt and libtorrent.
 * Copyright (C) 2026  qBittorrent contributors
 *
 * This file is part of qBittorrent.
 *
 * qBittorrent is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 */

#pragma once

#include <QCoreApplication>
#include <QString>
#include <QVariant>

namespace
{
    constexpr auto EXIT_REASON_PROPERTY = "qBittorrentExitReason";
}

inline void recordExitReason(const QString &reason)
{
    if (QCoreApplication *app = QCoreApplication::instance())
        app->setProperty(EXIT_REASON_PROPERTY, QVariant::fromValue(reason));
}
