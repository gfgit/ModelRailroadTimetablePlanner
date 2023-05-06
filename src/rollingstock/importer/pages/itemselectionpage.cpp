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

#include "itemselectionpage.h"

#include <QTableView>
#include <QHeaderView>
#include <QStyledItemDelegate>
#include <QLabel>
#include <QVBoxLayout>
#include <QMessageBox>

#include "utils/owningqpointer.h"

#include "../rsimportwizard.h"
#include "../intefaces/irsimportmodel.h"
#include "../intefaces/iduplicatesitemmodel.h"
#include "../model/duplicatesimportedrsmodel.h" //HACK

#include "fixduplicatesdlg.h"
#include "utils/delegates/sql/sqlfkfielddelegate.h"
#include "utils/delegates/sql/modelpageswitcher.h"
#include "rollingstock/rsmatchmodelfactory.h"

#include "../rsimportstrings.h"
#include "utils/rs_types_names.h"

static const char *title_strings[] = {
    QT_TRANSLATE_NOOP("RsTypeNames", "Owners"),
    QT_TRANSLATE_NOOP("RsTypeNames", "Models"),
    QT_TRANSLATE_NOOP("RsTypeNames", "Rollingstock")
};

static const char *descr_strings[] = {
    QT_TRANSLATE_NOOP("RsImportStrings", "Select owners of rollingstock you want to import"),
    QT_TRANSLATE_NOOP("RsImportStrings", "Select models of rollingstock you want to import"),
    QT_TRANSLATE_NOOP("RsImportStrings", "Select rollingstock pieces you want to import")
};

static const char *error_strings[] = {
    QT_TRANSLATE_NOOP("RsImportStrings", "You must select at least 1 owner"),
    QT_TRANSLATE_NOOP("RsImportStrings", "You must select at least 1 model"),
    QT_TRANSLATE_NOOP("RsImportStrings", "You must select at least 1 rollingsock piece")
};

static const char *display_strings[] = { //FIXME plurals
    QT_TRANSLATE_NOOP("RsImportStrings", "%1 owners selected"),
    QT_TRANSLATE_NOOP("RsImportStrings", "%1 models selected"),
    QT_TRANSLATE_NOOP("RsImportStrings", "%1 rollingstock pieces selected")
};

ItemSelectionPage::ItemSelectionPage(RSImportWizard *w, IRsImportModel *m, QItemEditorFactory *edFactory, IFKField *ifaceDelegate, int delegateCol, ModelModes::Mode mode, QWidget *parent) :
    QWizardPage(parent),
    mWizard(w),
    model(m),
    editorFactory(edFactory),
    m_mode(mode)
{
    QVBoxLayout *lay = new QVBoxLayout(this);

    //TODO: add SelectAll, SelectNone

    view = new QTableView(this);
    view->setEditTriggers(QTableView::DoubleClicked); //Prevent changing names by accidentally pressing a key
    lay->addWidget(view);

    auto ps = new ModelPageSwitcher(false, this);
    lay->addWidget(ps);

    statusLabel = new QLabel;
    lay->addWidget(statusLabel);

    view->setModel(model);
    ps->setModel(model);
    connect(model, &IRsImportModel::modelError, this, &ItemSelectionPage::showModelError);

    if(ifaceDelegate)
    {
        SqlFKFieldDelegate *delegate = new SqlFKFieldDelegate(new RSMatchModelFactory(m_mode, model->getDb(), this), ifaceDelegate, this);
        view->setItemDelegateForColumn(delegateCol, delegate);

    }
    else if(editorFactory)
    {
        QStyledItemDelegate *delegate = new QStyledItemDelegate(this);
        delegate->setItemEditorFactory(editorFactory);
        view->setItemDelegateForColumn(delegateCol, delegate);
    }

    //Custom colun sorting
    //NOTE: leave disconnect() in the old SIGLAL()/SLOT() version in order to work
    QHeaderView *header = view->horizontalHeader();
    disconnect(header, SIGNAL(sectionPressed(int)), view, SLOT(selectColumn(int)));
    disconnect(header, SIGNAL(sectionEntered(int)), view, SLOT(_q_selectColumn(int)));

    connect(header, &QHeaderView::sectionClicked, this, &ItemSelectionPage::sectionClicked);
    header->setSortIndicatorShown(true);
    header->setSortIndicator(model->getSortingColumn(), Qt::AscendingOrder);

    connect(model, &IRsImportModel::importCountChanged, this, &ItemSelectionPage::updateStatus);
    updateStatus();

    setTitle(RsTypeNames::tr(title_strings[m_mode]));
    setSubTitle(RsImportStrings::tr(descr_strings[m_mode]));

    if(m_mode == ModelModes::Rollingstock)
    {
        setCommitPage(true);
        setButtonText(QWizard::CommitButton, RsImportStrings::tr("Import"));
    }
}

void ItemSelectionPage::initializePage()
{
    //Check for duplicates
    std::unique_ptr<IDuplicatesItemModel> dupModel;
    dupModel.reset(IDuplicatesItemModel::createModel(m_mode, model->getDb(), model, this));

    bool canGoBack = mWizard->currentId() != RSImportWizard::SelectOwnersIdx;
    OwningQPointer<FixDuplicatesDlg> dlg = new FixDuplicatesDlg(dupModel.get(), canGoBack, this);
    if(m_mode == ModelModes::Rollingstock && editorFactory)
    {
        QStyledItemDelegate *delegate = new QStyledItemDelegate(this);
        delegate->setItemEditorFactory(editorFactory);
        dlg->setItemDelegateForColumn(DuplicatesImportedRSModel::NewNumber, delegate);
    }

    //First load
    int res = dlg->blockingReloadCount(IDuplicatesItemModel::LoadingData);
    if(res == FixDuplicatesDlg::GoBackToPrevPage && canGoBack)
    {
        /* HACK:
         * we are inside 'initializePage()' which is called before QWizard
         * stores that we are current page
         * So if we call 'back()' now QWizard would try to go back by 2 pages
         * (that is 1 before current that is already the old page)
         * Solution: call it after 'initializePage()' has finished by posting an event
         */
        mWizard->goToPrevPageQueued();
        return;
    }
    else if(res != QDialog::Accepted)
    {
        //Prevent showing another message box asking user if he is sure about quitting
        mWizard->done(RSImportWizard::RejectWithoutAsking);
        return;
    }

    if(dupModel->getItemCount() > 0)
    {
        //We have duplicates, run dialog
        res = dlg->exec();
        if(res == FixDuplicatesDlg::GoBackToPrevPage && canGoBack)
        {
            /* HACK: see above */
            mWizard->goToPrevPageQueued();
            return;
        }
        else if(res != QDialog::Accepted)
        {
            //Prevent showing another message box asking user if he is sure about quitting
            mWizard->done(RSImportWizard::RejectWithoutAsking);
            return;
        }
    }

    //Duplicates are now fixed, refresh main model
    model->refreshData();
}

void ItemSelectionPage::cleanupPage()
{
    model->clearCache();
}

bool ItemSelectionPage::validatePage()
{
    int count = model->countImported();
    if(count == 0)
    {
        QMessageBox::warning(this, RsImportStrings::tr("Invalid Operation"), RsImportStrings::tr(error_strings[m_mode]));
        return false;
    }

    model->clearCache();

    if(m_mode == ModelModes::Rollingstock)
    {
        mWizard->startImportTask();
    }

    return true;
}

void ItemSelectionPage::updateStatus()
{
    int count = model->countImported();
    statusLabel->setText(RsImportStrings::tr(display_strings[m_mode]).arg(count));
}

void ItemSelectionPage::sectionClicked(int col)
{
    model->setSortingColumn(col);
    view->horizontalHeader()->setSortIndicator(model->getSortingColumn(), Qt::AscendingOrder);
}

void ItemSelectionPage::showModelError(const QString& text)
{
    QMessageBox::warning(this, RsImportStrings::tr("Invalid Operation"), text);
}
