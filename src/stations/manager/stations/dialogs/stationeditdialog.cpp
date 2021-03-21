#include "stationeditdialog.h"
#include "ui_stationeditdialog.h"

StationEditDialog::StationEditDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::StationEditDialog)
{
    ui->setupUi(this);
}

StationEditDialog::~StationEditDialog()
{
    delete ui;
}
