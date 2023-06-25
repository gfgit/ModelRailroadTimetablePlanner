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

#ifndef RSCOUPLEDIALOG_H
#define RSCOUPLEDIALOG_H

#include <QDialog>

#include "utils/types.h"

class RSCouplingInterface;
class RSProxyModel;
class QPushButton;

namespace sqlite3pp {
class database;
}

// FIXME: on-demand load and filter
class RSCoupleDialog : public QDialog
{
    Q_OBJECT
public:
    RSCoupleDialog(RSCouplingInterface *mgr, RsOp o, QWidget *parent = nullptr);

    void loadProxyModels(sqlite3pp::database &db, db_id jobId, db_id stopId, db_id stationId,
                         const QTime &arrival);

protected:
    void done(int ret) override;

private slots:
    void toggleLegend();

private:
    void updateButText();
    void setLegendVisible(bool val);

private:
    RSCouplingInterface *couplingMgr;

    RSProxyModel *engModel;
    RSProxyModel *coachModel;
    RSProxyModel *freightModel;

    QPushButton *showHideLegendBut;
    QWidget *legend;
    bool m_showLegend;

    RsOp op;
};

#endif // RSCOUPLEDIALOG_H
