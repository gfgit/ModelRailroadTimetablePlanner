#include "selectionpage.h"
#include "printwizard.h"

#include <QTableView>
#include <QPushButton>
#include <QComboBox>
#include <QLabel>

#include <QGridLayout>
#include <QBoxLayout>

#include <QDialog>
#include <QDialogButtonBox>
#include <QPointer>

#include "graph/view/linegraphselectionwidget.h"

#include "sceneselectionmodel.h"

PrintSelectionPage::PrintSelectionPage(PrintWizard *w, QWidget *parent) :
    QWizardPage (parent),
    mWizard(w)
{
    SceneSelectionModel *model = mWizard->getSelectionModel();

    setupComboBoxes();

    addBut = new QPushButton(tr("Add"));
    remBut = new QPushButton(tr("Remove"));
    removeAllBut = new QPushButton(tr("Unselect All"));

    connect(addBut, &QPushButton::clicked, this, &PrintSelectionPage::onAddItem);
    connect(remBut, &QPushButton::clicked, this, &PrintSelectionPage::onRemoveItem);
    connect(removeAllBut, &QPushButton::clicked, model, &SceneSelectionModel::removeAll);

    view = new QTableView;
    view->setModel(model);

    statusLabel = new QLabel;
    connect(model, &SceneSelectionModel::selectionCountChanged, this, &PrintSelectionPage::updateSelectionCount);

    QVBoxLayout *lay = new QVBoxLayout(this);
    lay->addWidget(modeCombo);
    lay->addWidget(typeCombo);

    QHBoxLayout *subLay = new QHBoxLayout;
    subLay->addWidget(addBut);
    subLay->addWidget(remBut);
    subLay->addWidget(removeAllBut);

    lay->addLayout(subLay);
    lay->addWidget(view);
    lay->addWidget(statusLabel);

    setTitle(tr("Selection page"));
    setSubTitle(tr("Select one or more items to be printed"));
}

bool PrintSelectionPage::isComplete() const
{
    const qint64 count = mWizard->getSelectionModel()->getSelectionCount();
    return count > 0;
}

int PrintSelectionPage::nextId() const
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

void PrintSelectionPage::updateComboBoxes()
{
    SceneSelectionModel *model = mWizard->getSelectionModel();

    const SceneSelectionModel::SelectionMode mode = model->getMode();
    const LineGraphType type = model->getSelectedType();

    modeCombo->setCurrentIndex(int(mode));
    typeCombo->setCurrentIndex(int(type));

    typeCombo->setEnabled(mode == SceneSelectionModel::AllOfTypeExceptSelected);
}

void PrintSelectionPage::updateSelectionCount()
{
    const qint64 count = mWizard->getSelectionModel()->getSelectionCount();
    statusLabel->setText(tr("%1 items selected.").arg(count));

    emit completeChanged();
}

void PrintSelectionPage::onAddItem()
{
    SceneSelectionModel *model = mWizard->getSelectionModel();

    const LineGraphType requestedType = model->getSelectedType();

    QPointer<QDialog> dlg = new QDialog(this);
    QVBoxLayout *lay = new QVBoxLayout(dlg);

    LineGraphSelectionWidget *selectionWidget = new LineGraphSelectionWidget;
    selectionWidget->setGraphType(requestedType);
    lay->addWidget(selectionWidget);

    QDialogButtonBox *box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(box, &QDialogButtonBox::accepted, dlg, &QDialog::accept);
    connect(box, &QDialogButtonBox::rejected, dlg, &QDialog::reject);
    lay->addWidget(box);

    auto updateDlg = [selectionWidget, box, requestedType]()
    {
        LineGraphType type = selectionWidget->getGraphType();
        if(requestedType != LineGraphType::NoGraph && type != requestedType)
        {
            selectionWidget->setGraphType(requestedType);
            return;
        }

        box->button(QDialogButtonBox::Ok)->setEnabled(selectionWidget->getObjectId());
    };

    connect(selectionWidget, &LineGraphSelectionWidget::graphChanged, this, updateDlg);
    updateDlg();

    if(dlg->exec() == QDialog::Accepted && dlg)
    {
        SceneSelectionModel::Entry entry;
        entry.objectId = selectionWidget->getObjectId();
        entry.name = selectionWidget->getObjectName();
        entry.type = selectionWidget->getGraphType();

        model->addEntry(entry);
    }

    delete dlg;
}

void PrintSelectionPage::onRemoveItem()
{
    if(!view->selectionModel()->hasSelection())
        return;

    QModelIndex idx = view->currentIndex();
    if(!idx.isValid())
        return;

    mWizard->getSelectionModel()->removeAt(idx.row());
}

void PrintSelectionPage::setupComboBoxes()
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

    connect(model, &SceneSelectionModel::selectionModeChanged, this, &PrintSelectionPage::updateComboBoxes);
    updateComboBoxes();
}
