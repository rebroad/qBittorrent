/*
 * Bittorrent Client using Qt and libtorrent.
 * Copyright (C) 2026 The qBittorrent project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include "base/net/geoipdatabase.h"

#include <QByteArray>
#include <QHostAddress>
#include <QtTest>

#include <memory>

namespace
{
    void appendString(QByteArray &data, const QByteArray &value)
    {
        Q_ASSERT(value.size() <= 28);
        data.append(static_cast<char>(0x40 | value.size()));
        data.append(value);
    }

    void appendUInt16(QByteArray &data, const quint16 value)
    {
        data.append(static_cast<char>(0xA2));
        data.append(static_cast<char>(value >> 8));
        data.append(static_cast<char>(value));
    }

    void appendUInt32(QByteArray &data, const quint32 value)
    {
        data.append(static_cast<char>(0xC4));
        data.append(static_cast<char>(value >> 24));
        data.append(static_cast<char>(value >> 16));
        data.append(static_cast<char>(value >> 8));
        data.append(static_cast<char>(value));
    }

    void appendUInt64(QByteArray &data, const quint64 value)
    {
        data.append(static_cast<char>(8)); // Extended data type, 8-byte payload.
        data.append(static_cast<char>(2)); // Integer64 type number minus 7.
        for (int i = 7; i >= 0; --i)
            data.append(static_cast<char>(value >> (i * 8)));
    }

    QByteArray makeMetadata()
    {
        QByteArray metadata;
        metadata.append(static_cast<char>(0xE7)); // Seven-entry map.
        appendString(metadata, QByteArrayLiteral("binary_format_major_version"));
        appendUInt16(metadata, 2);
        appendString(metadata, QByteArrayLiteral("binary_format_minor_version"));
        appendUInt16(metadata, 0);
        appendString(metadata, QByteArrayLiteral("ip_version"));
        appendUInt16(metadata, 6);
        appendString(metadata, QByteArrayLiteral("record_size"));
        appendUInt16(metadata, 28);
        appendString(metadata, QByteArrayLiteral("node_count"));
        appendUInt32(metadata, 1);
        appendString(metadata, QByteArrayLiteral("database_type"));
        appendString(metadata, QByteArrayLiteral("Test"));
        appendString(metadata, QByteArrayLiteral("build_epoch"));
        appendUInt64(metadata, 1);
        return metadata;
    }

    QByteArray makeDatabase()
    {
        constexpr quint32 nodeCount = 1;
        constexpr quint32 nodeSize = 7;
        constexpr quint32 recordID = 1U << 24;
        constexpr quint32 dataOffset = recordID - nodeCount - 16;
        constexpr qsizetype recordPosition = (nodeCount * nodeSize) + 16 + dataOffset;

        QByteArray data(recordPosition, '\0');

        // The left record is 0x01000000: its high nibble belongs in the
        // high nibble of byte 3. The right record equals nodeCount (no data).
        data[2] = '\0';
        data[3] = static_cast<char>(0x10);
        data[6] = '\1';

        QByteArray record;
        record.append(static_cast<char>(0xE1)); // One-entry map.
        appendString(record, QByteArrayLiteral("country"));
        record.append(static_cast<char>(0xE1)); // One-entry map.
        appendString(record, QByteArrayLiteral("iso_code"));
        appendString(record, QByteArrayLiteral("US"));
        data.append(record);

        data.append(QByteArray(16, '\0')); // Data section separator before metadata.
        data.append(QByteArrayLiteral("\xAB\xCD\xEFMaxMind.com"));
        data.append(makeMetadata());
        return data;
    }
}

class TestGeoIPDatabase final : public QObject
{
    Q_OBJECT

private slots:
    void lookup28BitNodeWithNonzeroHighNibble();
};

void TestGeoIPDatabase::lookup28BitNodeWithNonzeroHighNibble()
{
    QString error;
    const std::unique_ptr<GeoIPDatabase> database {GeoIPDatabase::load(makeDatabase(), error)};
    QVERIFY2(database, qUtf8Printable(error));

    QCOMPARE(database->lookup(QHostAddress {QStringLiteral("::")}), QStringLiteral("US"));
}

QTEST_APPLESS_MAIN(TestGeoIPDatabase)

#include "testgeoipdatabase.moc"
