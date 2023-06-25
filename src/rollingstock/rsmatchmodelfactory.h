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

#ifndef RSMATCHMODELFACTORY_H
#define RSMATCHMODELFACTORY_H

#include "utils/delegates/sql/imatchmodelfactory.h"

#include "utils/model_mode.h"

namespace sqlite3pp {
class database;
}

class RSMatchModelFactory : public IMatchModelFactory
{
public:
    RSMatchModelFactory(ModelModes::Mode mode, sqlite3pp::database &db, QObject *parent);

    ISqlFKMatchModel *createModel() override;

private:
    sqlite3pp::database &mDb;
    ModelModes::Mode m_mode;
};

#endif // RSMATCHMODELFACTORY_H
