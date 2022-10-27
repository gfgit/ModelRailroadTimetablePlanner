#ifndef SHIFTGRAPHEDITOR_H
#define SHIFTGRAPHEDITOR_H

#include <QWidget>

class QToolBar;

class ShiftGraphView;
class ShiftGraphScene;

class ShiftGraphEditor : public QWidget
{
    Q_OBJECT
public:
    ShiftGraphEditor(QWidget *parent = nullptr);
    virtual ~ShiftGraphEditor();

private slots:
    void redrawGraph();

    void exportSVG();
    void exportPDF();
    void printGraph();

private:
    QToolBar *toolBar;

    ShiftGraphView *view;
    ShiftGraphScene *m_scene;
};

#endif // SHIFTGRAPHEDITOR_H
