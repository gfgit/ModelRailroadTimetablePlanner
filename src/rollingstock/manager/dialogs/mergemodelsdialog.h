#ifndef MODELMERGEDIALOG_H
#define MODELMERGEDIALOG_H

#include <QDialog>

#include "utils/types.h"

#include <sqlite3pp/sqlite3pp.h>

class CustomCompletionLineEdit;
class RSModelsMatchModel;
class QLabel;

namespace Ui {
class MergeModelsDialog;
}

class MergeModelsDialog : public QDialog
{
    Q_OBJECT

public:
    MergeModelsDialog(sqlite3pp::database &db, QWidget *parent = nullptr);
    ~MergeModelsDialog();

protected:
    void done(int r) override;

private slots:
    void sourceModelChanged(db_id modelId);
    void destModelChanged(db_id modelId);

private:
    void fillModelInfo(QLabel *label, db_id modelId);

private:
    int mergeModels(db_id sourceModelId, db_id destModelId, bool removeSource);

private:
    Ui::MergeModelsDialog *ui;

    CustomCompletionLineEdit *sourceModelEdit;
    CustomCompletionLineEdit *destModelEdit;

    RSModelsMatchModel *model;

    sqlite3pp::database &mDb;
    sqlite3pp::query q_getModelInfo;
};

#endif // MODELMERGEDIALOG_H
