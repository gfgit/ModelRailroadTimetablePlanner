#include "printwizard.h"

#include "app/session.h"

#include "selectionpage.h"
#include "fileoptionspage.h"
#include "printeroptionspage.h"
#include "progresspage.h"

#include <QPrinter>

PrintWizard::PrintWizard(QWidget *parent) :
    QWizard (parent),
    differentFiles(false),
    type(Print::Native)
{
    printer = new QPrinter;

    setPage(0, new SelectionPage(this));
    setPage(1, new FileOptionsPage(this));
    setPage(2, new PrinterOptionsPage(this));
    setPage(3, new ProgressPage(this));

    setWindowTitle(tr("Print Wizard"));
}

PrintWizard::~PrintWizard()
{
    delete printer;
}

void PrintWizard::setOutputType(Print::OutputType out)
{
    type = out;
}

Print::OutputType PrintWizard::getOutputType() const
{
    return type;
}
