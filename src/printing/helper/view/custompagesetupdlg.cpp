#include "custompagesetupdlg.h"

#include <QVBoxLayout>
#include <QGroupBox>
#include <QComboBox>
#include <QRadioButton>
#include <QDialogButtonBox>

#include "printing/helper/model/pagesizemodel.h"

CustomPageSetupDlg::CustomPageSetupDlg(QWidget *parent) :
    QDialog(parent),
    m_pageSize(QPageSize::A4),
    m_pageOrient(QPageLayout::Portrait)
{
    QVBoxLayout *mainLay = new QVBoxLayout(this);

    QGroupBox *pageSizeBox = new QGroupBox(tr("Page Size"));
    QVBoxLayout *lay1 = new QVBoxLayout(pageSizeBox);

    pageSizeCombo = new QComboBox;

    PageSizeModel *m = new PageSizeModel(this);
    pageSizeCombo->setModel(m);

    lay1->addWidget(pageSizeCombo);

    QGroupBox *pageOrientBox = new QGroupBox(tr("Page Orientation"));
    QVBoxLayout *lay2 = new QVBoxLayout(pageOrientBox);

    portraitRadioBut = new QRadioButton(tr("Portrait"));
    lay2->addWidget(portraitRadioBut);

    landscapeRadioBut = new QRadioButton(tr("Landscape"));
    lay2->addWidget(landscapeRadioBut);

    mainLay->addWidget(pageSizeBox);
    mainLay->addWidget(pageOrientBox);

    QDialogButtonBox *buttBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    mainLay->addWidget(buttBox);

    //Default to A4 Portrait
    portraitRadioBut->setChecked(true);
    pageSizeCombo->setCurrentIndex(int(m_pageSize.id()));

    connect(pageSizeCombo, qOverload<int>(&QComboBox::activated),
            this, &CustomPageSetupDlg::onPageComboActivated);
    connect(portraitRadioBut, &QRadioButton::toggled,
            this, &CustomPageSetupDlg::onPagePortraitToggled);

    connect(buttBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void CustomPageSetupDlg::setPageSize(const QPageSize &pageSz)
{
    if(m_pageSize == pageSz)
        return;

    m_pageSize = pageSz;
    int idx = int(m_pageSize.id());
    pageSizeCombo->setCurrentIndex(idx);
}

void CustomPageSetupDlg::setPageOrient(QPageLayout::Orientation orient)
{
    if(m_pageOrient == orient)
        return;

    m_pageOrient = orient;
    if(m_pageOrient == QPageLayout::Portrait)
        portraitRadioBut->setChecked(true);
    else
        landscapeRadioBut->setChecked(true);
}

void CustomPageSetupDlg::onPageComboActivated(int idx)
{
    QPageSize::PageSizeId pageId = QPageSize::PageSizeId(idx);
    setPageSize(QPageSize(pageId));
}

void CustomPageSetupDlg::onPagePortraitToggled(bool val)
{
    QPageLayout::Orientation orient = val ? QPageLayout::Portrait : QPageLayout::Landscape;
    setPageOrient(orient);
}
