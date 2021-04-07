#ifndef RAILWAYNODEEDITOR_H
#define RAILWAYNODEEDITOR_H

#include <QDialog>

#include "utils/types.h"
#include "railwaynodemode.h"

class QToolBar;
class QTableView;
class RailwayNodeModel;
class SqlFKFieldDelegate;
class StationOrLineMatchFactory;
class ComboDelegate;

namespace sqlite3pp {
class database;
}

//TODO: deprecated remove
class RailwayNodeEditor : public QDialog
{
    Q_OBJECT
public:
    RailwayNodeEditor(sqlite3pp::database &db, QWidget *parent = nullptr);

    void setMode(const QString &name, db_id id, RailwayNodeMode mode);

public slots:
    void addItem();
    void removeCurrentItem();

private:
    QToolBar *toolBar;
    QTableView *view;
    RailwayNodeModel *model;
    SqlFKFieldDelegate *lineOrStationDelegate;
    StationOrLineMatchFactory *factory;
    ComboDelegate *directionDelegate;
};

#endif // RAILWAYNODEEDITOR_H
