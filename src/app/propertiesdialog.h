#ifndef PROPERTIESDIALOG_H
#define PROPERTIESDIALOG_H

#include <QDialog>

class QLabel;
class QLineEdit;

class PropertiesDialog : public QDialog
{
    Q_OBJECT
public:
    explicit PropertiesDialog(QWidget *parent = nullptr);

private:
    QLabel *pathLabel;
    QLineEdit *pathReadOnlyEdit;
};

#endif // PROPERTIESDIALOG_H
