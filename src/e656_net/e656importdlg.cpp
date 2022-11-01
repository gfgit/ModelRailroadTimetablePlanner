#include "e656importdlg.h"

#include <QVBoxLayout>
#include <QHBoxLayout>

#include <QPushButton>
#include <QLineEdit>
#include <QTableView>

#include <QMessageBox>

#include <QUrl>
#include <QNetworkReply>

#include "e656netimporter.h"
#include "model/e656stationmodel.h"

E656ImportDlg::E656ImportDlg(sqlite3pp::database &db, QWidget *parent) :
    QDialog(parent)
{
    importer = new E656NetImporter(db, this);
    model = new E656StationModel(this);

    QVBoxLayout *lay = new QVBoxLayout(this);

    QHBoxLayout *hlay = new QHBoxLayout;
    urlEdit = new QLineEdit;
    hlay->addWidget(urlEdit);

    urlImportBut = new QPushButton("Parse");
    hlay->addWidget(urlImportBut);

    lay->addLayout(hlay);

    view = new QTableView;
    view->setModel(model);
    lay->addWidget(view);

    connect(importer, &E656NetImporter::errorOccurred, this, &E656ImportDlg::showError);
    connect(urlImportBut, &QPushButton::clicked, this, &E656ImportDlg::startUrlRequest);
}

void E656ImportDlg::startUrlRequest()
{
    QUrl url = QUrl::fromUserInput(urlEdit->text());
    if(!url.isValid())
    {
        showError(tr("Invalid URL"));
        return;
    }

    if(!url.path().contains(QLatin1String("stazione")))
    {
        showError(tr("Not a station URL"));
        return;
    }

    QNetworkReply *reply = importer->startImportJob(url);
    auto onFinished = [this,  reply]()
    {
        reply->deleteLater();

        if(reply->error() != QNetworkReply::NoError)
        {
            qDebug() << "Error:" << reply->errorString();

            showError(tr("Network Error:\n"
                         "%1\n"
                         "%2")
                          .arg(reply->url().toString(), reply->errorString()));
            return;
        }

        QVector<ImportedJobItem> jobs;
        if(!importer->readStationTable(reply, jobs))
            return;

        model->setJobs(jobs);
    };

    connect(reply, &QNetworkReply::finished, this, onFinished);
}

void E656ImportDlg::showError(const QString &msg)
{
    QMessageBox::warning(this, tr("Import Error"), msg);
}
