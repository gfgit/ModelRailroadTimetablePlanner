/*
 * ModelRailroadTimetablePlanner
 * Copyright 2016-2023, Filippo Gentile
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "recentdirstore.h"

#include <QHash>

#include <QFileInfo>
#include <QStandardPaths>

class RecentDirStorePrivate
{
public:
    QHash<QString, QString> m_dirs;
};

Q_GLOBAL_STATIC(RecentDirStorePrivate, recent_dirs)

inline QStandardPaths::StandardLocation convertFlag(RecentDirStore::DefaultDir kind)
{
    switch (kind)
    {
    case RecentDirStore::DefaultDir::Documents:
    default:
        return QStandardPaths::DocumentsLocation;
    case RecentDirStore::DefaultDir::Images:
        return QStandardPaths::PicturesLocation;
    }
}

QString RecentDirStore::getDir(const QString &key, DefaultDir kind)
{
    auto it = recent_dirs()->m_dirs.find(key);
    if (it == recent_dirs()->m_dirs.end())
    {
        // Insert new dir
        QString path = QStandardPaths::writableLocation(convertFlag(kind));
        it           = recent_dirs->m_dirs.insert(key, path);
    }

    return it.value();
}

void RecentDirStore::setPath(const QString &key, const QString &path)
{
    auto it = recent_dirs()->m_dirs.find(key);
    if (it == recent_dirs()->m_dirs.end())
        return;

    QFileInfo info(path);
    if (info.isDir())
        it.value() = info.absoluteFilePath();
    else
    {
        QString path2 = info.absolutePath();
        info.setFile(path2);
        if (info.isDir())
            it.value() = path2;
    }
}
