#include "stationimportwizard.h"

#include "rollingstock/importer/pages/choosefilepage.h" //FIXME: move to utils
#include "pages/selectstationpage.h"

#include "utils/file_format_names.h"

#include "stations/match_models/stationsmatchmodel.h"

#include <sqlite3pp/sqlite3pp.h>
using namespace sqlite3pp;

StationImportWizard::StationImportWizard(QWidget *parent) :
    QWizard(parent),
    mInMemory(false)
{
    mTempDB = new database;

    QStringList fileFormats;
    fileFormats << FileFormats::tr(FileFormats::tttFormat);
    fileFormats << FileFormats::tr(FileFormats::sqliteFormat);
    fileFormats << FileFormats::tr(FileFormats::allFiles);

    //Setup Pages
    QWizardPage *optionsPage = new QWizardPage;
    optionsPage->setTitle(tr("Import Stations"));
    optionsPage->setSubTitle(tr("Follow the steps to import stations"));

    ChooseFilePage *chooseFilePage = new ChooseFilePage;
    chooseFilePage->setFileDlgOptions(tr("Open Session"), fileFormats);
    connect(chooseFilePage, &ChooseFilePage::fileChosen, this, &StationImportWizard::onFileChosen);

    setPage(OptionsPageIdx, optionsPage);
    setPage(ChooseFileIdx, chooseFilePage);
    setPage(SelectStationIdx, new SelectStationPage(this));


    setWindowTitle(tr("Import Stations"));
    resize(700, 500);
}

StationImportWizard::~StationImportWizard()
{
    closeDatabase();

    if(mTempDB)
    {
        delete mTempDB;
        mTempDB = nullptr;
    }
}

void StationImportWizard::onFileChosen(const QString &fileName)
{
    createDatabase(false, fileName);
}

bool StationImportWizard::createDatabase(bool inMemory, const QString &fileName)
{
    closeDatabase();

    SelectStationPage *p = static_cast<SelectStationPage *>(page(SelectStationIdx));

    int flags = SQLITE_OPEN_READWRITE;
    if(inMemory)
        flags |= SQLITE_OPEN_MEMORY;

    int ret = mTempDB->connect(fileName.toUtf8(), flags);
    if(ret != SQLITE_OK)
        return false;

    mInMemory = inMemory;

    StationsMatchModel *m = new StationsMatchModel(*mTempDB, this);
    m->setFilter(0);

    p->setupModel(m);
    return true;
}

bool StationImportWizard::closeDatabase()
{
    if(!mTempDB)
        return true;

    SelectStationPage *p = static_cast<SelectStationPage *>(page(SelectStationIdx));
    p->finalizeModel();

    return mTempDB->disconnect() == SQLITE_OK;
}
