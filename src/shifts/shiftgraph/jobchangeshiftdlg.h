#ifndef JOBCHANGESHIFTDLG_H
#define JOBCHANGESHIFTDLG_H

#include <QDialog>

#include "utils/types.h"

namespace sqlite3pp {
class database;
}

class CustomCompletionLineEdit;
class QLabel;

class JobChangeShiftDlg : public QDialog
{
    Q_OBJECT
public:
    JobChangeShiftDlg(sqlite3pp::database &db, QWidget *parent = nullptr);

    void setJob(db_id jobId);
    void setJob(db_id jobId, db_id shiftId, JobCategory jobCat);

public slots:
    virtual void done(int ret) override;

private slots:
    void onJobChanged(db_id jobId, db_id oldJobId);

private:
    db_id mJobId;
    db_id originalShiftId;
    JobCategory mCategory;

    CustomCompletionLineEdit *shiftCombo;
    QLabel *label;

    sqlite3pp::database &mDb;
};

#endif // JOBCHANGESHIFTDLG_H
