#include "openfileinfolder.h"

#include <QFileInfo>
#include <QDir>
#include <QUrl>

#include <QProcess>
#include <QDesktopServices>

#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QDialogButtonBox>

#include "utils/owningqpointer.h"

namespace utils {

static bool openDefAppOrShowFolder(const QFileInfo& info, bool folder)
{
    if(info.isDir())
        folder = true;

    QString path = folder ? info.absolutePath() : info.absoluteFilePath();

#if defined(Q_OS_WIN) && !defined (Q_OS_WINRT)
    if(folder && !info.isDir())
    {
        //NOTE: on Windows desktop we can also select a file inside a folder
        //So prefer it instead of default QDesktopServices
        QString args = QStringLiteral("/select,%1").arg(QDir::toNativeSeparators(info.absoluteFilePath()));
        QProcess explorer;
        explorer.setProgram(QLatin1String("explorer.exe"));
        explorer.setNativeArguments(args); //This must not be quoted
        if(explorer.startDetached())
            return true;
    }
#endif

    return QDesktopServices::openUrl(QUrl::fromLocalFile(path));
}

OpenFileInFolderDlg::OpenFileInFolderDlg(QWidget *parent) :
    QDialog(parent)
{
    QVBoxLayout *lay = new QVBoxLayout(this);

    mLabel = new QLabel;
    lay->addWidget(mLabel);

    QDialogButtonBox *box = new QDialogButtonBox(Qt::Horizontal);
    lay->addWidget(box);

    openFileBut = box->addButton(tr("Open In App"), QDialogButtonBox::AcceptRole);
    openFolderBut = box->addButton(tr("Show Folder"), QDialogButtonBox::AcceptRole);
    QPushButton *okBut = box->addButton(QDialogButtonBox::Ok);

    //Open Folder by default
    openFolderBut->setDefault(true);
    openFolderBut->setFocus();

    //Only close dialog with Ok button, ignore others
    connect(okBut, &QPushButton::clicked, this, &QDialog::accept);
    connect(openFileBut, &QPushButton::clicked, this, &OpenFileInFolderDlg::onOpenFile);
    connect(openFolderBut, &QPushButton::clicked, this, &OpenFileInFolderDlg::onOpenFolder);
}

void OpenFileInFolderDlg::setFilePath(const QString &newFilePath)
{
    info.setFile(newFilePath);
    openFileBut->setVisible(info.isFile());
}

void OpenFileInFolderDlg::setLabelText(const QString &text)
{
    mLabel->setText(text);
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

} //namespace utils
