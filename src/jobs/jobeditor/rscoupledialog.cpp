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

#include "rscoupledialog.h"

#include <QLabel>
#include <QListView>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QFrame>

#include <QGridLayout>

#include "model/rsproxymodel.h"

#include "utils/rs_utils.h"

#include <sqlite3pp/sqlite3pp.h>

#include "app/session.h"

RSCoupleDialog::RSCoupleDialog(RSCouplingInterface *mgr, RsOp o, QWidget *parent) :
    QDialog(parent),
    couplingMgr(mgr),
    legend(nullptr),
    m_showLegend(false),
    op(o)
{
    engModel         = new RSProxyModel(couplingMgr, op, RsType::Engine, this);
    coachModel       = new RSProxyModel(couplingMgr, op, RsType::Coach, this);
    freightModel     = new RSProxyModel(couplingMgr, op, RsType::FreightWagon, this);

    QGridLayout *lay = new QGridLayout(this);

    QFont f;
    f.setBold(true);
    f.setPointSize(10);

    QListView *engView = new QListView;
    engView->setModel(engModel);
    QLabel *engLabel = new QLabel(tr("Engines"));
    engLabel->setAlignment(Qt::AlignCenter);
    engLabel->setFont(f);
    lay->addWidget(engLabel, 0, 0);
    lay->addWidget(engView, 1, 0);

    QListView *coachView = new QListView;
    coachView->setModel(coachModel);
    QLabel *coachLabel = new QLabel(tr("Coaches"));
    coachLabel->setAlignment(Qt::AlignCenter);
    coachLabel->setFont(f);
    lay->addWidget(coachLabel, 0, 1);
    lay->addWidget(coachView, 1, 1);

    QListView *freightView = new QListView;
    freightView->setModel(freightModel);
    QLabel *freightLabel = new QLabel(tr("Freight Wagons"));
    freightLabel->setAlignment(Qt::AlignCenter);
    freightLabel->setFont(f);
    lay->addWidget(freightLabel, 0, 2);
    lay->addWidget(freightView, 1, 2);

    legend = new QFrame;
    lay->addWidget(legend, 2, 0, 1, 3);
    legend->hide();

    QHBoxLayout *buttonLay = new QHBoxLayout;
    lay->addLayout(buttonLay, 3, 0, 1, 3);

    showHideLegendBut = new QPushButton;
    buttonLay->addWidget(showHideLegendBut);

    QDialogButtonBox *box =
      new QDialogButtonBox(QDialogButtonBox::Ok); // TODO: implement also cancel
    connect(box, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(box, &QDialogButtonBox::rejected, this, &QDialog::reject);
    buttonLay->addWidget(box);

    connect(showHideLegendBut, &QPushButton::clicked, this, &RSCoupleDialog::toggleLegend);
    updateButText();

    setMinimumSize(400, 300);
    setWindowFlag(Qt::WindowMaximizeButtonHint);

    setLegendVisible(AppSettings.getShowCouplingLegend());
}

void RSCoupleDialog::loadProxyModels(sqlite3pp::database &db, db_id jobId, db_id stopId,
                                     db_id stationId, const QTime &arrival)
{
    QList<RSProxyModel::RsItem> engines, freight, coaches;

    sqlite3pp::query q(db);

    if (op == RsOp::Coupled)
    {
        /* Show Couple-able RS:
         * - RS free in this station
         * - Unused RS (green)
         * - RS without operations before this time (=arrival) (cyan)
         *
         * Plus:
         * - Possible wrong operations to let the user remove them
         */

        q.prepare(
          "SELECT MAX(sub.p), sub.rs_id, rs_list.number, rs_models.name, rs_models.suffix, "
          "rs_models.type, rs_models.sub_type, sub.arr, sub.job_id, jobs.category FROM ("

          // Select possible wrong operations to let user remove (un-check) them
          " SELECT 1 AS p, coupling.rs_id AS rs_id, NULL AS arr, NULL AS job_id FROM coupling "
          "WHERE coupling.stop_id=?3 AND coupling.operation=1"
          " UNION ALL"

          // Select RS uncoupled before our arrival (included RS uncoupled at exact same time)
          // (except uncoupled by us)
          " SELECT 2 AS p, coupling.rs_id AS rs_id, MAX(stops.arrival) AS arr, stops.job_id AS "
          "job_id"
          " FROM stops"
          " JOIN coupling ON coupling.stop_id=stops.id"
          " WHERE stops.station_id=?1 AND stops.arrival <= ?2 AND stops.id<>?3"
          " GROUP BY coupling.rs_id"
          " HAVING coupling.operation=0"
          " UNION ALL"

          // Select RS coupled after our arrival (excluded RS coupled at exact same time)
          " SELECT 3 AS p, coupling.rs_id, MIN(stops.arrival) AS arr, stops.job_id AS job_id"
          " FROM coupling"
          " JOIN stops ON stops.id=coupling.stop_id"
          " WHERE stops.station_id=?1 AND stops.arrival > ?2"
          " GROUP BY coupling.rs_id"
          " HAVING coupling.operation=1"
          " UNION ALL"

          // Select coupled RS for first time
          " SELECT 4 AS p, rs_list.id AS rs_id, NULL AS arr, NULL AS job_id"
          " FROM rs_list"
          " WHERE NOT EXISTS ("
          " SELECT coupling.rs_id FROM coupling"
          " JOIN stops ON stops.id=coupling.stop_id WHERE coupling.rs_id=rs_list.id AND "
          "stops.arrival<?2)"
          " UNION ALL"

          // Select unused RS (RS without any operation)
          " SELECT 5 AS p, rs_list.id AS rs_id, NULL AS arr, NULL AS job_id"
          " FROM rs_list"
          " WHERE NOT EXISTS (SELECT coupling.rs_id FROM coupling WHERE coupling.rs_id=rs_list.id)"
          " UNION ALL"

          // Select RS coupled before our arrival (already coupled by this job or occupied by other
          // job)
          " SELECT 7 AS p, c1.rs_id, MAX(s1.arrival) AS arr, s1.job_id AS job_id"
          " FROM coupling c1"
          " JOIN coupling c2 ON c2.rs_id=c1.rs_id"
          " JOIN stops s1 ON s1.id=c2.stop_id"
          " WHERE c1.stop_id=?3 AND c1.operation=1 AND s1.arrival<?2"
          " GROUP BY c1.rs_id"
          " HAVING c2.operation=1"
          " ) AS sub"

          " JOIN rs_list ON rs_list.id=sub.rs_id" // FIXME: it seems it is better to join in the
                                                  // subquery directly, also avoids some LEFT in
                                                  // joins
          " LEFT JOIN rs_models ON rs_models.id=rs_list.model_id"
          " LEFT JOIN jobs ON jobs.id=sub.job_id"
          " GROUP BY sub.rs_id"
          " ORDER BY rs_models.name, rs_list.number");

        q.bind(1, stationId);
        q.bind(2, arrival);
        q.bind(3, stopId);
    }
    else
    {
        /*Show Uncouple-able:
         * - All RS coupled up to now (train asset before this stop)
         *
         * Plus:
         * - Show possible wrong operations to let user remove them
         */

        q.prepare(
          "SELECT MAX(sub.p), sub.rs_id, rs_list.number, rs_models.name, rs_models.suffix, "
          "rs_models.type, rs_models.sub_type, sub.arr, NULL AS job_id, NULL AS category FROM("
          "SELECT 8 AS p, coupling.rs_id AS rs_id, MAX(stops.arrival) AS arr"
          " FROM stops"
          " JOIN coupling ON coupling.stop_id=stops.id"
          " WHERE stops.arrival<?2"
          " GROUP BY coupling.rs_id"
          " HAVING coupling.operation=1 AND stops.job_id=?1"
          " UNION ALL"

          // Select possible wrong operations to let user remove them
          " SELECT 6 AS p, coupling.rs_id AS rs_id, NULL AS arr FROM coupling WHERE "
          "coupling.stop_id=?3 AND coupling.operation=0"
          ") AS sub"
          " JOIN rs_list ON rs_list.id=sub.rs_id" // FIXME: it seems it is better to join in the
                                                  // subquery directly, also avoids some LEFT in
                                                  // joins
          " LEFT JOIN rs_models ON rs_models.id=rs_list.model_id"
          " GROUP BY sub.rs_id"
          " ORDER BY rs_models.name, rs_list.number");

        q.bind(1, jobId);
        q.bind(2, arrival);
        q.bind(3, stopId);
    }

    for (auto rs : q)
    {
        /*Priority flag:
         * 1: The RS is not free in this station, it's shown to let the user remove the operation
         * 2: The RS is free and has no following operation in this station
         *    (It could have wrong operation in other station that should be fixed by user,
         *     as shown in RsErrorWidget)
         * 3: The RS is free but a job will couple it in the future so you should bring it back here
         * before that time 4: The RS is used for first time 5: The RS is unsed 6: RS was not
         * coupled before this stop and you are trying to uncouple it 7: Normal RS uncouple-able,
         * used to win against 6
         */

        RSProxyModel::RsItem item;
        item.flag               = rs.get<int>(0);
        item.rsId               = rs.get<db_id>(1);

        int number              = rs.get<int>(2);
        int modelNameLen        = sqlite3_column_bytes(q.stmt(), 3);
        const char *modelName   = reinterpret_cast<char const *>(sqlite3_column_text(q.stmt(), 3));

        int modelSuffixLen      = sqlite3_column_bytes(q.stmt(), 4);
        const char *modelSuffix = reinterpret_cast<char const *>(sqlite3_column_text(q.stmt(), 4));
        RsType type             = RsType(rs.get<int>(5));
        RsEngineSubType subType = RsEngineSubType(rs.get<int>(6));

        item.rsName     = rs_utils::formatNameRef(modelName, modelNameLen, number, modelSuffix,
                                                  modelSuffixLen, type);

        item.engineType = RsEngineSubType::Invalid;

        item.time       = rs.get<QTime>(7);
        item.jobId      = rs.get<db_id>(8);
        item.jobCat     = JobCategory(rs.get<int>(9));

        switch (type)
        {
        case RsType::Engine:
        {
            item.engineType = subType;
            engines.append(item);
            break;
        }
        case RsType::FreightWagon:
        {
            freight.append(item);
            break;
        }
        case RsType::Coach:
        {
            coaches.append(item);
            break;
        }
        default:
            break;
        }
    }

    engModel->loadData(engines);
    freightModel->loadData(freight);
    coachModel->loadData(coaches);
}

void RSCoupleDialog::done(int ret)
{
    // Save legend state
    AppSettings.setShowCouplingLegend(m_showLegend);

    QDialog::done(ret);
}

void RSCoupleDialog::toggleLegend()
{
    setLegendVisible(!m_showLegend);
}

void RSCoupleDialog::updateButText()
{
    showHideLegendBut->setText(m_showLegend ? tr("Hide legend") : tr("Show legend"));
}

void RSCoupleDialog::setLegendVisible(bool val)
{
    m_showLegend = val;

    if (legend->isVisible() && !m_showLegend)
    {
        legend->hide();
    }
    else if (m_showLegend && !legend->isVisible())
    {
        legend->show();
        if (!legend->layout())
        {
            double fontPt          = font().pointSizeF() * 1.2;

            QVBoxLayout *legendLay = new QVBoxLayout(legend);
            QLabel *label          = new QLabel(
              tr("<style>\n"
                                   "table, td {"
                                   "border: 1px solid black;"
                                   "border-collapse:collapse;"
                                   "padding:5px;"
                                   " }"
                                   "</style>"
                                   "<table style=\"font-size:%1pt;padding:10pt\"><tr>"
                                   "<td><span style=\"background-color:#FFFFFF\">___</span> This item is free in "
                                   "current station.</td>"
                                   "<td><span style=\"background-color:#FF3d43\">___</span> The item isn't in this "
                                   "station.</td>"
                                   "</tr><tr>"
                                   "<td><span style=\"background-color:#00FFFF\">___</span> First use of this "
                                   "item.</td>"
                                   "<td><span style=\"background-color:#FF56FF\">___</span> The item isn't coupled "
                                   "before or already coupled.</td>"
                                   "</tr><tr>"
                                   "<td><span style=\"background-color:#00FF00\">___</span> This item is never used "
                                   "in this session.</td>"
                                   "<td><span style=\"color:#0000FF;background-color:#FFFFFF\">\\\\\\\\</span> "
                                   "Railway line doesn't allow electric traction.</td>"
                                   "</tr></table>")
                .arg(fontPt));
            label->setTextFormat(Qt::RichText);
            label->setWordWrap(true);
            legendLay->addWidget(label);
            legendLay->setContentsMargins(QMargins());
            legend->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        }
    }
    updateButText();
    adjustSize();
}
