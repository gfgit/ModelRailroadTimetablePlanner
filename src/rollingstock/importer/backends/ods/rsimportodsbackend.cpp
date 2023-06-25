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

#include "rsimportodsbackend.h"

#include "odsoptionswidget.h"
#include "loadodstask.h"

#include "utils/files/file_format_names.h"

RSImportODSBackend::RSImportODSBackend()
{
}

QString RSImportODSBackend::getBackendName()
{
    return FileFormats::tr(FileFormats::odsFormat);
}

IOptionsWidget *RSImportODSBackend::createOptionsWidget()
{
    return new ODSOptionsWidget;
}

ILoadRSTask *RSImportODSBackend::createLoadTask(const QMap<QString, QVariant> &arguments,
                                                sqlite3pp::database &db, int mode, int defSpeed,
                                                RsType defType, const QString &fileName,
                                                QObject *receiver)
{
    return new LoadODSTask(arguments, db, mode, defSpeed, defType, fileName, receiver);
}
