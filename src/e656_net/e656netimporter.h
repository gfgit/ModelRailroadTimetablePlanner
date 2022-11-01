#ifndef E656NETIMPORTER_H
#define E656NETIMPORTER_H

#include <QObject>

#include "model/e656_utils.h"

class QUrl;
class QNetworkAccessManager;
class QNetworkReply;
class QIODevice;

namespace sqlite3pp {
class database;
}

class E656NetImporter : public QObject
{
    Q_OBJECT
public:
    E656NetImporter(sqlite3pp::database &db, QObject *parent = nullptr);

    QNetworkReply* startImportJob(const QUrl& url);
    bool readStationTable(QIODevice *dev, QVector<ImportedJobItem> &items);
    void doImportJob(QNetworkReply *reply, const ImportedJobItem& item);

signals:
    void errorOccurred(const QString& msg);

private:
    sqlite3pp::database &mDb;

    QNetworkAccessManager *manager;
};

#endif // E656NETIMPORTER_H
