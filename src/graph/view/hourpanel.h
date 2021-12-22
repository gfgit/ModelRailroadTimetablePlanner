#ifndef HOURPANEL_H
#define HOURPANEL_H

#include <QWidget>

/*!
 * \brief Helper widget to draw hour labels
 *
 * Draws hour labels on the view vertical header
 *
 * \sa LineGraphScene
 * \sa LineGraphView
 * \sa BackgroundHelper
 */
class HourPanel : public QWidget
{
    Q_OBJECT
public:
    explicit HourPanel(QWidget *parent = nullptr);

public slots:
    void setScroll(int value);
    void setZoom(int val);

protected:
    void paintEvent(QPaintEvent *);

private:
    int verticalScroll;
    int mZoom;
};

#endif // HOURPANEL_H
