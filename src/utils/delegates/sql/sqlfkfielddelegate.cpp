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

#include "sqlfkfielddelegate.h"

#include "customcompletionlineedit.h"

#include "imatchmodelfactory.h"
#include "isqlfkmatchmodel.h"
#include "IFKField.h"

SqlFKFieldDelegate::SqlFKFieldDelegate(IMatchModelFactory *factory, IFKField *iface,
                                       QObject *parent) :
    QStyledItemDelegate(parent),
    mIface(iface),
    mFactory(factory)
{
}

QWidget *SqlFKFieldDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem & /*option*/,
                                          const QModelIndex & /*index*/) const
{
    ISqlFKMatchModel *model          = mFactory->createModel();
    CustomCompletionLineEdit *editor = new CustomCompletionLineEdit(model, parent);
    model->setParent(editor);
    connect(editor, &CustomCompletionLineEdit::completionDone, this,
            &SqlFKFieldDelegate::handleCompletionDone);
    return editor;
}

void SqlFKFieldDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    CustomCompletionLineEdit *ed = static_cast<CustomCompletionLineEdit *>(editor);
    db_id dataId                 = 0;
    QString name;
    if (mIface->getFieldData(index.row(), index.column(), dataId, name))
    {
        ed->setData(dataId, name);
    }
}

void SqlFKFieldDelegate::setModelData(QWidget *editor, QAbstractItemModel * /*model*/,
                                      const QModelIndex &index) const
{
    CustomCompletionLineEdit *ed = static_cast<CustomCompletionLineEdit *>(editor);
    // FIXME: use also validateData() ?
    db_id dataId = 0;
    QString name;
    ed->getData(dataId, name);
    mIface->setFieldData(index.row(), index.column(), dataId, name);
}

void SqlFKFieldDelegate::handleCompletionDone(CustomCompletionLineEdit *editor)
{
    emit commitData(editor);
    emit closeEditor(editor);
}
