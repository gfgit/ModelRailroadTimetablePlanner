#include "printeroptionspage.h"
#include "printwizard.h"

#include <QPushButton>
#include <QPrintDialog>

#include <QFormLayout>

#include <QPointer>

PrinterOptionsPage::PrinterOptionsPage(PrintWizard *w, QWidget *parent) :
    QWizardPage (parent),
    mWizard(w)
{
    optionsBut = new QPushButton(tr("Open settings"));
    connect(optionsBut, &QPushButton::clicked, this, &PrinterOptionsPage::onOpenDialog);

    QFormLayout *l = new QFormLayout;
    l->addRow(tr("Printer"), optionsBut);
    setLayout(l);

    setTitle(tr("Printer options"));
}

void PrinterOptionsPage::onOpenDialog()
{
    QPointer<QPrintDialog> dlg = new QPrintDialog(mWizard->getPrinter(), this);
    dlg->exec();
    delete dlg;
}
