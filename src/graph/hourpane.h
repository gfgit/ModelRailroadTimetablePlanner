#ifndef HOURPANE_H
#define HOURPANE_H

#include <QWidget>

class BackgroundHelper;

class HourPane : public QWidget
{
    Q_OBJECT
public:
    HourPane(BackgroundHelper *h, QWidget *parent);

public slots:
    void setScroll(int value);

protected:
    void paintEvent(QPaintEvent *);

private:
    BackgroundHelper *helper;
    int verticalScroll;
};

#endif // HOURPANE_H
