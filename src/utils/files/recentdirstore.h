#ifndef RECENTDIRSTORE_H
#define RECENTDIRSTORE_H

#include <QString>

class RecentDirStore
{
public:
    enum DefaultDir
    {
        Documents = 0,
        Images
    };

    static QString getDir(const QString& key, DefaultDir kind);
    static void setPath(const QString& key, const QString& path);
};

#endif // RECENTDIRSTORE_H
