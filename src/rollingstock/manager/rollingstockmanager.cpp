#include "rollingstockmanager.h"

#include "app/session.h"

#include "utils/rs_types_names.h"

#include <QToolBar>
#include <QTableView>
#include <QAction>
#include <QVBoxLayout>
#include <QActionGroup>
#include <QMenu>

#include <QWindow>

#include "viewmanager/viewmanager.h"

#include "utils/delegates/sql/filterheaderview.h"
#include "utils/delegates/sql/modelpageswitcher.h"

#include "../rollingstocksqlmodel.h"
#include "../rsmodelssqlmodel.h"
#include "../rsownerssqlmodel.h"

#include "utils/delegates/sql/sqlfkfielddelegate.h"
#include "utils/delegates/sql/chooseitemdlg.h"
#include "utils/delegates/sql/isqlfkmatchmodel.h"
#include "../rsmatchmodelfactory.h"

#include "utils/delegates/kmspinbox/spinboxeditorfactory.h"
#include "delegates/rstypedelegate.h"
#include "delegates/rsnumberdelegate.h"

#include "utils/owningqpointer.h"

#include <QInputDialog>
#include <QMessageBox>

#include "../importer/rsimportwizard.h"

#include "dialogs/mergemodelsdialog.h"
#include "dialogs/mergeownersdialog.h"

#include <QDebug>

RollingStockManager::RollingStockManager(QWidget *parent) :
    QWidget(parent),
    oldCurrentTab(RollingstockTab),
    clearModelTimers{0, 0, 0},
    m_readOnly(false),
    windowConnected(false)
{
    QVBoxLayout *lay = new QVBoxLayout(this);
    lay->setContentsMargins(2, 2, 2, 2);
    tabWidget = new QTabWidget;
    lay->addWidget(tabWidget);
    setupPages();
    setReadOnly(false);

    tabWidget->setCurrentIndex(RollingstockTab);
    connect(tabWidget, &QTabWidget::currentChanged, this, &RollingStockManager::updateModels);

    setWindowTitle(tr("Rollingstock Manager"));
    resize(700, 500);
}

void RollingStockManager::setupPages()
{
    //RollingStock Page
    QWidget *rollingstockTab = new QWidget;
    tabWidget->addTab(rollingstockTab, RsTypeNames::tr("Rollingstock"));
    QVBoxLayout *rsLay = new QVBoxLayout(rollingstockTab);
    rsLay->setContentsMargins(2, 2, 2, 2);

    rsToolBar = new QToolBar;
    rsLay->addWidget(rsToolBar);

    actNewRs      = rsToolBar->addAction(tr("New Rollingstock"),
                                            this, &RollingStockManager::onNewRs);
    actDeleteRs   = rsToolBar->addAction(tr("Remove"),
                                            this, &RollingStockManager::onRemoveRs);
    rsToolBar->addSeparator();

    actViewRSPlan = rsToolBar->addAction(tr("View Plan"),
                                            this, &RollingStockManager::onViewRSPlan);
    QMenu *actionRsPlanMenu = new QMenu;
    actViewRSPlanSearch = actionRsPlanMenu->addAction(tr("Search rollingstock item"),
                                                         this, &RollingStockManager::onViewRSPlanSearch);
    actViewRSPlan->setMenu(actionRsPlanMenu);
    rsToolBar->addSeparator();

    actDeleteAllRs = rsToolBar->addAction(tr("Delete All Rollingstock"),
                                             this, &RollingStockManager::onRemoveAllRs);
    rsToolBar->addSeparator();

    rsToolBar->addAction(tr("Import"), this, &RollingStockManager::onImportRS);
    rsToolBar->addAction(tr("Session Summary"), this, &RollingStockManager::showSessionRSViewer);

    rsView = new QTableView;
    rsView->setSelectionMode(QTableView::SingleSelection);
    rsLay->addWidget(rsView);

    FilterHeaderView *header = new FilterHeaderView(rsView);
    header->installOnTable(rsView);

    rsSQLModel = new RollingstockSQLModel(Session->m_Db, this);
    rsView->setModel(rsSQLModel);

    auto ps = new ModelPageSwitcher(false, this);
    rsLay->addWidget(ps);
    ps->setModel(rsSQLModel);

    connect(rsView->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &RollingStockManager::onRollingstockSelectionChanged);
    connect(rsSQLModel, &QAbstractItemModel::modelReset,
            this, &RollingStockManager::onRollingstockSelectionChanged);
    connect(rsSQLModel, &RollingstockSQLModel::modelError,
            this, &RollingStockManager::onModelError);

    //Models Page
    QWidget *modelsTab = new QWidget;
    tabWidget->addTab(modelsTab, RsTypeNames::tr("Models"));
    QVBoxLayout *modelsLay = new QVBoxLayout(modelsTab);
    modelsLay->setContentsMargins(2, 2, 2, 2);

    modelToolBar = new QToolBar;
    modelsLay->addWidget(modelToolBar);

    actNewModel    = modelToolBar->addAction(tr("New Model"),
                                                this, &RollingStockManager::onNewRsModel);
    actDeleteModel = modelToolBar->addAction(tr("Remove"),
                                                this, &RollingStockManager::onRemoveRsModel);
    actMergeModels = modelToolBar->addAction(tr("Merge Models"),
                                                this, &RollingStockManager::onMergeModels);
    actNewModelWithSuffix = modelToolBar->addAction(tr("New with suffix"),
                                                       this, &RollingStockManager::onNewRsModelWithDifferentSuffixFromCurrent);
    QMenu *actionModelSuffixMenu = new QMenu;
    actNewModelWithSuffixSearch = actionModelSuffixMenu->addAction(tr("Search rollingstock model"),
                                                                      this, &RollingStockManager::onNewRsModelWithDifferentSuffixFromSearch);
    actNewModelWithSuffix->setMenu(actionModelSuffixMenu);

    modelToolBar->addSeparator();
    actDeleteAllRsModels = modelToolBar->addAction(tr("Delete All Models"),
                                                      this, &RollingStockManager::onRemoveAllRsModels);

    rsModelsView = new QTableView;
    rsModelsView->setSelectionMode(QTableView::SingleSelection);
    modelsLay->addWidget(rsModelsView);

    header = new FilterHeaderView(rsModelsView);
    header->installOnTable(rsModelsView);

    modelsSQLModel = new RSModelsSQLModel(Session->m_Db, this);
    rsModelsView->setModel(modelsSQLModel);

    ps = new ModelPageSwitcher(false, this);
    modelsLay->addWidget(ps);
    ps->setModel(modelsSQLModel);

    connect(rsModelsView->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &RollingStockManager::onRsModelSelectionChanged);
    connect(modelsSQLModel, &QAbstractItemModel::modelReset,
            this, &RollingStockManager::onRsModelSelectionChanged);
    connect(modelsSQLModel, &RSModelsSQLModel::modelError,
            this, &RollingStockManager::onModelError);

    //Owners Page
    QWidget *ownersTab = new QWidget;
    tabWidget->addTab(ownersTab, RsTypeNames::tr("Owners"));
    QVBoxLayout *ownersLay = new QVBoxLayout(ownersTab);
    ownersLay->setContentsMargins(2, 2, 2, 2);

    ownersToolBar = new QToolBar;
    ownersLay->addWidget(ownersToolBar);

    actNewOwner    = ownersToolBar->addAction(tr("New Owner"), this,
                                                 &RollingStockManager::onNewOwner);
    actDeleteOwner = ownersToolBar->addAction(tr("Remove"), this,
                                                 &RollingStockManager::onRemoveOwner);
    actMergeOwners = ownersToolBar->addAction(tr("Merge Owners"), this,
                                                 &RollingStockManager::onMergeOwners);
    ownersToolBar->addSeparator();
    actDeleteAllRsOwners = ownersToolBar->addAction(tr("Delete All Owners"),
                                                       this, &RollingStockManager::onRemoveAllRsOwners);

    ownersView = new QTableView;
    ownersView->setSelectionMode(QTableView::SingleSelection);
    ownersLay->addWidget(ownersView);

    header = new FilterHeaderView(ownersView);
    header->installOnTable(ownersView);

    ownersSQLModel = new RSOwnersSQLModel(Session->m_Db, this);
    ownersView->setModel(ownersSQLModel);

    ps = new ModelPageSwitcher(false, this);
    ownersLay->addWidget(ps);
    ps->setModel(ownersSQLModel);

    connect(ownersView->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &RollingStockManager::onRsOwnerSelectionChanged);
    connect(ownersSQLModel, &QAbstractItemModel::modelReset,
            this, &RollingStockManager::onRsOwnerSelectionChanged);
    connect(ownersSQLModel, &RSOwnersSQLModel::modelError,
            this, &RollingStockManager::onModelError);

    //Setup Delegates
    //auto modelDelegate = new RSModelDelegate(modelsModel, this);
    auto modelDelegate = new SqlFKFieldDelegate(new RSMatchModelFactory(ModelModes::Models, Session->m_Db, this), rsSQLModel, this);
    rsView->setItemDelegateForColumn(RollingstockSQLModel::Model, modelDelegate);

    //auto ownerDelegate = new RSOwnerDelegate(ownersModel, this);
    auto ownerDelegate = new SqlFKFieldDelegate(new RSMatchModelFactory(ModelModes::Owners, Session->m_Db, this), rsSQLModel, this);
    rsView->setItemDelegateForColumn(RollingstockSQLModel::Owner, ownerDelegate);

    auto numberDelegate = new RsNumberDelegate(this);
    rsView->setItemDelegateForColumn(RollingstockSQLModel::Number, numberDelegate);

    auto rsSpeedDelegate = new QStyledItemDelegate(this);
    speedSpinFactory = new SpinBoxEditorFactory;
    speedSpinFactory->setRange(1, 999);
    speedSpinFactory->setSuffix(" km/h");
    speedSpinFactory->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    rsSpeedDelegate->setItemEditorFactory(speedSpinFactory);
    rsModelsView->setItemDelegateForColumn(RSModelsSQLModel::MaxSpeed, rsSpeedDelegate);

    auto rsAxesDelegate = new QStyledItemDelegate(this);
    axesSpinFactory = new SpinBoxEditorFactory;
    axesSpinFactory->setRange(2, 999);
    axesSpinFactory->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    rsAxesDelegate->setItemEditorFactory(axesSpinFactory);
    rsModelsView->setItemDelegateForColumn(RSModelsSQLModel::Axes, rsAxesDelegate);

    auto rsTypeDelegate = new RSTypeDelegate(false, this);
    rsModelsView->setItemDelegateForColumn(RSModelsSQLModel::TypeCol, rsTypeDelegate);

    auto rsSubTypeDelegate = new RSTypeDelegate(true, this);
    rsModelsView->setItemDelegateForColumn(RSModelsSQLModel::SubTypeCol, rsSubTypeDelegate);

    //Setup actions
    editActGroup = new QActionGroup(this);
    editActGroup->addAction(actNewRs);
    editActGroup->addAction(actDeleteRs);
    editActGroup->addAction(actDeleteAllRs);
    editActGroup->addAction(actNewModel);
    editActGroup->addAction(actNewModelWithSuffix);
    editActGroup->addAction(actNewModelWithSuffixSearch);
    editActGroup->addAction(actDeleteModel);
    editActGroup->addAction(actDeleteAllRsModels);
    editActGroup->addAction(actMergeModels);
    editActGroup->addAction(actNewOwner);
    editActGroup->addAction(actDeleteOwner);
    editActGroup->addAction(actDeleteAllRsOwners);
    editActGroup->addAction(actMergeOwners);
}

RollingStockManager::~RollingStockManager()
{
    delete speedSpinFactory;
    delete axesSpinFactory;

    for(int i = 0; i < NTabs; i++)
    {
        if(clearModelTimers[i] > 0)
        {
            killTimer(clearModelTimers[i]);
            clearModelTimers[i] = 0;
        }
    }
}

void RollingStockManager::setReadOnly(bool readOnly)
{
    if(m_readOnly == readOnly)
        return;

    m_readOnly = readOnly;

    editActGroup->setDisabled(m_readOnly);

    if(m_readOnly)
    {
        rsView->setEditTriggers(QAbstractItemView::NoEditTriggers);
        rsModelsView->setEditTriggers(QAbstractItemView::NoEditTriggers);
        ownersView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    }
    else
    {
        rsView->setEditTriggers(QAbstractItemView::DoubleClicked);
        rsModelsView->setEditTriggers(QAbstractItemView::DoubleClicked);
        ownersView->setEditTriggers(QAbstractItemView::DoubleClicked);
    }
}

void RollingStockManager::updateModels()
{
    int curTab = tabWidget->currentIndex();

    if(clearModelTimers[curTab] > 0)
    {
        //This page was already cached, stop it from clearing
        killTimer(clearModelTimers[curTab]);
    }
    else if(clearModelTimers[curTab] == ModelCleared)
    {
        //This page wasn't already cached
        switch (curTab)
        {
        case RollingstockTab:
        {
            rsSQLModel->refreshData(true);
            break;
        }
        case ModelsTab:
        {
            modelsSQLModel->refreshData(true);
            break;
        }
        case OwnersTab:
        {
            ownersSQLModel->refreshData(true);
            break;
        }
        }
    }
    clearModelTimers[curTab] = ModelLoaded;

    //Now start timer to clear old current page if not already done
    if(oldCurrentTab != curTab && clearModelTimers[oldCurrentTab] == ModelLoaded) //Wait 10 seconds and then clear cache
    {
        clearModelTimers[oldCurrentTab] = startTimer(ClearModelTimeout, Qt::VeryCoarseTimer);
    }

    oldCurrentTab = curTab;
}

void RollingStockManager::visibilityChanged(int v)
{
    if(v == QWindow::Minimized || v == QWindow::Hidden)
    {
        //If the window is minimized start timer to clear model cache of current tab
        //The other tabs already have been cleared or are waiting with their timers
        if(clearModelTimers[oldCurrentTab] == ModelLoaded)
        {
            clearModelTimers[oldCurrentTab] = startTimer(ClearModelTimeout, Qt::VeryCoarseTimer);
        }
    }else{
        updateModels();
    }
}

void RollingStockManager::onModelError(const QString &msg)
{
    QMessageBox::warning(this, tr("Rollingstock Error"), msg);
}

void RollingStockManager::importRS(bool resume, QWidget *parent)
{
    OwningQPointer<RSImportWizard> w = new RSImportWizard(resume, parent);
    w->exec();
}

void RollingStockManager::onViewRSPlan()
{
    //TODO: use also a search if requested RS is not in current page
    QModelIndex idx = rsView->currentIndex();
    if(!idx.isValid())
        return;

    db_id rsId = rsSQLModel->getIdAtRow(idx.row());
    if(!rsId)
        return;
    Session->getViewManager()->requestRSInfo(rsId);
}

void RollingStockManager::onViewRSPlanSearch()
{
    //TODO: add search dialog also for deleting owners/models/RS items.

    RSMatchModelFactory factory(ModelModes::Rollingstock, Session->m_Db, this);
    std::unique_ptr<ISqlFKMatchModel> matchModel;
    matchModel.reset(factory.createModel());

    OwningQPointer<ChooseItemDlg> dlg = new ChooseItemDlg(matchModel.get(), this);
    dlg->setDescription(tr("Please choose a rollingstock item"));
    dlg->setPlaceholder(tr("[model][.][number][:owner]"));

    int ret = dlg->exec();

    if(ret != QDialog::Accepted || !dlg)
        return;

    Session->getViewManager()->requestRSInfo(dlg->getItemId());
}

void RollingStockManager::onNewRs()
{
    if(m_readOnly)
        return;

    QString errText;
    int row = 0;
    db_id rsId = rsSQLModel->addRSItem(&row, &errText);
    if(!rsId)
    {
        QMessageBox::warning(this, tr("Error adding rollingstock piece"), errText);
        return;
    }
}

void RollingStockManager::onRemoveRs()
{
    if(m_readOnly)
        return;

    QModelIndex idx = rsView->currentIndex();
    if(!idx.isValid())
        return;

    if(!rsSQLModel->removeRSItemAt(idx.row()))
    {
        //ERRORMSG: display error
        return;
    }
}

void RollingStockManager::onRemoveAllRs()
{
    int ret = QMessageBox::question(this, tr("Delete All Rollingstock?"),
                                    tr("Are you really sure you want to delete all rollingstock from this session?\n"
                                       "NOTE: this will not erease model and owners, just rollingstock pieces."));
    if(ret == QMessageBox::Yes)
    {
        if(!rsSQLModel->removeAllRS())
        {
            QMessageBox::warning(this, tr("Error"),
                                 tr("Failed to remove rollingstock.\n"
                                    "Make sure there are no more couplings in this session.\n"
                                    "NOTE: you can remove all jobs at once from the Jobs Manager."));
        }
    }
}

void RollingStockManager::onNewRsModel()
{
    if(m_readOnly)
        return;

    int row = 0;

    QString errText;
    db_id modelId = modelsSQLModel->addRSModel(&row, 0, QString(), &errText);
    if(!modelId)
    {
        QMessageBox::warning(this, tr("Error adding model"), errText);
        return;
    }

    QModelIndex idx = modelsSQLModel->index(row, RSModelsSQLModel::Name);

    rsModelsView->setCurrentIndex(idx);
    rsModelsView->scrollTo(idx);
    rsModelsView->edit(idx); //TODO: delay call until row is fetched!!! Like save it and wait for a signal from model
}

void RollingStockManager::onNewRsModelWithDifferentSuffixFromCurrent()
{
    if(m_readOnly)
        return;

    QModelIndex idx = rsModelsView->currentIndex();
    if(!idx.isValid())
        return;

    db_id modelId = modelsSQLModel->getModelIdAtRow(idx.row());
    if(modelId)
    {
        QString errMsg;
        if(!createRsModelWithDifferentSuffix(modelId, errMsg, this))
        {
            QMessageBox::warning(this, tr("Error"), errMsg);
        }
    }
}

void RollingStockManager::onNewRsModelWithDifferentSuffixFromSearch()
{
    if(m_readOnly)
        return;

    RSMatchModelFactory factory(ModelModes::Models, Session->m_Db, this);
    std::unique_ptr<ISqlFKMatchModel> matchModel;
    matchModel.reset(factory.createModel());

    OwningQPointer<ChooseItemDlg> dlg = new ChooseItemDlg(matchModel.get(), this);
    dlg->setDescription(tr("Please choose a rollingstock model"));
    dlg->setPlaceholder(tr("Model"));
    dlg->setCallback([this, &dlg](db_id modelId, QString &errMsg) -> bool
    {
        if(!modelId)
        {
            errMsg = tr("You must select a valid rollingstock model.");
            return false;
        }

        return createRsModelWithDifferentSuffix(modelId, errMsg, dlg);
    });

    if(dlg->exec() != QDialog::Accepted)
        return;

    //TODO: select and edit the new item
}

bool RollingStockManager::createRsModelWithDifferentSuffix(db_id sourceModelId, QString &errMsg, QWidget *w)
{
    OwningQPointer<QInputDialog> dlg = new QInputDialog(w);
    dlg->setLabelText(tr("Please choose an unique suffix for this model, or leave empty"));
    dlg->setWindowTitle(tr("Choose Suffix"));
    dlg->setInputMode(QInputDialog::TextInput);

    if(dlg->exec() != QDialog::Accepted || !dlg)
        return true; //Default: Abort without errors

    return modelsSQLModel->addRSModel(nullptr, sourceModelId, dlg->textValue(), &errMsg);
}

void RollingStockManager::onRemoveRsModel()
{
    if(m_readOnly)
        return;

    QModelIndex idx = rsModelsView->currentIndex();
    if(!idx.isValid())
        return;

    if(!modelsSQLModel->removeRSModelAt(idx.row()))
    {
        //ERRORMSG:
    }
}

void RollingStockManager::onRemoveAllRsModels()
{
    int ret = QMessageBox::question(this, tr("Delete All Rollingstock Models?"),
                                    tr("Are you really sure you want to delete all rollingstock models from this session?\n"
                                       "NOTE: this can be done only if there are no rollingstock pieces in this session."));
    if(ret == QMessageBox::Yes)
    {
        if(!modelsSQLModel->removeAllRSModels())
        {
            QMessageBox::warning(this, tr("Error"),
                                 tr("Failed to remove rollingstock models.\n"
                                    "Make sure there are no more rollingstock pieces in this session.\n"
                                    "NOTE: you can remove all rollinstock pieces at once from the Rollingstock tab."));
        }
    }
}

void RollingStockManager::onNewOwner()
{
    if(m_readOnly)
        return;

    int row = 0;
    if(!ownersSQLModel->addRSOwner(&row))
    {
        //ERRORMSG: display
        return;
    }

    QModelIndex idx = ownersSQLModel->index(row, 0);

    ownersView->setCurrentIndex(idx);
    ownersView->scrollTo(idx);
    ownersView->edit(idx);
}

void RollingStockManager::onRemoveOwner()
{
    if(m_readOnly)
        return;

    QModelIndex idx = ownersView->currentIndex();
    if(!idx.isValid())
        return;

    if(!ownersSQLModel->removeRSOwnerAt(idx.row()))
    {
        //ERRORMSG: display error
    }
}

void RollingStockManager::onRemoveAllRsOwners()
{
    int ret = QMessageBox::question(this, tr("Delete All Rollingstock Owners?"),
                                    tr("Are you really sure you want to delete all rollingstock owners from this session?\n"
                                       "NOTE: this can be done only if there are no rollingstock pieces in this session."));
    if(ret == QMessageBox::Yes)
    {
        if(!ownersSQLModel->removeAllRSOwners())
        {
            QMessageBox::warning(this, tr("Error"),
                                 tr("Failed to remove rollingstock owners.\n"
                                    "Make sure there are no more rollingstock pieces in this session.\n"
                                    "NOTE: you can remove all rollingstock pieces at once from the Rollingstock tab."));
        }
    }
}

void RollingStockManager::onMergeModels()
{
    if(m_readOnly)
        return;

    OwningQPointer<MergeModelsDialog> dlg = new MergeModelsDialog(Session->m_Db, this);
    dlg->exec();

    if(clearModelTimers[ModelsTab] == ModelLoaded)
    {
        modelsSQLModel->refreshData();
    }
    if(clearModelTimers[RollingstockTab] == ModelLoaded)
    {
        rsSQLModel->refreshData(true);
    }
}

void RollingStockManager::onMergeOwners()
{
    if(m_readOnly)
        return;

    OwningQPointer<MergeOwnersDialog> dlg = new MergeOwnersDialog(Session->m_Db, this);
    dlg->exec();

    if(clearModelTimers[OwnersTab] == ModelLoaded)
    {
        ownersSQLModel->refreshData();
    }
    if(clearModelTimers[RollingstockTab] == ModelLoaded)
    {
        rsSQLModel->refreshData(true);
    }
}

void RollingStockManager::onImportRS()
{
    importRS(false, this);

    rsSQLModel->refreshData();
    modelsSQLModel->refreshData();
    ownersSQLModel->refreshData();
}

void RollingStockManager::showSessionRSViewer()
{
    Session->getViewManager()->showSessionStartEndRSViewer();
}

void RollingStockManager::onRollingstockSelectionChanged()
{
    const bool hasSel = rsView->selectionModel()->hasSelection();
    actDeleteRs->setEnabled(hasSel);
    actViewRSPlan->setEnabled(hasSel);
}

void RollingStockManager::onRsModelSelectionChanged()
{
    const bool hasSel = rsModelsView->selectionModel()->hasSelection();
    actDeleteModel->setEnabled(hasSel);
    actNewModelWithSuffix->setEnabled(hasSel);
    actMergeModels->setEnabled(hasSel);
}

void RollingStockManager::onRsOwnerSelectionChanged()
{
    const bool hasSel = ownersView->selectionModel()->hasSelection();
    actDeleteOwner->setEnabled(hasSel);
    actMergeOwners->setEnabled(hasSel);
}

void RollingStockManager::timerEvent(QTimerEvent *e)
{
    if(e->timerId() == clearModelTimers[RollingstockTab])
    {
        rsSQLModel->clearCache();
        killTimer(e->timerId());
        clearModelTimers[RollingstockTab] = ModelCleared;
        return;
    }
    else if(e->timerId() == clearModelTimers[ModelsTab])
    {
        modelsSQLModel->clearCache();
        killTimer(e->timerId());
        clearModelTimers[ModelsTab] = ModelCleared;
        return;
    }
    else if(e->timerId() == clearModelTimers[OwnersTab])
    {
        ownersSQLModel->clearCache();
        killTimer(e->timerId());
        clearModelTimers[OwnersTab] = ModelCleared;
        return;
    }

    QWidget::timerEvent(e);
}

void RollingStockManager::showEvent(QShowEvent *e)
{
    if(!windowConnected)
    {
        QWindow *w = windowHandle();
        if(w)
        {
            windowConnected = true;
            connect(w, &QWindow::visibilityChanged, this, &RollingStockManager::visibilityChanged);
            updateModels();
        }
    }
    QWidget::showEvent(e);
}
