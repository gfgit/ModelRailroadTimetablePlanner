#ifndef E656IMPORTDLG_H
#define E656IMPORTDLG_H

#include <QDialog>

class QTableView;
class QLineEdit;
class QPushButton;

class E656StationModel;
class E656NetImporter;

namespace sqlite3pp {
class database;
}

class E656ImportDlg : public QDialog
{
    Q_OBJECT
public:
    explicit E656ImportDlg(sqlite3pp::database &db, QWidget *parent = nullptr);

private slots:
    void startUrlRequest();
    void showError(const QString& msg);

private:
    QLineEdit *urlEdit;
    QPushButton *urlImportBut;
    QTableView *view;
    QPushButton *jobImportBut;

    E656StationModel *model;
    E656NetImporter *importer;
};

#endif // E656IMPORTDLG_H
