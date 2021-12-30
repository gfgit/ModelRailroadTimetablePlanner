#ifndef CUSTOMLINEEDIT_H
#define CUSTOMLINEEDIT_H

#include <QLineEdit>

#include "utils/types.h"

class QTreeView;
class QModelIndex;
class ISqlFKMatchModel;

class CustomCompletionLineEdit : public QLineEdit
{
    Q_OBJECT
public:
    CustomCompletionLineEdit(ISqlFKMatchModel *m, QWidget *parent = nullptr);
    ~CustomCompletionLineEdit();

    void showPopup();

    bool getData(db_id &idOut, QString &nameOut) const;

    void setData(db_id id, const QString& name = QString());

    void setModel(ISqlFKMatchModel *m);

    void resizeColumnToContents();
    void selectFirstIndexOrNone(bool forceFirst);

signals:
    void completionDone(CustomCompletionLineEdit *self);
    void dataIdChanged(db_id id);
    void indexSelected(const QModelIndex& idx);

public slots:
    void setData_slot(db_id id);

protected:
    void timerEvent(QTimerEvent *event);
    void mousePressEvent(QMouseEvent *e);
    bool eventFilter(QObject *obj, QEvent *ev);

private slots:
    void doneCompletion(const QModelIndex &idx);
    void startSuggestionsTimer();
    void stopSuggestionsTimer();
    void resultsReady(bool forceFirst);

private:
    QTreeView *popup;
    ISqlFKMatchModel *model;
    db_id dataId;
    int suggestionsTimerId;
};

#endif // CUSTOMLINEEDIT_H
