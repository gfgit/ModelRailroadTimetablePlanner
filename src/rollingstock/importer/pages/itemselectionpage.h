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

#ifndef ITEMSELECTIONPAGE_H
#define ITEMSELECTIONPAGE_H

#include <QWizardPage>

#include "utils/model_mode.h"

class RSImportWizard;
class QTableView;
class QLabel;

class IRsImportModel;
class IFKField;
class QItemEditorFactory;

class ItemSelectionPage : public QWizardPage
{
    Q_OBJECT
public:
    ItemSelectionPage(RSImportWizard *w, IRsImportModel *m, QItemEditorFactory *edFactory, IFKField *ifaceDelegate, int delegateCol, ModelModes::Mode mode, QWidget *parent = nullptr);

    virtual void initializePage() override;
    virtual void cleanupPage() override;
    virtual bool validatePage() override;

private slots:
    void updateStatus();
    void sectionClicked(int col);
    void showModelError(const QString &text);

private:
    RSImportWizard *mWizard;
    IRsImportModel *model;
    QItemEditorFactory *editorFactory;

    QTableView *view;
    QLabel *statusLabel;

    ModelModes::Mode m_mode;
};

#endif // ITEMSELECTIONPAGE_H
