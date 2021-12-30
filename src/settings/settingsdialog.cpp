#include "settingsdialog.h"
#include "ui_settingsdialog.h"

#include "app/session.h"
#include "utils/jobcategorystrings.h"

#include "languagemodel.h"
#include "utils/languageutils.h"

#include <QMessageBox>
#include <QCloseEvent>

#include <QFormLayout>

SettingsDialog::SettingsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingsDialog),
    updateJobsColors(false),
    updateJobGraphOptions(false),
    updateShiftGraphOptions(false)
{
    ui->setupUi(this);

    setupLanguageBox();

    connect(this, &QDialog::accepted, this, &SettingsDialog::saveSettings);
    connect(ui->restoreBut, &QPushButton::clicked, this, &SettingsDialog::onRestore);
    connect(ui->resetSheetLangBut, &QPushButton::clicked, this, &SettingsDialog::resetSheetLanguage);

    //Setup default stop time section
    QFormLayout *lay = new QFormLayout(ui->stopDurationBox);
    const QString suffix = tr("minutes");
    for(int i = 0; i < int(JobCategory::NCategories); i++)
    {
        QSpinBox *spin = new QSpinBox;
        spin->setMinimum(0);
        spin->setMaximum(5 * 60); //Maximum 5 hours
        spin->setSuffix(suffix);

        lay->addRow(JobCategoryName::fullName(JobCategory(i)), spin);
        m_timeSpinBoxArr[i] = spin;
    }

    setupJobColorsPage();

    connect(ui->vertOffsetSpin,         static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &SettingsDialog::onJobGraphOptionsChanged);
    connect(ui->horizOffsetSpin,        static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &SettingsDialog::onJobGraphOptionsChanged);
    connect(ui->hourOffsetSpin,         static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &SettingsDialog::onJobGraphOptionsChanged);
    connect(ui->stationsOffsetSpin,     static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &SettingsDialog::onJobGraphOptionsChanged);
    connect(ui->platformsOffsetSpin,    static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &SettingsDialog::onJobGraphOptionsChanged);

    connect(ui->hourLineWidthSpin,      static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &SettingsDialog::onJobGraphOptionsChanged);
    connect(ui->jobLineWidthSpin,       static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &SettingsDialog::onJobGraphOptionsChanged);
    connect(ui->platformsLineWidthSpin, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &SettingsDialog::onJobGraphOptionsChanged);

    connect(ui->hourTextColor,      &ColorView::colorChanged, this, &SettingsDialog::onJobGraphOptionsChanged);
    connect(ui->hourLineColor,      &ColorView::colorChanged, this, &SettingsDialog::onJobGraphOptionsChanged);
    connect(ui->stationTextColor,   &ColorView::colorChanged, this, &SettingsDialog::onJobGraphOptionsChanged);
    connect(ui->mainPlatformColor,  &ColorView::colorChanged, this, &SettingsDialog::onJobGraphOptionsChanged);
    connect(ui->depotPlatformColor, &ColorView::colorChanged, this, &SettingsDialog::onJobGraphOptionsChanged);

    connect(ui->shiftHourOffsetSpin, static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
            this, &SettingsDialog::onShiftGraphOptionsChanged);
    connect(ui->shiftHorizOffsetSpin, static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
            this, &SettingsDialog::onShiftGraphOptionsChanged);
    connect(ui->shiftVertOffsetSpin, static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
            this, &SettingsDialog::onShiftGraphOptionsChanged);
    connect(ui->shiftJobOffsetSpin, static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
            this, &SettingsDialog::onShiftGraphOptionsChanged);
    connect(ui->shiftJobBoxOffsetSpin, static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
            this, &SettingsDialog::onShiftGraphOptionsChanged);
    connect(ui->shiftStLabelOffsetSpin, static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
            this, &SettingsDialog::onShiftGraphOptionsChanged);
    connect(ui->shiftHideSameStCheck, &QCheckBox::toggled, this, &SettingsDialog::onShiftGraphOptionsChanged);
}

void SettingsDialog::setupJobColorsPage()
{
    //TODO: add scroll area also to other pages like stops
    QScrollArea *jobColors = new QScrollArea;
    ui->tabWidget->insertTab(3, jobColors, tr("Job Colors"));

    QWidget *jobColorsViewPort = new QWidget;
    QVBoxLayout *jobColorsLay = new QVBoxLayout(jobColorsViewPort);

    QGroupBox *freightBox = new QGroupBox(tr("Non Passenger"));
    QFormLayout *freightLay = new QFormLayout(freightBox);

    for(int cat = 0; cat < int(FirstPassengerCategory); cat++)
    {
        ColorView *color = new ColorView(this);
        connect(color, &ColorView::colorChanged, this, &SettingsDialog::onJobColorChanged);
        freightLay->addRow(JobCategoryName::fullName(JobCategory(cat)), color);
        m_colorViews[cat] = color;
    }
    jobColorsLay->addWidget(freightBox);

    QGroupBox *passengerBox = new QGroupBox(tr("Passenger"));
    QFormLayout *passengerLay = new QFormLayout(passengerBox);

    for(int cat = int(FirstPassengerCategory); cat < int(JobCategory::NCategories); cat++)
    {
        ColorView *color = new ColorView(this);
        connect(color, &ColorView::colorChanged, this, &SettingsDialog::onJobColorChanged);
        passengerLay->addRow(JobCategoryName::fullName(JobCategory(cat)), color);
        m_colorViews[cat] = color;
    }
    jobColorsLay->addWidget(passengerBox);

    jobColors->setWidget(jobColorsViewPort);
    jobColors->setWidgetResizable(true);
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}

void SettingsDialog::setupLanguageBox()
{
    languageModel = new LanguageModel(this);
    languageModel->loadAvailableLanguages();

    ui->appLanguageCombo->setModel(languageModel);
    ui->sheetLanguageCombo->setModel(languageModel);
}

void SettingsDialog::setSheetLanguage(const QLocale &sheetLoc)
{
    QTranslator *sheetTranslator = nullptr;
    if(Session->getAppLanguage() != sheetLoc && sheetLoc != MeetingSession::embeddedLocale)
    {
        //Sheet Language is different from original (currently loaded) App Language
        //And it's not default language embedded in executable

        //Sheet Export needs a different translation
        if(Session->getSheetExportLocale() == sheetLoc)
        {
            //Try to re-use old one
            sheetTranslator = Session->getSheetExportTranslator();
        }

        if(!sheetTranslator)
            sheetTranslator = utils::language::loadAppTranslator(sheetLoc);
    }

    Session->setSheetExportTranslator(sheetTranslator, sheetLoc);
}

inline void set(QSpinBox *spin, int val)
{
    spin->blockSignals(true);
    spin->setValue(val);
    spin->blockSignals(false);
}

inline void set(QDoubleSpinBox *spin, double val)
{
    spin->blockSignals(true);
    spin->setValue(val);
    spin->blockSignals(false);
}

void SettingsDialog::loadSettings()
{
    auto &settings = AppSettings;

    //General
    QLocale loc = settings.getLanguage();
    ui->appLanguageCombo->setCurrentIndex(languageModel->findMatchingRow(loc));

    loc = Session->getSheetExportLocale();
    ui->sheetLanguageCombo->setCurrentIndex(languageModel->findMatchingRow(loc));

    //Job Graph
    set(ui->hourOffsetSpin, settings.getHourOffset());
    set(ui->stationsOffsetSpin, settings.getStationOffset());
    set(ui->platformsOffsetSpin, settings.getPlatformOffset());
    set(ui->horizOffsetSpin, settings.getHorizontalOffset());
    set(ui->vertOffsetSpin, settings.getVerticalOffset());

    set(ui->hourLineWidthSpin, settings.getHourLineWidth());
    set(ui->jobLineWidthSpin, settings.getJobLineWidth());
    set(ui->platformsLineWidthSpin, settings.getPlatformLineWidth());

    ui->hourTextColor->     setColor(settings.getHourTextColor());
    ui->hourLineColor->     setColor(settings.getHourLineColor());
    ui->stationTextColor->  setColor(settings.getStationTextColor());
    ui->mainPlatformColor-> setColor(settings.getMainPlatfColor());
    ui->depotPlatformColor->setColor(settings.getDepotPlatfColor());

    ui->followJobSelectionCheck->setChecked(settings.getFollowSelectionOnGraphChange());
    ui->syncJobSelectionCheck->setChecked(settings.getSyncSelectionOnAllGraphs());

    //Job Colors
    for(int cat = 0; cat < int(JobCategory::NCategories); cat++)
    {
        QColor col = settings.getCategoryColor(cat);
        m_colorViews[cat]->setColor(col);
    }

    //Stops
    for(int cat = 0; cat < int(JobCategory::NCategories); cat++)
    {
        m_timeSpinBoxArr[cat]->blockSignals(true);
        m_timeSpinBoxArr[cat]->setValue(settings.getDefaultStopMins(cat));
        m_timeSpinBoxArr[cat]->blockSignals(false);
    }
    ui->autoInsertTransitsCheckBox->setChecked(settings.getAutoInsertTransits());
    ui->autoMoveLastCouplingsCheck->setChecked(settings.getAutoShiftLastStopCouplings());
    ui->autoUncoupleAllAtLastStopCheck->setChecked(settings.getAutoUncoupleAtLastStop());

    //Shift Graph
    set(ui->shiftHourOffsetSpin, settings.getShiftHourOffset());
    set(ui->shiftHorizOffsetSpin, settings.getShiftHorizOffset());
    set(ui->shiftVertOffsetSpin, settings.getShiftVertOffset());
    set(ui->shiftJobOffsetSpin, settings.getShiftJobOffset());
    set(ui->shiftJobBoxOffsetSpin, settings.getShiftJobBoxOffset());
    set(ui->shiftStLabelOffsetSpin, settings.getShiftStationOffset());
    ui->shiftHideSameStCheck->setChecked(settings.getShiftHideSameStations());

    //Rollingstock
    ui->mergeModelRemoveCheck->setChecked(settings.getRemoveMergedSourceModel());
    ui->mergeOwnerRemoveCheck->setChecked(settings.getRemoveMergedSourceOwner());

    //RS Import
    set(ui->odsFirstRowSpin, settings.getODSFirstRow());
    set(ui->odsNumColumnSpin, settings.getODSNumCol());
    set(ui->odsModelColumnSpin, settings.getODSNameCol());

    //Sheet Export ODT
    ui->sheetExportHeaderEdit->setText(settings.getSheetHeader());
    ui->sheetExportFooterEdit->setText(settings.getSheetFooter());
    ui->sheetMetadataCheckBox->setChecked(settings.getSheetStoreLocationDateInMeta());

    //Background Tasks
    ui->rsErrCheckAtFileOpen->setChecked(settings.getCheckRSWhenOpeningDB());
    ui->rsErrCheckOnJobEdited->setChecked(settings.getCheckRSOnJobEdit());

    updateJobsColors = false;
    updateJobGraphOptions = false;
}

void SettingsDialog::saveSettings()
{
    auto &settings = AppSettings;

    //General
    int idx = ui->appLanguageCombo->currentIndex();
    QLocale appLoc = languageModel->getLocaleAt(idx);
    settings.setLanguage(appLoc);

    idx = ui->sheetLanguageCombo->currentIndex();
    QLocale sheetLoc = languageModel->getLocaleAt(idx);
    setSheetLanguage(sheetLoc);

    //Job Graph
    settings.setHourOffset(ui->hourOffsetSpin->value());
    settings.setStationOffset(ui->stationsOffsetSpin->value());
    settings.setPlatformOffset(ui->platformsOffsetSpin->value());
    settings.setHorizontalOffset(ui->horizOffsetSpin->value());
    settings.setVerticalOffset(ui->vertOffsetSpin->value());

    settings.setHourLineWidth(ui->hourLineWidthSpin->value());
    settings.setJobLineWidth(ui->jobLineWidthSpin->value());
    settings.setPlatformLineWidth(ui->platformsLineWidthSpin->value());

    settings.setHourTextColor(ui->hourTextColor->color());
    settings.setHourLineColor(ui->hourLineColor->color());
    settings.setStationTextColor(ui->stationTextColor->color());
    settings.setMainPlatfColor(ui->mainPlatformColor->color());
    settings.setDepotPlatfColor(ui->depotPlatformColor->color());

    settings.setFollowSelectionOnGraphChange(ui->followJobSelectionCheck->isChecked());
    settings.setSyncSelectionOnAllGraphs(ui->syncJobSelectionCheck->isChecked());

    //Job Colors
    for(int cat = 0; cat < int(JobCategory::NCategories); cat++)
    {
        settings.setCategoryColor(cat, m_colorViews[cat]->color());
    }

    //Stops
    for(int cat = 0; cat < int(JobCategory::NCategories); cat++)
    {
        settings.setDefaultStopMins(cat, m_timeSpinBoxArr[cat]->value());
    }
    bool newVal = ui->autoInsertTransitsCheckBox->isChecked();
    bool stopSetingsChanged = settings.getAutoInsertTransits() != newVal;
    settings.setAutoInsertTransits(newVal);

    newVal = ui->autoMoveLastCouplingsCheck->isChecked();
    stopSetingsChanged |= settings.getAutoShiftLastStopCouplings() != newVal;
    settings.setAutoShiftLastStopCouplings(newVal);

    newVal = ui->autoUncoupleAllAtLastStopCheck->isChecked();
    stopSetingsChanged |= settings.getAutoUncoupleAtLastStop() != newVal;
    settings.setAutoUncoupleAtLastStop(newVal);

    //Shift Graph
    settings.setShiftHourOffset(ui->shiftHourOffsetSpin->value());
    settings.setShiftHorizOffset(ui->shiftHorizOffsetSpin->value());
    settings.setShiftVertOffset(ui->shiftVertOffsetSpin->value());
    settings.setShiftJobOffset(ui->shiftJobOffsetSpin->value());
    settings.setShiftJobBoxOffset(ui->shiftJobBoxOffsetSpin->value());
    settings.setShiftStationOffset(ui->shiftStLabelOffsetSpin->value());
    settings.setShiftHideSameStations(ui->shiftHideSameStCheck->isChecked());

    //Rollingstock
    settings.setRemoveMergedSourceModel(ui->mergeModelRemoveCheck->isChecked());
    settings.setRemoveMergedSourceOwner(ui->mergeOwnerRemoveCheck->isChecked());

    //RS Import
    settings.setODSFirstRow(ui->odsFirstRowSpin->value());
    settings.setODSNumCol(ui->odsNumColumnSpin->value());
    settings.setODSNameCol(ui->odsModelColumnSpin->value());

    //Sheet Export ODT
    settings.setSheetHeader(ui->sheetExportHeaderEdit->text());
    settings.setSheetFooter(ui->sheetExportFooterEdit->text());
    settings.setSheetStoreLocationDateInMeta(ui->sheetMetadataCheckBox->isChecked());

    //Background Tasks
    settings.setCheckRSWhenOpeningDB(ui->rsErrCheckAtFileOpen->isChecked());
    settings.setCheckRSOnJobEdit(ui->rsErrCheckOnJobEdited->isChecked());

    settings.saveSettings(); //Sync to file

    if(updateJobGraphOptions)
    {
        emit settings.jobGraphOptionsChanged();
        updateJobGraphOptions = false;
    }

    if(updateJobsColors)
    {
        emit settings.jobColorsChanged();
        updateJobsColors = false;
    }

    if(updateShiftGraphOptions)
    {
        emit settings.shiftGraphOptionsChanged();
        updateShiftGraphOptions = false;
    }

    if(stopSetingsChanged)
    {
        emit settings.stopOptionsChanged();
    }
}

void SettingsDialog::restoreDefaults()
{
    AppSettings.restoreDefaultSettings();
    loadSettings();
    updateJobsColors = true;
    updateJobGraphOptions = true;
    updateShiftGraphOptions = true;
    saveSettings();
}

void SettingsDialog::closeEvent(QCloseEvent *e)
{
    int ret = QMessageBox::question(this,
                                    tr("Settings"),
                                    tr("Do you want to save settings?"),
                                    QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
    if(ret == QMessageBox::Cancel)
    {
        e->ignore();
        return;
    }

    if(ret == QMessageBox::Yes)
        accept();

    return QDialog::closeEvent(e);
}

void SettingsDialog::onRestore()
{
    int ret = QMessageBox::question(this,
                                    tr("Settings"),
                                    tr("Do you want to restore default settings?"),
                                    QMessageBox::Yes | QMessageBox::No);
    if(ret == QMessageBox::Yes)
    {
        restoreDefaults();
    }
}

void SettingsDialog::resetSheetLanguage()
{
    //Set Sheet Language to same value of original Application Language
    QLocale origAppLanguage = Session->getAppLanguage();
    ui->sheetLanguageCombo->setCurrentIndex(languageModel->findMatchingRow(origAppLanguage));
}

void SettingsDialog::onJobColorChanged()
{
    updateJobsColors = true;
}

void SettingsDialog::onJobGraphOptionsChanged()
{
    updateJobGraphOptions = true;
}

void SettingsDialog::onShiftGraphOptionsChanged()
{
    updateShiftGraphOptions = true;
}
