#include "colordelegate.h"

#include "utils/model_roles.h"

#include <QPainter>

#include <QColorDialog>

ColorDelegate::ColorDelegate(QObject *parent) :
    QStyledItemDelegate(parent)
{

}

void ColorDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QRgb rgb = index.data(COLOR_ROLE).toUInt();
    QColor col(rgb);
    painter->fillRect(option.rect, col);
}

QSize ColorDelegate::sizeHint(const QStyleOptionViewItem &/*option*/, const QModelIndex &/*index*/) const
{
    return QSize();
}

QWidget *ColorDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &/*option*/, const QModelIndex &/*index*/) const
{
    QColorDialog *dlg = new QColorDialog(parent);
    connect(dlg, &QColorDialog::colorSelected, this, &ColorDelegate::commitAndCloseEditor);
    return dlg;
}

void ColorDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    QColorDialog *ed = static_cast<QColorDialog *>(editor);
    ed->setCurrentColor(index.data(COLOR_ROLE).toUInt());
}

void ColorDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    QColorDialog *ed = static_cast<QColorDialog *>(editor);
    model->setData(index, ed->currentColor().rgb(), COLOR_ROLE);
}

void ColorDelegate::commitAndCloseEditor()
{
    QColorDialog *ed = qobject_cast<QColorDialog *>(sender());
    ed->setEnabled(false);
    emit commitData(ed);
    emit closeEditor(ed);
}
