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

#ifndef CUSTOMLINEEDIT_H
#define CUSTOMLINEEDIT_H

#include <QLineEdit>

#include "utils/types.h"

class QTreeView;
class QModelIndex;
class ISqlFKMatchModel;

class CustomCompletionLineEdit : public QLineEdit
{
    Q_OBJECT
public:
    CustomCompletionLineEdit(ISqlFKMatchModel *m, QWidget *parent = nullptr);
    ~CustomCompletionLineEdit();

    void showPopup(bool showAlsoIfEmpty = false);

    bool getData(db_id &idOut, QString &nameOut) const;

    void setData(db_id id, const QString &name = QString());

    void setModel(ISqlFKMatchModel *m);

    void resizeColumnToContents();
    void selectFirstIndexOrNone(bool forceFirst);

signals:
    void completionDone(CustomCompletionLineEdit *self);
    void dataIdChanged(db_id id);
    void indexSelected(const QModelIndex &idx);

public slots:
    void setData_slot(db_id id);

protected:
    void timerEvent(QTimerEvent *event);
    void mousePressEvent(QMouseEvent *e);
    bool eventFilter(QObject *obj, QEvent *ev);

private slots:
    void doneCompletion(const QModelIndex &idx);
    void startSuggestionsTimer();
    void stopSuggestionsTimer();
    void resultsReady(bool forceFirst);

private:
    QTreeView *popup;
    ISqlFKMatchModel *model;
    db_id dataId;
    int suggestionsTimerId;
};

#endif // CUSTOMLINEEDIT_H
