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

#include "languagemodel.h"

#include "utils/localization/languageutils.h"

LanguageModel::LanguageModel(QObject *parent) :
    QAbstractListModel(parent)
{
}

QVariant LanguageModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
    {
        if (section == 0)
            return tr("Language");
    }

    return QAbstractListModel::headerData(section, orientation, role);
}

int LanguageModel::rowCount(const QModelIndex &p) const
{
    return p.isValid() ? 0 : m_data.size();
}

QVariant LanguageModel::data(const QModelIndex &idx, int role) const
{
    if (!idx.isValid() || idx.row() >= m_data.size())
        return QVariant();

    const QLocale &loc = m_data.at(idx.row());

    switch (role)
    {
    case Qt::DisplayRole:
    {
        // NOTE: Do not show "American English" for default locale (row 0)
        if (idx.row() > 0)
            return loc.nativeLanguageName();
        Q_FALLTHROUGH();
    }
    case Qt::ToolTipRole:
        return QLocale::languageToString(loc.language()); // Return english name in tooltip
    }

    return QVariant();
}

void LanguageModel::loadAvailableLanguages()
{
    beginResetModel();
    m_data = utils::language::getAvailableTranslations();
    endResetModel();
}

QLocale LanguageModel::getLocaleAt(int idx)
{
    return m_data.value(idx, QLocale::c());
}

int LanguageModel::findMatchingRow(const QLocale &loc)
{
    int exactMatchIdx   = -1;
    int partialMatchIdx = -1;
    for (int i = 0; i < m_data.size(); i++)
    {
        const QLocale &item = m_data.at(i);
        if (item.language() == loc.language())
        {
            partialMatchIdx = i;

            if (item.territory() == loc.territory())
            {
                exactMatchIdx = i;
                break;
            }
        }
    }

    if (exactMatchIdx != -1)
        return exactMatchIdx;

    if (partialMatchIdx != -1)
        return partialMatchIdx;

    return 0;
}
