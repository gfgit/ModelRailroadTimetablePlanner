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

#ifndef FIXDUPLICATESDLG_H
#define FIXDUPLICATESDLG_H

#include <QDialog>

class IDuplicatesItemModel;
class QDialogButtonBox;
class QAbstractItemDelegate;
class QTableView;
class QProgressDialog;

class FixDuplicatesDlg : public QDialog
{
    Q_OBJECT
public:
    enum
    {
        GoBackToPrevPage = QDialog::Accepted + 1
    };

    FixDuplicatesDlg(IDuplicatesItemModel *m, bool enableGoBack, QWidget *parent = nullptr);

    void setItemDelegateForColumn(int column, QAbstractItemDelegate *delegate);

    int blockingReloadCount(int mode);

public slots:
    void done(int res) override;

private slots:
    void showModelError(const QString &text);
    void handleProgress(int progress, int max);
    void handleModelState(int state);

private:
    int warnCancel(QWidget *w);

private:
    friend class ProgressDialog;
    IDuplicatesItemModel *model;
    QTableView *view;
    QDialogButtonBox *box;
    QProgressDialog *progressDlg;
    bool canGoBack;
};

#endif // FIXDUPLICATESDLG_H
