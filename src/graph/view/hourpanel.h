#ifndef HOURPANEL_H
#define HOURPANEL_H

#include <QWidget>

class HourPanel : public QWidget
{
    Q_OBJECT
public:
    explicit HourPanel(QWidget *parent = nullptr);

public slots:
    void setScroll(int value);

protected:
    void paintEvent(QPaintEvent *);

private:
    int verticalScroll;
};

#endif // HOURPANEL_H
