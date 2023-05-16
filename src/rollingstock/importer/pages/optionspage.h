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

#ifndef OPTIONSPAGE_H
#define OPTIONSPAGE_H

#include <QWizardPage>

class QGroupBox;
class QCheckBox;
class QComboBox;
class QSpinBox;
class IOptionsWidget;
class QScrollArea;

class OptionsPage : public QWizardPage
{
    Q_OBJECT
public:
    explicit OptionsPage(QWidget *parent = nullptr);

    virtual void initializePage() override;
    virtual bool validatePage() override;

private slots:
    void updateGeneralCheckBox();
    void setSource(int backendIdx);

private:
    void setMode(int m);
    int getMode();

private:
    QGroupBox *generalBox;
    QCheckBox *importOwners;
    QCheckBox *importModels;
    QCheckBox *importRS;
    QSpinBox  *defaultSpeedSpin;
    QComboBox *defaultTypeCombo;

    QGroupBox *specificBox;
    QComboBox *backendCombo;
    IOptionsWidget *optionsWidget;
    QScrollArea *scrollArea;
};

#endif // OPTIONSPAGE_H
