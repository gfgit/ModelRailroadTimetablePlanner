#include "lineobj.h"

#include "graph/graphicsscene.h"
#include <QGraphicsLineItem>

#include "app/session.h"

LineObj::LineObj() :
    lineId(0),
    scene(new GraphicsScene),
    refCount(0)
{

}

StationObj::StationObj() :
    stId(0),
    platfs(1),
    depots(0),
    platfRgb(qRgb(255, 255, 255))
{

}

/* void StationObj::removeJob(db_id jobId)
 * Removes all job stops 'reflected' in this station.
 * Used when a Job is deleted of if Job Name changed or path is redrawn.
*/
void StationObj::removeJob(db_id jobId)
{
    for(Graph* g : qAsConst(lineGraphs))
    {
        //There are multiple values with same key
        auto it = g->stops.find(jobId);
        while (it != g->stops.end() && it.key() == jobId)
        {
            //Cleanup
            delete it->line;
            delete it->text;
            //Remove from list
            it = g->stops.erase(it);
        }
    }
}

/* void StationObj::addJobStop(db_id stopId, db_id jobId, db_id lineId, const QString &label, qreal y1, qreal y2, int platf)
 * Adds a line to platform 'platf' from arrival 'y1' to departure 'y2'
 * (Times are already converted in graph cordinates following AppSettings values,
 *  i.e. vertical offset and hour offset)
 * Adds a label with the Job name.
 *
 * Line and label are added to each line graph containing this line.
 * This is used because if a Job stops in a station, its graph is only shown in
 * at most 2 line graphs (Current Line, Next Line, if they are the same then just 1 graph)
 *
 * So to inform other line graph that platform 'platf' is occupied
 * we 'reflect' the information in all other line grphs except for lineId (Current Line)
*/
void StationObj::addJobStop(db_id stopId, db_id jobId, db_id lineId, const QString &label, qreal y1, qreal y2, int platf)
{
    QFont font("Arial", 15, QFont::Bold); //TODO: settings
    QBrush brush(Qt::darkGray); //TODO: settings

    QPen p(Qt::darkGray);
    p.setWidth(Session->jobLineWidth);

    for(Graph* g : qAsConst(lineGraphs))
    {
        if(g->lineId == lineId)
            continue;

        //There are multiple values with same key
        auto it = g->stops.find(jobId);
        while (it != g->stops.end() && it.key() == jobId)
        {
            if(it->stopId == stopId)
            {
                //Remove the old stop to avoid duplicates when redrawing the Job.
                //Cleanup
                delete it->line;
                delete it->text;
                //Remove from list
                it = g->stops.erase(it);
            }
            else
            {
                it++;
            }
        }

        QGraphicsScene *scene = nullptr;
        if(!g->platforms.isEmpty())
            scene = g->platforms.first()->scene();
        if(!scene)
            return; //Error!

        double x = g->xPos;
        double offset = Session->platformOffset;

        if(platf < 0)
        {
            //Depots are after Platforms, but negative
            x += offset * (platfs - platf - 1);
        }
        else
        {
            //Main platforms
            x += platf * offset;
        }

        JobStop s;
        s.stopId = stopId;
        s.line = scene->addLine(x, y1, x, y2, p);
        s.line->setToolTip(label);

        s.text = scene->addSimpleText(label, font);
        s.text->setPos(x + 5, y1);
        s.text->setZValue(1); //Labels must be above jobs and platforms
        s.text->setBrush(brush);

        g->stops.insert(jobId, s);
    }
}

/* void StationObj::setPlatfColor(QRgb rgb)
 * Sets a custom color for main platform of this station (in Job Graph)
 * and updates the graph in all lines containing this station.
 * Depots platform remain unchanged and use default color from AppSettings.
 * If rgb is white (255, 255, 255) the main platform color is reset to
 * AppSettings default main platform color.
 *
 * Default value is white so newly created stations use AppSettings default color.
*/
void StationObj::setPlatfColor(QRgb rgb)
{
    platfRgb = rgb;

    QColor col(platfRgb);
    if(platfRgb == qRgb(255, 255, 255)) //If white use default color
        col = AppSettings.getMainPlatfColor();

    QPen pen(col, AppSettings.getPlatformLineWidth());

    for(Graph* g : qAsConst(lineGraphs))
    {
        //Do not alter Depots, just main platforms
        const int size = qMin(int(platfs), g->platforms.size());
        for(int i = 0; i < size; i++)
        {
            g->platforms[i]->setPen(pen);
        }
    }
}
