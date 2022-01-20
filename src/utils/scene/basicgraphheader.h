#ifndef BASICGRAPHHEADER_H
#define BASICGRAPHHEADER_H

#include <QWidget>

/*!
 * \brief Helper widget to draw scene header
 *
 * Scene header will be always visible regardless of scrolling
 *
 * \sa IGraphScene
 * \sa BasicGraphView
 */

class IGraphScene;

class BasicGraphHeader : public QWidget
{
    Q_OBJECT
public:
    BasicGraphHeader(Qt::Orientation orient, QWidget *parent = nullptr);

    void setScene(IGraphScene *scene);

public slots:
    void setScroll(int value);
    void setZoom(int value);

protected:
    void paintEvent(QPaintEvent *e);

private:
    IGraphScene *m_scene;

    int m_scroll;
    int mZoom;
    Qt::Orientation m_orientation;
};

#endif // BASICGRAPHHEADER_H
