#include "editlinedlg.h"

#include <QVBoxLayout>
#include <QTableView>
#include <QDialogButtonBox>

#include "stations/manager/lines/model/linesegmentsmodel.h"

EditLineDlg::EditLineDlg(sqlite3pp::database &db, QWidget *parent) :
    QDialog(parent)
{
    model = new LineSegmentsModel(db, this);

    QVBoxLayout *lay = new QVBoxLayout(this);

    view = new QTableView;
    view->setModel(model);

    lay->addWidget(view);

    QDialogButtonBox *box = new QDialogButtonBox(QDialogButtonBox::Ok, Qt::Horizontal);
    lay->addWidget(box);

    connect(box, &QDialogButtonBox::accepted, this, &EditLineDlg::accept);
    connect(box, &QDialogButtonBox::rejected, this, &EditLineDlg::reject);

    setWindowTitle(tr("Edit Line"));
    setMinimumSize(500, 300);
}

void EditLineDlg::setLineId(db_id lineId)
{
    model->setLine(lineId);

    //TODO: Reload UI
    QString lineName;
    int startMeters = 0;
    if(!model->getLineInfo(lineName, startMeters))
    {
        return; //Error
    }
}
