#ifndef PRINTWORKER_H
#define PRINTWORKER_H

#include <QObject>

#include <QVector>

#include "printdefs.h"

#include "utils/types.h"

class QPrinter;
class QPainter;
class QGraphicsScene;
class GraphManager;

class PrintWorker : public QObject
{
    Q_OBJECT
public:

    typedef struct Scene_
    {
        db_id lineId;
        QGraphicsScene *scene;
        QString name;
    } Scene;

    typedef QVector<Scene> Scenes;

    explicit PrintWorker(QObject *parent = nullptr);

    void setScenes(const Scenes &scenes);

    void setBackground(QGraphicsScene *background);
    void setGraphMgr(GraphManager *value);

    void setPrinter(QPrinter *printer);

    inline int getMaxProgress() { return m_scenes.size(); }

    void setOutputType(Print::OutputType type);
    void setFileOutput(const QString &value, bool different);

    void printPdfMultipleFiles();
    void printSvg();
    void printNormal();

signals:
    void progress(int val);
    void description(const QString& text);
    void finished();

public slots:
    void doWork();

private:
    void printScene(QPainter *painter, const Scene &s);

private:
    Scenes m_scenes;
    QPrinter *m_printer;
    GraphManager *graphMgr;

    QString fileOutput;
    bool differentFiles;
    Print::OutputType outType;
};

#endif // PRINTWORKER_H
