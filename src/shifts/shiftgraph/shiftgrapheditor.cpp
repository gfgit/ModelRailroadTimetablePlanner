#include "shiftgrapheditor.h"

#include <QVBoxLayout>

#include <QToolBar>

#include "view/shiftgraphview.h"
#include "model/shiftgraphscene.h"

#include "app/session.h"

#include "utils/files/openfileinfolder.h"

#include "utils/owningqpointer.h"
#include "shifts/shiftgraph/view/shiftgraphprintdlg.h"

#include "info.h"

#include <QDebug>

ShiftGraphEditor::ShiftGraphEditor(QWidget *parent) :
    QWidget(parent)
{
    toolBar = new QToolBar;
    toolBar->addAction(tr("Refresh"), this, &ShiftGraphEditor::redrawGraph);
    toolBar->addSeparator();
    toolBar->addAction(tr("Print"), this, &ShiftGraphEditor::printGraph);
    toolBar->addAction(tr("Save PDF"), this, &ShiftGraphEditor::exportPDF);
    toolBar->addAction(tr("Save SVG"), this, &ShiftGraphEditor::exportSVG);


    m_scene = new ShiftGraphScene(Session->m_Db, this);
    m_scene->loadShifts();

    view = new ShiftGraphView;
    view->setScene(m_scene);

    QVBoxLayout *lay = new QVBoxLayout(this);
    lay->addWidget(toolBar);
    lay->addWidget(view);

    setMinimumSize(500, 400);
    setWindowTitle(tr("Shift Graph"));
}

ShiftGraphEditor::~ShiftGraphEditor()
{

}

void ShiftGraphEditor::redrawGraph()
{
    m_scene->loadShifts();
}

void ShiftGraphEditor::printGraph()
{
    OwningQPointer<ShiftGraphPrintDlg> dlg = new ShiftGraphPrintDlg(Session->m_Db, this);
    dlg->setOutputType(Print::OutputType::Native);
    dlg->exec();
}

void ShiftGraphEditor::exportSVG()
{
    OwningQPointer<ShiftGraphPrintDlg> dlg = new ShiftGraphPrintDlg(Session->m_Db, this);
    dlg->setOutputType(Print::OutputType::Svg);
    dlg->exec();
}

void ShiftGraphEditor::exportPDF()
{
    OwningQPointer<ShiftGraphPrintDlg> dlg = new ShiftGraphPrintDlg(Session->m_Db, this);
    dlg->setOutputType(Print::OutputType::Pdf);
    dlg->exec();
}
