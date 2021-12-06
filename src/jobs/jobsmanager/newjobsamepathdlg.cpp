#include "newjobsamepathdlg.h"

#include "utils/jobcategorystrings.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QCheckBox>
#include <QTimeEdit>
#include <QDialogButtonBox>

#include <QMessageBox>

NewJobSamePathDlg::NewJobSamePathDlg(QWidget *parent) :
    QDialog(parent)
{
    QVBoxLayout *lay = new QVBoxLayout(this);
    label = new QLabel;
    label->setTextFormat(Qt::RichText);
    lay->addWidget(label);

    startTimeEdit = new QTimeEdit;
    lay->addWidget(startTimeEdit);

    copyRsCheck = new QCheckBox(tr("Copy Rollingstock items"));
    copyRsCheck->setChecked(true); //Enabled by default
    lay->addWidget(copyRsCheck);

    reversePathCheck = new QCheckBox(tr("Reverse path"));
    reversePathCheck->setChecked(false); //Disabled by default
    lay->addWidget(reversePathCheck);

    QDialogButtonBox *box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    lay->addWidget(box);

    connect(box, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(box, &QDialogButtonBox::rejected, this, &QDialog::reject);

    connect(startTimeEdit, &QTimeEdit::timeChanged, this, &NewJobSamePathDlg::checkTimeIsValid);

    setMinimumSize(200, 100);
    setWindowTitle(tr("New Job With Same Path"));
}

void NewJobSamePathDlg::setSourceJob(db_id jobId, JobCategory cat, const QTime &start, const QTime &end)
{
    sourceJobId = jobId;
    sourceJobCat = cat;
    sourceStart = start;
    sourceEnd = end;

    label->setText(tr("Create a new job with same path of <b>%1</b>.<br>"
                      "Original job starts at <b>%2</b> and ends at <b>%3</b>.<br>"
                      "Please select below when the new job should start.")
                       .arg(JobCategoryName::jobName(sourceJobId, sourceJobCat))
                       .arg(sourceStart.toString("HH:mm"))
                       .arg(sourceEnd.toString("HH:mm")));

    //Prevent calling checkTimeIsValid()
    QSignalBlocker blk(startTimeEdit);
    startTimeEdit->setTime(sourceStart);
}

QTime NewJobSamePathDlg::getNewStartTime() const
{
    return startTimeEdit->time();
}

bool NewJobSamePathDlg::shouldCopyRs() const
{
    return copyRsCheck->isChecked();
}

bool NewJobSamePathDlg::shouldReversePath() const
{
    return reversePathCheck->isChecked();
}

void NewJobSamePathDlg::checkTimeIsValid()
{
    const QTime lastValidTime = QTime(23, 59);
    const int travelDurationMsecs = sourceStart.msecsTo(sourceEnd);

    QTime newStart = startTimeEdit->time();
    int msecsToMidnight = newStart.msecsTo(lastValidTime);
    if(travelDurationMsecs > msecsToMidnight)
    {
        //New job would end after midnigth
        QMessageBox::warning(this, tr("Invalid Start Time"),
                             tr("New job would end past midnight."));

        //Go back from midnight to get maximum start value
        newStart = lastValidTime.addMSecs(-travelDurationMsecs);

        //Prevent recursion
        QSignalBlocker blk(startTimeEdit);
        startTimeEdit->setTime(newStart);
    }
}