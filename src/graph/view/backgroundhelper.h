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
    static void drawHourPanel(QPainter *painter, const QRectF& rect, double verticalScroll);

    static void drawBackgroundHourLines(QPainter *painter, const QRectF& rect);

    static void drawStationHeader(QPainter *painter, LineGraphScene *scene, const QRectF& rect, double horizontalScroll);

    static void drawStations(QPainter *painter, LineGraphScene *scene, const QRectF& rect);

    static void drawJobStops(QPainter *painter, LineGraphScene *scene, const QRectF& rect, bool drawSelection);

    static void drawJobSegments(QPainter *painter, LineGraphScene *scene, const QRectF &rect, bool drawSelection);

public:
    static constexpr double SelectedJobWidthFactor = 3.0;
    static constexpr int SelectedJobAlphaFactor = 127;
};

#endif // BACKGROUNDHELPER_H
