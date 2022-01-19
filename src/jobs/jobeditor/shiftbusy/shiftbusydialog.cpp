#include "shiftbusydialog.h"
#include "shiftbusymodel.h"

#include <QVBoxLayout>

#include <QLabel>
#include <QTableView>
#include <QDialogButtonBox>

#include <QScrollArea>

ShiftBusyDlg::ShiftBusyDlg(QWidget *parent) :
    QDialog(parent),
    model(nullptr)
{
    QVBoxLayout *lay = new QVBoxLayout(this);

    m_label = new QLabel;
    lay->addWidget(m_label);

    view = new QTableView;
    lay->addWidget(view);

    QDialogButtonBox *box = new QDialogButtonBox(QDialogButtonBox::Ok);
    connect(box, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(box, &QDialogButtonBox::rejected, this, &QDialog::reject);
    lay->addWidget(box);

    setWindowTitle(tr("Shift is busy"));
    setMinimumSize(350, 200);
}

void ShiftBusyDlg::setModel(ShiftBusyModel *m)
{
    model = m;
    view->setModel(m);

    m_label->setText(tr("Cannot set shift <b>%1</b> to job <b>%2</b>.<br>"
                        "The selected shift is busy:<br>"
                        "From: %3 To: %4")
                         .arg(model->getShiftName(),
                              model->getJobName(),
                              model->getStart().toString("HH:mm"),
                              model->getEnd().toString("HH:mm")));
}
