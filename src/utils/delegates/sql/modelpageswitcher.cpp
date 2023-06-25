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

#include "modelpageswitcher.h"

#include "pageditemmodel.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>

ModelPageSwitcher::ModelPageSwitcher(bool autoHide, QWidget *parent) :
    QWidget(parent),
    model(nullptr),
    statusLabel(nullptr),
    goToPageBut(nullptr),
    refreshModelBut(nullptr),
    autoHideMode(autoHide)
{
    QHBoxLayout *lay = new QHBoxLayout(this);

    prevBut          = new QPushButton(autoHideMode ? QChar('<') : tr("Prev"));
    nextBut          = new QPushButton(autoHideMode ? QChar('>') : tr("Next"));

    // NOTE: use released() instead of clicked() and QueuedConnection because prevBut and nextBut
    // might get disabled
    //       after switching the page and there is an artifact if it is disabled while being clicked
    //       so defer disabling
    connect(prevBut, &QPushButton::released, this, &ModelPageSwitcher::prevPage,
            Qt::QueuedConnection);
    connect(nextBut, &QPushButton::released, this, &ModelPageSwitcher::nextPage,
            Qt::QueuedConnection);

    pageSpin = new QSpinBox;
    pageSpin->setMinimum(1);
    pageSpin->setKeyboardTracking(false);

    if (autoHideMode)
    {
        connect(pageSpin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this,
                &ModelPageSwitcher::goToPage);
    }
    else
    {
        statusLabel = new QLabel;
        statusLabel->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
        goToPageBut     = new QPushButton(tr("Go"));
        refreshModelBut = new QPushButton(tr("Refresh"));

        connect(goToPageBut, &QPushButton::clicked, this, &ModelPageSwitcher::goToPage);
        connect(refreshModelBut, &QPushButton::clicked, this, &ModelPageSwitcher::refreshModel);
    }

    // Layout
    if (autoHideMode)
    {
        lay->addWidget(prevBut);
        lay->addWidget(pageSpin);
        lay->addWidget(nextBut);
        lay->setAlignment(Qt::AlignRight);
    }
    else
    {
        lay->addWidget(statusLabel);
        lay->addWidget(prevBut);
        lay->addWidget(nextBut);
        lay->addWidget(pageSpin);
        lay->addWidget(goToPageBut);
        lay->addWidget(refreshModelBut);
    }
}

void ModelPageSwitcher::setModel(IPagedItemModel *m)
{
    if (model)
    {
        disconnect(model, &IPagedItemModel::totalItemsCountChanged, this,
                   &ModelPageSwitcher::totalItemCountChanged);
        disconnect(model, &IPagedItemModel::pageCountChanged, this,
                   &ModelPageSwitcher::pageCountChanged);
        disconnect(model, &IPagedItemModel::currentPageChanged, this,
                   &ModelPageSwitcher::currentPageChanged);
        disconnect(model, &IPagedItemModel::destroyed, this, &ModelPageSwitcher::onModelDestroyed);
    }

    model = m;

    if (model)
    {
        connect(model, &IPagedItemModel::totalItemsCountChanged, this,
                &ModelPageSwitcher::totalItemCountChanged);
        connect(model, &IPagedItemModel::pageCountChanged, this,
                &ModelPageSwitcher::pageCountChanged);
        connect(model, &IPagedItemModel::currentPageChanged, this,
                &ModelPageSwitcher::currentPageChanged);
        connect(model, &IPagedItemModel::destroyed, this, &ModelPageSwitcher::onModelDestroyed);

        recalcPages();
    }
}

void ModelPageSwitcher::onModelDestroyed()
{
    model = nullptr;
}

void ModelPageSwitcher::totalItemCountChanged()
{
    recalcText();
}

void ModelPageSwitcher::pageCountChanged()
{
    recalcPages();
}

void ModelPageSwitcher::currentPageChanged()
{
    recalcPages();
}

void ModelPageSwitcher::recalcText()
{
    if (!statusLabel)
        return;

    statusLabel->setText(tr("Page %1/%2 (%3 items)")
                           .arg(model->currentPage() + 1)
                           .arg(model->getPageCount())
                           .arg(model->getTotalItemsCount()));
}

void ModelPageSwitcher::recalcPages()
{
    int curPage = model->currentPage();
    int count   = model->getPageCount();
    if (count == 0)
        count = 1;

    pageSpin->setMaximum(count);
    prevBut->setEnabled(curPage > 0);
    nextBut->setEnabled(curPage < count - 1);

    // NOTE: redraw to avoid artifact like disabled but still highlighted
    prevBut->update();
    nextBut->update();

    if (autoHideMode)
    {
        pageSpin->setValue(curPage + 1);

        if (count == 1)
            hide();
        else
            show();
    }

    recalcText();
}

void ModelPageSwitcher::prevPage()
{
    if (model)
        model->switchToPage(model->currentPage() - 1);
}

void ModelPageSwitcher::nextPage()
{
    if (model)
        model->switchToPage(model->currentPage() + 1);
}

void ModelPageSwitcher::goToPage()
{
    if (model)
        model->switchToPage(pageSpin->value() - 1);
}

void ModelPageSwitcher::refreshModel()
{
    if (model)
    {
        model->refreshData(true);
    }
}
