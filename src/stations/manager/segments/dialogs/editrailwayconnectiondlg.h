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

#ifndef EDITRAILWAYCONNECTIONDLG_H
#define EDITRAILWAYCONNECTIONDLG_H

#include <QDialog>

class RailwaySegmentConnectionsModel;

class SpinBoxEditorFactory;
class QTableView;

class EditRailwayConnectionDlg : public QDialog
{
    Q_OBJECT
public:
    EditRailwayConnectionDlg(RailwaySegmentConnectionsModel *m, QWidget *parent = nullptr);
    ~EditRailwayConnectionDlg();

    virtual void done(int res) override;

    void setReadOnly(bool readOnly);

private slots:
    void onModelError(const QString &msg);
    void addTrackConn();
    void removeSelectedTrackConn();
    void addDefaultConnections();

private:
    RailwaySegmentConnectionsModel *model;

    QTableView *view;
    SpinBoxEditorFactory *fromTrackFactory;
    SpinBoxEditorFactory *toTrackFactory;
};

#endif // EDITRAILWAYCONNECTIONDLG_H
