#ifndef COLORVIEW_H
#define COLORVIEW_H

#include <QWidget>

class ColorView : public QWidget
{
    Q_OBJECT
public:
    explicit ColorView(QWidget *parent = nullptr);

    inline QColor color() const
    {
        return mColor;
    }

    void setColor(const QColor &color, bool user = false);

    void openColorDialog();

signals:
    void colorChanged(const QColor&);
    void editingFinished();

protected:
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *e) override;

private:
    QColor mColor;
};

#endif // COLORVIEW_H
