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

#include "loadingpage.h"
#include "../backends/loadprogressevent.h"

#include <QProgressBar>
#include <QVBoxLayout>

LoadingPage::LoadingPage(QWidget *parent) :
    QWizardPage(parent),
    m_isComplete(false)
{
    QVBoxLayout *lay = new QVBoxLayout(this);

    progressBar      = new QProgressBar;
    lay->addWidget(progressBar);
}

bool LoadingPage::isComplete() const
{
    return m_isComplete;
}

void LoadingPage::handleProgress(int pr, int max)
{
    if (max == LoadProgressEvent::ProgressMaxFinished)
    {
        progressBar->setValue(progressBar->maximum());
        emit completeChanged();
    }
    else
    {
        progressBar->setMaximum(max);
        progressBar->setValue(pr);
    }
}

void LoadingPage::setProgressCompleted(bool val)
{
    m_isComplete = val;
    emit completeChanged();
}
