#ifndef EDITLINEDLG_H
#define EDITLINEDLG_H

#include <QDialog>

#include "utils/types.h"

namespace sqlite3pp {
class database;
}

class QTableView;
class QLineEdit;
class KmSpinBox;

class LineSegmentsModel;

class EditLineDlg : public QDialog
{
    Q_OBJECT
public:
    EditLineDlg(sqlite3pp::database &db, QWidget *parent = nullptr);

    virtual void done(int res) override;

    void setLineId(db_id lineId);

private slots:
    void onModelError(const QString& msg);
    void addStation();
    void removeAfterCurrentPos();

private:
    sqlite3pp::database& mDb;
    LineSegmentsModel *model;

    QTableView *view;
    QLineEdit *lineNameEdit;
    KmSpinBox *lineStartKmSpin;
};

#endif // EDITLINEDLG_H
