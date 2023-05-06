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

#include "meetinginformationdialog.h"
#include "ui_meetinginformationdialog.h"

#include "app/session.h"
#include "metadatamanager.h"

#include "imagemetadata.h"

#include <QImageReader>
#include <QImageWriter>
#include <QBuffer>
#include <QMessageBox>
#include <QFileDialog>
#include "utils/files/recentdirstore.h"

#include "utils/delegates/imageviewer/imageviewer.h"
#include "utils/owningqpointer.h"

#include <QDebug>

MeetingInformationDialog::MeetingInformationDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::MeetingInformationDialog),
    needsToSaveImg(false),
    headerIsNull(false),
    footerIsNull(false)
{
    ui->setupUi(this);

    connect(ui->viewPictureBut, &QPushButton::clicked, this, &MeetingInformationDialog::showImage);
    connect(ui->importPictureBut, &QPushButton::clicked, this, &MeetingInformationDialog::importImage);
    connect(ui->removePictureBut, &QPushButton::clicked, this, &MeetingInformationDialog::removeImage);
    connect(ui->resetHeaderBut, &QPushButton::clicked, this, &MeetingInformationDialog::toggleHeader);
    connect(ui->resetFooterBut, &QPushButton::clicked, this, &MeetingInformationDialog::toggleFooter);
    connect(ui->startDate, &QDateEdit::dateChanged, this, &MeetingInformationDialog::updateMinumumDate);

    QSizePolicy sp = ui->headerEdit->sizePolicy();
    sp.setRetainSizeWhenHidden(true);
    ui->headerEdit->setSizePolicy(sp);
    ui->footerEdit->setSizePolicy(sp);

    //Use similar font to the actual font used in sheet export
    QFont font;
    font.setBold(true);
    font.setPointSize(18);
    ui->descrEdit->document()->setDefaultFont(font);

    if(!loadData())
    {
        QMessageBox::warning(this, tr("Database Error"),
                             tr("This database doesn't support metadata.\n"
                                "Make sure it was created by a recent version of the application and was not manipulated."));
        setDisabled(true);
    }
}

MeetingInformationDialog::~MeetingInformationDialog()
{
    delete ui;
}

bool MeetingInformationDialog::loadData()
{
    MetaDataManager *meta = Session->getMetaDataManager();

    qint64 tmp = 0;
    QDate date;

    switch (meta->getInt64(tmp, MetaDataKey::MeetingStartDate))
    {
    case MetaDataKey::Result::ValueFound:
    {
        date = QDate::fromJulianDay(tmp);
        break;
    }
    case MetaDataKey::Result::NoMetaDataTable:
        return false; //Database has no well-formed metadata
    default:
        date = QDate::currentDate();
    }
    ui->startDate->setDate(date);

    switch (meta->getInt64(tmp, MetaDataKey::MeetingEndDate))
    {
    case MetaDataKey::Result::ValueFound:
    {
        date = QDate::fromJulianDay(tmp);
        break;
    }
    default:
        date = ui->startDate->date();
    }
    ui->endDate->setDate(date);

    qint64 showDates = 1;
    meta->getInt64(showDates, MetaDataKey::MeetingShowDates);
    ui->showDatesBox->setChecked(showDates == 1);

    QString text;
    meta->getString(text, MetaDataKey::MeetingLocation);
    ui->locationEdit->setText(text.simplified());

    text.clear();
    meta->getString(text, MetaDataKey::MeetingHostAssociation);
    ui->associationEdit->setText(text.simplified());

    text.clear();
    meta->getString(text, MetaDataKey::MeetingDescription);
    ui->descrEdit->setPlainText(text);
    //Align all text to center
    QTextCursor c = ui->descrEdit->textCursor();
    c.select(QTextCursor::Document);
    QTextBlockFormat fmt;
    fmt.setAlignment(Qt::AlignTop | Qt::AlignHCenter);
    c.mergeBlockFormat(fmt);
    ui->descrEdit->setTextCursor(c);

    text.clear();
    headerIsNull = meta->getString(text, MetaDataKey::SheetHeaderText) != MetaDataKey::ValueFound;
    setSheetText(ui->headerEdit, ui->resetHeaderBut, text, headerIsNull);

    text.clear();
    footerIsNull = meta->getString(text, MetaDataKey::SheetFooterText) != MetaDataKey::ValueFound;
    setSheetText(ui->footerEdit, ui->resetFooterBut, text, footerIsNull);

    return true;
}

void MeetingInformationDialog::setSheetText(QLineEdit *lineEdit, QPushButton *but, const QString& text, bool isNull)
{
    lineEdit->setVisible(!isNull);

    if(isNull)
    {
        but->setText(tr("Set custom text"));
        lineEdit->setText(QString());
    }
    else
    {
        but->setText(tr("Reset"));
        lineEdit->setText(text.simplified());
    }
}

void MeetingInformationDialog::saveData()
{
    MetaDataManager *meta = Session->getMetaDataManager();

    meta->setInt64(ui->startDate->date().toJulianDay(), false, MetaDataKey::MeetingStartDate);
    meta->setInt64(ui->endDate->date().toJulianDay(), false, MetaDataKey::MeetingEndDate);
    meta->setInt64(ui->showDatesBox->isChecked() ? 1 : 0, false, MetaDataKey::MeetingShowDates);

    meta->setString(ui->locationEdit->text().simplified(), false, MetaDataKey::MeetingLocation);
    meta->setString(ui->associationEdit->text().simplified(), false, MetaDataKey::MeetingHostAssociation);
    meta->setString(ui->descrEdit->toPlainText(), false, MetaDataKey::MeetingDescription);

    meta->setString(ui->headerEdit->text().simplified(), headerIsNull, MetaDataKey::SheetHeaderText);
    meta->setString(ui->footerEdit->text().simplified(), footerIsNull, MetaDataKey::SheetFooterText);

    if(needsToSaveImg)
    {
        if(img.isNull())
        {
            ImageMetaData::setImage(Session->m_Db, MetaDataKey::MeetingLogoPicture, nullptr, 0);
        }
        else
        {
            QByteArray arr;
            QBuffer buf(&arr);
            buf.open(QIODevice::WriteOnly);

            QImageWriter writer(&buf, "PNG");
            if(writer.canWrite() && writer.write(img))
            {
                ImageMetaData::setImage(Session->m_Db, MetaDataKey::MeetingLogoPicture, arr.data(), arr.size());
            }else{
                qDebug() << "MeetingInformationDialog: error saving image," << writer.errorString();
            }
        }
    }
}

void MeetingInformationDialog::showImage()
{
    OwningQPointer<ImageViewer> dlg = new ImageViewer(this);

    if(img.isNull() && !needsToSaveImg)
    {
        std::unique_ptr<ImageMetaData::ImageBlobDevice> imageIO;
        imageIO.reset(ImageMetaData::getImage(Session->m_Db, MetaDataKey::MeetingLogoPicture));
        if(imageIO && imageIO->open(QIODevice::ReadOnly))
        {
            QImageReader reader(imageIO.get());
            if(reader.canRead())
            {
                img = reader.read(); //ERRORMSG: handle errors, show to user
            }

            if(img.isNull())
            {
                qDebug() << "MeetingInformationDialog: error loading image," << reader.errorString();
            }

            imageIO->close();
        }else{
            qDebug() << "MeetingInformationDialog: error query image," << Session->m_Db.error_msg();
        }
    }

    dlg->setImage(img);

    dlg->exec();

    if(!needsToSaveImg)
        img = QImage(); //Cleanup to free memory
}

void MeetingInformationDialog::importImage()
{
    const QLatin1String meeting_image_key = QLatin1String("meeting_image_key");

    OwningQPointer<QFileDialog> dlg = new QFileDialog(this, tr("Import image"));
    dlg->setFileMode(QFileDialog::ExistingFile);
    dlg->setAcceptMode(QFileDialog::AcceptOpen);
    dlg->setDirectory(RecentDirStore::getDir(meeting_image_key, RecentDirStore::Images));

    QList<QByteArray> mimes = QImageReader::supportedMimeTypes();
    QStringList filters;
    filters.reserve(mimes.size() + 1);
    for(const QByteArray &ba : mimes)
        filters.append(QString::fromUtf8(ba));

    filters << "application/octet-stream"; // will show "All files (*)"

    dlg->setMimeTypeFilters(filters);

    if(dlg->exec() != QDialog::Accepted || !dlg)
        return;

    QString fileName = dlg->selectedUrls().value(0).toLocalFile();
    if(fileName.isEmpty())
        return;

    RecentDirStore::setPath(meeting_image_key, fileName);

    QImageReader reader(fileName);
    reader.setQuality(100);
    if(reader.canRead())
    {
        QImage image = reader.read();
        if(image.isNull())
        {
            QMessageBox::warning(this, tr("Importing error"),
                                 tr("The image format is not supported or the file is corrupted."));
            qDebug() << "MeetingInformationDialog: error importing image," << reader.errorString();
            return;
        }

        img = image;
        needsToSaveImg = true;
    }
}

void MeetingInformationDialog::removeImage()
{
    int ret = QMessageBox::question(this, tr("Remove image?"),
                                    tr("Are you sure to remove the image logo?"));
    if(ret != QMessageBox::Yes)
        return;

    img = QImage(); //Cleanup to free memory
    needsToSaveImg = true;
}

void MeetingInformationDialog::toggleHeader()
{
    headerIsNull = !headerIsNull;
    setSheetText(ui->headerEdit, ui->resetHeaderBut, QString(), headerIsNull);
}

void MeetingInformationDialog::toggleFooter()
{
    footerIsNull = !footerIsNull;
    setSheetText(ui->footerEdit, ui->resetFooterBut, QString(), footerIsNull);
}

void MeetingInformationDialog::updateMinumumDate()
{
    ui->endDate->setMinimumDate(ui->startDate->date());
}
