#include "colorview.h"
#include <QPainter>

#include <QColorDialog>
#include <QMouseEvent>

ColorView::ColorView(QWidget *parent) :
    QWidget(parent)
{
    setMinimumSize(30, 30);
}

void ColorView::setColor(const QColor &color, bool user)
{
    if(mColor == color)
        return;

    mColor = color;

    if(user)
    {
        emit colorChanged(mColor);
    }
}

void ColorView::openColorDialog()
{
    QColor col = QColorDialog::getColor(mColor,
                           this,
                           tr("Choose a color"));
    setColor(col, true);

    emit editingFinished();
}

void ColorView::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.fillRect(rect(), mColor);
}

void ColorView::mousePressEvent(QMouseEvent *e)
{
    e->accept();
    openColorDialog();
}
