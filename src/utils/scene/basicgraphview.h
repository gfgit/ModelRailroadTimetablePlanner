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

#ifndef BASICGRAPHVIEW_H
#define BASICGRAPHVIEW_H

#include <QAbstractScrollArea>

class IGraphScene;
class BasicGraphHeader;

/*!
 * \brief Widget to display a IGraphScene
 *
 * A scrollable widget which renders IGraphScene contents
 *
 * \sa IGraphScene
 */
class BasicGraphView : public QAbstractScrollArea
{
    Q_OBJECT
public:
    explicit BasicGraphView(QWidget *parent = nullptr);

    IGraphScene *scene() const;
    virtual void setScene(IGraphScene *newScene);

    /*!
     * \brief ensure point is visible
     *
     * Scrolls the contents of the scroll area so that the point (\a x, \a y) is visible
     * inside the region of the viewport with margins specified in pixels by \a xmargin and
     * \a ymargin. If the specified point cannot be reached, the contents are scrolled to
     * the nearest valid position. The default value for both margins is 50 pixels.
     *
     * Vertical and horizontal headers are excluded from the visible zone
    */
    void ensureVisible(double x, double y, double xmargin, double ymargin);


    /*!
     * \brief getZoomLevel
     * \return view zoom level
     *
     * 100 is normal zoom
     *
     * \sa setZoomLevel()
     * \sa zoomLevelChanged()
     */
    inline int getZoomLevel() const { return mZoom; }

    /*!
     * \brief mapToScene
     * \param pos position in viewport coordinates
     * \return position in scene coordinates
     *
     * Applies scrolling and scaling
     */
    QPointF mapToScene(const QPointF& pos) const;

signals:
    void zoomLevelChanged(int zoom);

public slots:
    /*!
     * \brief Triggers graph redrawing
     *
     * \sa updateScrollBars()
     */
    void redrawGraph();

    /*!
     * \brief ensure a rect is visible in the viewport
     *
     * \sa ensureVisible()
     */
    void ensureRectVisible(const QRectF& r);

    /*!
     * \brief setZoomLevel
     * \param zoom zoom level
     *
     * Sets view zoom level and redraws view
     *
     * Zoom is applied also to headers.
     * 100 is normal zoom.
     * Emits zoomLevelChanged()
     *
     * \sa getZoomLevel()
     * \sa zoomLevelChanged()
     */
    void setZoomLevel(int zoom);

protected:
    /*!
     * \brief React to layout and style change
     *
     * On layout and style change update scroll bars
     * \sa updateScrollBars()
     */
    bool event(QEvent *e) override;

    void paintEvent(QPaintEvent *e) override;
    void resizeEvent(QResizeEvent *) override;

    /*!
     * \brief Activate view
     *
     * This view (and its scene) is now active
     * It will receive requests to show items
     *
     * \sa LineGraphScene::activateScene()
     * \sa LineGraphManager::setActiveScene()
     */
    void focusInEvent(QFocusEvent *e) override;

private slots:
    void onSceneDestroyed();
    void resizeHeaders();

private:
    /*!
     * \brief Update scrollbar size
     */
    void updateScrollBars();

private:
    BasicGraphHeader *m_verticalHeader;
    BasicGraphHeader *m_horizontalHeader;

    IGraphScene *m_scene;

    int mZoom;
};

#endif // BASICGRAPHVIEW_H
