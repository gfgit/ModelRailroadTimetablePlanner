#ifndef SHIFTGRAPHHOURPANEL_H
#define SHIFTGRAPHHOURPANEL_H

#include <QWidget>

class ShiftGraphScene;

class ShiftGraphHourPanel : public QWidget
{
    Q_OBJECT
public:
    explicit ShiftGraphHourPanel(QWidget *parent = nullptr);

    void setScene(ShiftGraphScene *newScene);

public slots:
    void setScroll(int value);
    void setZoom(int val);

protected:
    void paintEvent(QPaintEvent *);

private:
    ShiftGraphScene *m_scene;
    int horizontalScroll;
    int mZoom;
};

#endif // SHIFTGRAPHHOURPANEL_H
