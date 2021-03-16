#include "graphoptions.h"

#include <QVBoxLayout>
#include <QGridLayout>

#include <QListView>
#include <QCheckBox>
#include <QPushButton>

#include <QDialogButtonBox>

GraphOptions::GraphOptions(QWidget *parent) :
    QDialog(parent)
{
    view = new QListView;

    selectAllBut = new QPushButton(tr("Select All"));
    selectNoneBut = new QPushButton(tr("Unselect All"));

    stationCheck = new QCheckBox(tr("Hide same station at job start"));

    QVBoxLayout *lay = new QVBoxLayout(this);
    lay->addWidget(stationCheck);

    QGridLayout *viewLay = new QGridLayout;
    viewLay->addWidget(selectAllBut, 0, 0);
    viewLay->addWidget(selectNoneBut, 0, 1);
    viewLay->addWidget(view, 1, 0, 1, 2);

    lay->addLayout(viewLay);

    QDialogButtonBox *box = new QDialogButtonBox(QDialogButtonBox::Ok
                                                 | QDialogButtonBox::Cancel);
    lay->addWidget(box);

    connect(box, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(box, &QDialogButtonBox::rejected, this, &QDialog::reject);

    setMinimumSize(300, 200);
    setWindowTitle(tr("Shifts Graph Options"));
}

bool GraphOptions::hideSameStation() const
{
    return stationCheck->isChecked();
}

void GraphOptions::setHideSameStations(bool value)
{
    stationCheck->setChecked(value);
}
