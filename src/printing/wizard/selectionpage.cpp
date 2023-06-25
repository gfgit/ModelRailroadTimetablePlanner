/*
 * ModelRailroadTimetablePlanner
 * Copyright 2016-2023, Filippo Gentile
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

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
#include "utils/owningqpointer.h"

#include "graph/view/linegraphselectionwidget.h"

#include "sceneselectionmodel.h"

PrintSelectionPage::PrintSelectionPage(PrintWizard *w, QWidget *parent) :
    QWizardPage(parent),
    mWizard(w)
{
    SceneSelectionModel *model = mWizard->getSelectionModel();

    setupComboBoxes();

    addBut       = new QPushButton(tr("Add"));
    remBut       = new QPushButton(tr("Remove"));
    removeAllBut = new QPushButton(tr("Unselect All"));

    connect(addBut, &QPushButton::clicked, this, &PrintSelectionPage::onAddItem);
    connect(remBut, &QPushButton::clicked, this, &PrintSelectionPage::onRemoveItem);
    connect(removeAllBut, &QPushButton::clicked, model, &SceneSelectionModel::removeAll);

    view = new QTableView;
    view->setModel(model);

    statusLabel = new QLabel;
    connect(model, &SceneSelectionModel::selectionCountChanged, this,
            &PrintSelectionPage::updateSelectionCount);
    updateSelectionCount();

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
    const qint64 count = mWizard->getSelectionModel()->getItemCount();
    return count > 0;
}

void PrintSelectionPage::comboBoxesChanged()
{
    SceneSelectionModel *model                    = mWizard->getSelectionModel();

    const int modeIdx                             = modeCombo->currentIndex();
    const SceneSelectionModel::SelectionMode mode = SceneSelectionModel::SelectionMode(modeIdx);
    LineGraphType type                            = LineGraphType(typeCombo->currentIndex());

    if (mode == SceneSelectionModel::AllOfTypeExceptSelected)
    {
        // Type cannot be NoGraph
        if (type == LineGraphType::NoGraph)
            type = model->getSelectedType();

        // Default to RailwayLine if still NoGraph
        if (type == LineGraphType::NoGraph)
            type = LineGraphType::RailwayLine;

        // Update combo box
        typeCombo->setCurrentIndex(int(type));
    }

    model->setMode(mode, type);
}

void PrintSelectionPage::updateComboBoxesFromModel()
{
    SceneSelectionModel *model                    = mWizard->getSelectionModel();

    const SceneSelectionModel::SelectionMode mode = model->getMode();
    const LineGraphType type                      = model->getSelectedType();

    modeCombo->setCurrentIndex(int(mode));
    typeCombo->setCurrentIndex(int(type));

    typeCombo->setEnabled(mode == SceneSelectionModel::AllOfTypeExceptSelected);
}

void PrintSelectionPage::updateSelectionCount()
{
    const qint64 count = mWizard->getSelectionModel()->getItemCount();
    statusLabel->setText(tr("%1 items selected.").arg(count));

    emit completeChanged();
}

void PrintSelectionPage::onAddItem()
{
    SceneSelectionModel *model                = mWizard->getSelectionModel();

    const LineGraphType requestedType         = model->getSelectedType();

    OwningQPointer<QDialog> dlg               = new QDialog(this);
    QVBoxLayout *lay                          = new QVBoxLayout(dlg);

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
        if (requestedType != LineGraphType::NoGraph && type != requestedType)
        {
            selectionWidget->setGraphType(requestedType);
            return;
        }

        box->button(QDialogButtonBox::Ok)->setEnabled(selectionWidget->getObjectId());
    };

    connect(selectionWidget, &LineGraphSelectionWidget::graphChanged, this, updateDlg);
    updateDlg();

    if (dlg->exec() != QDialog::Accepted)
        return;

    SceneSelectionModel::Entry entry;
    entry.objectId = selectionWidget->getObjectId();
    entry.name     = selectionWidget->getObjectName();
    entry.type     = selectionWidget->getGraphType();

    model->addEntry(entry);
}

void PrintSelectionPage::onRemoveItem()
{
    if (!view->selectionModel()->hasSelection())
        return;

    QModelIndex idx = view->currentIndex();
    if (!idx.isValid())
        return;

    mWizard->getSelectionModel()->removeAt(idx.row());
}

void PrintSelectionPage::setupComboBoxes()
{
    SceneSelectionModel *model = mWizard->getSelectionModel();

    modeCombo                  = new QComboBox;
    typeCombo                  = new QComboBox;

    connect(modeCombo, qOverload<int>(&QComboBox::activated), this,
            &PrintSelectionPage::comboBoxesChanged);
    connect(typeCombo, qOverload<int>(&QComboBox::activated), this,
            &PrintSelectionPage::comboBoxesChanged);

    QStringList items;
    items.reserve(SceneSelectionModel::NModes);
    for (int i = 0; i < SceneSelectionModel::NModes; i++)
    {
        items.append(SceneSelectionModel::getModeName(SceneSelectionModel::SelectionMode(i)));
    }
    modeCombo->addItems(items);

    items.clear();
    items.reserve(int(LineGraphType::NTypes));
    for (int i = 0; i < int(LineGraphType::NTypes); i++)
        items.append(utils::getLineGraphTypeName(LineGraphType(i)));
    typeCombo->addItems(items);

    connect(model, &SceneSelectionModel::selectionModeChanged, this,
            &PrintSelectionPage::updateComboBoxesFromModel);
    updateComboBoxesFromModel();
}
