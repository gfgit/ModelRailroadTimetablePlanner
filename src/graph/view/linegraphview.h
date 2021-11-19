#ifndef LINEGRAPHVIEW_H
#define LINEGRAPHVIEW_H

#include <QAbstractScrollArea>

class LineGraphScene;

class StationLabelsHeader;
class HourPanel;

/*!
 * \brief Widget to display a LineGraphScene
 *
 * A scrollable widget which renders LineGraphScene contents
 * Moving the mouse cursor on the contents, tooltips will show
 *
 * \sa LineGraphWidget
 * \sa BackgroundHelper
 */
class LineGraphView : public QAbstractScrollArea
{
    Q_OBJECT
public:
    explicit LineGraphView(QWidget *parent = nullptr);

    LineGraphScene *scene() const;
    void setScene(LineGraphScene *newScene);

    /*!
     * \brief ensure point is visible
     * \param x point coordinate
     * \param y point coordinate
     * \param xmargin margin at left and right of point
     * \param ymargin margin above and below point
     *
     * Ensures a point is visible with margins in the viewport
     */
    void ensureVisible(int x, int y, int xmargin, int ymargin);

signals:
    /*!
     * \brief Sync toolbar on click
     *
     * \sa syncToolbarToScene()
     */
    void syncToolbarToScene();

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

protected:
    /*!
     * \brief React to layout and style change
     *
     * On layout and style change update scroll bars
     * \sa updateScrollBars()
     */
    bool event(QEvent *e) override;

    /*!
     * \brief Show Tooltips
     *
     * Show tooltips on viewport
     * \sa LineGraphScene::getJobAt()
     */
    bool viewportEvent(QEvent *e) override;

    void paintEvent(QPaintEvent *e) override;
    void resizeEvent(QResizeEvent *) override;

    /*!
     * \brief Sync toolbar on click
     *
     * \sa syncToolbarToScene()
     */
    void mousePressEvent(QMouseEvent *e) override;

    /*!
     * \brief Select jobs
     *
     * Select jobs by double clicking on them
     * \sa LineGraphScene::getJobAt()
     */
    void mouseDoubleClickEvent(QMouseEvent *e) override;

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
    StationLabelsHeader *stationHeader;
    HourPanel *hourPanel;

    LineGraphScene *m_scene;
};

#endif // LINEGRAPHVIEW_H
