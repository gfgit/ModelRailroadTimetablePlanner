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

#ifndef IGRAPHSCENE_H
#define IGRAPHSCENE_H

#include <QObject>
#include <QSizeF>

class QPainter;
class QRectF;

/*!
 * \brief Scene interface
 *
 * Stores information to draw content in a BasicGraphView
 *
 * \sa BasicGraphView
 * \sa BasicGraphHeader
 */
class IGraphScene : public QObject
{
    Q_OBJECT
public:
    explicit IGraphScene(QObject *parent = nullptr);

    /*!
     * \brief get scene contents size
     *
     * Scene contents size is cached for performance.
     * When a subclass changes size \ref m_cachedContentsSize
     * it must emit \ref redrawGraph() signal to update view scrollbars
     *
     * \sa redrawGraph()
     */
    inline QSizeF getContentsSize() const
    {
        return m_cachedContentsSize;
    }

    /*!
     * \brief get header size
     *
     * Header size is cached for performance.
     * When a subclass changes size \ref m_cachedHeaderSize
     * it must emit \ref headersSizeChanged() signal to resize view headers.
     *
     * Size width is vertical header width.
     * Size height is horizontal header height.
     *
     * \sa headersSizeChanged()
     */
    inline QSizeF getHeaderSize() const
    {
        return m_cachedHeaderSize;
    }

    /*!
     * \brief activate scene
     * \param self A pointer to IGraphScene instance
     *
     * For scenes registered on a manager, this tells
     * our instance is now the active one and will therefore receive
     * user requests to show items
     *
     * \sa sceneActivated()
     */
    inline void activateScene() { emit sceneActivated(this); }

    /*!
     * \brief renderContents
     * \param painter A painter to render to
     * \param sceneRect Rect to render in scene coordinates
     *
     * Renders requested portion of scene contents
     */
    virtual void renderContents(QPainter *painter, const QRectF& sceneRect) = 0;

    /*!
     * \brief render header in scene coordinates
     * \param painter A painter to render to
     * \param sceneRect Rect to render in scene coordinates
     * \param orient Header orientation
     * \param scroll scrolling in opposite orientation of orient argument
     *
     * Renders requested portion of header.
     *
     * \sa renderHeaderScroll()
     */
    virtual void renderHeader(QPainter *painter, const QRectF& sceneRect,
                              Qt::Orientation orient, double scroll) = 0;

signals:
    /*!
     * \brief request BasicGraphView to redraw
     *
     * This signal must be emitted both if content changes or if content size changes.
     * Always check content size after this signal
     */
    void redrawGraph();

    /*!
     * \brief tell BasicGraphView to resize headers
     *
     * Subclass must emit this signal when changing header size
     *
     * \sa getHeaderSize()
     */
    void headersSizeChanged();

    /*!
     * \brief request BasicGraphView to show this rect
     *
     * The view will ensure this rect is visible if possible
     */
    void requestShowRect(const QRectF& rect);

    /*!
     * \brief Signal for activation
     * \param self a pointer to IGraphScene instance
     *
     * \sa activateScene()
     */
    void sceneActivated(IGraphScene *self);

protected:
    /*!
     * \brief m_cachedContentsSize
     *
     * Cached content size
     * \sa getContentsSize()
     */
    QSizeF m_cachedContentsSize;

    /*!
     * \brief m_cachedHeaderSize
     *
     * Cached header size
     * \sa getHeaderSize()
     */
    QSizeF m_cachedHeaderSize;
};

#endif // IGRAPHSCENE_H
