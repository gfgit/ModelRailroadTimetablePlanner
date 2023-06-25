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
    void colorChanged(const QColor &);
    void editingFinished();

protected:
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *e) override;

private:
    QColor mColor;
};

#endif // COLORVIEW_H
