/*
 * ModelRailroadTimetablePlanner
 * Copyright 2016-2023, Filippo Gentile
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "chooseitemdlg.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QPushButton>

#include "utils/delegates/sql/customcompletionlineedit.h"

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
