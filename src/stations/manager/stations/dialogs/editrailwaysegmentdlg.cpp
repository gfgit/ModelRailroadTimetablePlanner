#include "editrailwaysegmentdlg.h"

#include <QGridLayout>
#include <QFormLayout>

#include <QMessageBox>

#include <QGroupBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QDialogButtonBox>
#include "utils/sqldelegate/customcompletionlineedit.h"

#include "stations/stationsmatchmodel.h"
#include "stations/stationgatesmatchmodel.h"

#include "stations/manager/stations/model/railwaysegmenthelper.h"

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

    toStationEdit = new CustomCompletionLineEdit(toStationMatch);
    toGateEdit = new CustomCompletionLineEdit(toGateMatch);
    connect(toStationEdit, &CustomCompletionLineEdit::dataIdChanged,
            this,          &EditRailwaySegmentDlg::onToStationChanged);

    helper = new RailwaySegmentHelper(db);

    segmentNameEdit = new QLineEdit;
    segmentNameEdit->setPlaceholderText(tr("Segment name..."));

    distanceSpin = new QSpinBox;
    distanceSpin->setRange(100, 999999);
    distanceSpin->setSuffix(tr(" meters"));

    maxSpeedSpin = new QSpinBox;
    maxSpeedSpin->setRange(10, 999);
    maxSpeedSpin->setSuffix(tr(" km/h"));

    electifiedCheck = new QCheckBox;

    QGroupBox *fromBox = new QGroupBox(tr("From"));
    QFormLayout *fromLay = new QFormLayout(fromBox);
    fromLay->addRow(tr("Station:"), fromStationEdit);
    fromLay->addRow(tr("Gate:"), fromGateEdit);

    QGroupBox *toBox = new QGroupBox(tr("To"));
    QFormLayout *toLay = new QFormLayout(toBox);
    toLay->addRow(tr("Station:"), toStationEdit);
    toLay->addRow(tr("Gate:"), toGateEdit);

    QGroupBox *segmentBox = new QGroupBox(tr("Segment"));
    QFormLayout *segmentLay = new QFormLayout(segmentBox);
    segmentLay->addRow(tr("Name:"), segmentNameEdit);
    segmentLay->addRow(tr("Distance:"), distanceSpin);
    segmentLay->addRow(tr("Max. Speed:"), maxSpeedSpin);
    segmentLay->addRow(tr("Electrified:"), electifiedCheck);

    QGridLayout *lay = new QGridLayout(this);
    lay->addWidget(fromBox, 0, 0);
    lay->addWidget(toBox, 0, 1);
    lay->addWidget(segmentBox, 1, 0, 1, 2);

    QDialogButtonBox *box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                                                 Qt::Horizontal);
    connect(box, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(box, &QDialogButtonBox::accepted, this, &QDialog::accept);
    lay->addWidget(box, 2, 0, 1, 2);
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

        if(!helper->setSegmentInfo(m_segmentId, m_segmentId == 0,
                                    segName, utils::RailwaySegmentType(int(type)),
                                    distanceMeters, speedKmH,
                                    fromGateId, toGateId))
        {
            //FIXME: show error message from helper
            QMessageBox::warning(this, tr("Error"),
                                 tr("Database error."));
            return;
        }
    }
    else
    {
        //FIXME: cannot cancel railway connections
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

    QString segName;
    QFlags<utils::RailwaySegmentType> type;
    int distance = 10000; // 10 km
    int maxSpeed = 120;   // 120 km/h

    db_id fromStId = m_lockStationId;
    db_id fromGateId = m_lockGateId;
    db_id toStId = 0;
    db_id toGateId = 0;

    if(segmentId)
    {
        //Load details TODO
        utils::RailwaySegmentType tmpType;

        if(!helper->getSegmentInfo(segmentId,
                                    segName, tmpType,
                                    distance, maxSpeed,
                                    fromStId, fromGateId,
                                    toStId, toGateId))
        {
            return; //TODO: error reporting
        }
        type = tmpType;

        if(m_lockStationId == toStId)
        {
            //Reverse segment
            //Locked station must appear as 'From:'
            qSwap(fromStId, toStId);
            qSwap(fromGateId, toGateId);
            reversed = true;
        }
        else if(m_lockStationId != fromStId)
        {
            //It's neither normal nor reversed
            m_lockStationId = DoNotLock;
            m_lockGateId = DoNotLock;
        }

        if(m_lockGateId == LockToCurrentValue)
        {
            //Lock to current gate
            m_lockGateId = fromGateId;
        }
        else if(m_lockGateId != fromGateId)
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

    segmentNameEdit->setText(segName);
    distanceSpin->setValue(distance);
    maxSpeedSpin->setValue(maxSpeed);
    electifiedCheck->setChecked(type.testFlag(utils::RailwaySegmentType::Electrified));
    //FIXME: add support for other types

    fromStationEdit->setData(fromStId);
    fromGateEdit->setData(fromGateId);
    toStationEdit->setData(toStId);
    toGateEdit->setData(toGateId);

    if(m_lockStationId == DoNotLock)
    {
        fromStationMatch->setFilter(0);
        fromStationMatch->refreshData();
        toStationMatch->setFilter(0);
    }
    else
    {
        //Filter out 'From:' station
        toStationMatch->setFilter(fromStId);
    }
    toStationMatch->refreshData();

    //If origin is locked prevent editing
    fromStationEdit->setReadOnly(m_lockStationId != DoNotLock);
    fromGateEdit->setReadOnly(m_lockGateId != DoNotLock);
}

void EditRailwaySegmentDlg::onFromStationChanged(db_id stationId)
{
    fromGateMatch->setFilter(stationId, true);
    fromGateEdit->setData(0); //Clear gate
}

void EditRailwaySegmentDlg::onToStationChanged(db_id stationId)
{
    toGateMatch->setFilter(stationId, true);
    toGateEdit->setData(0); //Clear gate
}
