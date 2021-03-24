#include "rsimportwizard.h"
#include "rsimportstrings.h"

#include "model/rsimportedmodelsmodel.h"
#include "model/rsimportedownersmodel.h"
#include "model/rsimportedrollingstockmodel.h"

#include "app/session.h"

#include "pages/optionspage.h"
#include "pages/choosefilepage.h"
#include "pages/loadingpage.h"
#include "pages/itemselectionpage.h"

#include "utils/spinbox/spinboxeditorfactory.h"
#include <QStyledItemDelegate>

#include "backends/ioptionswidget.h"
#include "backends/optionsmodel.h"
#include "backends/loadtaskutils.h"
#include "backends/loadprogressevent.h"
#include "backends/importtask.h"
#include <QThreadPool>

//Backends
#include "backends/ods/loadodstask.h"
#include "backends/ods/odsoptionswidget.h"
#include "backends/sqlite/loadsqlitetask.h"
#include "backends/sqlite/sqliteoptionswidget.h"

#include <QMessageBox>
#include <QPushButton>

#include <QPointer>

RSImportWizard::RSImportWizard(bool resume, QWidget *parent) :
    QWizard (parent),
    loadTask(nullptr),
    importTask(nullptr),
    isStoppingTask(false),
    defaultSpeed(120),
    defaultRsType(RsType::FreightWagon),
    importMode(RSImportMode::ImportRSPieces),
    importSource(ImportSource::OdsImport)
{
    sourcesModel = new OptionsModel(this);

    modelsModel = new RSImportedModelsModel(Session->m_Db, this);
    ownersModel = new RSImportedOwnersModel(Session->m_Db, this);
    listModel   = new RSImportedRollingstockModel(Session->m_Db, this);

    loadFilePage = new LoadingPage(this);
    loadFilePage->setCommitPage(true);
    loadFilePage->setTitle(RsImportStrings::tr("File loading"));
    loadFilePage->setSubTitle(RsImportStrings::tr("Parsing file data..."));

    //HACK: I don't like the 'Commit' button. This hack makes it similar to 'Next' button
    loadFilePage->setButtonText(QWizard::CommitButton, buttonText(QWizard::NextButton));

    importPage = new LoadingPage(this);
    importPage->setTitle(RsImportStrings::tr("Importing"));
    importPage->setSubTitle(RsImportStrings::tr("Importing data..."));

    spinFactory = new SpinBoxEditorFactory;
    spinFactory->setRange(-1, 99999);
    spinFactory->setSpecialValueText(RsImportStrings::tr("Original"));
    spinFactory->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    setPage(OptionsPageIdx,  new OptionsPage);
    setPage(ChooseFileIdx,   new ChooseFilePage);
    setPage(LoadFileIdx,     loadFilePage);
    setPage(SelectOwnersIdx, new ItemSelectionPage(this, ownersModel, nullptr, ownersModel,   RSImportedOwnersModel::MatchExisting, ModelModes::Owners));
    setPage(SelectModelsIdx, new ItemSelectionPage(this, modelsModel, nullptr, modelsModel,   RSImportedModelsModel::MatchExisting, ModelModes::Models));
    setPage(SelectRsIdx,     new ItemSelectionPage(this, listModel, spinFactory, nullptr, RSImportedRollingstockModel::NewNumber, ModelModes::Rollingstock));
    setPage(ImportRsIdx,     importPage);

    if(resume)
        setStartId(SelectOwnersIdx);

    resize(700, 500);
}

RSImportWizard::~RSImportWizard()
{
    abortLoadTask();
    abortImportTask();
    delete spinFactory;
}

void RSImportWizard::done(int result)
{
    if(result == QDialog::Rejected || result == RejectWithoutAsking)
    {
        if(!isStoppingTask)
        {
            if(result == QDialog::Rejected) //RejectWithoutAsking skips this
            {
                QPointer<QMessageBox> msgBox = new QMessageBox(this);
                msgBox->setIcon(QMessageBox::Question);
                msgBox->setWindowTitle(RsImportStrings::tr("Abort import?"));
                msgBox->setText(RsImportStrings::tr("Do you want to import process? No data will be imported"));
                QPushButton *abortBut = msgBox->addButton(QMessageBox::Abort);
                QPushButton *noBut = msgBox->addButton(QMessageBox::No);
                msgBox->setDefaultButton(noBut);
                msgBox->setEscapeButton(noBut); //Do not Abort if dialog is closed by Esc or X window button
                msgBox->exec();
                bool abortClicked = msgBox && msgBox->clickedButton() != abortBut;
                delete msgBox;
                if(!abortClicked)
                    return;
            }

            if(loadTask)
            {
                loadTask->stop();
                isStoppingTask = true;
                loadFilePage->setSubTitle(RsImportStrings::tr("Aborting..."));
            }

            if(importTask)
            {
                importTask->stop();
                isStoppingTask = true;
                importPage->setSubTitle(RsImportStrings::tr("Aborting..."));
            }
        }
        else
        {
            if(loadTask || importTask)
                return; //Already sent 'stop', just wait
        }

        //Reset to standard value because QWizard doesn't know about RejectWithoutAsking
        result = QDialog::Rejected;
    }

    //Clear tables after import process completed or was aborted
    Session->clearImportRSTables();

    QWizard::done(result);
}

void RSImportWizard::initializePage(int id)
{
    QWizard::initializePage(id);
}

void RSImportWizard::cleanupPage(int id)
{
    QWizard::cleanupPage(id);
}

bool RSImportWizard::validateCurrentPage()
{
    if(QWizard::validateCurrentPage())
    {
        if(nextId() == ImportRsIdx)
        {
            startImportTask();
        }
        return true;
    }
    return false;
}

int RSImportWizard::nextId() const
{
    int id = QWizard::nextId();
    switch (currentId())
    {
    case LoadFileIdx:
    {
        if((importMode & RSImportMode::ImportRSOwners) == 0)
        {
            //Skip owners page
            id = SelectModelsIdx;
        }
        break;
    }
    case SelectOwnersIdx:
    {
        if((importMode & RSImportMode::ImportRSModels) == 0)
        {
            //Skip models and rollingstock pages
            id = ImportRsIdx;
        }
        break;
    }
    case SelectModelsIdx:
    {
        if((importMode & RSImportMode::ImportRSPieces) == 0)
        {
            //Skip rollingstock page
            id = ImportRsIdx;
        }
        break;
    }
    }
    return id;
}

bool RSImportWizard::event(QEvent *e)
{
    if(e->type() == LoadProgressEvent::_Type)
    {
        LoadProgressEvent *ev = static_cast<LoadProgressEvent *>(e);
        ev->setAccepted(true);

        if(ev->task == loadTask)
        {
            QString errText;
            if(ev->max == LoadProgressEvent::ProgressMaxFinished)
            {
                if(ev->progress == LoadProgressEvent::ProgressError)
                {
                    errText = loadTask->getErrorText();
                }

                loadFilePage->setSubTitle(tr("Completed."));

                //Delete task before handling event because otherwise it is detected as still running
                delete loadTask;
                loadTask = nullptr;
            }

            loadFilePage->handleProgress(ev->progress, ev->max);

            if(ev->progress == LoadProgressEvent::ProgressError)
            {
                QMessageBox::warning(this, RsImportStrings::tr("Loading Error"), errText);
                reject();
            }
            else if(ev->progress == LoadProgressEvent::ProgressAbortedByUser)
            {
                reject(); //Reject the second time
            }
        }
        else if(ev->task == importTask)
        {
            if(ev->max == LoadProgressEvent::ProgressMaxFinished)
            {
                //Delete task before handling event because otherwise it is detected as still running
                delete importTask;
                importTask = nullptr;
            }

            importPage->handleProgress(ev->progress, ev->max);

            if(ev->progress == LoadProgressEvent::ProgressError)
            {
                //QMessageBox::warning(this, RsImportStrings::tr("Loading Error"), errText); TODO
                reject();
            }
            else if(ev->progress == LoadProgressEvent::ProgressAbortedByUser)
            {
                reject(); //Reject the second time
            }
        }

        return true;
    }
    else if(e->type() == QEvent::Type(CustomEvents::RsImportGoBackPrevPage))
    {
        e->setAccepted(true);
        back();
    }
    return QWizard::event(e);
}



bool RSImportWizard::startLoadTask(const QString& fileName)
{
    abortLoadTask();
    
    //Clear tables before starting new import process
    Session->clearImportRSTables();

    loadTask = createLoadTask(optionsMap, fileName);

    if(!loadTask)
    {
        QMessageBox::warning(this, RsImportStrings::tr("Error"),
                             RsImportStrings::tr("Invalid option selected. Please try again."));
        return false;
    }

    QThreadPool::globalInstance()->start(loadTask);
    return true;
}

void RSImportWizard::abortLoadTask()
{
    if(loadTask)
    {
        loadTask->cleanup();
        loadTask->stop();
        loadTask = nullptr;
    }
}

void RSImportWizard::startImportTask()
{
    abortImportTask();

    importTask = new ImportTask(Session->m_Db, this);
    QThreadPool::globalInstance()->start(importTask);
}

void RSImportWizard::abortImportTask()
{
    if(importTask)
    {
        importTask->cleanup();
        importTask->stop();
        importTask = nullptr;
    }
}

void RSImportWizard::goToPrevPageQueued()
{
    qApp->postEvent(this, new QEvent(QEvent::Type(CustomEvents::RsImportGoBackPrevPage)));
}

void RSImportWizard::setDefaultTypeAndSpeed(RsType t, int speed)
{
    defaultRsType = t;
    defaultSpeed = speed;
}

void RSImportWizard::setImportMode(int m)
{
    if(m == 0)
        m = RSImportMode::ImportRSPieces;
    if(m & RSImportMode::ImportRSPieces)
        m |= RSImportMode::ImportRSOwners | RSImportMode::ImportRSModels;
    importMode = m;
}

IOptionsWidget *RSImportWizard::createOptionsWidget(RSImportWizard::ImportSource source, QWidget *parent)
{
    IOptionsWidget *w = nullptr;
    switch (source)
    {
    case ImportSource::OdsImport:
        w = new ODSOptionsWidget(parent);
        break;
    case ImportSource::SQLiteImport:
        w = new SQLiteOptionsWidget(parent);
        break;
    default:
        break;
    }

    if(w)
    {
        w->loadSettings(optionsMap);
    }
    return w;
}

void RSImportWizard::setSource(RSImportWizard::ImportSource source, IOptionsWidget *options)
{
    importSource = source;
    optionsMap.clear();
    options->saveSettings(optionsMap);
}

ILoadRSTask *RSImportWizard::createLoadTask(const QMap<QString, QVariant> &arguments, const QString& fileName)
{
    ILoadRSTask *task = nullptr;
    switch (importSource)
    {
    case ImportSource::OdsImport:
    {
        task = new LoadODSTask(arguments, Session->m_Db, importMode, defaultSpeed, defaultRsType, fileName, this);
        break;
    }
    case ImportSource::SQLiteImport:
    {
        task = new LoadSQLiteTask(Session->m_Db, importMode, fileName, this);
        break;
    }
    default:
        break;
    }
    return task;
}
