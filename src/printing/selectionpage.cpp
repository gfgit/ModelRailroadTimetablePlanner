#include "selectionpage.h"
#include "printwizard.h"

#include <QListView>
#include <QPushButton>
#include <QComboBox>
#include <QLabel>

#include <QFormLayout>

#include "sceneselectionmodel.h"

#include "app/session.h"

SelectionPage::SelectionPage(PrintWizard *w, QWidget *parent) :
    QWizardPage (parent),
    mWizard(w)
{
    SceneSelectionModel *model = mWizard->getSelectionModel();

    setupComboBoxes();

    view = new QListView;
    view->setModel(model);

    statusLabel = new QLabel;
    connect(model, &SceneSelectionModel::selectionCountChanged, this, &SelectionPage::updateSelectionCount);

    selectNoneBut = new QPushButton(tr("Unselect All"));
    connect(selectNoneBut, &QPushButton::clicked, model, &SceneSelectionModel::removeAll);

    QFormLayout *lay = new QFormLayout(this);
    lay->addRow(tr("Mode:"), modeCombo);
    lay->addRow(tr("Type:"), typeCombo);
    lay->addRow(selectNoneBut);
    lay->addRow(view);
    lay->addRow(statusLabel);

    setTitle(tr("Selection page"));
    setSubTitle(tr("Select one or more items to be printed"));
}

bool SelectionPage::isComplete() const
{
    const qint64 count = mWizard->getSelectionModel()->getSelectionCount();
    return count > 0;
}

int SelectionPage::nextId() const
{
    int ret = 0;
    switch (mWizard->getOutputType())
    {
    case Print::Native:
        ret = 2;
        break;
    case Print::Pdf:
    case Print::Svg:
        ret = 1;
    }
    return ret;
}

void SelectionPage::updateComboBoxes()
{
    SceneSelectionModel *model = mWizard->getSelectionModel();

    const SceneSelectionModel::SelectionMode mode = model->getMode();
    const LineGraphType type = model->getSelectedType();

    modeCombo->setCurrentIndex(int(mode));
    typeCombo->setCurrentIndex(int(type));

    typeCombo->setEnabled(mode == SceneSelectionModel::AllOfTypeExceptSelected);
}

void SelectionPage::updateSelectionCount()
{
    const qint64 count = mWizard->getSelectionModel()->getSelectionCount();
    statusLabel->setText(tr("%1 items selected.").arg(count));

    emit completeChanged();
}

void SelectionPage::setupComboBoxes()
{
    SceneSelectionModel *model = mWizard->getSelectionModel();

    modeCombo = new QComboBox;
    typeCombo = new QComboBox;

    QStringList  items;
    items.reserve(SceneSelectionModel::NModes);
    for(int i = 0; i < SceneSelectionModel::NModes; i++)
    {
        items.append(SceneSelectionModel::getModeName(SceneSelectionModel::SelectionMode(i)));
    }
    modeCombo->addItems(items);

    items.clear();
    items.reserve(int(LineGraphType::NTypes));
    for(int i = 0; i < int(LineGraphType::NTypes); i++)
        items.append(utils::getLineGraphTypeName(LineGraphType(i)));
    typeCombo->addItems(items);

    connect(model, &SceneSelectionModel::selectionModeChanged, this, &SelectionPage::updateComboBoxes);
    updateComboBoxes();
}
