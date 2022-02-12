#ifndef STATIONJOBVIEW_H
#define STATIONJOBVIEW_H

#include <QWidget>

#include "utils/types.h"

namespace Ui {
class StationJobView;
}

class StationPlanModel;

class StationJobView : public QWidget
{
    Q_OBJECT

public:
    explicit StationJobView(QWidget *parent = nullptr);
    ~StationJobView();

    void setStation(db_id stId);

    void updateJobsList();
    void updateName();

private slots:
    void showContextMenu(const QPoint &pos);
    void onSaveSheet();
    void updateInfo();

private:
    Ui::StationJobView *ui;

    StationPlanModel *model;

    db_id m_stationId;

    QPixmap mGraph;
};

#endif // STATIONJOBVIEW_H
