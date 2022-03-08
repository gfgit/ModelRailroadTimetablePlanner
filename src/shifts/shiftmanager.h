#ifndef SHIFTMANAGER_H
#define SHIFTMANAGER_H

#include <QWidget>

#include "utils/types.h"

#include <sqlite3pp/sqlite3pp.h>

using namespace sqlite3pp;

class QToolBar;
class QTableView;
class ShiftsModel;

class QActionGroup;

class ShiftManager : public QWidget
{
    Q_OBJECT
public:
    explicit ShiftManager(QWidget *parent = nullptr);

    void setReadOnly(bool readOnly = true);

protected:
    void showEvent(QShowEvent *e) override;

private slots:
    void onNewShift();
    void onRemoveShift();
    void onViewShift();
    void onSaveSheet();
    void displayGraph();

    void onShiftSelectionChanged();

    void onModelError(const QString& msg);

private:
    QToolBar *toolBar;
    QTableView *view;

    ShiftsModel *model;

    QAction *act_New;
    QAction *act_Remove;
    QAction *act_displayShift;
    QAction *act_Sheet;
    QAction *act_Graph;

    QActionGroup *actionGroup;

    bool m_readOnly;
    bool edited;
};

#endif // SHIFTMANAGER_H
