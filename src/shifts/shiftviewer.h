#ifndef SHIFTVIEWER_H
#define SHIFTVIEWER_H

#include <QWidget>

#include "utils/types.h"

#include <sqlite3pp/sqlite3pp.h>

using namespace sqlite3pp;

class QTableView;
class ShiftJobsModel;

class ShiftViewer : public QWidget
{
    Q_OBJECT
public:
    ShiftViewer(QWidget *parent = nullptr);
    virtual ~ShiftViewer();

    void updateName();
    void updateJobsModel();
    void setShift(db_id id);

private slots:
    void showContextMenu(const QPoint &pos);

private:
    db_id shiftId;

    query q_shiftName;

    ShiftJobsModel *model;
    QTableView *view;
};

#endif // SHIFTVIEWER_H
