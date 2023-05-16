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

#ifndef IMAGEBLOBDEVICE_H
#define IMAGEBLOBDEVICE_H

#include <QIODevice>

#include "metadatamanager.h"

typedef struct sqlite3 sqlite3;
typedef struct sqlite3_blob sqlite3_blob;

namespace ImageMetaData
{

//TODO: move to utils
class ImageBlobDevice : public QIODevice
{
    Q_OBJECT
public:
    ImageBlobDevice(sqlite3 *db, QObject *parent = nullptr);
    ~ImageBlobDevice() override;

    void setBlobInfo(const QByteArray& table, const QByteArray& column, qint64 rowId);

    bool reserveSizeAndReset(qint64 len);

    virtual bool open(OpenMode mode) override;
    virtual void close() override;

    virtual qint64 size() const override;

protected:
    virtual qint64 readData(char *data, qint64 maxlen) override;
    virtual qint64 writeData(const char *data, qint64 len) override;

private:
    qint64 mRowId;
    qint64 mSize;
    sqlite3 *mDb;
    sqlite3_blob *mBlob;

    QByteArray mTable;
    QByteArray mColumn;
};

ImageBlobDevice *getImage(sqlite3pp::database& db, const MetaDataManager::Key& key);
void setImage(sqlite3pp::database& db, const MetaDataManager::Key &key, const void *data, int size);

} // namespace ImageMetaData

#endif // IMAGEBLOBDEVICE_H
