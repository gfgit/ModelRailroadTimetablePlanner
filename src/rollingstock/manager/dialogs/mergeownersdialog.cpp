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

#include "mergeownersdialog.h"
#include "ui_mergeownersdialog.h"

#include "app/session.h"

#include "rollingstock/rsownersmatchmodel.h"

#include <QMessageBox>
#include "utils/delegates/sql/customcompletionlineedit.h"

#include <QDebug>

MergeOwnersDialog::MergeOwnersDialog(database &db, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::MergeOwnersDialog),
    mDb(db)
{
    ui->setupUi(this);

    model           = new RSOwnersMatchModel(mDb, this);
    sourceOwnerEdit = new CustomCompletionLineEdit(model);
    destOwnerEdit   = new CustomCompletionLineEdit(model);

    sourceOwnerEdit->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    destOwnerEdit->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    int idx = ui->verticalLayout->indexOf(ui->descriptionLabel);
    ui->verticalLayout->insertWidget(idx + 1, destOwnerEdit);
    ui->verticalLayout->insertWidget(idx + 1, sourceOwnerEdit);

    connect(sourceOwnerEdit, &CustomCompletionLineEdit::editingFinished, this,
            &MergeOwnersDialog::resetModel);
    connect(destOwnerEdit, &CustomCompletionLineEdit::editingFinished, this,
            &MergeOwnersDialog::resetModel);

    ui->removeSourceCheckBox->setChecked(AppSettings.getRemoveMergedSourceOwner());
}

MergeOwnersDialog::~MergeOwnersDialog()
{
    delete ui;
}

void MergeOwnersDialog::done(int r)
{
    if (r == QDialog::Accepted)
    {
        db_id sourceOwnerId = 0;
        db_id destOwnerId   = 0;
        QString tmp;
        sourceOwnerEdit->getData(sourceOwnerId, tmp);
        destOwnerEdit->getData(destOwnerId, tmp);

        // Check input is valid
        if (!sourceOwnerId || !destOwnerId || sourceOwnerId == destOwnerId)
        {
            QMessageBox::warning(this, tr("Invalid Owners"),
                                 tr("Owners must not be null and must be different"));
            return; // We don't want the dialog to be closed
        }

        if (mergeOwners(sourceOwnerId, destOwnerId, ui->removeSourceCheckBox->isChecked()))
        {
            // Operation succeded, inform user
            QMessageBox::information(this, tr("Merging completed"),
                                     tr("Owners merged succesfully."));
        }
        else
        {
            QMessageBox::warning(this, tr("Error while merging"),
                                 tr("Some error occurred while merging owners."));
            // Accept dialog to close it, so don't return here
        }
    }

    QDialog::done(r);
}

void MergeOwnersDialog::resetModel()
{
    model->autoSuggest(QString());
}

bool MergeOwnersDialog::mergeOwners(db_id sourceOwnerId, db_id destOwnerId, bool removeSource)
{
    command q_mergeOwners(mDb, "UPDATE rs_list SET owner_id=? WHERE owner_id=?");
    q_mergeOwners.bind(1, destOwnerId);
    q_mergeOwners.bind(2, sourceOwnerId);
    int ret = q_mergeOwners.execute();
    q_mergeOwners.reset();

    if (ret != SQLITE_OK)
    {
        qDebug() << "Merging Owners" << sourceOwnerId << destOwnerId;
        qDebug() << "DB Error:" << ret << mDb.error_msg() << mDb.extended_error_code();
        return false;
    }

    if (removeSource)
    {
        command q_removeSource(mDb, "DELETE FROM rs_owners WHERE id=?");
        q_removeSource.bind(1, sourceOwnerId);
        ret = q_removeSource.execute();
        if (ret != SQLITE_OK)
        {
            qDebug() << "Removing owner" << sourceOwnerId;
            qDebug() << "DB Error:" << ret << mDb.error_msg() << mDb.extended_error_code();
        }
    }

    return true;
}
