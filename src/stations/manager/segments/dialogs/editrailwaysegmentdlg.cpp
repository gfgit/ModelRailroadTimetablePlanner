#include "editrailwaysegmentdlg.h"

#include <QGridLayout>
#include <QFormLayout>

#include <QMessageBox>

#include <QGroupBox>
#include <QSpinBox>
#include "utils/delegates/kmspinbox/kmspinbox.h"
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QPushButton>
#include "utils/delegates/sql/customcompletionlineedit.h"

#include "utils/owningqpointer.h"

#include "stations/match_models/stationsmatchmodel.h"
#include "stations/match_models/stationgatesmatchmodel.h"

#include "stations/manager/segments/model/railwaysegmenthelper.h"
#include "stations/manager/segments/model/railwaysegmentconnectionsmodel.h"

#include "stations/manager/segments/dialogs/editrailwayconnectiondlg.h"

EditRailwaySegmentDlg::EditRailwaySegmentDlg(sqlite3pp::database &db, QWidget *parent) :
    QDialog(parent),
    m_segmentId(0),
    m_lockStationId(0),
    m_lockGateId(0),
    reversed(false)
{
    fromStationMatch = new StationsMatchModel(db, this);
    toStationMatch = new StationsMatchModel(db, this);

    fromGateMatch = new StationGatesMatchModel(db, this);
    toGateMatch = new StationGatesMatchModel(db, this);

    fromStationEdit = new CustomCompletionLineEdit(fromStationMatch);
    fromGateEdit = new CustomCompletionLineEdit(fromGateMatch);
    connect(fromStationEdit, &CustomCompletionLineEdit::dataIdChanged,
            this,            &EditRailwaySegmentDlg::onFromStationChanged);
    connect(fromGateEdit, &CustomCompletionLineEdit::dataIdChanged,
            this,          &EditRailwaySegmentDlg::updateTrackConnectionModel);

    toStationEdit = new CustomCompletionLineEdit(toStationMatch);
    toGateEdit = new CustomCompletionLineEdit(toGateMatch);
    connect(toStationEdit, &CustomCompletionLineEdit::dataIdChanged,
            this,          &EditRailwaySegmentDlg::onToStationChanged);
    connect(toGateEdit, &CustomCompletionLineEdit::dataIdChanged,
            this,          &EditRailwaySegmentDlg::updateTrackConnectionModel);

    helper = new RailwaySegmentHelper(db);
    connModel = new RailwaySegmentConnectionsModel(db, this);

    segmentNameEdit = new QLineEdit;
    segmentNameEdit->setPlaceholderText(tr("Segment name..."));

    distanceSpin = new KmSpinBox;
    distanceSpin->setPrefix(tr("Km "));

    maxSpeedSpin = new QSpinBox;
    maxSpeedSpin->setRange(10, 999);
    maxSpeedSpin->setSuffix(tr(" km/h"));

    electifiedCheck = new QCheckBox;

    fromBox = new QGroupBox;
    QFormLayout *fromLay = new QFormLayout(fromBox);
    fromLay->addRow(tr("Station:"), fromStationEdit);
    fromLay->addRow(tr("Gate:"), fromGateEdit);

    toBox = new QGroupBox;
    QFormLayout *toLay = new QFormLayout(toBox);
    toLay->addRow(tr("Station:"), toStationEdit);
    toLay->addRow(tr("Gate:"), toGateEdit);

    QGroupBox *segmentBox = new QGroupBox(tr("Segment"));
    QFormLayout *segmentLay = new QFormLayout(segmentBox);
    segmentLay->addRow(tr("Name:"), segmentNameEdit);
    segmentLay->addRow(tr("Distance:"), distanceSpin);
    segmentLay->addRow(tr("Max. Speed:"), maxSpeedSpin);
    segmentLay->addRow(tr("Electrified:"), electifiedCheck);

    QPushButton *editConnBut = new QPushButton(tr("Edit"));
    connect(editConnBut, &QPushButton::clicked, this, &EditRailwaySegmentDlg::editSegmentTrackConnections);
    segmentLay->addRow(tr("Track connections:"), editConnBut);

    QGridLayout *lay = new QGridLayout(this);
    lay->addWidget(fromBox, 0, 0);
    lay->addWidget(toBox, 0, 1);
    lay->addWidget(segmentBox, 1, 0, 1, 2);

    QDialogButtonBox *box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                                                 Qt::Horizontal);
    connect(box, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(box, &QDialogButtonBox::accepted, this, &QDialog::accept);
    lay->addWidget(box, 2, 0, 1, 2);

    setMinimumSize(200, 200);
}

EditRailwaySegmentDlg::~EditRailwaySegmentDlg()
{
    delete helper;
    helper = nullptr;
}

void EditRailwaySegmentDlg::done(int res)
{
    if(res == QDialog::Accepted)
    {
        QString tmp;
        db_id fromStId = 0;
        db_id fromGateId = 0;
        db_id toStId = 0;
        db_id toGateId = 0;
        const QString segName = segmentNameEdit->text().simplified();

        if(!fromStationEdit->getData(fromStId, tmp))
        {
            QMessageBox::warning(this, tr("Error"),
                                 tr("Origin station must be valid."));
            return;
        }
        if(!fromGateEdit->getData(fromGateId, tmp))
        {
            QMessageBox::warning(this, tr("Error"),
                                 tr("Origin station gate must be valid."));
            return;
        }
        if(!toStationEdit->getData(toStId, tmp))
        {
            QMessageBox::warning(this, tr("Error"),
                                 tr("Destination station must be valid."));
            return;
        }
        if(!toGateEdit->getData(toGateId, tmp))
        {
            QMessageBox::warning(this, tr("Error"),
                                 tr("Destination station gate must be valid."));
            return;
        }

        if(segName.isEmpty())
        {
            QMessageBox::warning(this, tr("Error"),
                                 tr("Segment name must not be empty."));
            return;
        }

        QFlags<utils::RailwaySegmentType> type;
        if(electifiedCheck->isChecked()) //FIXME: other types also
            type.setFlag(utils::RailwaySegmentType::Electrified);

        const int distanceMeters = distanceSpin->value();
        const int speedKmH = maxSpeedSpin->value();

        if(reversed)
        {
            //Revert to original
            qSwap(fromStId, toStId);
            qSwap(fromGateId, toGateId);
        }

        QString errMsg;
        if(!helper->setSegmentInfo(m_segmentId, m_segmentId == 0,
                                    segName, utils::RailwaySegmentType(int(type)),
                                    distanceMeters, speedKmH,
                                    fromGateId, toGateId, &errMsg))
        {
            QMessageBox::warning(this, tr("Error"),
                                 tr("Database error: %1").arg(errMsg));
            return;
        }

        connModel->applyChanges(m_segmentId);
    }

    QDialog::done(res);
}

void EditRailwaySegmentDlg::setSegment(db_id segmentId, db_id lockStId, db_id lockGateId)
{
    m_segmentId = segmentId;
    m_lockStationId = lockStId;
    m_lockGateId = lockGateId;
    reversed = false;

    if(m_lockStationId == LockToCurrentValue)
    {
        //Not supported on station
        //Because we need to know if segment is reversed
        m_lockStationId = DoNotLock;
    }
    if(m_lockStationId == DoNotLock)
        m_lockGateId = DoNotLock; //Cannot lock gate without locking station

    RailwaySegmentHelper::SegmentInfo info;
    info.segmentId = m_segmentId;
    info.distanceMeters = 10000; // 10 km
    info.maxSpeedKmH = 120;   // 120 km/h

    info.from.stationId = m_lockStationId;
    info.from.gateId = m_lockGateId;
    info.to.stationId = 0;
    info.to.gateId = 0;

    QFlags<utils::RailwaySegmentType> type;

    if(segmentId)
    {
        if(!helper->getSegmentInfo(info))
        {
            return; //TODO: error reporting
        }
        type = info.type;

        if(m_lockStationId == info.to.stationId)
        {
            //Reverse segment
            //Locked station must appear as 'From:'
            qSwap(info.from, info.to);
            reversed = true;
        }
        else if(m_lockStationId != info.from.stationId)
        {
            //It's neither normal nor reversed
            m_lockStationId = DoNotLock;
            m_lockGateId = DoNotLock;
        }

        if(m_lockGateId == LockToCurrentValue)
        {
            //Lock to current gate
            m_lockGateId = info.from.gateId;
        }
        else if(m_lockGateId != info.from.gateId)
        {
            //User passed different gate, do not lock
            m_lockGateId = DoNotLock;
        }

        setWindowTitle(tr("Edit Railway Segment"));
    }
    else
    {
        //It's a new segment
        setWindowTitle(tr("New Railway Segment"));
    }

    segmentNameEdit->setText(info.segmentName);
    distanceSpin->setValue(info.distanceMeters);
    maxSpeedSpin->setValue(info.maxSpeedKmH);
    electifiedCheck->setChecked(type.testFlag(utils::RailwaySegmentType::Electrified));
    //FIXME: add support for other types

    fromStationEdit->setData(info.from.stationId);
    fromGateEdit->setData(info.from.gateId);
    toStationEdit->setData(info.to.stationId);
    toGateEdit->setData(info.to.gateId);

    if(m_lockStationId == DoNotLock)
    {
        fromStationMatch->setFilter(0);
        fromStationMatch->refreshData();
        toStationMatch->setFilter(0);
    }
    else
    {
        //Filter out 'From:' station
        toStationMatch->setFilter(info.from.stationId);
    }
    toStationMatch->refreshData();

    //If origin is locked prevent editing
    fromStationEdit->setReadOnly(m_lockStationId != DoNotLock);
    fromGateEdit->setReadOnly(m_lockGateId != DoNotLock);

    fromBox->setTitle(m_lockGateId == DoNotLock ? tr("From") : tr("From (Locked)"));
    toBox->setTitle(reversed ? tr("To (Reversed)") : tr("To"));
}

void EditRailwaySegmentDlg::onFromStationChanged(db_id stationId)
{
    fromGateMatch->setFilter(stationId, true, m_segmentId);
    fromGateEdit->setData(0); //Clear gate
}

void EditRailwaySegmentDlg::onToStationChanged(db_id stationId)
{
    toGateMatch->setFilter(stationId, true, m_segmentId);
    toGateEdit->setData(0); //Clear gate
}

void EditRailwaySegmentDlg::updateTrackConnectionModel()
{
    QString tmp;
    db_id fromGateId = 0;
    db_id toGateId = 0;

    fromGateEdit->getData(fromGateId, tmp);
    toGateEdit->getData(toGateId, tmp);

    if(!fromGateId || !toGateId)
    {
        connModel->clear();
        return;
    }

    connModel->setSegment(m_segmentId, fromGateId, toGateId, reversed);
    connModel->resetData();
    if(connModel->getActualCount() == 0)
        connModel->createDefaultConnections();
}

void EditRailwaySegmentDlg::editSegmentTrackConnections()
{
    OwningQPointer<EditRailwayConnectionDlg> dlg(new EditRailwayConnectionDlg(connModel, this));
    dlg->exec();
}
