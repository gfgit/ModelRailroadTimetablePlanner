#include "rsjobviewer.h"

#include <QTimeEdit>
#include <QTableView>
#include <QPushButton>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QLabel>

#include "app/session.h"
#include "app/scopedebug.h"

#include "utils/rs_utils.h"
#include "utils/rs_types_names.h"

#include "rsplanmodel.h"

#include "viewmanager/viewmanager.h"

#include "utils/owningqpointer.h"
#include <QMenu>

RSJobViewer::RSJobViewer(QWidget *parent) :
    QWidget(parent)
{
    QVBoxLayout *lay = new QVBoxLayout(this);

    infoLabel = new QLabel(this);
    lay->addWidget(infoLabel);

    view = new QTableView(this);
    model = new RsPlanModel(Session->m_Db, this);
    view->setModel(model);

    view->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(view, &QTableView::customContextMenuRequested, this, &RSJobViewer::showContextMenu);

    QPushButton *updateBut = new QPushButton(tr("Update"), this);
    lay->addWidget(updateBut);
    lay->addWidget(view);

    connect(updateBut, &QPushButton::clicked, this, &RSJobViewer::updateInfo);

    setMinimumSize(350, 400);
}

RSJobViewer::~RSJobViewer()
{

}

void RSJobViewer::setRS(db_id id)
{
    m_rsId = id;
}

void RSJobViewer::updatePlan()
{
    //TODO: clear data 5 seconds after window is hidden
    model->loadPlan(m_rsId);
    view->resizeColumnsToContents();
}

void RSJobViewer::showContextMenu(const QPoint &pos)
{
    QModelIndex idx = view->indexAt(pos);
    if(!idx.isValid())
        return;

    RsPlanItem item = model->getItem(idx.row());

    OwningQPointer<QMenu> menu = new QMenu(this);

    QAction *showInJobEditor = new QAction(tr("Show in Job Editor"), menu);

    menu->addAction(showInJobEditor);

    QAction *act = menu->exec(view->viewport()->mapToGlobal(pos));
    if(act == showInJobEditor)
    {
        Session->getViewManager()->requestJobEditor(item.jobId, item.stopId);
    }
}

void RSJobViewer::updateRsInfo()
{
    query q_getRSInfo(Session->m_Db, "SELECT rs_list.number,rs_models.name,rs_models.suffix,rs_models.type,rs_owners.name"
                                     " FROM rs_list"
                                     " LEFT JOIN rs_models ON rs_models.id=rs_list.model_id"
                                     " LEFT JOIN rs_owners ON rs_owners.id=rs_list.owner_id"
                                     " WHERE rs_list.id=?");
    q_getRSInfo.bind(1, m_rsId);
    int ret = q_getRSInfo.step();
    if(ret != SQLITE_ROW)
    {
        //Error: RS does not exist!
        close();
        return;
    }

    auto rs = q_getRSInfo.getRows();
    sqlite3_stmt *stmt = q_getRSInfo.stmt();
    int number = rs.get<int>(0);
    int modelNameLen = sqlite3_column_bytes(stmt, 1);
    const char *modelName = reinterpret_cast<char const*>(sqlite3_column_text(stmt, 1));

    int modelSuffixLen = sqlite3_column_bytes(stmt, 2);
    const char *modelSuffix = reinterpret_cast<char const*>(sqlite3_column_text(stmt, 2));
    RsType type = RsType(rs.get<int>(3));

    QString owner = rs.get<QString>(4);

    const QString name = rs_utils::formatNameRef(modelName, modelNameLen,
                                                 number,
                                                 modelSuffix, modelSuffixLen,
                                                 type);
    setWindowTitle(name);

    const QString info = tr("Type:  <b>%1</b><br>"
                            "Owner: <b>%2</b>").arg(RsTypeNames::name(type));

    infoLabel->setText(owner.isEmpty() ? info.arg(tr("Not set!")) : info.arg(owner));
}

void RSJobViewer::updateInfo()
{
    updateRsInfo();
    updatePlan();
}
