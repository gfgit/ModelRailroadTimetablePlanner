#include "railwaynodeeditor.h"

#include "railwaynodemodel.h"

#include "utils/combodelegate.h"
#include "kmdelegate.h"

#include "utils/sqldelegate/isqlfkmatchmodel.h"
#include "utils/sqldelegate/sqlfkfielddelegate.h"
#include "stationorlinematchfactory.h"

#include "utils/model_roles.h"

#include <QVBoxLayout>
#include <QToolBar>
#include <QTableView>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QHeaderView>

#include "utils/sqldelegate/modelpageswitcher.h"

#include "utils/sqldelegate/chooseitemdlg.h"

RailwayNodeEditor::RailwayNodeEditor(sqlite3pp::database &db, QWidget *parent) :
    QDialog(parent)
{
    QVBoxLayout *l = new QVBoxLayout(this);
    toolBar = new QToolBar(this);
    l->addWidget(toolBar);

    toolBar->addAction(tr("Add"), this, &RailwayNodeEditor::addItem);
    toolBar->addAction(tr("Remove"), this, &RailwayNodeEditor::removeCurrentItem);

    view = new QTableView(this);
    l->addWidget(view);

    auto ps = new ModelPageSwitcher(false, this);
    l->addWidget(ps);

    model = new RailwayNodeModel(db, this);
    view->setModel(model);
    ps->setModel(model);

    //Custom colun sorting
    //NOTE: leave disconnect() in the old SIGLAL()/SLOT() version in order to work
    QHeaderView *header = view->horizontalHeader();
    disconnect(header, SIGNAL(sectionPressed(int)), view, SLOT(selectColumn(int)));
    disconnect(header, SIGNAL(sectionEntered(int)), view, SLOT(_q_selectColumn(int)));
    connect(header, &QHeaderView::sectionClicked, this, [this, header](int section)
    {
        model->setSortingColumn(section);
        header->setSortIndicator(model->getSortingColumn(), Qt::AscendingOrder);
    });
    header->setSortIndicatorShown(true);
    header->setSortIndicator(model->getSortingColumn(), Qt::AscendingOrder);

    factory = new StationOrLineMatchFactory(db, this);
    lineOrStationDelegate = new SqlFKFieldDelegate(factory, model, this);
    view->setItemDelegateForColumn(RailwayNodeModel::LineOrStationCol, lineOrStationDelegate);

    KmDelegate *kmDelegate = new KmDelegate(this);
    view->setItemDelegateForColumn(RailwayNodeModel::KmCol, kmDelegate);

    QStringList directionNames;
    const int size = sizeof (DirectionNamesTable)/sizeof (DirectionNamesTable[0]);
    directionNames.reserve(size);
    for(int i = 0; i < size; i++)
    {
        directionNames.append(DirectionNames::tr(DirectionNamesTable[i]));
    }
    directionDelegate = new ComboDelegate(directionNames, DIRECTION_ROLE, this);
    view->setItemDelegateForColumn(RailwayNodeModel::DirectionCol, directionDelegate);

    QDialogButtonBox *box = new QDialogButtonBox(QDialogButtonBox::Ok, this);
    connect(box, &QDialogButtonBox::accepted, this, &QDialog::accept);
    l->addWidget(box);

    setMinimumSize(500, 300);
}

void RailwayNodeEditor::setMode(const QString& name, db_id id, RailwayNodeMode mode)
{
    factory->setMode(mode);
    model->setMode(id, mode);
    setWindowTitle(name);
}

void RailwayNodeEditor::addItem()
{
    ISqlFKMatchModel *matchModel = factory->createModel();
    ChooseItemDlg dlg(matchModel, this);
    matchModel->setParent(&dlg);
    if(model->getMode() == RailwayNodeMode::StationLinesMode)
    {
        dlg.setDescription(tr("Please choose a line for the new entry"));
        dlg.setPlaceholder(tr("Line"));
    }else{
        dlg.setDescription(tr("Please choose a station for the new entry"));
        dlg.setPlaceholder(tr("Station"));
    }
    dlg.setCallback([this](db_id itemId, QString &errMsg) -> bool
    {
        if(!itemId)
        {
            if(model->getMode() == RailwayNodeMode::StationLinesMode)
                errMsg = tr("You must select a valid line.");
            else
                errMsg = tr("You must select a valid station.");
            return false;
        }

        if(!model->addItem(itemId, nullptr))
        {
            if(model->getMode() == RailwayNodeMode::StationLinesMode)
                errMsg = tr("You cannot add the same line twice to the same station.");
            else
                errMsg = tr("You cannot add the same station twice to the same line.");
            return false;
        }
        return true;
    });

    if(dlg.exec() == QDialog::Accepted)
    {
        //TODO: select and edit the new item
    }
}

void RailwayNodeEditor::removeCurrentItem()
{
    if(!view->selectionModel()->hasSelection())
        return;
    int row = view->currentIndex().row();
    model->removeItem(row);
}
