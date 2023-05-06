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

#include "sqliteoptionswidget.h"

#include "utils/files/file_format_names.h"

#include <QVBoxLayout>
#include <QLabel>

SQLiteOptionsWidget::SQLiteOptionsWidget(QWidget *parent) :
    IOptionsWidget(parent)
{
    //SQLite Option
    QVBoxLayout *lay = new QVBoxLayout(this);
    lay->addWidget(new QLabel(tr("Import rollingstock pieces, models and owners from another MRTPlanner session file.\n"
                                 "The file must be a valid MRTPlanner session of recent version\n"
                                 "Extension: (*.ttt)")));
    lay->setAlignment(Qt::AlignTop | Qt::AlignRight);
}

void SQLiteOptionsWidget::loadSettings(const QMap<QString, QVariant> &/*settings*/)
{

}

void SQLiteOptionsWidget::saveSettings(QMap<QString, QVariant> &/*settings*/)
{

}

void SQLiteOptionsWidget::getFileDialogOptions(QString &title, QStringList &fileFormats)
{
    title = tr("Open Session");

    fileFormats.reserve(3);
    fileFormats << FileFormats::tr(FileFormats::tttFormat);
    fileFormats << FileFormats::tr(FileFormats::sqliteFormat);
    fileFormats << FileFormats::tr(FileFormats::allFiles);
}
