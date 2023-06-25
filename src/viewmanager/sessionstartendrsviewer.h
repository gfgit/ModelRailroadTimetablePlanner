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

#ifndef SESSIONSTARTENDRSVIEWER_H
#define SESSIONSTARTENDRSVIEWER_H

#include <QWidget>

class QTreeView;
class QComboBox;
class SessionStartEndModel;

class SessionStartEndRSViewer : public QWidget
{
    Q_OBJECT
public:
    explicit SessionStartEndRSViewer(QWidget *parent = nullptr);

private slots:
    void orderChanged();
    void modeChanged();

    void exportSheet();

private:
    SessionStartEndModel *model;

    QTreeView *view;

    QComboBox *modeCombo;
    QComboBox *orderCombo;
};

#endif // SESSIONSTARTENDRSVIEWER_H
