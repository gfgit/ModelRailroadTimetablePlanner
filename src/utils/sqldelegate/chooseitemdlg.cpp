#include "chooseitemdlg.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QPushButton>

#include "utils/sqldelegate/customcompletionlineedit.h"

ChooseItemDlg::ChooseItemDlg(ISqlFKMatchModel *matchModel, QWidget *parent) :
    QDialog(parent),
    itemId(0)
{
    QVBoxLayout *lay = new QVBoxLayout(this);

    label = new QLabel;
    lay->addWidget(label);

    lineEdit = new CustomCompletionLineEdit(matchModel);
    connect(lineEdit, &CustomCompletionLineEdit::dataIdChanged, this, &ChooseItemDlg::itemChosen);
    lay->addWidget(lineEdit);

    buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    lay->addWidget(buttonBox);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    itemChosen(0);

    setWindowTitle(tr("Insert value"));
    setMinimumSize(250, 100);
}

void ChooseItemDlg::setDescription(const QString &text)
{
    label->setText(text);
}

void ChooseItemDlg::setPlaceholder(const QString &text)
{
    lineEdit->setPlaceholderText(text);
}

void ChooseItemDlg::done(int res)
{
    if(res == QDialog::Accepted)
    {
        QString errMsg;
        if(m_callback && !m_callback(itemId, errMsg))
        {
            QMessageBox::warning(this, tr("Error"), errMsg);
            return;
        }
    }

    QDialog::done(res);
}

void ChooseItemDlg::itemChosen(db_id id)
{
    itemId = id;
    QPushButton *okBut = buttonBox->button(QDialogButtonBox::Ok);
    if(itemId)
    {
        okBut->setToolTip(QString());
        okBut->setEnabled(true);
    }else{
        okBut->setToolTip(tr("In order to proceed you must select a valid item."));
        okBut->setEnabled(false);
    }
}

void ChooseItemDlg::setCallback(const Callback &callback)
{
    m_callback = callback;
}
