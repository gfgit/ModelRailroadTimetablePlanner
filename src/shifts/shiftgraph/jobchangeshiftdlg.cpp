#include "jobchangeshiftdlg.h"

#include <QDialogButtonBox>
#include <QLabel>
#include <QComboBox>
#include <QMessageBox>
#include <QVBoxLayout>
#include "utils/owningqpointer.h"

#include "utils/sqldelegate/customcompletionlineedit.h"
#include "shifts/shiftcombomodel.h"

#include "jobs/jobeditor/shiftbusy/shiftbusydialog.h"
#include "jobs/jobeditor/shiftbusy/shiftbusymodel.h"

#include "utils/jobcategorystrings.h"

#include <sqlite3pp/sqlite3pp.h>

#include "app/session.h"

JobChangeShiftDlg::JobChangeShiftDlg(sqlite3pp::database &db, QWidget *parent) :
    QDialog(parent),
    mJobId(0),
    originalShiftId(0),
    mDb(db)
{
    QVBoxLayout *lay = new QVBoxLayout(this);

    label = new QLabel;
    lay->addWidget(label);

    ShiftComboModel *shiftComboModel = new ShiftComboModel(db, this);
    shiftComboModel->refreshData();
    shiftCombo = new CustomCompletionLineEdit(shiftComboModel);
    lay->addWidget(shiftCombo);

    QDialogButtonBox *box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    lay->addWidget(box);

    connect(box, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(box, &QDialogButtonBox::accepted, this, &QDialog::accept);

    connect(Session, &MeetingSession::jobChanged, this, &JobChangeShiftDlg::onJobChanged);

    setMinimumSize(200, 100);
}

void JobChangeShiftDlg::done(int ret)
{
    if(ret == QDialog::Accepted)
    {
        db_id shiftId = 0;
        QString shiftName;
        shiftCombo->getData(shiftId, shiftName);

        if(shiftId != originalShiftId)
        {
            if(shiftId)
            {
                //Setting a new shift, check if we overlap with other jobs
                sqlite3pp::query q(mDb);
                q.prepare("SELECT MIN(departure) FROM stops WHERE job_id=?");
                q.bind(1, mJobId);
                q.step();
                QTime start = q.getRows().get<QTime>(0);
                q.reset();

                q.prepare("SELECT MAX(arrival) FROM stops WHERE job_id=?");
                q.bind(1, mJobId);
                q.step();
                QTime end = q.getRows().get<QTime>(0);
                q.reset();

                ShiftBusyModel model(mDb);
                model.loadData(shiftId, mJobId, start, end);
                if(model.hasConcurrentJobs())
                {
                    OwningQPointer<ShiftBusyDlg> dlg = new ShiftBusyDlg(this);
                    dlg->setModel(&model);
                    dlg->exec();
                    return;
                }
            }

            sqlite3pp::command cmd(mDb, "UPDATE jobs SET shift_id=? WHERE id=?");
            if(shiftId)
                cmd.bind(1, shiftId);
            else
                cmd.bind(1); //Bind NULL
            cmd.bind(2, mJobId);
            ret = cmd.execute();
            if(ret != SQLITE_OK)
            {
                //Error
                QMessageBox::warning(this, tr("Shift Error"),
                                     tr("Error while setting shift <b>%1</b> to job <b>%2</b>.<br>"
                                        "Msg: %3")
                                         .arg(shiftName)
                                         .arg(JobCategoryName::jobName(mJobId, mCategory))
                                         .arg(mDb.error_msg()));
                return;
            }

            emit Session->shiftJobsChanged(originalShiftId, mJobId);
            emit Session->shiftJobsChanged(shiftId, mJobId);
        }
    }

    QDialog::done(ret);
}

void JobChangeShiftDlg::onJobChanged(db_id jobId, db_id oldJobId)
{
    //Update id/category
    if(mJobId == oldJobId)
        setJob(jobId);
}

void JobChangeShiftDlg::setJob(db_id jobId)
{
    query q_getJobInfo(mDb, "SELECT shift_id,category FROM jobs WHERE id=?");
    q_getJobInfo.bind(1, jobId);
    if(q_getJobInfo.step() != SQLITE_ROW)
        return;
    auto j = q_getJobInfo.getRows();
    setJob(jobId, j.get<db_id>(0), JobCategory(j.get<int>(1)));
}

void JobChangeShiftDlg::setJob(db_id jobId, db_id shiftId, JobCategory jobCat)
{
    mJobId = jobId;
    originalShiftId = shiftId;
    mCategory = jobCat;

    QString jobName = JobCategoryName::jobName(mJobId, mCategory);

    label->setText(tr("Change shift for job <b>%1</b>:").arg(jobName));
    shiftCombo->setData(shiftId);
    setWindowTitle(jobName);
}
