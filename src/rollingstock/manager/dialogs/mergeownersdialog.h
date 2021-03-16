#ifndef MERGEOWNERSDIALOG_H
#define MERGEOWNERSDIALOG_H

#include <QDialog>

#include "utils/types.h"

class CustomCompletionLineEdit;
class RSOwnersMatchModel;
class QLabel;

namespace sqlite3pp {
class database;
}

namespace Ui {
class MergeOwnersDialog;
}

class MergeOwnersDialog : public QDialog
{
    Q_OBJECT

public:
    MergeOwnersDialog(sqlite3pp::database &db, QWidget *parent = nullptr);
    ~MergeOwnersDialog();

protected:
    void done(int r) override;

private slots:
    void resetModel();

private:
    bool mergeOwners(db_id sourceOwnerId, db_id destOwnerId, bool removeSource);

private:
    Ui::MergeOwnersDialog *ui;

    CustomCompletionLineEdit *sourceOwnerEdit;
    CustomCompletionLineEdit *destOwnerEdit;

    RSOwnersMatchModel *model;

    sqlite3pp::database &mDb;
};

#endif // MERGEOWNERSDIALOG_H
