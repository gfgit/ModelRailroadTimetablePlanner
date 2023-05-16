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

#include "colordelegate.h"

#include "utils/model_roles.h"

#include <QPainter>

#include <QColorDialog>

ColorDelegate::ColorDelegate(QObject *parent) :
    QStyledItemDelegate(parent)
{

}

void ColorDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QRgb rgb = index.data(COLOR_ROLE).toUInt();
    QColor col(rgb);
    painter->fillRect(option.rect, col);
}

QSize ColorDelegate::sizeHint(const QStyleOptionViewItem &/*option*/, const QModelIndex &/*index*/) const
{
    return QSize();
}

QWidget *ColorDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &/*option*/, const QModelIndex &/*index*/) const
{
    QColorDialog *dlg = new QColorDialog(parent);
    connect(dlg, &QColorDialog::colorSelected, this, &ColorDelegate::commitAndCloseEditor);
    return dlg;
}

void ColorDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    QColorDialog *ed = static_cast<QColorDialog *>(editor);
    ed->setCurrentColor(index.data(COLOR_ROLE).toUInt());
}

void ColorDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    QColorDialog *ed = static_cast<QColorDialog *>(editor);
    model->setData(index, ed->currentColor().rgb(), COLOR_ROLE);
}

void ColorDelegate::commitAndCloseEditor()
{
    QColorDialog *ed = qobject_cast<QColorDialog *>(sender());
    ed->setEnabled(false);
    emit commitData(ed);
    emit closeEditor(ed);
}
