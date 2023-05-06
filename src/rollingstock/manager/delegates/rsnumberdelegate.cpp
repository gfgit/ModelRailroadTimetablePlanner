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

#include "rsnumberdelegate.h"

#include "utils/model_roles.h"

#include "rsnumspinbox.h"

//TODO: remove
RsNumberDelegate::RsNumberDelegate(QObject *parent) :
    QStyledItemDelegate(parent)
{

}

QWidget *RsNumberDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &/*option*/, const QModelIndex &index) const
{
    QSpinBox *ed = nullptr;
    QVariant v = index.data(RS_IS_ENGINE);
    if(!v.toBool())
    {
        //Custom spinbox for Wagons 'XXX-X'
        ed = new RsNumSpinBox(parent);
    }
    else
    {
        //Normal spinbox for Engines
        ed = new QSpinBox(parent);
        ed->setRange(0, 9999);
    }

    return ed;
}

void RsNumberDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    QSpinBox *ed = static_cast<QSpinBox *>(editor);
    QVariant v = index.data(RS_NUMBER);
    int val = 0;
    if(v.isValid())
    {
        val = v.toInt();
    }

    ed->setValue(val);
}

void RsNumberDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    QSpinBox *ed = static_cast<QSpinBox *>(editor);
    model->setData(index, ed->value(), RS_NUMBER);
}

void RsNumberDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &/*index*/) const
{
    editor->setGeometry(option.rect);
}
