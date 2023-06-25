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

#ifndef CHOOSEITEMDLG_H
#define CHOOSEITEMDLG_H

#include <QDialog>

#include "utils/types.h"

#include <functional>

class QDialogButtonBox;
class QLabel;
class CustomCompletionLineEdit;
class ISqlFKMatchModel;

class ChooseItemDlg : public QDialog
{
    Q_OBJECT
public:
    typedef std::function<bool(db_id, QString &)> Callback;

    ChooseItemDlg(ISqlFKMatchModel *matchModel, QWidget *parent);

    void setDescription(const QString &text);
    void setPlaceholder(const QString &text);

    void setCallback(const Callback &callback);

    inline db_id getItemId() const
    {
        return itemId;
    }

public slots:
    void done(int res) override;

private slots:
    void itemChosen(db_id id);

private:
    QLabel *label;
    CustomCompletionLineEdit *lineEdit;
    QDialogButtonBox *buttonBox;
    db_id itemId;
    Callback m_callback;
};

#endif // CHOOSEITEMDLG_H
