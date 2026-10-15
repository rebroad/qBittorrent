/*
 * Bittorrent Client using Qt and libtorrent.
 * Copyright (C) 2016  Vladimir Golovnev <glassez@yandex.ru>
 * Copyright (C) 2014  sledgehammer999 <hammered999@gmail.com>
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

#include "settingsstorage.h"

#include <chrono>
#include <memory>

#include <QFile>
#include <QHash>

#include "global.h"
#include "logger.h"
#include "path.h"
#include "profile.h"
#include "utils/fs.h"

using namespace std::chrono_literals;

namespace
{
    const QString CHURN_SETTINGS_NAME = u"qBittorrent-ui"_s;

    bool isHighChurnKey(const QString &key)
    {
        return (key == u"GUI/Qt6/TorrentProperties/PeerListState"_s)
            || (key == u"GUI/Qt6/TransferList/HeaderState"_s)
            || (key == u"MainWindow/geometry"_s)
            || (key == u"TorrentProperties/CurrentTab"_s)
            || (key == u"OptionsDialog/LastViewedPage"_s);
    }
}

SettingsStorage *SettingsStorage::m_instance = nullptr;

SettingsStorage::SettingsStorage()
    : m_nativeSettingsName {u"qBittorrent"_s}
{
    readNativeSettings();

    m_timer.setSingleShot(true);
    m_timer.setInterval(5s);
    connect(&m_timer, &QTimer::timeout, this, &SettingsStorage::save);
    if (m_dirty || m_churnDirty)
        m_timer.start();
}

SettingsStorage::~SettingsStorage()
{
    save();
}

void SettingsStorage::initInstance()
{
    if (!m_instance)
        m_instance = new SettingsStorage;
}

void SettingsStorage::freeInstance()
{
    delete m_instance;
    m_instance = nullptr;
}

SettingsStorage *SettingsStorage::instance()
{
    return m_instance;
}

bool SettingsStorage::save()
{
    // return `true` only when settings is different AND is saved successfully

    const QWriteLocker locker(&m_lock);  // guard for `m_dirty` too
    const bool hadDirtyData = m_dirty || m_churnDirty;
    if (!hadDirtyData) return false;

    if (m_dirty && writeNativeSettings(m_nativeSettingsName, m_data))
        m_dirty = false;
    if (m_churnDirty && writeNativeSettings(CHURN_SETTINGS_NAME, m_churnData))
        m_churnDirty = false;

    if (m_dirty || m_churnDirty)
        m_timer.start();
    return hadDirtyData && !m_dirty && !m_churnDirty;
}

QVariant SettingsStorage::loadValueImpl(const QString &key, const QVariant &defaultValue) const
{
    const QReadLocker locker(&m_lock);
    return (isHighChurnKey(key) ? m_churnData : m_data).value(key, defaultValue);
}

void SettingsStorage::storeValueImpl(const QString &key, const QVariant &value)
{
    const QWriteLocker locker(&m_lock);
    QVariantHash &data = isHighChurnKey(key) ? m_churnData : m_data;
    QVariant &currentValue = data[key];
    if (currentValue != value)
    {
        (isHighChurnKey(key) ? m_churnDirty : m_dirty) = true;
        currentValue = value;
        m_timer.start();
    }
}

void SettingsStorage::readNativeSettings()
{
    // We return actual file names used by QSettings because
    // there is no other way to get that name except actually create a QSettings object.
    // If serialization operation was not successful we return empty string.
    const auto deserialize = [](QVariantHash &data, const QString &nativeSettingsName) -> Path
    {
        std::unique_ptr<QSettings> nativeSettings = Profile::instance()->applicationSettings(nativeSettingsName);
        if (nativeSettings->allKeys().isEmpty())
            return {};

        // Copy everything into memory. This means even keys inserted in the file manually
        // or that we don't touch directly in this code (eg disabled by ifdef). This ensures
        // that they will be copied over when save our settings to disk.
        for (const QString &key : asConst(nativeSettings->allKeys()))
        {
            const QVariant value = nativeSettings->value(key);
            if (value.isValid())
                data[key] = value;
        }

        return Path(nativeSettings->fileName());
    };

    QVariantHash loadedData;
    bool migratedChurnData = false;
    const Path newPath = deserialize(loadedData, (m_nativeSettingsName + u"_new"));
    if (!newPath.isEmpty())
    {
        // "_new" file is NOT empty
        // This means that the PC closed either due to power outage
        // or because the disk was full. In any case the settings weren't transferred
        // in their final position. So assume that qbittorrent_new.ini/qbittorrent_new.conf
        // contains the most recent settings.
        LogMsg(tr("Detected unclean program exit. Using fallback file to restore settings: %1")
               .arg(newPath.toString()), Log::WARNING);

        QString finalPathStr = newPath.data();
        const qsizetype index = finalPathStr.lastIndexOf(u"_new", -1, Qt::CaseInsensitive);
        finalPathStr.remove(index, 4);

        const Path finalPath {finalPathStr};
        Utils::Fs::removeFile(finalPath);
        Utils::Fs::renameFile(newPath, finalPath);
    }
    else
    {
        deserialize(loadedData, m_nativeSettingsName);
    }

    for (auto i = loadedData.cbegin(); i != loadedData.cend(); ++i)
    {
        if (isHighChurnKey(i.key()))
        {
            m_churnData.insert(i.key(), i.value());
            m_dirty = true; // Remove migrated values from the main config on save.
            migratedChurnData = true;
        }
        else
        {
            m_data.insert(i.key(), i.value());
        }
    }

    // Existing config files may contain these keys. Prefer the companion file
    // when present, while retaining the old values as a migration fallback.
    QVariantHash churnData;
    const Path churnNewPath = deserialize(churnData, CHURN_SETTINGS_NAME + u"_new"_s);
    if (!churnNewPath.isEmpty())
    {
        QString finalPathStr = churnNewPath.data();
        const qsizetype index = finalPathStr.lastIndexOf(u"_new", -1, Qt::CaseInsensitive);
        finalPathStr.remove(index, 4);
        const Path finalPath {finalPathStr};
        Utils::Fs::removeFile(finalPath);
        Utils::Fs::renameFile(churnNewPath, finalPath);
    }
    else
    {
        deserialize(churnData, CHURN_SETTINGS_NAME);
    }
    for (auto i = churnData.cbegin(); i != churnData.cend(); ++i)
    {
        if (isHighChurnKey(i.key()))
        {
            m_churnData.insert(i.key(), i.value());
        }
    }
    m_churnDirty = migratedChurnData;
}

bool SettingsStorage::writeNativeSettings(const QString &name, const QVariantHash &data) const
{
    std::unique_ptr<QSettings> nativeSettings = Profile::instance()->applicationSettings(name + u"_new");

    // QSettings deletes the file before writing it out. This can result in problems
    // if the disk is full or a power outage occurs. Those events might occur
    // between deleting the file and recreating it. This is a safety measure.
    // Write everything to qBittorrent_new.ini/qBittorrent_new.conf and if it succeeds
    // replace qBittorrent.ini/qBittorrent.conf with it.
    for (auto i = data.cbegin(); i != data.cend(); ++i)
        nativeSettings->setValue(i.key(), i.value());

    nativeSettings->sync(); // Important to get error status
    const QSettings::Status status = nativeSettings->status();
    const Path newPath {nativeSettings->fileName()};

    nativeSettings.reset();  // close QSettings

    switch (status)
    {
    case QSettings::NoError:
        break;
    case QSettings::AccessError:
        LogMsg(tr("An access error occurred while trying to write the configuration file."), Log::CRITICAL);
        break;
    case QSettings::FormatError:
        LogMsg(tr("A format error occurred while trying to write the configuration file."), Log::CRITICAL);
        break;
    default:
        LogMsg(tr("An unknown error occurred while trying to write the configuration file."), Log::CRITICAL);
        break;
    }

    if (status != QSettings::NoError)
    {
        Utils::Fs::removeFile(newPath);
        return false;
    }

    QString finalPathStr = newPath.data();
    const qsizetype index = finalPathStr.lastIndexOf(u"_new", -1, Qt::CaseInsensitive);
    finalPathStr.remove(index, 4);

    const Path finalPath {finalPathStr};
    Utils::Fs::removeFile(finalPath);
    return Utils::Fs::renameFile(newPath, finalPath);
}

void SettingsStorage::removeValue(const QString &key)
{
    const QWriteLocker locker(&m_lock);
    const bool churnKey = isHighChurnKey(key);
    if ((churnKey ? m_churnData : m_data).remove(key))
    {
        (churnKey ? m_churnDirty : m_dirty) = true;
        m_timer.start();
    }
}

bool SettingsStorage::hasKey(const QString &key) const
{
    const QReadLocker locker {&m_lock};
    return (isHighChurnKey(key) ? m_churnData : m_data).contains(key);
}

bool SettingsStorage::isEmpty() const
{
    const QReadLocker locker {&m_lock};
    return m_data.isEmpty() && m_churnData.isEmpty();
}
