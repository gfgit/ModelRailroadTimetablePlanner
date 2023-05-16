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

#include "combodelegate.h"
#include "app/scopedebug.h"

#include <QComboBox>

ComboDelegate::ComboDelegate(const QStringList &list, int role, QObject *parent) :
    QStyledItemDelegate(parent),
    mList(list),
    mRole(role)
{
    //DEBUG_ENTRY;
    qDebug() << mList;
}

QWidget *ComboDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &/*option*/, const QModelIndex &/*index*/) const
{
    //DEBUG_ENTRY;
    QComboBox *combo = new QComboBox(parent);
    connect(combo, static_cast<void(QComboBox::*)(int)>(&QComboBox::activated), this, &ComboDelegate::onItemClicked);
    combo->addItems(mList);
    return combo;
}

void ComboDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    //DEBUG_ENTRY;
    QComboBox *combo = static_cast<QComboBox*>(editor);
    QVariant v = index.model()->data(index, mRole);
    if(v.isValid())
    {
        int val = v.toInt();
        combo->setCurrentIndex(val);
    }
    else
    {
        combo->setCurrentIndex(0); //TODO??? -1 --> 0 ?
    }
    combo->showPopup();
}

void ComboDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    //DEBUG_ENTRY;
    QComboBox *combo = static_cast<QComboBox*>(editor);
    int val = combo->currentIndex();
    model->setData(index, val, mRole);
    //QString text = combo->currentText();
    //model->setData(index, text, Qt::DisplayRole);
}

void ComboDelegate::onItemClicked()
{
    QComboBox *combo = qobject_cast<QComboBox *>(sender());
    if(combo)
    {
        emit commitData(combo);
        emit closeEditor(combo);
    }
}

void ComboDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &/*index*/) const
{
    editor->setGeometry(option.rect);
}
