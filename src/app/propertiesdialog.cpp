#include "propertiesdialog.h"

#include "app/session.h"

#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>

#include <QDir>

PropertiesDialog::PropertiesDialog(QWidget *parent) :
    QDialog(parent)
{
    setWindowTitle(tr("File Properties"));

    QGridLayout *lay = new QGridLayout(this);
    lay->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    lay->setContentsMargins(9, 9, 9, 9);

    pathLabel = new QLabel;
    lay->addWidget(pathLabel, 0, 0);

    pathReadOnlyEdit = new QLineEdit;
    lay->addWidget(pathReadOnlyEdit, 0, 1);

    pathLabel->setText(tr("File Path:"));
    pathReadOnlyEdit->setText(QDir::toNativeSeparators(Session->fileName));
    pathReadOnlyEdit->setPlaceholderText(tr("No opened file"));
    pathReadOnlyEdit->setReadOnly(true);

    //TODO: make pretty and maybe add other informations like metadata versions

    setMinimumSize(200, 200);
}
