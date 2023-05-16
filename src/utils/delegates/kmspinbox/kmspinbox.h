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

#ifndef KMSPINBOX_H
#define KMSPINBOX_H

#include <QSpinBox>

class KmSpinBox : public QSpinBox
{
    Q_OBJECT
public:
    explicit KmSpinBox(QWidget *parent = nullptr);

    enum Sections
    {
        KmSection = 0,
        MetersSection
    };

    void stepBy(int steps) override;

protected:
    QValidator::State validate(QString &input, int &pos) const override;
    int valueFromText(const QString &text) const override;
    QString textFromValue(int val) const override;
    void fixup(QString &str) const override;

    bool focusNextPrevChild(bool next) override;
    void focusInEvent(QFocusEvent *e) override;
    void keyPressEvent(QKeyEvent *e) override;

private slots:
    void cursorPosChanged(int oldPos, int newPos);

private:
    void setCurrentSection(int section);
    QString stripped(const QString &t, int *pos) const;

private:
    int currentSection;
};

#endif // KMSPINBOX_H
