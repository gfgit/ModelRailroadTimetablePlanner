#ifndef STOPDELEGATE_H
#define STOPDELEGATE_H

#include <QStyledItemDelegate>

namespace sqlite3pp {
class database;
}

class StopEditor;

/*!
 * \brief The StopDelegate class
 *
 * Item delegate to draw job stops in JobPathEditor
 *
 * \sa JobPathEditor
 * \sa StopEditor
 * \sa StopModel
 */
class StopDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
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

signals:
    /*!
     * \brief popupEditorSegmentCombo
     *
     * Tell editor to popup segment combo
     * \sa StopEditor::popupSegmentCombo()
     */
    void popupEditorSegmentCombo();

private slots:
    /*!
     * \brief onEditorSegmentChosen
     * \param editor the instance which needs to be closed
     *
     * User has chosen a valid next segment for current stop
     * If editor should be closed, close it and edit next stop if available
     *
     * \sa StopEditor::closeOnSegmentChosen()
     * \sa StopEditor::nextSegmentChosen()
     */
    void onEditorSegmentChosen(StopEditor *editor);

private:
    void refreshPixmaps();

private:
    sqlite3pp::database &mDb;

    QPixmap m_lightningPix;
    QPixmap m_reverseDirPix;

    static constexpr int NormalStopHeight = 100;
    static constexpr int TransitStopHeight = 80;
    static constexpr int AddHereHeight = 30;

    static constexpr int PixWidth = 35;
    static constexpr int PixHeight = PixWidth;
};

#endif // STOPDELEGATE_H
