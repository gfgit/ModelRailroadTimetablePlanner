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

#include "progresspage.h"
#include "printwizard.h"

#include <QLabel>
#include <QProgressBar>
#include <QVBoxLayout>

PrintProgressPage::PrintProgressPage(PrintWizard *w, QWidget *parent) :
    QWizardPage(parent),
    mWizard(w)
{
    m_label = new QLabel(tr("Printing..."));
    m_progressBar = new QProgressBar;
    m_progressBar->setMinimum(0);

    QVBoxLayout *l = new QVBoxLayout;
    l->addWidget(m_label);
    l->addWidget(m_progressBar);
    setLayout(l);

    setFinalPage(true);

    setTitle(tr("Printing"));
}

bool PrintProgressPage::isComplete() const
{
    return !mWizard->taskRunning();
}

void PrintProgressPage::handleProgressStart(int max)
{
    //We are starting new progress
    //Enable progress bar and set maximum
    m_progressBar->setMaximum(max);
    m_progressBar->setEnabled(true);
    emit completeChanged();
}

void PrintProgressPage::handleProgress(int val, const QString& text)
{
    const bool finished = val < 0;

    if(val == m_progressBar->maximum() && !finished)
        m_progressBar->setMaximum(val + 1); //Increase maximum

    m_label->setText(text);
    if(finished)
    {
        //Set progress to max and set disabled
        m_progressBar->setValue(m_progressBar->maximum());
        m_progressBar->setEnabled(false);
        emit completeChanged();
    }
    else
    {
        m_progressBar->setValue(val);
    }
}
