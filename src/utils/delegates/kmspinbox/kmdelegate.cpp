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

#include "kmdelegate.h"
#include "kmspinbox.h"

KmDelegate::KmDelegate(QObject *parent) :
    QStyledItemDelegate(parent),
    minimum(0)
{

}

QWidget *KmDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &/*option*/, const QModelIndex &/*index*/) const
{
    KmSpinBox *spin = new KmSpinBox(parent);
    spin->setMinimum(minimum);
    spin->setPrefix(prefix);
    return spin;
}

void KmDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    KmSpinBox *spin = static_cast<KmSpinBox *>(editor);
    spin->setValue(index.data(Qt::EditRole).toInt());
}

void KmDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    KmSpinBox *spin = static_cast<KmSpinBox *>(editor);
    model->setData(index, spin->value(), Qt::EditRole);
}

void KmDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &/*index*/) const
{
    editor->setGeometry(option.rect);
}

void KmDelegate::setMinAndPrefix(int min, const QString &pref)
{
    minimum = min;
    prefix = pref;
}
