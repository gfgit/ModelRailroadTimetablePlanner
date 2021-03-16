#include "selectionpage.h"
#include "printwizard.h"

#include <QListView>
#include <QPushButton>
#include <QGridLayout>

#include "utils/checkproxymodel.h"

#include "app/session.h"

SelectionPage::SelectionPage(PrintWizard *w, QWidget *parent) :
    QWizardPage (parent),
    mWizard(w)
{
    proxyModel = new CheckProxyModel(this);
    connect(proxyModel, &CheckProxyModel::hasCheck, this, &QWizardPage::completeChanged);

    view = new QListView;
    view->setModel(proxyModel);

    selectAllBut = new QPushButton(tr("Select All"));
    selectNoneBut = new QPushButton(tr("Unselect All"));

    connect(selectAllBut, &QPushButton::clicked, proxyModel, &CheckProxyModel::selectAll);
    connect(selectNoneBut, &QPushButton::clicked, proxyModel, &CheckProxyModel::selectNone);

    QGridLayout *l = new QGridLayout;
    l->addWidget(selectAllBut, 0, 0);
    l->addWidget(selectNoneBut, 0, 1);
    l->addWidget(view, 1, 0, 1, 2);
    setLayout(l);

    setTitle(tr("Selection page"));
    setSubTitle(tr("Select one or more lines to be printed"));
}

void SelectionPage::initializePage()
{
    proxyModel->setSourceModel(nullptr);
}

bool SelectionPage::isComplete() const
{
    return proxyModel->hasAtLeastACheck();
}

bool SelectionPage::validatePage()
{
    mWizard->m_checks = proxyModel->checks();
    return true;
}

int SelectionPage::nextId() const
{
    int ret = 0;
    switch (mWizard->type)
    {
    case Print::Native:
        ret = 2;
        break;
    case Print::Pdf:
    case Print::Svg:
        ret = 1;
    }
    return ret;
}
