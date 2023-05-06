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

#ifndef MEETINGINFORMATIONDIALOG_H
#define MEETINGINFORMATIONDIALOG_H

#include <QDialog>

class QLineEdit;

namespace Ui {
class MeetingInformationDialog;
}

class MeetingInformationDialog : public QDialog
{
    Q_OBJECT

public:
    explicit MeetingInformationDialog(QWidget *parent = nullptr);
    ~MeetingInformationDialog();

public:
    bool loadData();
    void saveData();

private slots:
    void showImage();
    void importImage();
    void removeImage();

    void toggleHeader();
    void toggleFooter();

    void updateMinumumDate();

private:
    void setSheetText(QLineEdit *lineEdit, QPushButton *but, const QString &text, bool isNull);

private:
    Ui::MeetingInformationDialog *ui;
    QImage img;

    bool needsToSaveImg;
    bool headerIsNull;
    bool footerIsNull;
};

#endif // MEETINGINFORMATIONDIALOG_H
