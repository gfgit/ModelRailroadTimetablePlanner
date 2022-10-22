#ifndef E656NETIMPORTER_H
#define E656NETIMPORTER_H

#include <QObject>

class QUrl;
class QNetworkAccessManager;
class QNetworkReply;

namespace sqlite3pp {
class database;
}

class E656NetImporter : public QObject
{
    Q_OBJECT
public:
    E656NetImporter(sqlite3pp::database &db, QObject *parent = nullptr);

    bool startImportJob(const QUrl& url, bool overwriteExisting = false);

private slots:
    void doImportJob(QNetworkReply *reply);

signals:
    void errorOccurred(const QString& msg);

private:
    sqlite3pp::database &mDb;

    QNetworkAccessManager *manager;
};

#endif // E656NETIMPORTER_H
