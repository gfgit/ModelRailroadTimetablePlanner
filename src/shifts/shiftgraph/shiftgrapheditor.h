#ifndef SHIFTGRAPHEDITOR_H
#define SHIFTGRAPHEDITOR_H

#include <QWidget>

#include "utils/types.h"

class QGraphicsView;
class QToolBar;

class ShiftGraphHolder;

class QPrinter;

class ShiftGraphEditor : public QWidget
{
    Q_OBJECT
public:
    ShiftGraphEditor(QWidget *parent = nullptr);
    virtual ~ShiftGraphEditor();

    void exportSVG(const QString &fileName);
    void exportPDF(const QString &fileName);
    void print(QPrinter *printer);
    void updateJobColors();

public slots:
    void onSaveGraph();
    void onPrintGraph();
    void onShowOptions();

    void calculateGraph();

    void refreshView();

private slots:
    void showShiftMenuForJob(db_id jobId);
private:
    QToolBar *toolBar;
    QGraphicsView *view;
    ShiftGraphHolder *graph;
};

#endif // SHIFTGRAPHEDITOR_H
