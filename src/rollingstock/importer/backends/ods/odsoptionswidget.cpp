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

#include "odsoptionswidget.h"
#include "options.h"

#include "utils/files/file_format_names.h"

#include <QFormLayout>
#include <QLabel>
#include <QSpinBox>

#include "app/session.h"

ODSOptionsWidget::ODSOptionsWidget(QWidget *parent) :
    IOptionsWidget(parent)
{
    // ODS Option
    QFormLayout *lay = new QFormLayout(this);
    lay->addRow(
      new QLabel(tr("Import rollingstock pieces, models and owners from a spreadsheet file.\n"
                    "The file must be a valid Open Document Format Spreadsheet V1.2\n"
                    "Extension: (*.ods)")));
    odsFirstRowSpin = new QSpinBox;
    odsFirstRowSpin->setRange(1, 9999);
    lay->addRow(tr("First non-empty row that contains rollingstock piece information"),
                odsFirstRowSpin);
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

void ODSOptionsWidget::getFileDialogOptions(QString &title, QStringList &fileFormats)
{
    title = tr("Open Spreadsheet");

    fileFormats.reserve(2);
    fileFormats << FileFormats::tr(FileFormats::odsFormat);
    fileFormats << FileFormats::tr(FileFormats::allFiles);
}
