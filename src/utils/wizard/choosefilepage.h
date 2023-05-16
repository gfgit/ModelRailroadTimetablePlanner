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

#ifndef CHOOSEFILEPAGE_H
#define CHOOSEFILEPAGE_H

#include <QWizardPage>

class QLineEdit;
class QPushButton;

class ChooseFilePage : public QWizardPage
{
    Q_OBJECT
public:
    explicit ChooseFilePage(QWidget *parent = nullptr);

    bool isComplete() const override;
    bool validatePage() override;
    void initializePage() override;

    void setFileDlgOptions(const QString& dlgTitle, const QStringList& fileFormats);

signals:
    void fileChosen(const QString& fileName);

private slots:
    void onChoose();

private:
    QLineEdit *pathEdit;
    QPushButton *chooseBut;

    QString fileDlgTitle;
    QStringList fileDlgFormats;
};

#endif // CHOOSEFILEPAGE_H
