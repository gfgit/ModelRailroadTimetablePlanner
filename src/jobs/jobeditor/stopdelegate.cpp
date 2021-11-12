#include "stopdelegate.h"

#include "stopeditor.h"

#include "utils/model_roles.h"

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
    font.setBold(item.type != StopType::Transit); //TransitLineChange is bold too to notify the line change

    painter->setFont(font);

    painter->setBrush(option.palette.text());

    const bool isTransit = (item.type == StopType::Transit || item.type == StopType::TransitLineChange);

    const double top = rect.top();
    const double bottom = rect.bottom();
    const double left = rect.left();
    const double width = rect.width();
    const double height = rect.height();

    const double stHeight = top + (isTransit ? 0.0 : height * 0.1);
    const double timeHeight = top + height * (item.type == TransitLineChange ? 0.3 : 0.4);
    const double lineHeight = top + height * (isTransit ? 0.6 : 0.7);

    const double arrX = left + width * (isTransit ? 0.4 : 0.2);
    const double depX = left + width * 0.6;
    const double lineX = left + (isTransit ? width * 0.1 : 0.0);


    int addHere = index.data(ADDHERE_ROLE).toInt();
    if(addHere == 0)
    {
        //Draw item
        //Station name
        painter->drawText(QRectF(left, stHeight, width, bottom - stHeight),
                          station,
                          QTextOption(Qt::AlignHCenter));

        if(item.type != First)
        {
            //Arrival
            painter->drawText(QRectF(arrX, timeHeight, width, bottom - timeHeight),
                              item.arrival.toString("HH:mm"));
        }

        if(item.type == First || item.type == Normal) //Last, Transit, TransitLineChange don't have a separate departure
        {
            //Departure
            painter->drawText(QRectF(depX, timeHeight, width, bottom - timeHeight),
                              item.departure.toString("HH:mm"));
        }

        if(item.nextLine != 0 || item.type == TransitLineChange || item.type == First)
        {
            q.prepare("SELECT type,name FROM lines WHERE id=?");
            q.bind(1, item.nextLine);
            q.step();
            auto r = q.getRows();
            LineType nextLineType = LineType(r.get<int>(0));
            QString lineName = r.get<QString>(1);
            q.reset();

            //Line name (on First or on line change)
            painter->drawText(QRectF(lineX, lineHeight, width, bottom - lineHeight),
                              tr("Line: %1").arg(lineName),
                              QTextOption(Qt::AlignHCenter));

            if(item.type != Last)
            {
                LineType oldLineType = LineType(-1);
                if(item.type != First)
                {
                    q.bind(1, item.curLine);
                    q.step();
                    oldLineType = LineType(q.getRows().get<int>(0));
                    q.reset();
                }

                if(oldLineType != nextLineType)
                {
                    QSizeF s = QSizeF(renderer->defaultSize()).scaled(width, height / 2, Qt::KeepAspectRatio);
                    QRectF lightningRect(left, top + height / 4, s.width(), s.height());
                    renderer->render(painter, lightningRect);

                    if(nextLineType != LineType::Electric)
                    {
                        painter->setPen(QPen(Qt::red, 4));
                        painter->drawLine(lightningRect.topLeft(), lightningRect.bottomRight());
                    }
                }
            }
        }

        if(isTransit)
        {
            painter->setPen(QPen(Qt::red, 5));
            painter->setBrush(Qt::red);
            painter->drawLine(QLineF(rect.left() + rect.width() * 0.2, rect.top(),
                                     rect.left() + rect.width() * 0.2, rect.bottom()));

            painter->drawEllipse(QRectF(rect.left() + rect.width() * 0.2 - 12 / 2,
                                        rect.top() + rect.height() * 0.4,
                                        12, 12));
        }

    }
    else if (addHere == 1)
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
    StopType t = getStopType(index);
    if(t == Transit)
        h = 60;
    else if(t == TransitLineChange)
        h = 80;
    if(index.data(ADDHERE_ROLE).toInt() != 0)
        h = 30;
    return QSize(w, h);
}

QWidget *StopDelegate::createEditor(QWidget *parent,
                                    const QStyleOptionViewItem &/*option*/,
                                    const QModelIndex &index) const

{
    StopType type = getStopType(index);
    int addHere = index.data(ADDHERE_ROLE).toInt();
    if( addHere != 0)
    {
        qDebug() << index << "is AddHere";
        return nullptr;
    }

    StopEditor *editor = new StopEditor(mDb, parent);
    editor->setAutoFillBackground(true);
    editor->setStopType(type);
    editor->setEnabled(false); //Mark it

    //Prevent JobPathEditor context menu in table view during stop editing
    editor->setContextMenuPolicy(Qt::PreventContextMenu);

    //See 'StopEditor::popupLinesCombo'
    connect(this, &StopDelegate::popupEditorLinesCombo, editor, &StopEditor::popupLinesCombo);
    connect(editor, &StopEditor::lineChosen, this, &StopDelegate::onLineChosen);

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

    int r = index.row();
    if(r > 0)
    {
        const StopItem prev = model->getItemAt(r - 1);
        ed->setPrevSt(prev.stationId);
        ed->setPrevDeparture(prev.departure);
    }

    ed->setStation(item.stationId);
    ed->setArrival(item.arrival);
    ed->setDeparture(item.departure);
    ed->setCurLine(item.curLine);
    if(item.nextLine)
        ed->setNextLine(item.nextLine);

    ed->calcInfo();
}

void StopDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
                                const QModelIndex &index) const
{
    DEBUG_IMPORTANT_ENTRY;

    qDebug() << "End editing: stop" << index.row();
    StopEditor *ed = static_cast<StopEditor*>(editor);
    model->setData(index, ed->getArrival(), ARR_ROLE);
    model->setData(index, ed->getDeparture(), DEP_ROLE);
    model->setData(index, ed->getStation(), STATION_ID);

    db_id nextLine = ed->getNextLine();
    qDebug() << "NextLine:" << nextLine;
    model->setData(index, nextLine, NEXT_LINE_ROLE);
}

void StopDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &/*index*/) const
{
    editor->setGeometry(option.rect);
}

void StopDelegate::loadIcon(const QString &fileName)
{
    renderer->load(fileName);
}

void StopDelegate::onLineChosen(StopEditor *editor)
{
    if(editor->closeOnLineChosen())
    {
        commitData(editor);
        closeEditor(editor, StopDelegate::EditNextItem);
    }
}
