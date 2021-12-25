#include "openfileinfolder.h"

#include <QCoreApplication>
#include <QFileInfo>
#include <QUrl>

#include <QProcess>
#include <QDesktopServices>

#include "utils/owningqpointer.h"
#include <QMessageBox>
#include <QPushButton>

namespace utils {

class OpenFileInFolder
{
    Q_DECLARE_TR_FUNCTIONS(OpenFileInFolder)
};

bool openFileInFolder(const QFileInfo& info, bool folder)
{
    if(info.isDir())
        folder = true;

    QString path = folder ? info.absolutePath() : info.absoluteFilePath();

#if defined(Q_OS_WIN) && !defined (Q_OS_WINRT)
    if(!folder)
    {
        QString args = QStringLiteral("/select,%1").arg(path);
        QProcess explorer;
        explorer.setProgram(QLatin1String("explorer.exe"));
        explorer.setNativeArguments(args); //This must not be quoted
        return explorer.startDetached();
    }
#endif

    return QDesktopServices::openUrl(QUrl::fromLocalFile(path));
}

} //namespace utils

void utils::openFileInFolder(const QString &title, const QString &filePath, QWidget *parent)
{
    QFileInfo info(filePath);

    OwningQPointer<QMessageBox> msgBox = new QMessageBox(parent);
    msgBox->setIcon(QMessageBox::Information);
    msgBox->setWindowTitle(title);
    msgBox->setText(OpenFileInFolder::tr("Do you want to open file?<br><b>%1</b>").arg(info.canonicalFilePath()));

    QPushButton *openFileBut = nullptr;
    if(info.isFile())
        openFileBut = msgBox->addButton(OpenFileInFolder::tr("Open In App"), QMessageBox::AcceptRole);
    QPushButton *folderBut = msgBox->addButton(OpenFileInFolder::tr("Show Folder"), QMessageBox::AcceptRole);
    msgBox->addButton(QMessageBox::Ok);
    if(msgBox->exec() != QMessageBox::Accepted || !msgBox)
        return;

    if(openFileBut && msgBox->clickedButton() == openFileBut)
    {
        openFileInFolder(info, false);
    }
    else if(msgBox->clickedButton() == folderBut)
    {
        openFileInFolder(info, true);
    }
}
