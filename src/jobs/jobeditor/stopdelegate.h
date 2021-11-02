#ifndef STOPDELEGATE_H
#define STOPDELEGATE_H

#include <QStyledItemDelegate>

#include "utils/model_roles.h"
#include "utils/types.h"

#include <QSvgRenderer>

namespace sqlite3pp {
class database;
}

class StopEditor;

class StopDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:

    static inline StopType getStopType(const QModelIndex& idx)
    {
        return static_cast<StopType>(idx.data(STOP_TYPE_ROLE).toInt());
    }

    static inline void setStopType(QAbstractItemModel *m, const QModelIndex& idx, StopType type)
    {
        m->setData(idx, int(type), STOP_TYPE_ROLE);
    }

    StopDelegate(sqlite3pp::database &db, QObject *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const override;
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                          const QModelIndex &index) const override;
    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    void setModelData(QWidget *editor, QAbstractItemModel *model,
                      const QModelIndex &index) const override;

    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &) const override;

    void loadIcon(const QString& fileName);

private:
    QSvgRenderer *renderer;
    sqlite3pp::database &mDb;
};

#endif // STOPDELEGATE_H
