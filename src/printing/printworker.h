#ifndef PRINTWORKER_H
#define PRINTWORKER_H

#include <QObject>

#include <QVector>

#include "printdefs.h"

#include "utils/types.h"

class QPrinter;
class QPainter;

class SceneSelectionModel;

class LineGraphScene;

namespace sqlite3pp {
class database;
}

class PrintWorker : public QObject
{
    Q_OBJECT
public:
    PrintWorker(sqlite3pp::database &db, QObject *parent = nullptr);

    void setPrinter(QPrinter *printer);

    void setOutputType(Print::OutputType type);
    void setFileOutput(const QString &value, bool different);

    void setSelection(SceneSelectionModel *newSelection);
    int getMaxProgress() const;

signals:
    void progress(int val);
    void description(const QString& text);
    void errorOccured(const QString& msg);
    void finished();

public slots:
    void doWork();

private:
    void printSvg();
    void printPdf();
    void printPdfMultipleFiles();
    void printPaged();

private:
    typedef std::function<bool(QPainter *painter, bool firstPage,
                               const QString& title, const QRectF& sourceRect)> BeginPaintFunc;

    void printInternal(BeginPaintFunc func, bool endPaintingEveryPage);

private:
    QPrinter *m_printer;
    SceneSelectionModel *selection;

    QString fileOutput;
    bool differentFiles;
    Print::OutputType outType;

    LineGraphScene *scene;
};

#endif // PRINTWORKER_H
