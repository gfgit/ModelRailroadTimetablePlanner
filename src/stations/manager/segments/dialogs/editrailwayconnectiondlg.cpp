#include "editrailwayconnectiondlg.h"

#include "stations/manager/segments/model/railwaysegmentconnectionsmodel.h"

#include <QBoxLayout>
#include <QTableView>
#include <QToolButton>
#include <QDialogButtonBox>

#include <QMessageBox>

#include "utils/delegates/kmspinbox/spinboxeditorfactory.h"
#include <QStyledItemDelegate>

EditRailwayConnectionDlg::EditRailwayConnectionDlg(RailwaySegmentConnectionsModel *m, QWidget *parent) :
    QDialog(parent),
    model(m)
{
    QVBoxLayout *lay = new QVBoxLayout(this);

    QHBoxLayout *toolLay = new QHBoxLayout;

    QToolButton *addConnBut = new QToolButton;
    addConnBut->setText(tr("Add"));
    toolLay->addWidget(addConnBut);

    QToolButton *removeConnBut = new QToolButton;
    removeConnBut->setText(tr("Remove"));
    toolLay->addWidget(removeConnBut);

    QToolButton *addDefaultConnBut = new QToolButton;
    addDefaultConnBut->setText(tr("Add Default"));
    toolLay->addWidget(addDefaultConnBut);

    toolLay->addStretch();

    lay->addLayout(toolLay);

    view = new QTableView;
    lay->addWidget(view);

    QDialogButtonBox *box = new QDialogButtonBox(QDialogButtonBox::Ok, Qt::Horizontal);
    lay->addWidget(box);

    view->setModel(model);

    fromTrackFactory = new SpinBoxEditorFactory;
    fromTrackFactory->setRange(-1, model->getFromGateTrackCount() - 1);
    fromTrackFactory->setSpecialValueText(tr("NULL"));
    auto fromTrackDelegate = new QStyledItemDelegate(this);
    fromTrackDelegate->setItemEditorFactory(fromTrackFactory);
    view->setItemDelegateForColumn(RailwaySegmentConnectionsModel::FromGateTrackCol,
                                   fromTrackDelegate);

    toTrackFactory = new SpinBoxEditorFactory;
    toTrackFactory->setRange(-1, model->getFromGateTrackCount() - 1);
    toTrackFactory->setSpecialValueText(tr("NULL"));
    auto toTrackDelegate = new QStyledItemDelegate(this);
    toTrackDelegate->setItemEditorFactory(toTrackFactory);
    view->setItemDelegateForColumn(RailwaySegmentConnectionsModel::ToGateTrackCol,
                                   toTrackDelegate);

    connect(addConnBut, &QToolButton::clicked, this, &EditRailwayConnectionDlg::addTrackConn);
    connect(removeConnBut, &QToolButton::clicked, this, &EditRailwayConnectionDlg::removeSelectedTrackConn);
    connect(addDefaultConnBut, &QToolButton::clicked, this, &EditRailwayConnectionDlg::addDefaultConnections);

    connect(model, &RailwaySegmentConnectionsModel::modelError,
            this, &EditRailwayConnectionDlg::onModelError);

    connect(box, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(box, &QDialogButtonBox::rejected, this, &QDialog::reject);

    setWindowTitle(tr("Edit Track Connections"));
    setMinimumSize(200, 200);
    setReadOnly(model->isReadOnly());
}

EditRailwayConnectionDlg::~EditRailwayConnectionDlg()
{
    delete fromTrackFactory;
    delete toTrackFactory;
}

void EditRailwayConnectionDlg::done(int res)
{
    if(!model->isReadOnly() && model->getActualCount() == 0 && res == QDialog::Accepted)
    {
        QMessageBox::warning(this, tr("Error"),
                             tr("Add at least 1 segment track connection."));
        return;
    }

    //Prevent rejecting because it's done by main dialog but let user close
    //even if no connection was created for this segment to avoid getting stuck on errors
    Q_UNUSED(res)
    QDialog::done(QDialog::Accepted);
}

void EditRailwayConnectionDlg::onModelError(const QString &msg)
{
    QMessageBox::warning(this, tr("Error"), msg);
}

void EditRailwayConnectionDlg::addTrackConn()
{
    int row = 0;
    model->addNewConnection(&row);
    if(row < 0)
        return;

    QModelIndex idx = model->index(row, 0);
    view->scrollTo(idx);
    view->edit(idx);
}

void EditRailwayConnectionDlg::removeSelectedTrackConn()
{
    if(!view->selectionModel()->hasSelection())
        return;

    QModelIndex idx = view->currentIndex();
    if(!idx.isValid())
        return;

    model->removeAtRow(idx.row());
}

void EditRailwayConnectionDlg::addDefaultConnections()
{
    model->createDefaultConnections();
}

void EditRailwayConnectionDlg::setReadOnly(bool readOnly)
{
    model->setReadOnly(readOnly);
    view->setEditTriggers(readOnly ? QTableView::NoEditTriggers : QTableView::DoubleClicked);
}
