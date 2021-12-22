#include "optionspage.h"
#include "../rsimportwizard.h"

#include "../backends/ioptionswidget.h"
#include "../backends/importmodes.h"

#include "../rsimportstrings.h"

#include <QGroupBox>
#include <QCheckBox>
#include <QComboBox>
#include <QSpinBox>
#include <QMessageBox>

#include <QScrollArea>

#include <QVBoxLayout>
#include <QFormLayout>

#include <QStringListModel>
#include "utils/rs_types_names.h"

OptionsPage::OptionsPage(QWidget *parent) :
    QWizardPage(parent),
    optionsWidget(nullptr)
{
    QVBoxLayout *lay = new QVBoxLayout(this);

    //General options
    generalBox = new QGroupBox(RsImportStrings::tr("General options"));
    QFormLayout *generalLay = new QFormLayout(generalBox);

    importOwners = new QCheckBox(RsImportStrings::tr("Import rollingstick owners"));
    connect(importOwners, &QCheckBox::toggled, this, &OptionsPage::updateGeneralCheckBox);
    generalLay->addRow(importOwners);

    importModels = new QCheckBox(RsImportStrings::tr("Import rollingstick models"));
    connect(importModels, &QCheckBox::toggled, this, &OptionsPage::updateGeneralCheckBox);
    generalLay->addRow(importModels);

    importRS = new QCheckBox(RsImportStrings::tr("Import rollingstick pieces"));
    connect(importRS, &QCheckBox::toggled, this, &OptionsPage::updateGeneralCheckBox);
    generalLay->addRow(importRS);

    //NOTE: see 'RollingStockManager::setupPages()' in 'Setup delegates' section
    defaultSpeedSpin = new QSpinBox;
    defaultSpeedSpin->setRange(1, 999);
    defaultSpeedSpin->setSuffix(" km/h");
    defaultSpeedSpin->setValue(120);
    defaultSpeedSpin->setToolTip(tr("Default speed is applied when a rollingstock model is not matched to an existing one"
                                    " and has to be created from scratch"));
    generalLay->addRow(tr("Default speed"), defaultSpeedSpin);

    defaultTypeCombo = new QComboBox;
    QStringList list;
    list.reserve(int(RsType::NTypes));
    for(int i = 0; i < int(RsType::NTypes); i++)
        list.append(RsTypeNames::name(RsType(i)));
    QStringListModel *rsTypeModel = new QStringListModel(list, this);
    defaultTypeCombo->setModel(rsTypeModel);
    defaultTypeCombo->setCurrentIndex(int(RsType::FreightWagon));
    defaultTypeCombo->setToolTip(tr("Default type is applied when a rollingstock model is not matched to an existing one"
                                    " and has to be created from scratch"));
    generalLay->addRow(tr("Default type"), defaultTypeCombo);

    lay->addWidget(generalBox);

    //Specific options
    specificBox = new QGroupBox(RsImportStrings::tr("Import options"));
    QFormLayout *specificlLay = new QFormLayout(specificBox);
    backendCombo = new QComboBox;
    connect(backendCombo, static_cast<void(QComboBox::*)(int)>(&QComboBox::activated), this, &OptionsPage::setSource);
    specificlLay->addRow(RsImportStrings::tr("Import source:"), backendCombo);
    scrollArea = new QScrollArea;
    scrollArea->setWidgetResizable(true);
    scrollArea->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    specificlLay->addRow(scrollArea);
    specificBox->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    lay->addWidget(specificBox);
}

void OptionsPage::initializePage()
{
    const RSImportWizard *w = static_cast<RSImportWizard *>(wizard());
    backendCombo->setModel(w->getBackendsModel());
    setSource(w->getBackendIdx());
    setMode(w->getImportMode());
}

bool OptionsPage::validatePage()
{
    RSImportWizard *w = static_cast<RSImportWizard *>(wizard());
    int backendIdx = backendCombo->currentIndex();
    if(backendCombo->currentIndex() < 0 || !optionsWidget)
        return false;

    w->setDefaultTypeAndSpeed(RsType(defaultTypeCombo->currentIndex()), defaultSpeedSpin->value());
    w->setSource(backendIdx, optionsWidget);
    w->setImportMode(getMode());
    return true;
}

void OptionsPage::setMode(int m)
{
    if(m == 0)
        m = RSImportMode::ImportRSPieces;
    if(m & RSImportMode::ImportRSPieces)
    {
        m = RSImportMode::ImportRSOwners | RSImportMode::ImportRSModels;

        importRS->blockSignals(true);
        importRS->setChecked(true);
        importRS->blockSignals(false);
    }
    if(m & RSImportMode::ImportRSOwners)
    {
        importOwners->blockSignals(true);
        importOwners->setChecked(true);
        importOwners->blockSignals(false);
    }
    if(m & RSImportMode::ImportRSModels)
    {
        importModels->blockSignals(true);
        importModels->setChecked(true);
        importModels->blockSignals(false);
    }
    updateGeneralCheckBox();
}

int OptionsPage::getMode()
{
    int mode = 0;
    if(importOwners->isChecked())
        mode |= RSImportMode::ImportRSOwners;
    if(importModels->isChecked())
        mode |= RSImportMode::ImportRSModels;
    if(importRS->isChecked())
        mode |= RSImportMode::ImportRSPieces;
    return mode;
}

void OptionsPage::setSource(int backendIdx)
{
    backendCombo->setCurrentIndex(backendIdx);
    if(optionsWidget)
    {
        scrollArea->takeWidget();
        delete optionsWidget;
        optionsWidget = nullptr;
    }

    RSImportWizard *w = static_cast<RSImportWizard *>(wizard());
    optionsWidget = w->createOptionsWidget(backendIdx, this);
    scrollArea->setWidget(optionsWidget);
}

void OptionsPage::updateGeneralCheckBox()
{
    if(!importOwners->isChecked() && !importModels->isChecked())
    {
        QMessageBox::warning(this, RsImportStrings::tr("Invalid option"),
                             RsImportStrings::tr("You must at least import owners or models"));
        importModels->setChecked(true); //Tiggers an updateGeneralCheckBox() so return
        return;
    }

    importRS->blockSignals(true);

    if(importOwners->isChecked() && importModels->isChecked())
    {
        importRS->setEnabled(true);
        importRS->setToolTip(QString());
    }
    else
    {
        if(importRS->isChecked())
        {
            QMessageBox::warning(this, RsImportStrings::tr("No rolloingstock imported"),
                                 RsImportStrings::tr("No rollingstock piece will be imported.\n"
                                                     "In order to import rollingstock pieces you must also import models and owners."));
        }

        importRS->setEnabled(false);
        importRS->setChecked(false);
        importRS->setToolTip(RsImportStrings::tr("In order to import rollingstock pieces you must also import models and owners."));
    }
    importRS->blockSignals(false);

    //Default type and speed have meaning only if importing models
    defaultSpeedSpin->setEnabled(importModels->isChecked());
    defaultTypeCombo->setEnabled(importModels->isChecked());
}
