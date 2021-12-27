#ifndef OPENFILEINFOLDER_H
#define OPENFILEINFOLDER_H

#include <QDialog>

#include <QFileInfo>

class QLabel;
class QPushButton;

namespace utils {

/*!
 * \brief The OpenFileInFolderDlg class
 *
 * A small dialog to open file of folder in default app or show its
 * containing folder in system file manager
 *
 * This does not use QMessageBox because it would close
 * when user clicks any button.
 * This dialogs instead closes only with Ok button
 *
 * Windows: special case, it can also select a file inside explorer.exe
 */
class OpenFileInFolderDlg : public QDialog
{
    Q_OBJECT
public:
    OpenFileInFolderDlg(QWidget *parent = nullptr);

    void setFilePath(const QString &newFilePath);
    void setLabelText(const QString &text);

    inline QFileInfo getInfo() const { return info; }

    /*!
     * \brief askUser
     * \param title dialog title
     * \param filePath file or folder to open
     * \param parent parent window
     *
     * This convinience function opens standard dialog with default text
     */
    static void askUser(const QString& title, const QString& filePath, QWidget *parent);

private slots:
    void onOpenFile();
    void onOpenFolder();

private:
    QLabel *mLabel;
    QPushButton *openFileBut;
    QPushButton *openFolderBut;

    QFileInfo info;
};

}

#endif // OPENFILEINFOLDER_H
