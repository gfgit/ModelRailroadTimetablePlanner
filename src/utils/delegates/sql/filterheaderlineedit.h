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

#ifndef FILTERHEADERLINEEDIT_H
#define FILTERHEADERLINEEDIT_H

#include <QLineEdit>

/*!
 * \brief The FilterHeaderLineEdit class
 *
 * Line edit with delayed signal and custom context menu and tooltip
 *
 * \sa FilterHeaderView
 */
class FilterHeaderLineEdit : public QLineEdit
{
    Q_OBJECT
public:
    FilterHeaderLineEdit(int col, QWidget *parent = nullptr);
    ~FilterHeaderLineEdit();

    void updateTextWithoutEmitting(const QString& str);

    inline int getColumn() const { return m_column; }

    inline int allowNull() const { return m_allowNull; }
    void setAllowNull(bool val);

signals:
    void delayedTextChanged(FilterHeaderLineEdit *self, const QString& str);
    void tooltipRequested(FilterHeaderLineEdit *self, const QPoint& globalPos);

private slots:
    void startTextTimer();
    void stopTextTimer();
    void delayedTimerFinished();
    void setNULLFilter();

protected:
    bool event(QEvent *e) override;
    void timerEvent(QTimerEvent *e) override;
    void contextMenuEvent(QContextMenuEvent *e) override;

private:
    QAction *actionFilterNull;

    QString lastValue;
    int m_column;
    int m_textTimerId;
    bool m_allowNull;
};

#endif // FILTERHEADERLINEEDIT_H
