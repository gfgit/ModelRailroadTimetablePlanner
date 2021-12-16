#include "stopdelegate.h"

#include "stopeditor.h"

#include <QPainter>

#include <QDebug>

#include "app/session.h"
#include "model/stopmodel.h"

#include "app/scopedebug.h"

#include <QCoreApplication>

StopDelegate::StopDelegate(sqlite3pp::database &db, QObject *parent) :
    QStyledItemDelegate(parent),
    mDb(db)
{
    renderer = new QSvgRenderer(this);
    loadIcon(QCoreApplication::instance()->applicationDirPath() + QStringLiteral("/icons/lightning.svg"));
}

void StopDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                         const QModelIndex &index) const
{
    QRect rect = option.rect.adjusted(5, 5, -5, -5);

    const StopModel *model = static_cast<const StopModel *>(index.model());
    const StopItem item = model->getItemAt(index.row());
    const bool isTransit = item.type == StopType::Transit;

    query q(mDb, "SELECT name FROM stations WHERE id=?");
    q.bind(1, item.stationId);
    q.step();
    QString station = q.getRows().get<QString>(0);
    q.reset();

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    //Draw bottom border
    painter->setPen(QPen(Qt::black, 1));
    painter->drawLine(option.rect.bottomLeft(), option.rect.bottomRight());

    if (option.state & QStyle::State_Selected)
        painter->fillRect(option.rect, option.palette.highlight());

    painter->setPen(QPen(option.palette.text(), 3));

    QFont font = option.font;
    font.setPointSize(12);
    font.setBold(!isTransit);

    painter->setFont(font);

    painter->setBrush(option.palette.text());

    const double top = rect.top();
    const double bottom = rect.bottom();
    const double left = rect.left();
    const double width = rect.width();
    const double height = rect.height();

    const double stHeight = top + (isTransit ? 0.0 : height * 0.1);
    const double timeHeight = top + height * 0.4;
    const double lineHeight = top + height * 0.65;

    const double arrX = left + width * (isTransit ? 0.4 : 0.2);
    const double depX = left + width * 0.6;
    const double lineX = left + (isTransit ? width * 0.1 : 0.0);


    if(item.addHere == 0)
    {
        //Draw item
        //Station name
        painter->drawText(QRectF(left, stHeight, width, bottom - stHeight),
                          station,
                          QTextOption(Qt::AlignHCenter));

        if(item.type != StopType::First)
        {
            //Arrival
            painter->drawText(QRectF(arrX, timeHeight, width, bottom - timeHeight),
                              item.arrival.toString("HH:mm"));
        }

        if(item.type == StopType::First || item.type == StopType::Normal) //Last, Transit don't have a separate departure
        {
            //Departure
            painter->drawText(QRectF(depX, timeHeight, width, bottom - timeHeight),
                              item.departure.toString("HH:mm"));
        }

        if(item.type != StopType::Last && item.nextSegment.segmentId)
        {
            //Last has no next segment so do not draw lightning

            bool nextSegmentElectrified = model->isRailwayElectrifiedAfterRow(index.row());
            bool prevSegmentElectrified = !nextSegmentElectrified; //Trigger change on First stop

            if(item.type != StopType::First && index.row() >= 0)
            {
                //Get real previous railway type
                prevSegmentElectrified = model->isRailwayElectrifiedAfterRow(index.row() - 1);
            }

            if(nextSegmentElectrified != prevSegmentElectrified)
            {
                //Railway type changed, draw a lightning
                QSizeF s = QSizeF(renderer->defaultSize()).scaled(width, height / 2, Qt::KeepAspectRatio);
                QRectF lightningRect(left, top + height / 4, s.width(), s.height());
                renderer->render(painter, lightningRect);

                if(!nextSegmentElectrified)
                {
                    //Next railway is not electrified, cross the lightning
                    //Then keep red pen to draw next segment name
                    painter->setPen(QPen(Qt::red, 4));
                    painter->drawLine(lightningRect.topLeft(), lightningRect.bottomRight());
                }
            }

            //Draw next segment name
            q.prepare("SELECT name FROM railway_segments WHERE id=?");
            q.bind(1, item.nextSegment.segmentId);
            q.step();
            auto r = q.getRows();
            const QString segName = r.get<QString>(0);
            q.reset();

            const double lineRightX = left + width * 0.8;
            painter->drawText(QRectF(lineX, lineHeight, lineRightX - left, bottom - lineHeight),
                              tr("Seg: %1").arg(segName),
                              QTextOption(Qt::AlignHCenter));

            if(item.toGate.trackNum != 0)
            {
                painter->setPen(QPen(Qt::red, 4));
                painter->drawText(QRectF(lineRightX, lineHeight, left + width - lineRightX, bottom - lineHeight),
                                  QString::number(item.toGate.trackNum),
                                  QTextOption(Qt::AlignHCenter));
            }
        }

        if(isTransit)
        {
            const double transitLinePos = rect.left() + rect.width() * 0.1;
            painter->setPen(QPen(Qt::red, 5));
            painter->setBrush(Qt::red);
            painter->drawLine(QLineF(transitLinePos, rect.top(),
                                     transitLinePos, rect.bottom()));

            painter->drawEllipse(QRectF(transitLinePos - 12 / 2,
                                        rect.top() + rect.height() * 0.4,
                                        12, 12));
        }

    }
    else if (item.addHere == 1)
    {
        painter->drawText(rect, "Add Here", QTextOption(Qt::AlignCenter));
    }
    else
    {
        painter->drawText(rect, "Insert Here", QTextOption(Qt::AlignCenter));
    }

    painter->restore();
}

QSize StopDelegate::sizeHint(const QStyleOptionViewItem &/*option*/,
                             const QModelIndex &index) const
{
    int w = 200;
    int h = 100;
    const StopModel *model = static_cast<const StopModel *>(index.model());
    if(index.row() < 0 || index.row() >= model->rowCount())
        return QSize(w, 30);

    const StopItem& item = model->getItemAt(index.row());
    if(item.type == StopType::Transit)
        h = 80;
    if(item.addHere != 0)
        h = 30;
    return QSize(w, h);
}

QWidget *StopDelegate::createEditor(QWidget *parent,
                                    const QStyleOptionViewItem &/*option*/,
                                    const QModelIndex &index) const

{
    StopModel *model = const_cast<StopModel*>(static_cast<const StopModel *>(index.model()));
    if(model->isAddHere(index))
    {
        qDebug() << index << "is AddHere";
        return nullptr;
    }

    StopEditor *editor = new StopEditor(mDb, model, parent);
    editor->setAutoFillBackground(true);
    editor->setEnabled(false); //Mark it

    //Prevent JobPathEditor context menu in table view during stop editing
    editor->setContextMenuPolicy(Qt::PreventContextMenu);

    //See 'StopEditor::popupLinesCombo'
    connect(this, &StopDelegate::popupEditorSegmentCombo, editor, &StopEditor::popupSegmentCombo);
    connect(editor, &StopEditor::nextSegmentChosen, this, &StopDelegate::onEditorSegmentChosen);

    return editor;
}

void StopDelegate::setEditorData(QWidget *editor,
                                 const QModelIndex &index) const
{
    StopEditor *ed = static_cast<StopEditor*>(editor);
    if(ed->isEnabled()) //We already set data
        return;
    ed->setEnabled(true); //Mark it

    const StopModel *model = static_cast<const StopModel *>(index.model());

    const StopItem item = model->getItemAt(index.row());
    StopItem prev;

    int r = index.row();
    if(r > 0)
    {
        //Current stop is not First, get also previous one
        prev = model->getItemAt(r - 1);
    }

    ed->setStop(item, prev);
}

void StopDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
                                const QModelIndex &index) const
{
    DEBUG_IMPORTANT_ENTRY;

    qDebug() << "End editing: stop" << index.row();
    StopEditor *ed = static_cast<StopEditor*>(editor);
    StopModel *stopModel = static_cast<StopModel *>(model);

    stopModel->setStopInfo(index, ed->getCurItem(), ed->getPrevItem().nextSegment);
}

void StopDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &/*index*/) const
{
    editor->setGeometry(option.rect);
}

void StopDelegate::loadIcon(const QString &fileName)
{
    renderer->load(fileName);
}

void StopDelegate::onEditorSegmentChosen(StopEditor *editor)
{
    if(editor->closeOnSegmentChosen())
    {
        emit commitData(editor);
        emit closeEditor(editor, StopDelegate::EditNextItem);
    }
}
