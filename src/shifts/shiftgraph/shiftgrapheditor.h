#ifndef SHIFTGRAPHEDITOR_H
#define SHIFTGRAPHEDITOR_H

#include <QWidget>

#include "utils/types.h"

class QToolBar;

class ShiftGraphView;
class ShiftGraphScene;

class QPainter;

class ShiftGraphEditor : public QWidget
{
    Q_OBJECT
public:
    ShiftGraphEditor(QWidget *parent = nullptr);
    virtual ~ShiftGraphEditor();

    void exportSVG(const QString &fileName);
    void exportPDF(const QString &fileName);

private slots:
    void redrawGraph();

    void onSaveGraph();
    void onPrintGraph();

private:
    void renderGraph(QPainter *painter);

private:
    QToolBar *toolBar;

    ShiftGraphView *view;
    ShiftGraphScene *m_scene;
};

#endif // SHIFTGRAPHEDITOR_H
