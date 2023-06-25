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

#ifndef IOPTIONSWIDGET_H
#define IOPTIONSWIDGET_H

#include <QWidget>

#include <QMap>

class IOptionsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit IOptionsWidget(QWidget *parent = nullptr);

    virtual void loadSettings(const QMap<QString, QVariant> &settings)          = 0;
    virtual void saveSettings(QMap<QString, QVariant> &settings)                = 0;

    virtual void getFileDialogOptions(QString &title, QStringList &fileFormats) = 0;
};

#endif // IOPTIONSWIDGET_H
