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

#include "backgroundhelper.h"

#include "app/session.h"

#include  "graph/model/linegraphscene.h"
#include "utils/jobcategorystrings.h"

#include <QPainter>
#include "utils/font_utils.h"

#include <QtMath>

#include <QDebug>

void BackgroundHelper::drawHourPanel(QPainter *painter, const QRectF& rect)
{
    //TODO: settings
    QFont hourTextFont;
    setFontPointSizeDPI(hourTextFont, 15, painter);

    QPen hourTextPen(AppSettings.getHourTextColor());

    const int vertOffset = Session->vertOffset;
    const int hourOffset = Session->hourOffset;

    painter->setFont(hourTextFont);
    painter->setPen(hourTextPen);

    //qDebug() << "Drawing hours..." << rect << scroll;
    const QString fmt(QStringLiteral("%1:00"));

    const qreal top = rect.top() - vertOffset;
    const qreal bottom = rect.bottom();

    int h = qFloor(top / hourOffset);
    if(h < 0)
        h = 0;

    QRectF labelRect = rect;
    labelRect.setWidth(labelRect.width() * 0.9);
    labelRect.setHeight(hourOffset);
    labelRect.moveTop(h * hourOffset + vertOffset - hourOffset / 2);

    for(; h <= 24 && labelRect.top() <= bottom; h++)
    {
        //qDebug() << "Y:" << y << fmt.arg(h);
        painter->drawText(labelRect, fmt.arg(h), QTextOption(Qt::AlignVCenter | Qt::AlignRight));
        labelRect.moveTop(labelRect.top() + hourOffset);
    }
}

void BackgroundHelper::drawBackgroundHourLines(QPainter *painter, const QRectF &rect)
{
    const double horizOffset = Session->horizOffset;
    const double vertOffset = Session->vertOffset;
    const double hourOffset = Session->hourOffset;

    QPen hourLinePen(AppSettings.getHourLineColor(), AppSettings.getHourLineWidth());

    const qreal x1 = qMax(qreal(horizOffset), rect.left());
    const qreal x2 = rect.right();
    const qreal t = qMax(rect.top(), vertOffset);
    const qreal b = rect.bottom();


    if(x1 > x2 || b < vertOffset || t > b)
        return;

    int firstH = qCeil((t - vertOffset) / hourOffset);
    int lastH = qFloor((b - vertOffset) / hourOffset);

    if(firstH > 24 || lastH < 0)
        return;

    if(firstH < 0)
        firstH = 0;
    if(lastH > 24)
        lastH = 24;

    const int n = lastH - firstH + 1;
    if(n <= 0)
        return;

    qreal y = vertOffset + firstH * hourOffset;

    QLineF *arr = new QLineF[n];
    for(int i = 0; i < n; i++)
    {
        arr[i] = QLineF(x1, y, x2, y);
        y += hourOffset;
    }

    painter->setPen(hourLinePen);
    painter->drawLines(arr, n);
    delete [] arr;
}

void BackgroundHelper::drawStationHeader(QPainter *painter, LineGraphScene *scene, const QRectF &rect)
{
    QFont stationFont;
    stationFont.setBold(true);
    setFontPointSizeDPI(stationFont, 25, painter);

    QPen stationPen(AppSettings.getStationTextColor());

    QFont platfBoldFont = stationFont;
    setFontPointSizeDPI(platfBoldFont, 16, painter);

    QFont platfNormalFont = platfBoldFont;
    platfNormalFont.setBold(false);

    QPen electricPlatfPen(Qt::blue);
    QPen nonElectricPlatfPen(Qt::black);

    const qreal platformOffset = Session->platformOffset;
    const int stationOffset = Session->stationOffset;

    //On left go back by half station offset to center station label
    //and center platform label by going a back of half platformOffset
    const int leftOffset = -stationOffset/2 - platformOffset /2;

    const double margin = stationOffset * 0.1;

    QRectF r = rect;

    for(auto st : qAsConst(scene->stations))
    {
        const double left = st.xPos + leftOffset;
        const double right = left + st.platforms.count() * platformOffset + stationOffset;

        if(right < r.left() || left >= r.right())
            continue; //Skip station, it's not visible

        QRectF labelRect = r;
        labelRect.setLeft(left + margin);
        labelRect.setRight(right - margin);
        labelRect.setBottom(r.bottom() - r.height() / 3);

        painter->setPen(stationPen);
        painter->setFont(stationFont);
        painter->drawText(labelRect, Qt::AlignVCenter | Qt::AlignCenter, st.stationName);

        labelRect = r;
        labelRect.setTop(r.top() + r.height() * 2/3);

        //Go to start of station (first platform)
        //We need to compensate the half stationOffset used to center station label
        double xPos = left + stationOffset/2;
        labelRect.setWidth(platformOffset);
        for(const StationGraphObject::PlatformGraph& platf : qAsConst(st.platforms))
        {
            if(platf.platformType.testFlag(utils::StationTrackType::Electrified))
                painter->setPen(electricPlatfPen);
            else
                painter->setPen(nonElectricPlatfPen);

            if(platf.platformType.testFlag(utils::StationTrackType::Through))
                painter->setFont(platfBoldFont);
            else
                painter->setFont(platfNormalFont);

            labelRect.moveLeft(xPos);

            painter->drawText(labelRect, Qt::AlignCenter, platf.platformName);

            xPos += platformOffset;
        }
    }
}

void BackgroundHelper::drawStations(QPainter *painter, LineGraphScene *scene, const QRectF &rect)
{
    const QRgb white = qRgb(255, 255, 255);

    //const int horizOffset = Session->horizOffset;
    const int vertOffset = Session->vertOffset;
    //const int stationOffset = Session->stationOffset;
    const double platfOffset = Session->platformOffset;
    const int lastY = vertOffset + Session->hourOffset * 24 + 10;

    const int width = AppSettings.getPlatformLineWidth();
    const QColor mainPlatfColor = AppSettings.getMainPlatfColor();

    QPen platfPen (mainPlatfColor,  width);

    QPointF top(0, vertOffset);
    QPointF bottom(0, lastY);

    for(const StationGraphObject &st : qAsConst(scene->stations))
    {
        const double left = st.xPos;
        const double right = left + st.platforms.count() * platfOffset;

        if(left > rect.right() || right < rect.left())
            continue; //Skip station, it's not visible

        top.rx() = bottom.rx() = st.xPos;

        for(const StationGraphObject::PlatformGraph& platf : st.platforms)
        {
            if(platf.color == white)
                platfPen.setColor(mainPlatfColor);
            else
                platfPen.setColor(platf.color);

            painter->setPen(platfPen);

            painter->drawLine(top, bottom);

            top.rx() += platfOffset;
            bottom.rx() += platfOffset;
        }
    }
}

void BackgroundHelper::drawJobStops(QPainter *painter, LineGraphScene *scene, const QRectF &rect, bool drawSelection)
{
    const double platfOffset = Session->platformOffset;
    const double stationOffset = Session->stationOffset;

    QFont jobNameFont;
    setFontPointSizeDPI(jobNameFont, 20, painter);
    painter->setFont(jobNameFont);

    QPen jobPen;
    jobPen.setWidth(AppSettings.getJobLineWidth());
    jobPen.setCapStyle(Qt::RoundCap);
    jobPen.setJoinStyle(Qt::RoundJoin);

    QPen selectedJobPen;

    const JobStopEntry selectedJob = scene->getSelectedJob();
    if(drawSelection && selectedJob.jobId)
    {
        selectedJobPen.setWidthF(jobPen.widthF() * SelectedJobWidthFactor);
        selectedJobPen.setCapStyle(Qt::RoundCap);
        selectedJobPen.setJoinStyle(Qt::RoundJoin);

        QColor color = Session->colorForCat(selectedJob.category);
        color.setAlpha(SelectedJobAlphaFactor);
        selectedJobPen.setColor(color);
    }

    QPointF top;
    QPointF bottom;

    JobCategory lastJobCategory = JobCategory::NCategories;
    QTextOption textOption(Qt::AlignTop | Qt::AlignLeft);

    for(const StationGraphObject &st : qAsConst(scene->stations))
    {
        const double left = st.xPos;
        const double right = left + st.platforms.count() * platfOffset;

        //Set a maximum right edge to Job labels
        //This allows to determine if they have to be drawn
        const double maxJobLabelX = right + stationOffset;

        if(left > rect.right() || maxJobLabelX < rect.left())
            continue; //Skip station, it's not visible

        top.rx() = bottom.rx() = st.xPos;

        for(const StationGraphObject::PlatformGraph& platf : st.platforms)
        {
            for(const StationGraphObject::JobStopGraph& jobStop : platf.jobStops)
            {
                //NOTE: departure comes AFTER arrival in time, opposite than job segment
                if(jobStop.arrivalY > rect.bottom() || jobStop.departureY < rect.top())
                    continue; //Skip, job not visible

                top.setY(jobStop.arrivalY);
                bottom.setY(jobStop.departureY);

                const bool nullStopDuration = qFuzzyCompare(top.y(), bottom.y());

                if(drawSelection && selectedJob.jobId == jobStop.stop.jobId)
                {
                    //Draw selection around segment
                    painter->setPen(selectedJobPen);

                    if(nullStopDuration)
                        painter->drawPoint(top);
                    else
                        painter->drawLine(top, bottom);

                    //Reset pen
                    painter->setPen(jobPen);
                }

                if(lastJobCategory != jobStop.stop.category)
                {
                    QColor color = Session->colorForCat(jobStop.stop.category);
                    jobPen.setColor(color);
                    painter->setPen(jobPen);
                    lastJobCategory = jobStop.stop.category;
                }

                if(nullStopDuration)
                    painter->drawPoint(top);
                else
                    painter->drawLine(top, bottom);

                if(jobStop.drawLabel)
                {
                    const QString jobName = JobCategoryName::jobName(jobStop.stop.jobId, jobStop.stop.category);

                    //Put label a bit to the left in respect to the stop arrival point
                    //Calculate width so it doesn't go after maxJobLabelX
                    const qreal topWithMargin = top.x() + platfOffset / 2;
                    QRectF r(topWithMargin, top.y(), maxJobLabelX - topWithMargin, 25);
                    painter->drawText(r, jobName, textOption);
                }
            }

            top.rx() += platfOffset;
            bottom.rx() += platfOffset;
        }
    }
}

void BackgroundHelper::drawJobSegments(QPainter *painter, LineGraphScene *scene, const QRectF &rect, bool drawSelection)
{
    const double stationOffset = Session->stationOffset;

    QFont jobNameFont;
    setFontPointSizeDPI(jobNameFont, 20, painter);
    painter->setFont(jobNameFont);

    QColor textBackground(Qt::white);
    textBackground.setAlpha(100);

    QPen jobPen;
    jobPen.setWidth(AppSettings.getJobLineWidth());
    jobPen.setCapStyle(Qt::RoundCap);
    jobPen.setJoinStyle(Qt::RoundJoin);

    QPen selectedJobPen;

    const JobStopEntry selectedJob = scene->getSelectedJob();
    if(drawSelection && selectedJob.jobId)
    {
        selectedJobPen.setWidthF(jobPen.widthF() * SelectedJobWidthFactor);
        selectedJobPen.setCapStyle(Qt::RoundCap);
        selectedJobPen.setJoinStyle(Qt::RoundJoin);

        QColor color = Session->colorForCat(selectedJob.category);
        color.setAlpha(SelectedJobAlphaFactor);
        selectedJobPen.setColor(color);
    }

    JobCategory lastJobCategory = JobCategory::NCategories;
    QTextOption textOption(Qt::AlignCenter);

    //Iterate until one but last
    //This way we can always acces next station
    for(int i = 0; i < scene->stationPositions.size() - 1; i++)
    {
        const LineGraphScene::StationPosEntry& stPos = scene->stationPositions.at(i);

        const double left = stPos.xPos;
        double right = 0;

        if(i < scene->stationPositions.size() - 2)
        {
            const LineGraphScene::StationPosEntry& afterNextPos = scene->stationPositions.at(i + 2);
            right = afterNextPos.xPos - stationOffset;
        }
        else
        {
            right = rect.right(); //Last station, use all space on right side
        }

        if(left > rect.right() || right < rect.left())
            continue; //Skip station, it's not visible

        for(const LineGraphScene::JobSegmentGraph& job : stPos.nextSegmentJobGraphs)
        {
            //NOTE: departure comes BEFORE arrival in time, opposite than job stop
            if(job.fromDeparture.y() > rect.bottom() || job.toArrival.y() < rect.top())
                continue; //Skip, job not visible

            const QLineF line(job.fromDeparture, job.toArrival);

            if(drawSelection && selectedJob.jobId == job.jobId)
            {
                //Draw selection around segment
                painter->setPen(selectedJobPen);
                painter->drawLine(line);

                //Reset pen
                painter->setPen(jobPen);
            }

            if(lastJobCategory != job.category)
            {
                QColor color = Session->colorForCat(job.category);
                jobPen.setColor(color);
                painter->setPen(jobPen);
                lastJobCategory = job.category;
            }

            painter->drawLine(line);

            const QString jobName = JobCategoryName::jobName(job.jobId, job.category);

            //Save old transformation to reset it after drawing text
            const QTransform oldTransf = painter->transform();

            //Move to line center, it will be rotation pivot
            painter->translate(line.center());

            //Rotate by line angle
            qreal angle = line.angle();
            if(job.fromDeparture.x() > job.toArrival.x())
                angle += 180.0; //Prevent flipping text

            painter->rotate(-angle); //minus because QPainter wants clockwise angle

            const double lineLength = line.length();
            QRectF textRect(-lineLength / 2, -30, lineLength, 25);

            //Try to avoid overlapping text of crossing jobs, move text towards arrival
            if(job.toArrival.x() > job.fromDeparture.x())
                textRect.moveLeft(textRect.left() + lineLength / 5);
            else
                textRect.moveLeft(textRect.left() - lineLength / 5);

            textRect = painter->boundingRect(textRect, jobName, textOption);

            //Draw a semi transparent background to ease text reading
            painter->fillRect(textRect, textBackground);
            painter->drawText(textRect, jobName, textOption);

            //Reset to old transformation
            painter->setTransform(oldTransf);
        }
    }
}
