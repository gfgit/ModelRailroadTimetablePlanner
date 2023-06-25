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

#include "printoptionspage.h"

#include "printwizard.h"

#include <QVBoxLayout>
#include "printing/helper/view/printeroptionswidget.h"

#include "utils/scene/igraphscene.h"

PrintOptionsPage::PrintOptionsPage(PrintWizard *w, QWidget *parent) :
    QWizardPage(parent),
    mWizard(w),
    m_scene(nullptr)
{
    QVBoxLayout *lay = new QVBoxLayout(this);
    optionsWidget    = new PrinterOptionsWidget;
    lay->addWidget(optionsWidget);

    connect(optionsWidget, &PrinterOptionsWidget::completeChanged, this,
            &QWizardPage::completeChanged);

    setTitle(tr("Print Options"));

    setCommitPage(true);

    // Change 'Commit' to 'Print' so user understands better
    setButtonText(QWizard::CommitButton, tr("Print"));
}

PrintOptionsPage::~PrintOptionsPage()
{
    // Reset scene
    setScene(nullptr);
}

bool PrintOptionsPage::validatePage()
{
    if (!optionsWidget->validateOptions())
        return false;

    // Update options
    mWizard->setPrintOpt(optionsWidget->getOptions());
    mWizard->setScenePageLay(optionsWidget->getScenePageLay());
    return true;
}

bool PrintOptionsPage::isComplete() const
{
    return optionsWidget->isComplete();
}

void PrintOptionsPage::setupPage()
{
    optionsWidget->setPrinter(mWizard->getPrinter());
    optionsWidget->setOptions(mWizard->getPrintOpt());
    optionsWidget->setScenePageLay(mWizard->getScenePageLay());
    setScene(mWizard->getFirstScene());
}

void PrintOptionsPage::setScene(IGraphScene *scene)
{
    if (m_scene)
        delete m_scene;
    m_scene = scene;
    optionsWidget->setSourceScene(m_scene);
}
