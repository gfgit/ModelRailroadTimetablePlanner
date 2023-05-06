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

#ifndef MODELMERGEDIALOG_H
#define MODELMERGEDIALOG_H

#include <QDialog>

#include "utils/types.h"

#include <sqlite3pp/sqlite3pp.h>

class CustomCompletionLineEdit;
class RSModelsMatchModel;
class QLabel;

namespace Ui {
class MergeModelsDialog;
}

class MergeModelsDialog : public QDialog
{
    Q_OBJECT

public:
    MergeModelsDialog(sqlite3pp::database &db, QWidget *parent = nullptr);
    ~MergeModelsDialog();

protected:
    void done(int r) override;

private slots:
    void sourceModelChanged(db_id modelId);
    void destModelChanged(db_id modelId);

private:
    void fillModelInfo(QLabel *label, db_id modelId);

private:
    int mergeModels(db_id sourceModelId, db_id destModelId, bool removeSource);

private:
    Ui::MergeModelsDialog *ui;

    CustomCompletionLineEdit *sourceModelEdit;
    CustomCompletionLineEdit *destModelEdit;

    RSModelsMatchModel *model;

    sqlite3pp::database &mDb;
    sqlite3pp::query q_getModelInfo;
};

#endif // MODELMERGEDIALOG_H
