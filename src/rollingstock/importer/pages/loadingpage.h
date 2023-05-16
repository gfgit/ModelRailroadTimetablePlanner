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

#ifndef LOADFILEPAGE_H
#define LOADFILEPAGE_H

#include <QWizardPage>

class QProgressBar;

class LoadingPage : public QWizardPage
{
    Q_OBJECT
public:
    explicit LoadingPage(QWidget *parent = nullptr);

    virtual bool isComplete() const override;

    void handleProgress(int pr, int max);
    void setProgressCompleted(bool val);

private:
    QProgressBar *progressBar;
    bool m_isComplete;
};

#endif // LOADFILEPAGE_H
