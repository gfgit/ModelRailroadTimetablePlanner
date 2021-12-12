#ifndef NEWJOBSAMEPATHDLG_H
#define NEWJOBSAMEPATHDLG_H

#include <QDialog>

#include "utils/types.h"
#include <QTime>

class QLabel;
class QCheckBox;
class QTimeEdit;

class NewJobSamePathDlg : public QDialog
{
    Q_OBJECT
public:
    explicit NewJobSamePathDlg(QWidget *parent = nullptr);

    void setSourceJob(db_id jobId, JobCategory cat, const QTime& start, const QTime& end);

    QTime getNewStartTime() const;
    bool shouldCopyRs() const;
    bool shouldReversePath() const;

private slots:
    void checkTimeIsValid();

private:
    QLabel *label;
    QTimeEdit *startTimeEdit;
    QCheckBox *copyRsCheck;
    QCheckBox *reversePathCheck;

    db_id sourceJobId = 0;
    JobCategory sourceJobCat = JobCategory::NCategories;
    QTime sourceStart;
    QTime sourceEnd;
};

#endif // NEWJOBSAMEPATHDLG_H
