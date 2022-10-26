#include "e656netimporter.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>

#include <QXmlStreamReader>
#include <QBuffer>

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
        qDebug() << "ENTITY:" << name;
        return QString(" ");
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

bool readJobTable(QXmlStreamReader &xml)
{
    int i = 0;

    xml.readNextStartElement(); //<tbody> table start
    while(!xml.atEnd())
    {
        xml.readNextStartElement(); //<tr> Row start
        if(xml.hasError())
            break;

        xml.readNextStartElement(); //<td> Station Cell
        if(xml.hasError())
            break;
        QString stName = xml.readElementText(QXmlStreamReader::IncludeChildElements).simplified();

        xml.readNextStartElement(); //<td> Arrival Cell
        QString arr = xml.readElementText(QXmlStreamReader::IncludeChildElements).simplified();

        xml.readNextStartElement(); //<td> Departure Cell
        QString dep = xml.readElementText(QXmlStreamReader::IncludeChildElements).simplified();

        xml.readNextStartElement(); //<td> Platform Cell
        QString platf = xml.readElementText(QXmlStreamReader::IncludeChildElements).simplified();

        //Skip current row
        xml.skipCurrentElement();

        qDebug().noquote() << i << stName.leftJustified(20, ' ')
                           << arr.leftJustified(8, ' ') << " "
                           << dep.leftJustified(8, ' ') << " "
                           << platf.rightJustified(2, ' ');
        i++;
    }

    return !xml.hasError();
}

bool readStationTableRow(QXmlStreamReader &xml, QString& jobCat, QString &jobNum, QString& urlPath)
{
    //<tr> Row start
    xml.readNextStartElement();
    if(xml.atEnd())
        return false;

    xml.readNextStartElement(); //<td> Platform Cell
    if(xml.hasError())
        return false;
    xml.skipCurrentElement();

    xml.readNextStartElement(); //<td> Train number Cell
    xml.readNextStartElement(); //<div>
    xml.readNextStartElement(); //<span>
    xml.readNextStartElement(); //<img>

    jobCat = xml.attributes().value("alt").toString();
    xml.skipCurrentElement(); //</img>
    xml.skipCurrentElement(); //</span>

    xml.readNextStartElement(); //<span>
    xml.readNextStartElement(); //<a>

    urlPath = xml.attributes().value("href").toString();

    jobNum = xml.readElementText().simplified();

    xml.skipCurrentElement(); //</span>
    xml.skipCurrentElement(); //</div>
    xml.skipCurrentElement(); //</td>
    xml.skipCurrentElement(); //</tr>

    return !xml.hasError();
}

E656NetImporter::E656NetImporter(sqlite3pp::database &db, QObject *parent) :
    QObject(parent),
    mDb(db)
{
    manager = new QNetworkAccessManager(this);
}

bool E656NetImporter::startImportJob(const QUrl &url, bool overwriteExisting)
{
    if(!overwriteExisting)
    {
        //TODO: check existing
    }

    QNetworkRequest req(url);
    QNetworkReply *reply = manager->get(req);
    connect(reply, &QNetworkReply::errorOccurred, this, [this, reply]()
        {
            emit errorOccurred(tr("Network Error:\n"
                                  "%1\n"
                                  "%2")
                                   .arg(reply->url().toString(),
                                       reply->errorString()));
            reply->deleteLater();
        });

    connect(reply, &QNetworkReply::finished, this, [this, reply]()
        {
            doImportJob(reply);
        });

    return true;
}

void E656NetImporter::doImportJob(QNetworkReply *reply)
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

    if(reply->url().toString().contains(QLatin1String("treno")))
    {
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

        if(!readJobTable(xml))
        {
            qDebug() << "XML ERROR:" << xml.error() << xml.errorString();
            return;
        }
    }
    else
    {
        //Station page

        if(!readStationTable(reply))
        {
            qDebug() << "XML ERROR: station";
            return;
        }
    }
}

bool E656NetImporter::readStationTable(QIODevice *dev)
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

    int jobCount = 0;

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

        QString jobCat, jobNum, urlPath;

        if(!readStationTableRow(xml, jobCat, jobNum, urlPath))
        {
            qDebug() << "Station row error";
        }
        else
        {
            jobCount++;

            QUrl url;
            url.setScheme(QLatin1String("https"));
            url.setHost(QLatin1String("www.e656.net"));
            url.setPath(urlPath);

            if(jobCount < 10)
                startImportJob(url);
        }

        //Go to next row
        rowBuf = deviceFindString2(dev, QByteArray::fromRawData(rowStart, rowStartSz),
                                   prevBuf, prevPos, false);
    }

    return true;
}
