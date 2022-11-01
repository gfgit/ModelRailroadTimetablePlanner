#include "e656netimporter.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>

#include <QXmlStreamReader>

#include "jobstopsimporter.h"

#include <QDebug>


/*!
 * \brief The ProxySeqDevice class
 *
 * Prepends a buffer to a sequential device
 * This is useful if you have already read some data and want to seek back
 */
class ProxySeqDevice : public QIODevice
{
public:
    ProxySeqDevice(QIODevice *dev, const QByteArray &ba);

    bool isSequential() const override { return true; }

protected:
    qint64 readData(char *data, qint64 maxlen) override;
    qint64 writeData(const char *data, qint64 len) override;

private:
    QIODevice *source;
    QByteArray buf;
};

ProxySeqDevice::ProxySeqDevice(QIODevice *dev, const QByteArray &ba)
    : source(dev)
    , buf(ba)
{
    open(QIODevice::ReadOnly);
}

qint64 ProxySeqDevice::readData(char *data, qint64 maxlen)
{
    char *newData = data;
    qint64 newMax = maxlen;
    qint64 devPos = pos();
    qint64 br = 0;

    if(devPos < buf.size())
    {
        //Read from buffer
        qint64 maxBufPos = qMin(devPos + newMax, qint64(buf.size()));
        br = maxBufPos - devPos;
        memcpy(data, buf.constData() + devPos, br);

        newMax -= br;
        newData += br;

        if(newMax == 0)
            return br;
    }

    //Read from original device
    return br + source->read(newData, newMax);
}

qint64 ProxySeqDevice::writeData(const char *data, qint64 len)
{
    Q_UNUSED(data);
    Q_UNUSED(len);
    return -1;
}

class CustomEntityResolver : public QXmlStreamEntityResolver
{
public:
    CustomEntityResolver() = default;

    QString resolveUndeclaredEntity(const QString &name) override
    {
        //Return harmless non-empty string to avoid parser error
        return QLatin1String(" ");
    }
};

QByteArray deviceFindString(QIODevice *dev, const QByteArray& str)
{
    constexpr int BUF_LEN = 1024;

    qint64 start = -1;
    qint64 pos = dev->pos();

    while (!dev->atEnd())
    {
        char buf[BUF_LEN];
        qint64 br = dev->read(buf, BUF_LEN);
        qint64 origPos = pos;
        pos += br;

        int i = 0;
        int idx = 0;
        for(; i < br; i++)
        {
            if(start == -1)
            {
                if(buf[i] == str[0])
                    start = origPos + i;
                continue;
            }

            idx = origPos + i - start;
            if(idx < str.size() && buf[i] != str[idx])
            {
                start = -1;
                continue;
            }

            if(idx >= str.size())
                break;
        }

        if(start != -1 && idx == str.size())
        {
            QByteArray ba(buf + start - origPos, br - (start - origPos));
            return ba;
        }
    }

    return QByteArray();
}

QByteArray deviceFindString2(QIODevice *dev, const QByteArray& str, QByteArray& prevBuf, qint64& prevPos, bool inside)
{
    //FIXME: optimize, do not use fixed size buf if prevBuf has more data already cached
    //To many memcpy

    constexpr int BUF_LEN = 1024;

    qint64 start = -1;

    qint64 pos = prevPos;

    QByteArray readSoFar;

    while (pos < prevPos + prevBuf.size() || !dev->atEnd())
    {
        char buf[BUF_LEN];
        qint64 br = 0;

        if(pos < prevPos + prevBuf.size())
        {
            //Read from buffer
            qint64 maxBufPos = qMin(pos + BUF_LEN, prevPos + prevBuf.size());
            br = maxBufPos - pos;
            memcpy(buf, prevBuf.constData() + (pos - prevPos), br);
        }

        if(br < BUF_LEN)
        {
            br += dev->read(buf + br, BUF_LEN - br);
        }

        qint64 origPos = pos;
        pos += br;

        int i = 0;
        int idx = 0;
        for(; i < br; i++)
        {
            if(start == -1)
            {
                if(buf[i] == str[0])
                    start = origPos + i;
                continue;
            }

            idx = origPos + i - start;
            if(idx < str.size() && buf[i] != str[idx])
            {
                start = -1;
                continue;
            }

            if(idx >= str.size())
                break;
        }

        if(start != -1 && idx == str.size())
        {
            const qint64 bufStartPos = start - origPos;
            if(inside)
            {
                //Read until start
                const qint64 prevSize = readSoFar.size();
                readSoFar.resize(prevSize + bufStartPos);
                memcpy(readSoFar.data() + prevSize, buf, bufStartPos);

                //Save the rest for next call
                prevBuf.resize(br - bufStartPos);
                memcpy(prevBuf.data(), buf + bufStartPos, prevBuf.size());
            }
            else
            {
                //Clear previous buffer
                prevBuf.clear();

                //Read after start
                readSoFar.resize(br - bufStartPos);
                memcpy(readSoFar.data(), buf + bufStartPos, readSoFar.size());
            }

            prevPos = start;
            return readSoFar;
        }

        if(inside)
        {
            //Store current buffer
            const qint64 prevSize = readSoFar.size();
            readSoFar.resize(prevSize + br);
            memcpy(readSoFar.data() + prevSize, buf, br);
        }
    }

    return QByteArray();
}

JobCategory matchCategory(const QString& name)
{
    if(name.contains(QLatin1String("FRECCIA"), Qt::CaseInsensitive) ||
        name.contains(QLatin1String("NTV"), Qt::CaseInsensitive))
    {
        return JobCategory::HIGH_SPEED;
    }

    if(name.contains(QLatin1String("INTERCITY"), Qt::CaseInsensitive))
    {
        return JobCategory::INTERCITY;
    }

    if(name.contains(QLatin1String("REGIONALE"), Qt::CaseInsensitive))
    {
        if(name.contains(QLatin1String("VELOCE"), Qt::CaseInsensitive))
            return JobCategory::FAST_REGIONAL;
        return JobCategory::REGIONAL;
    }

    return JobCategory::NCategories;
}

bool readJobTable(QXmlStreamReader &xml, QVector<JobStopsImporter::Stop>& stops)
{
    const QString timeFmt = QLatin1String("HH.mm.ss");

    int i = 0;

    xml.readNextStartElement(); //<tbody> table start
    while(!xml.atEnd())
    {
        //<tr> Row start
        if(!xml.readNextStartElement() || xml.hasError() || xml.atEnd())
            break;

        xml.readNextStartElement(); //<td> Station Cell
        if(xml.hasError() || xml.atEnd())
            break;

        JobStopsImporter::Stop stop;

        stop.stationName = xml.readElementText(QXmlStreamReader::IncludeChildElements).simplified();

        xml.readNextStartElement(); //<td> Arrival Cell
        QString arr = xml.readElementText(QXmlStreamReader::IncludeChildElements).simplified();
        stop.arrival = QTime::fromString(arr, timeFmt);

        xml.readNextStartElement(); //<td> Departure Cell
        QString dep = xml.readElementText(QXmlStreamReader::IncludeChildElements).simplified();
        stop.departure = QTime::fromString(dep, timeFmt);

        if(xml.readNextStartElement()) //<td> Platform Cell
        {
            stop.platformName = xml.readElementText(QXmlStreamReader::IncludeChildElements).simplified();

            //Skip current row
            xml.skipCurrentElement();
        }

        //Do not skip current row because it's already done by failed readNextStartElement()

        stops.append(stop);

        qDebug().noquote() << i << stop.stationName.leftJustified(20, ' ')
                           << arr.leftJustified(8, ' ') << " "
                           << dep.leftJustified(8, ' ') << " "
                           << stop.platformName.rightJustified(2, ' ');
        i++;
    }

    return !xml.hasError();
}

bool readStationTableRow(QXmlStreamReader &xml, ImportedJobItem& jobItem)
{
    const QString timeFmt = QLatin1String("HH:mm");
    const QString origTimeFmt = QLatin1String("(HH:mm)");

    //<tr> Row start
    xml.readNextStartElement();
    if(xml.atEnd())
        return false;

    xml.readNextStartElement(); //<td> Platform Cell
    if(xml.hasError())
        return false;

    if(xml.readNextStartElement())
    {
        //<span> platform span
        jobItem.platformName = xml.readElementText().simplified();
        xml.skipCurrentElement(); //</td>
    }
    //Do not skip </td> because it's already done by failed readNextStartElement()

    xml.readNextStartElement(); //<td> Train number Cell
    xml.readNextStartElement(); //<div>
    xml.readNextStartElement(); //<span>
    xml.readNextStartElement(); //<img>

    jobItem.catName = xml.attributes().value("alt").toString();
    xml.skipCurrentElement(); //</img>
    xml.skipCurrentElement(); //</span>

    xml.readNextStartElement(); //<span>
    xml.readNextStartElement(); //<a>

    jobItem.urlPath = xml.attributes().value("href").toString();

    jobItem.number = xml.readElementText().simplified().toLongLong();

    xml.skipCurrentElement(); //</span>
    xml.skipCurrentElement(); //</div>
    xml.skipCurrentElement(); //</td>

    xml.readNextStartElement(); //<td> Arrival
    QString arr = xml.readElementText().simplified();
    jobItem.arrival = QTime::fromString(arr, timeFmt);

    xml.readNextStartElement(); //<td> Departure
    QString dep = xml.readElementText().simplified();
    jobItem.departure = QTime::fromString(dep, timeFmt);

    xml.readNextStartElement(); //<td> Origin Station
    xml.readNextStartElement(); //<span>
    jobItem.originStation = xml.readElementText().simplified();

    if(xml.readNextStartElement())
    {
        //<span> Origin Time
        dep = xml.readElementText().simplified();
        jobItem.originTime = QTime::fromString(dep, origTimeFmt);
        xml.skipCurrentElement(); //</td>
    }
    //Do not skip </td> because it's already done by failed readNextStartElement()

    xml.readNextStartElement(); //<td> Destination Station
    xml.readNextStartElement(); //<span>
    jobItem.destinationStation = xml.readElementText().simplified();

    if(xml.readNextStartElement())
    {
        //<span> Destination Time
        arr = xml.readElementText().simplified();
        jobItem.destinationTime = QTime::fromString(arr, origTimeFmt);
        xml.skipCurrentElement(); //</td>
    }
    //Do not skip </td> because it's already done by failed readNextStartElement()

    xml.skipCurrentElement(); //</tr>

    return !xml.hasError();
}

E656NetImporter::E656NetImporter(sqlite3pp::database &db, QObject *parent) :
    QObject(parent),
    mDb(db)
{
    manager = new QNetworkAccessManager(this);
}

QNetworkReply* E656NetImporter::startImportJob(const QUrl &url)
{
    QNetworkRequest req(url);
    QNetworkReply *reply = manager->get(req);
    return reply;
}

void E656NetImporter::doImportJob(QNetworkReply *reply, const ImportedJobItem &item)
{
    qDebug() << "IMPORTING JOB:" << reply->url();

    reply->deleteLater();

    if(reply->error() != QNetworkReply::NoError)
    {
        qDebug() << "Error:" << reply->errorString();

        emit errorOccurred(tr("Network Error:\n"
                              "%1\n"
                              "%2")
                               .arg(reply->url().toString(),
                                   reply->errorString()));
        return;
    }

    //Single job page

    //Needed because HTML is not fully compatible with XML
    //So parser doesn't work.
    //As a workaroud we skip to the job stop table which is well-formed and read from there
    //To find the table we read in chunks, but device is sequential so we cannot seek back
    //to workaround this second issue, we save the chunk from table onward and use a fake device
    //which concatenates the buffer with the real device
    QByteArray seekBuf = deviceFindString(reply, "<tbody class=\"list-table\">");
    if(seekBuf.isEmpty())
        return;

    ProxySeqDevice proxy(reply, seekBuf);

    QXmlStreamReader xml(&proxy);

    CustomEntityResolver resolver;
    xml.setEntityResolver(&resolver);

    QVector<JobStopsImporter::Stop> stops;

    if(!readJobTable(xml, stops))
    {
        qDebug() << "XML ERROR:" << xml.error() << xml.errorString();
        return;
    }

    JobStopsImporter importer(mDb);
    importer.createJob(item.number, item.matchingCategory, stops);
}

bool E656NetImporter::readStationTable(QIODevice *dev, QVector<ImportedJobItem> &items)
{
    QByteArray prevBuf;
    qint64 prevPos = dev->pos();

    const char tableStart[] = "<tbody class=\"list-table\" ng-controller=\"bookingController\">";
    const int tableStartSize = sizeof(tableStart) - 1;

    const char rowStart[] = "<tr ng-class=\"{";
    const int rowStartSz = sizeof(rowStart) - 1;

    const char rowEnd[] = "</tr>";
    const int rowEndSz = sizeof(rowEnd) - 1;


    //Jump to table body start
    QByteArray rowBuf = deviceFindString2(dev, QByteArray::fromRawData(tableStart, tableStartSize),
                                          prevBuf, prevPos, false);
    rowBuf = rowBuf.right(rowBuf.size() - tableStartSize); //Skip table body start element
    prevPos += tableStartSize;

    QXmlStreamReader xml;

    CustomEntityResolver resolver;
    xml.setEntityResolver(&resolver);

    while (true)
    {
        //Read until row end
        prevBuf = rowBuf;
        rowBuf = deviceFindString2(dev, QByteArray::fromRawData(rowEnd, rowEndSz),
                                   prevBuf, prevPos, true);
        if(rowBuf.isEmpty())
            break;

        //Keep row end in the buf
        rowBuf.append(rowEnd, rowEndSz);

        //Parse row...
        xml.clear();
        xml.addData(rowBuf);

        ImportedJobItem jobItem;

        if(readStationTableRow(xml, jobItem))
        {
            jobItem.matchingCategory = matchCategory(jobItem.catName);
            items.append(jobItem);
        }
        else
        {
            qDebug() << "Station row error";
        }

        //Go to next row
        rowBuf = deviceFindString2(dev, QByteArray::fromRawData(rowStart, rowStartSz),
                                   prevBuf, prevPos, false);
    }

    return true;
}
