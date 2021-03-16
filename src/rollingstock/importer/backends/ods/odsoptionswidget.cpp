#include "odsoptionswidget.h"
#include "options.h"

#include <QFormLayout>
#include <QLabel>
#include <QSpinBox>

#include "app/session.h"

ODSOptionsWidget::ODSOptionsWidget(QWidget *parent) :
    IOptionsWidget(parent)
{
    //ODS Option
    QFormLayout *lay = new QFormLayout(this);
    lay->addRow(new QLabel(tr("Import rollingstock pieces, models and owners from a spreadsheet file.\n"
                              "The file must be a valid Open Document Format Spreadsheet V1.2\n"
                              "Extension: (*.ods)")));
    odsFirstRowSpin = new QSpinBox;
    odsFirstRowSpin->setRange(1, 9999);
    lay->addRow(tr("First non-empty row that contains rollingstock piece information"), odsFirstRowSpin);
    odsNumColSpin = new QSpinBox;
    odsNumColSpin->setRange(1, 9999);
    lay->addRow(tr("Column from which item number is extracted"), odsNumColSpin);
    odsNameColSpin = new QSpinBox;
    odsNameColSpin->setRange(1, 9999);
    lay->addRow(tr("Column from which item model name is extracted"), odsNameColSpin);
    lay->setAlignment(Qt::AlignTop | Qt::AlignRight);
}

void ODSOptionsWidget::loadSettings(const QMap<QString, QVariant> &settings)
{
    odsFirstRowSpin->setValue(settings.value(odsFirstRowKey, AppSettings.getODSFirstRow()).toInt());
    odsNumColSpin->setValue(settings.value(odsNumColKey, AppSettings.getODSNumCol()).toInt());
    odsNameColSpin->setValue(settings.value(odsNameColKey, AppSettings.getODSNameCol()).toInt());
}

void ODSOptionsWidget::saveSettings(QMap<QString, QVariant> &settings)
{
    settings.insert(odsFirstRowKey, odsFirstRowSpin->value());
    settings.insert(odsNumColKey, odsNumColSpin->value());
    settings.insert(odsNameColKey, odsNameColSpin->value());
}
