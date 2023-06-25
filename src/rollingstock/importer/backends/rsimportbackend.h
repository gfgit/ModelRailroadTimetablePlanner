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

#ifndef RSIMPORTBACKEND_H
#define RSIMPORTBACKEND_H

#include <QString>
#include <QVariant>

#include "utils/types.h"

class QObject;
class IOptionsWidget;
class ILoadRSTask;

namespace sqlite3pp {
class database;
}

class RSImportBackend
{
public:
    RSImportBackend();
    virtual ~RSImportBackend();

    virtual QString getBackendName()                       = 0;

    virtual IOptionsWidget *createOptionsWidget()          = 0;

    virtual ILoadRSTask *createLoadTask(const QMap<QString, QVariant> &arguments,
                                        sqlite3pp::database &db, int mode, int defSpeed,
                                        RsType defType, const QString &fileName,
                                        QObject *receiver) = 0;
};

#endif // RSIMPORTBACKEND_H
