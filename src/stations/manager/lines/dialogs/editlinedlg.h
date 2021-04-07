#ifndef EDITLINEDLG_H
#define EDITLINEDLG_H

#include <QDialog>

#include "utils/types.h"

namespace sqlite3pp {
class database;
}

class QTableView;

class LineSegmentsModel;

class EditLineDlg : public QDialog
{
    Q_OBJECT
public:
    EditLineDlg(sqlite3pp::database &db, QWidget *parent = nullptr);

    void setLineId(db_id lineId);

private:
    LineSegmentsModel *model;

    QTableView *view;
};

#endif // EDITLINEDLG_H
