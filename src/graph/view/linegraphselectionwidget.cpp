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

#include "linegraphselectionwidget.h"

#include <QComboBox>
#include <QPushButton>

#include "utils/delegates/sql/customcompletionlineedit.h"

#include "stations/match_models/stationsmatchmodel.h"
#include "stations/match_models/railwaysegmentmatchmodel.h"
#include "stations/match_models/linesmatchmodel.h"

#include "app/session.h"

#include <QHBoxLayout>

LineGraphSelectionWidget::LineGraphSelectionWidget(QWidget *parent) :
    QWidget(parent),
    matchModel(nullptr),
    m_objectId(0),
    m_graphType(LineGraphType::NoGraph)
{
    QHBoxLayout *lay = new QHBoxLayout(this);
    lay->setContentsMargins(0, 0, 0, 0);

    graphTypeCombo = new QComboBox;
    lay->addWidget(graphTypeCombo);

    objectCombo = new CustomCompletionLineEdit(nullptr, this);
    lay->addWidget(objectCombo);

    QStringList items;
    items.reserve(int(LineGraphType::NTypes));
    for (int i = 0; i < int(LineGraphType::NTypes); i++)
        items.append(utils::getLineGraphTypeName(LineGraphType(i)));
    graphTypeCombo->addItems(items);
    graphTypeCombo->setCurrentIndex(0);

    connect(graphTypeCombo, qOverload<int>(&QComboBox::activated), this,
            &LineGraphSelectionWidget::onTypeComboActivated);
    connect(objectCombo, &CustomCompletionLineEdit::completionDone, this,
            &LineGraphSelectionWidget::onCompletionDone);

    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
}

LineGraphSelectionWidget::~LineGraphSelectionWidget()
{
    if (matchModel)
    {
        objectCombo->setModel(nullptr);
        delete matchModel;
        matchModel = nullptr;
    }
}

LineGraphType LineGraphSelectionWidget::getGraphType() const
{
    return LineGraphType(graphTypeCombo->currentIndex());
}

void LineGraphSelectionWidget::setGraphType(LineGraphType type)
{
    if (getGraphType() == type)
        return;

    graphTypeCombo->setCurrentIndex(int(type));
    setupModel(type);

    emit graphChanged(int(m_graphType), m_objectId);
}

db_id LineGraphSelectionWidget::getObjectId() const
{
    return m_objectId;
}

const QString &LineGraphSelectionWidget::getObjectName() const
{
    return m_name;
}

void LineGraphSelectionWidget::setObjectId(db_id objectId, const QString &name)
{
    if (m_graphType == LineGraphType::NoGraph)
        return; // Object ID must be null

    m_name = name;
    if (!objectId)
        m_name.clear();

    objectCombo->setData(objectId, name);

    if (m_objectId != objectId)
        emit graphChanged(int(m_graphType), m_objectId);
}

void LineGraphSelectionWidget::onTypeComboActivated(int index)
{
    setupModel(LineGraphType(index));
    emit graphChanged(int(m_graphType), m_objectId);
}

void LineGraphSelectionWidget::onCompletionDone()
{
    if (!objectCombo->getData(m_objectId, m_name))
        return;

    emit graphChanged(int(m_graphType), m_objectId);
}

void LineGraphSelectionWidget::setupModel(LineGraphType type)
{
    if (type != m_graphType)
    {
        // Clear old model
        if (matchModel)
        {
            objectCombo->setModel(nullptr);
            delete matchModel;
            matchModel = nullptr;
        }

        // Manually clear line edit
        m_objectId = 0;
        m_name.clear();
        objectCombo->setData(m_objectId, m_name);

        switch (LineGraphType(type))
        {
        case LineGraphType::NoGraph:
        default:
        {
            // Prevent recursion on loadGraph() calling back to us
            type = LineGraphType::NoGraph;
            break;
        }
        case LineGraphType::SingleStation:
        {
            StationsMatchModel *m = new StationsMatchModel(Session->m_Db, this);
            m->setFilter(0);
            matchModel = m;
            break;
        }
        case LineGraphType::RailwaySegment:
        {
            RailwaySegmentMatchModel *m = new RailwaySegmentMatchModel(Session->m_Db, this);
            m->setFilter(0, 0, 0);
            matchModel = m;
            break;
        }
        case LineGraphType::RailwayLine:
        {
            LinesMatchModel *m = new LinesMatchModel(Session->m_Db, true, this);
            matchModel         = m;
            break;
        }
        }

        if (matchModel)
            objectCombo->setModel(matchModel);
    }
    m_graphType = type;
}
