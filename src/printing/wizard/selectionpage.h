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

#ifndef PRINTSELECTIONPAGE_H
#define PRINTSELECTIONPAGE_H

#include <QWizardPage>

class PrintWizard;
class QTableView;
class QPushButton;
class QComboBox;
class QLabel;

class PrintSelectionPage : public QWizardPage
{
    Q_OBJECT
public:
    PrintSelectionPage(PrintWizard *w, QWidget *parent = nullptr);

    bool isComplete() const override;

private slots:
    void comboBoxesChanged();
    void updateComboBoxesFromModel();
    void updateSelectionCount();

    void onAddItem();
    void onRemoveItem();

private:
    void setupComboBoxes();

private:
    PrintWizard *mWizard;

    QTableView *view;

    QPushButton *addBut;
    QPushButton *remBut;
    QPushButton *removeAllBut;
    QComboBox *modeCombo;
    QComboBox *typeCombo;

    QLabel *statusLabel;
};

#endif // PRINTSELECTIONPAGE_H
