#include "shiftgraphscene.h"

#include "app/session.h"

#include "utils/jobcategorystrings.h"

#include <sqlite3pp/sqlite3pp.h>
using namespace sqlite3pp;

#include <QPainter>
#include "utils/font_utils.h"

#include <QtMath>

static constexpr const char *sql_getStName = "SELECT name,short_name FROM stations"
                                             " WHERE id=?";

static constexpr const char *sql_countJobs = "SELECT COUNT(1) FROM jobs"
                                             " WHERE jobs.shift_id=?";

static constexpr const char *sql_getJobs = "SELECT jobs.id, jobs.category,"
                                           " MIN(s1.arrival), s1.station_id,"
                                           " MAX(s2.departure), s2.station_id"
                                           " FROM jobs"
                                           " JOIN stops s1 ON s1.job_id=jobs.id"
                                           " JOIN stops s2 ON s2.job_id=jobs.id"
                                           " WHERE jobs.shift_id=?"
                                           " GROUP BY jobs.id"
                                           " ORDER BY s1.arrival ASC";

ShiftGraphScene::ShiftGraphScene(sqlite3pp::database &db, QObject *parent) :
    QObject(parent),
    mDb(db)
{
    connect(&AppSettings, &MRTPSettings::jobColorsChanged,
            this, &ShiftGraphScene::redrawGraph);

    connect(&AppSettings, &MRTPSettings::shiftGraphOptionsChanged,
            this, &ShiftGraphScene::updateShiftGraphOptions);

    connect(Session, &MeetingSession::stationNameChanged, this, &ShiftGraphScene::onStationNameChanged);

    connect(Session, &MeetingSession::shiftNameChanged, this, &ShiftGraphScene::onShiftNameChanged);
    connect(Session, &MeetingSession::shiftRemoved, this, &ShiftGraphScene::onShiftRemoved);
    connect(Session, &MeetingSession::shiftJobsChanged, this, &ShiftGraphScene::onShiftJobsChanged);

    updateShiftGraphOptions();
}

void ShiftGraphScene::drawShifts(QPainter *painter, const QRectF &sceneRect)
{
    QTextOption jobTextOpt(Qt::AlignCenter);
    QTextOption fromStationTextOpt(Qt::AlignVCenter | Qt::AlignLeft);
    QTextOption toStationTextOpt(Qt::AlignVCenter | Qt::AlignRight);

    QFont jobFont;
    setFontPointSizeDPI(jobFont, 15, painter);

    QFont destStFont = jobFont;
    destStFont.setItalic(true);

    QPen jobPen;
    jobPen.setWidth(5);
    const double penMargin = jobPen.widthF() * 0.6;

    //Transparent white as background to make text more readable
    QColor textBGCol(255, 255, 255, 220);

    QPen textPen(Qt::black, 2);

    qreal y = vertOffset + rowSpaceOffset / 2;
    for(const ShiftGraph& shift : qAsConst(m_shifts))
    {
        const qreal top = y;
        qreal jobY = top + shiftRowHeight / 2;
        qreal bottomY = top + shiftRowHeight;
        y = bottomY + rowSpaceOffset;

        if(bottomY < sceneRect.top())
            continue; //Shift is above requested view
        if(top > sceneRect.bottom())
            break; //Shift is below requested view and next will be out too

        db_id lastStId = 0;

        double prevJobNameLastX = horizOffset;
        double prevStationNameLastX = horizOffset;

        for(const JobItem& item : shift.jobList)
        {
            double firstX = jobPos(item.start);
            double lastX = jobPos(item.end);

            if(lastX < sceneRect.left())
                continue; //Job is at left of requested view

            if(firstX > sceneRect.right())
                break; //Next Jobs are after in time so they will be out too

            jobPen.setColor(Session->colorForCat(item.job.category));
            painter->setPen(jobPen);

            //Draw Job line
            painter->drawLine(firstX, jobY, lastX, jobY);

            painter->setPen(textPen);
            painter->setFont(jobFont);

            //Draw Job Name above line
            const QString jobName = JobCategoryName::jobName(item.job.jobId, item.job.category);

            QRectF textRect(firstX, top, lastX - firstX, shiftRowHeight / 4 - penMargin);

            QRectF jobNameRect = painter->boundingRect(textRect, jobName, jobTextOpt);
            if(jobNameRect.left() < horizOffset)
                jobNameRect.moveLeft(horizOffset); //Do not go under ShiftGraphNameHeader

            if(jobNameRect.left() > prevJobNameLastX || prevJobNameLastX == horizOffset)
            {
                //We do not collide with previous Job Name
                //So we can take lower label row
                jobNameRect.moveBottom(jobY - penMargin);

                //Store last edge for next Job Name
                prevJobNameLastX = jobNameRect.right();
            }

            painter->fillRect(jobNameRect, textBGCol);
            painter->drawText(jobNameRect, jobName, jobTextOpt);

            //Draw Station names below line
            textRect.moveTop(jobY + jobPen.widthF());

            if(!hideSameStations || lastStId != item.fromStId)
            {
                //Origin is different from previous Job Destination
                QString stName = m_stationCache.value(item.fromStId, StationCache()).shortNameOrFallback;

                QRectF stationLabelRect = painter->boundingRect(textRect, stName, fromStationTextOpt);
                if(stationLabelRect.width() > textRect.width())
                {
                    //Try to align rigth (so move to left because we are longer than Job)
                    double newLeft = qMax(prevStationNameLastX, lastX - stationLabelRect.width());
                    stationLabelRect.moveLeft(newLeft);
                }
                prevStationNameLastX = stationLabelRect.right();

                painter->fillRect(stationLabelRect.adjusted(0, 1, 0, -1), textBGCol);
                painter->drawText(stationLabelRect, stName, fromStationTextOpt);
            }

            //Draw destination station
            QString stName = m_stationCache.value(item.toStId, StationCache()).shortNameOrFallback;

            painter->setFont(destStFont);
            QRectF stationLabelRect = painter->boundingRect(textRect, stName, toStationTextOpt);
            if(stationLabelRect.left() < prevStationNameLastX)
            {
                //We collide with previous station, move lower
                stationLabelRect.moveBottom(bottomY);
            }
            prevStationNameLastX = stationLabelRect.right();

            painter->fillRect(stationLabelRect.adjusted(0, 1, 0, -1), textBGCol);
            painter->drawText(stationLabelRect, stName, toStationTextOpt);

            lastStId = item.toStId;
        }

        //Draw line to separate from previous row
        painter->setPen(textPen);
        qreal sepLineY = bottomY + rowSpaceOffset / 2;
        painter->drawLine(QLineF(sceneRect.right(), sepLineY, sceneRect.left(), sepLineY));
    }
}

void ShiftGraphScene::drawHourLines(QPainter *painter, const QRectF &sceneRect)
{
    QPen hourTextPen(Qt::darkGray, 2);
    painter->setPen(hourTextPen);

    qreal top = qMax(sceneRect.top(), vertOffset);
    qreal bottom = sceneRect.bottom();

    qreal x = horizOffset;
    for(int h = 0; h <= 24; h++)
    {
        qreal left = x;
        x += hourOffset;

        if(left < sceneRect.left())
            continue;
        if(left > sceneRect.right())
            break;

        painter->drawLine(QLineF(left, top, left, bottom));
    }
}

void ShiftGraphScene::drawShiftHeader(QPainter *painter, const QRectF &rect, double vertScroll)
{
    QTextOption shiftTextOpt(Qt::AlignCenter);

    QFont shiftFont;
    shiftFont.setBold(true);
    setFontPointSizeDPI(shiftFont, 25, painter);
    painter->setFont(shiftFont);

    QPen shiftPen(Qt::red);
    painter->setPen(shiftPen);

    QRectF labelRect = rect;
    labelRect.setHeight(shiftRowHeight);

    qreal y = vertOffset + rowSpaceOffset / 2 - vertScroll;
    for(const ShiftGraph& shift : qAsConst(m_shifts))
    {
        const qreal top = y;
        qreal bottomY = top + shiftRowHeight;
        y = bottomY + rowSpaceOffset;

        if(bottomY < rect.top())
            continue; //Shift is above requested view
        if(top > rect.bottom())
            break; //Shift is below requested view and next will be out too

        labelRect.moveTop(top);
        painter->drawText(labelRect, shift.shiftName, shiftTextOpt);
    }
}

void ShiftGraphScene::drawHourHeader(QPainter *painter, const QRectF &rect, double horizScroll)
{
    QTextOption hourTextOpt(Qt::AlignCenter);

    QFont hourTextFont;
    setFontPointSizeDPI(hourTextFont, 20, painter);

    QPen hourTextPen(AppSettings.getHourTextColor());

    painter->setFont(hourTextFont);
    painter->setPen(hourTextPen);

    const QString fmt(QStringLiteral("%1:00"));

    QRectF labelRect = rect;
    labelRect.setWidth(hourOffset);

    qreal x = horizOffset - hourOffset / 2 - horizScroll;

    for(int h = 0; h <= 24; h++)
    {
        qreal left = x;
        qreal right = left + hourOffset;
        x = right;

        if(right < rect.left())
            continue;
        if(left > rect.right())
            break;

        labelRect.moveLeft(left);
        painter->drawText(labelRect, fmt.arg(h), hourTextOpt);
    }
}

QSize ShiftGraphScene::getContentSize() const
{
    //Set a margin after last hour of half hour offset
    double width = horizOffset + 24 * hourOffset + hourOffset / 2;
    double height = vertOffset + (shiftRowHeight + rowSpaceOffset) * m_shifts.count();
    return QSize(qRound(width), qRound(height));
}

JobEntry ShiftGraphScene::getJobAt(const QPointF &scenePos) const
{
    auto pos = getJobItemAt(scenePos);
    if(pos.first < 0 || pos.second < 0)
        return {};

    return m_shifts.at(pos.first).jobList.at(pos.second).job;
}

QString ShiftGraphScene::getTooltipAt(const QPointF &scenePos) const
{
    auto pos = getJobItemAt(scenePos);
    if(pos.first < 0 || pos.second < 0)
        return {};

    const ShiftGraph& shift = m_shifts.at(pos.first);
    const JobItem item = shift.jobList.at(pos.second);

    StationCache fromSt = m_stationCache.value(item.fromStId, StationCache());
    StationCache toSt = m_stationCache.value(item.toStId, StationCache());

    return tr("<table>"
              "<tr>"
              "<td>Job:</td>"
              "<td><b>%1</b></td>"
              "</tr>"
              "<tr>"
              "<td>Shift:</td>"
              "<td><b>%2</b></td>"
              "</tr>"
              "<tr>"
              "<td>From:</td>"
              "<td><b>%3</b></td>"
              "<td>at <b>%4</b></td>"
              "</tr>"
              "<tr>"
              "<td>To:</td>"
              "<td><b>%5</b></td>"
              "<td>at <b>%6</b></td>"
              "</tr>"
              "</table>")
        .arg(JobCategoryName::jobName(item.job.jobId, item.job.category),
             shift.shiftName,
             fromSt.name, item.start.toString("HH:mm"),
             toSt.name, item.end.toString("HH:mm"));
}

bool ShiftGraphScene::loadShifts()
{
    m_shifts.clear();

    m_stationCache.clear();
    m_stationCache.squeeze();

    query q(mDb, "SELECT COUNT(1) FROM jobshifts");
    q.step();
    int count = q.getRows().get<int>(0);

    if(count == 0)
    {
        //Nothing to load
        m_shifts.squeeze();
        return true;
    }

    m_shifts.reserve(count);

    query q_getStName(mDb, sql_getStName);
    query q_countJobs(mDb, sql_countJobs);
    query q_getJobs(mDb, sql_getJobs);

    q.prepare("SELECT id,name FROM jobshifts ORDER BY name");
    for(auto shift : q)
    {
        ShiftGraph obj;
        obj.shiftId = shift.get<db_id>(0);
        obj.shiftName = shift.get<QString>(1);

        loadShiftRow(obj, q_getStName, q_countJobs, q_getJobs);
        m_shifts.append(obj);
    }

    emit redrawGraph();

    return true;
}

void ShiftGraphScene::updateShiftGraphOptions()
{
    hourOffset  = AppSettings.getShiftHourOffset();
    horizOffset = AppSettings.getShiftHorizOffset();
    vertOffset  = AppSettings.getShiftVertOffset();

    shiftRowHeight = AppSettings.getShiftJobRowHeight();
    rowSpaceOffset = AppSettings.getShiftJobRowSpace();
    //jobBoxOffset = AppSettings.getShiftJobBoxOffset();
    //stationNameOffset = AppSettings.getShiftStationOffset();
    hideSameStations = AppSettings.getShiftHideSameStations();

    emit redrawGraph();
}

void ShiftGraphScene::onShiftNameChanged(db_id shiftId)
{
    query q(mDb, "SELECT name FROM jobshifts WHERE id=?");
    q.bind(1, shiftId);
    if(q.step() != SQLITE_ROW)
        return;

    QString newName = q.getRows().get<QString>(0);

    auto pos = lowerBound(shiftId, newName);

    if(pos.first == -1)
    {
        //This shift is new, add it
        ShiftGraph obj;
        obj.shiftId = shiftId;
        obj.shiftName = newName;

        //Load jobs
        query q_getStName(mDb, sql_getStName);
        query q_countJobs(mDb, sql_countJobs);
        query q_getJobs(mDb, sql_getJobs);
        loadShiftRow(obj, q_getStName, q_countJobs, q_getJobs);

        m_shifts.insert(pos.second, obj);
    }
    else
    {
        //Move shift to keep alphabetical order
        m_shifts[pos.first].shiftName = newName;
        if(pos.second > pos.first)
            pos.second--;
        m_shifts.move(pos.first, pos.second);
    }

    emit redrawGraph();
}

void ShiftGraphScene::onShiftRemoved(db_id shiftId)
{
    for(auto it = m_shifts.begin(); it != m_shifts.end(); it++)
    {
        if(it->shiftId == shiftId)
        {
            m_shifts.erase(it);
            break;
        }
    }

    emit redrawGraph();
}

void ShiftGraphScene::onShiftJobsChanged(db_id shiftId)
{
    //Reload single shift
    query q_getStName(mDb, sql_getStName);
    query q_countJobs(mDb, sql_countJobs);
    query q_getJobs(mDb, sql_getJobs);

    for(ShiftGraph& shift : m_shifts)
    {
        if(shift.shiftId == shiftId)
        {
            loadShiftRow(shift, q_getStName, q_countJobs, q_getJobs);
            break;
        }
    }

    emit redrawGraph();
}

void ShiftGraphScene::onStationNameChanged(db_id stationId)
{
    if(m_stationCache.contains(stationId))
    {
        //Reload name
        m_stationCache.remove(stationId);

        query q_getStName(mDb, sql_getStName);
        loadStationName(stationId, q_getStName);

        emit redrawGraph();
    }
}


bool ShiftGraphScene::loadShiftRow(ShiftGraph &shiftObj, query &q_getStName, query &q_countJobs, sqlite3pp::query &q_getJobs)
{
    shiftObj.jobList.clear();

    q_countJobs.bind(1, shiftObj.shiftId);
    q_countJobs.step();
    int count = q_countJobs.getRows().get<int>(0);
    q_countJobs.reset();

    if(count == 0)
    {
        //Nothing to load
        return true;
    }

    shiftObj.jobList.reserve(count);

    q_getJobs.bind(1, shiftObj.shiftId);
    for(auto job : q_getJobs)
    {
        JobItem item;

        item.job.jobId = job.get<db_id>(0);
        item.job.category = JobCategory(job.get<int>(1));

        item.start = job.get<QTime>(2);
        item.fromStId = job.get<db_id>(3);

        item.end = job.get<QTime>(4);
        item.toStId = job.get<db_id>(5);

        loadStationName(item.fromStId, q_getStName);
        loadStationName(item.toStId, q_getStName);

        shiftObj.jobList.append(item);
    }
    q_getJobs.reset();

    return true;
}

void ShiftGraphScene::loadStationName(db_id stationId, sqlite3pp::query &q_getStName)
{
    if(!m_stationCache.contains(stationId))
    {
        q_getStName.bind(1, stationId);
        if(q_getStName.step() != SQLITE_ROW)
            return;
        auto r = q_getStName.getRows();

        StationCache st;
        st.name = r.get<QString>(0);
        st.shortNameOrFallback = r.get<QString>(1);

        //If 'Short Name' is empty fallback to 'Full Name'
        if(st.shortNameOrFallback.isEmpty())
            st.shortNameOrFallback = st.name;

        m_stationCache.insert(stationId, st);
        q_getStName.reset();
    }
}

std::pair<int, int> ShiftGraphScene::lowerBound(db_id shiftId, const QString &name)
{
    //Find old shift position
    int oldIdx = -1;
    for(int i = 0; i < m_shifts.size(); i++)
    {
        if(m_shifts.at(i).shiftId == shiftId)
        {
            oldIdx = i;
            break;
        }
    }

    //Find new position to insert
    int firstIdx = 0;
    int len = m_shifts.size();

    while (len > 0)
    {
        int half = len >> 1;
        int middleIdx = firstIdx + half;

        const ShiftGraph& obj = m_shifts.at(middleIdx);
        if(obj.shiftId != shiftId && obj.shiftName < name)
        {
            firstIdx = middleIdx + 1;
            len = len - half - 1;
        }
        else
        {
            len = half;
        }
    }

    return {oldIdx, firstIdx};
}

std::pair<int, int> ShiftGraphScene::getJobItemAt(const QPointF &scenePos) const
{
    std::pair<int, int> ret{-1, -1};

    const qreal y = scenePos.y() - vertOffset - rowSpaceOffset / 2;
    int shiftIdx = qFloor(y / (shiftRowHeight + rowSpaceOffset));
    if(shiftIdx < 0 || shiftIdx >= m_shifts.size())
        return ret;

    const qreal x = scenePos.x() - horizOffset;
    if(x < 0 || x > 24 * hourOffset)
        return ret;

    QTime t = QTime::fromMSecsSinceStartOfDay(qFloor(x / hourOffset * MSEC_PER_HOUR));

    const ShiftGraph& shift = m_shifts.at(shiftIdx);

    int jobIdx = 0;
    for(const JobItem& item : shift.jobList)
    {
        if(item.start <= t && item.end >= t)
        {
            ret.first = shiftIdx;
            ret.second = jobIdx;
            break;
        }

        jobIdx++;
    }

    return ret;
}
