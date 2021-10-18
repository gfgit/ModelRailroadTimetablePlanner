#include "stationsvghelper.h"

#include <sqlite3pp/sqlite3pp.h>

#include "db_metadata/imagemetadata.h"

#include <QFileDevice>

const char stationTable[] = "stations";
const char stationSVGCol[] = "svg_data";

static ImageMetaData::ImageBlobDevice *loadImage_internal(sqlite3pp::database &db, db_id stationId)
{
    ImageMetaData::ImageBlobDevice *dev = new ImageMetaData::ImageBlobDevice(db.db());
    dev->setBlobInfo(stationTable, stationSVGCol, stationId);
    return dev;
}

bool StationSVGHelper::addImage(sqlite3pp::database &db, db_id stationId, QIODevice *source)
{
    std::unique_ptr<ImageMetaData::ImageBlobDevice> dest;
    dest.reset(loadImage_internal(db, stationId));

    if(!dest)
        return false;

    //Make room for storing data and open device
    dest->reserveSizeAndReset(source->size());

    constexpr int bufSize = 8192;
    char buf[bufSize];
    qint64 size = 0;
    while (!source->atEnd() || size < 0)
    {
        size = source->read(buf, bufSize);
        dest->write(buf, size);
    }

    source->close();
    return true;
}

bool StationSVGHelper::removeImage(sqlite3pp::database &db, db_id stationId)
{
    sqlite3pp::command cmd(db, "UPDATE stations SET svg_data = NULL WHERE id=?");
    cmd.bind(1, stationId);
    return cmd.execute() == SQLITE_OK;
}

bool StationSVGHelper::saveImage(sqlite3pp::database &db, db_id stationId, QIODevice *dest)
{
    std::unique_ptr<ImageMetaData::ImageBlobDevice> source;
    source.reset(loadImage_internal(db, stationId));

    if(!source || !source->open(QIODevice::ReadOnly))
        return false;

    //Optimization for files, resize to avoid reallocations
    if(QFileDevice *dev = qobject_cast<QFileDevice *>(dest))
        dev->resize(source->size());

    constexpr int bufSize = 4096;
    char buf[bufSize];
    qint64 size = 0;
    while (!source->atEnd() || size < 0)
    {
        size = source->read(buf, bufSize);
        dest->write(buf, size);
    }

    source->close();
    return true;
}

QIODevice *StationSVGHelper::loadImage(sqlite3pp::database &db, db_id stationId)
{
    return loadImage_internal(db, stationId);
}
