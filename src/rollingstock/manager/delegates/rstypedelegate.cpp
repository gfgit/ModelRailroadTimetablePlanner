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

#include "rstypedelegate.h"

#include "utils/types.h"
#include "utils/rs_types_names.h"

#include "utils/model_roles.h"

#include <QComboBox>

RSTypeDelegate::RSTypeDelegate(bool subType, QObject *parent) :
    QStyledItemDelegate(parent),
    m_subType(subType)
{
    QStringList list;
    if (subType)
    {
        list.reserve(int(RsEngineSubType::NTypes));
        for (int i = 0; i < int(RsEngineSubType::NTypes); i++)
            list.append(RsTypeNames::name(RsEngineSubType(i)));
    }
    else
    {
        list.reserve(int(RsType::NTypes));
        for (int i = 0; i < int(RsType::NTypes); i++)
            list.append(RsTypeNames::name(RsType(i)));
    }

    comboModel.setStringList(list);
}

QWidget *RSTypeDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem & /*options*/,
                                      const QModelIndex & /*idx*/) const
{
    QComboBox *combo = new QComboBox(parent);
    combo->setModel(const_cast<QStringListModel *>(&comboModel));
    return combo;
}

void RSTypeDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    QComboBox *combo = static_cast<QComboBox *>(editor);
    int type         = index.data(m_subType ? RS_SUB_TYPE_ROLE : RS_TYPE_ROLE).toInt();
    connect(combo, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated), this,
            &RSTypeDelegate::onItemClicked);
    combo->setCurrentIndex(type);
    combo->showPopup();
}

void RSTypeDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
                                  const QModelIndex &index) const
{
    QComboBox *combo = static_cast<QComboBox *>(editor);
    int type         = combo->currentIndex();
    model->setData(index, type, m_subType ? RS_SUB_TYPE_ROLE : RS_TYPE_ROLE);
}

void RSTypeDelegate::onItemClicked()
{
    QComboBox *combo = qobject_cast<QComboBox *>(sender());
    if (combo)
    {
        emit commitData(combo);
        emit closeEditor(combo);
    }
}
