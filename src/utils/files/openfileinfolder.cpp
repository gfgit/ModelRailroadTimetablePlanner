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

#include "openfileinfolder.h"

#include <QFileInfo>
#include <QDir>
#include <QUrl>
#include <QFileIconProvider>

#include <QProcess>
#include <QDesktopServices>

#include <QGridLayout>
#include <QPushButton>
#include <QLabel>
#include <QDialogButtonBox>
#include <QStyle>

#include "utils/owningqpointer.h"

namespace utils {

static bool openDefAppOrShowFolder(const QFileInfo &info, bool folder)
{
    if (info.isDir())
        folder = true;

    QString path = folder ? info.absolutePath() : info.absoluteFilePath();

#if defined(Q_OS_WIN) && !defined(Q_OS_WINRT)
    if (folder && !info.isDir())
    {
        // NOTE: on Windows desktop we can also select a file inside a folder
        // So prefer it instead of default QDesktopServices
        QString args =
          QStringLiteral("/select,%1").arg(QDir::toNativeSeparators(info.absoluteFilePath()));
        QProcess explorer;
        explorer.setProgram(QLatin1String("explorer.exe"));
        explorer.setNativeArguments(args); // This must not be quoted
        if (explorer.startDetached())
            return true;
    }
#endif

    return QDesktopServices::openUrl(QUrl::fromLocalFile(path));
}

OpenFileInFolderDlg::OpenFileInFolderDlg(QWidget *parent) :
    QDialog(parent)
{
    QGridLayout *lay = new QGridLayout(this);

    mIconLabel       = new QLabel;
    mIconLabel->setAlignment(Qt::AlignCenter);
    mIconLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    lay->addWidget(mIconLabel, 0, 0, Qt::AlignCenter);

    mDescrLabel = new QLabel;
    mDescrLabel->setWordWrap(true);
    lay->addWidget(mDescrLabel, 0, 1);

    QDialogButtonBox *box = new QDialogButtonBox(Qt::Horizontal);
    lay->addWidget(box, 1, 0, 1, 2);

    openFileBut        = box->addButton(tr("Open In App"), QDialogButtonBox::AcceptRole);
    openFolderBut      = box->addButton(tr("Show Folder"), QDialogButtonBox::AcceptRole);
    QPushButton *okBut = box->addButton(QDialogButtonBox::Ok);

    // Open Folder by default
    openFolderBut->setDefault(true);
    openFolderBut->setFocus();

    // Only close dialog with Ok button, ignore others
    connect(okBut, &QPushButton::clicked, this, &QDialog::accept);
    connect(openFileBut, &QPushButton::clicked, this, &OpenFileInFolderDlg::onOpenFile);
    connect(openFolderBut, &QPushButton::clicked, this, &OpenFileInFolderDlg::onOpenFolder);
}

void OpenFileInFolderDlg::setFilePath(const QString &newFilePath)
{
    info.setFile(newFilePath);
    openFileBut->setVisible(info.isFile());

    QFileIconProvider provider;
    QIcon fileIcon     = provider.icon(info);

    QStyle *s          = style();
    const int iconSize = s->pixelMetric(QStyle::PM_MessageBoxIconSize, 0, this);
    if (fileIcon.isNull())
        fileIcon = s->standardIcon(QStyle::SP_MessageBoxInformation, 0, this);

    QPixmap pix;
    if (!fileIcon.isNull())
    {
        QWindow *window = nativeParentWidget() ? nativeParentWidget()->windowHandle() : nullptr;
        pix             = fileIcon.pixmap(window, QSize(iconSize, iconSize));
    }

    mIconLabel->setPixmap(pix);
    mIconLabel->adjustSize();
}

void OpenFileInFolderDlg::setLabelText(const QString &text)
{
    mDescrLabel->setText(text);
}

void OpenFileInFolderDlg::askUser(const QString &title, const QString &filePath, QWidget *parent)
{
    OwningQPointer<OpenFileInFolderDlg> dlg = new OpenFileInFolderDlg(parent);
    dlg->setWindowTitle(title);
    dlg->setFilePath(filePath);
    dlg->setLabelText(OpenFileInFolderDlg::tr("Do you want to open file?<br><b>%1</b>")
                        .arg(dlg->getInfo().canonicalFilePath()));
    dlg->exec();
}

void OpenFileInFolderDlg::onOpenFile()
{
    openDefAppOrShowFolder(info, false);
}

void OpenFileInFolderDlg::onOpenFolder()
{
    openDefAppOrShowFolder(info, true);
}

} // namespace utils
