#ifndef JOBPATHEDITOR_H
#define JOBPATHEDITOR_H

#include <QDialog>

#include <QSet>

#include <QMenu>

#include "utils/types.h"

class StopDelegate;
class CustomCompletionLineEdit;

class StopModel;

namespace Ui {
class JobPathEditor;
}

class JobPathEditor : public QDialog
{
    Q_OBJECT

public:
    explicit JobPathEditor(QWidget *parent = nullptr);
    ~JobPathEditor()override;

    void prepareQueries();
    void finalizeQueries();

    bool setJob(db_id jobId);
    bool createNewJob(db_id *out = nullptr);

    bool clearJob();

    bool saveChanges();

    void discardChanges();

    db_id currentJobId() const;

    bool isEdited() const;

    bool maybeSave();

    void toggleTransit(const QModelIndex &index);
    void closeStopEditor();
    void setReadOnly(bool readOnly);

    bool getCanSetJob() const;

    void selectStop(db_id stopId);

signals:
    void stationChange(db_id stId);

public slots:
    void done(int) override;

    void onShiftError();
    void onSaveSheet();

    void updateSpinColor();

protected:
    virtual void closeEvent(QCloseEvent *e) override;

private slots:
    void setEdited(bool val);

    void showContextMenu(const QPoint &pos);
    void onIndexClicked(const QModelIndex &index);
    void onJobRemoved(db_id jobId);

    void onIdSpinValueChanged(int jobId);
    void onJobIdChanged(db_id jobId);

    void onCategoryChanged(int newCat);

    void onJobShiftChanged(db_id shiftId);

private:
    void setSpinColor(const QColor &col);

    bool setJob_internal(db_id jobId);

private:
    Ui::JobPathEditor *ui;

    CustomCompletionLineEdit *shiftCustomCombo;

    StopModel *stopModel;
    StopDelegate *delegate;

    //TODO: there are too many bools
    bool isClear;

    bool canSetJob; //TODO: better name
    bool m_readOnly;
};

#endif // JOBPATHEDITOR_H
