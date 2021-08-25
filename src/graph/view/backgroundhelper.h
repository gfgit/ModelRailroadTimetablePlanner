#ifndef BACKGROUNDHELPER_H
#define BACKGROUNDHELPER_H

#include <QRectF>

class QPainter;

class LineGraphScene;

/*!
 * \brief Helper class to render LineGraphView contents
 *
 * Contains static helper functions to draw each part of the view
 *
 * \sa LineGraphView
 * \sa HourPanel
 * \sa StationLabelsHeader
 */
class BackgroundHelper
{
public:
    static void drawHourPanel(QPainter *painter, const QRectF& rect, int verticalScroll);

    static void drawBackgroundHourLines(QPainter *painter, const QRectF& rect);

    static void drawStationHeader(QPainter *painter, LineGraphScene *scene, const QRectF& rect, int horizontalScroll);

    static void drawStations(QPainter *painter, LineGraphScene *scene, const QRectF& rect);

    static void drawJobStops(QPainter *painter, LineGraphScene *scene, const QRectF& rect);
};

#endif // BACKGROUNDHELPER_H
