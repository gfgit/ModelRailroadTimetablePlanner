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

#ifndef SQLFKFIELDDELEGATE_H
#define SQLFKFIELDDELEGATE_H

#include <QStyledItemDelegate>

class IFKField;
class IMatchModelFactory;
class CustomCompletionLineEdit;

class SqlFKFieldDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    SqlFKFieldDelegate(IMatchModelFactory *factory, IFKField *iface, QObject *parent = nullptr);

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                          const QModelIndex &index) const override;
    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    void setModelData(QWidget *editor, QAbstractItemModel *model,
                      const QModelIndex &index) const override;

private slots:
    void handleCompletionDone(CustomCompletionLineEdit *editor);

private:
    IFKField *mIface;
    IMatchModelFactory *mFactory;
};

#endif // SQLFKFIELDDELEGATE_H
