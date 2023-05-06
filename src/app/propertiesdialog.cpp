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

#include "propertiesdialog.h"

#include "app/session.h"

#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>

#include <QDir>

PropertiesDialog::PropertiesDialog(QWidget *parent) :
    QDialog(parent)
{
    setWindowTitle(tr("File Properties"));

    QGridLayout *lay = new QGridLayout(this);
    lay->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    lay->setContentsMargins(9, 9, 9, 9);

    pathLabel = new QLabel;
    lay->addWidget(pathLabel, 0, 0);

    pathReadOnlyEdit = new QLineEdit;
    lay->addWidget(pathReadOnlyEdit, 0, 1);

    pathLabel->setText(tr("File Path:"));
    pathReadOnlyEdit->setText(QDir::toNativeSeparators(Session->fileName));
    pathReadOnlyEdit->setPlaceholderText(tr("No opened file"));
    pathReadOnlyEdit->setReadOnly(true);

    //TODO: make pretty and maybe add other informations like metadata versions

    setMinimumSize(200, 200);
}
