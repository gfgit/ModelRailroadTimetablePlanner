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

#ifndef LINEGRAPHSELECTIONWIDGET_H
#define LINEGRAPHSELECTIONWIDGET_H

#include <QWidget>

#include "utils/types.h"
#include "graph/linegraphtypes.h"

class QComboBox;
class QPushButton;
class CustomCompletionLineEdit;
class ISqlFKMatchModel;

/*!
 * \brief Widget to select railway station, segment or line
 *
 * Consist of a combobox and a line edit
 * The combo box selects the content type (\sa LineGraphType)
 * The line edit allows to choose which item of selected type
 * should be shown.
 */
class LineGraphSelectionWidget : public QWidget
{
    Q_OBJECT

public:
    explicit LineGraphSelectionWidget(QWidget *parent = nullptr);
    ~LineGraphSelectionWidget();

    LineGraphType getGraphType() const;
    void setGraphType(LineGraphType type);

    db_id getObjectId() const;
    const QString &getObjectName() const;
    void setObjectId(db_id objectId, const QString &name);

    void setName(const QString &newName);

signals:
    void graphChanged(int type, db_id objectId);

private slots:
    void onTypeComboActivated(int index);
    void onCompletionDone();

private:
    void setupModel(LineGraphType type);

private:
    QComboBox *graphTypeCombo;
    CustomCompletionLineEdit *objectCombo;

    ISqlFKMatchModel *matchModel;

    db_id m_objectId;
    LineGraphType m_graphType;
    QString m_name;
};

#endif // LINEGRAPHSELECTIONWIDGET_H
