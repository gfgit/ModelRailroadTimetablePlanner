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
    if(it == recent_dirs()->m_dirs.end())
    {
        //Insert new dir
        QString path = QStandardPaths::writableLocation(convertFlag(kind));
        it = recent_dirs->m_dirs.insert(key, path);
    }

    return it.value();
}

void RecentDirStore::setPath(const QString &key, const QString &path)
{
    auto it = recent_dirs()->m_dirs.find(key);
    if(it == recent_dirs()->m_dirs.end())
        return;

    QFileInfo info(path);
    if(info.isDir())
        it.value() = info.absoluteFilePath();
    else
    {
        QString path2 = info.absolutePath();
        info.setFile(path2);
        if(info.isDir())
            it.value() = path2;
    }
}
