#ifndef STOPEDITOR_H
#define STOPEDITOR_H

#include <QFrame>
class StopModel;
struct StopItem;

class QTimeEdit;
class QSpinBox;
class QGridLayout;

namespace sqlite3pp {
class database;
}

class StopEditingHelper;

/*!
 * \brief The StopEditor class
 *
 * Widget to edit job stops inside JobPathEditor
 *
 * \sa JobPathEditor
 * \sa StopModel
 */
class StopEditor : public QFrame
{
    Q_OBJECT
public:
    StopEditor(sqlite3pp::database &db, StopModel *m, QWidget *parent = nullptr);
    ~StopEditor();

    void setStop(const StopItem& item, const StopItem& prev);

    const StopItem& getCurItem() const;
    const StopItem& getPrevItem() const;

    /*!
     * \brief closeOnSegmentChosen
     * \return true if editor should be closed after user has chosen a valid next segment
     */
    inline bool closeOnSegmentChosen() const { return m_closeOnSegmentChosen; };
    void setCloseOnSegmentChosen(bool value);

signals:
    /*!
     * \brief nextSegmentChosen
     * \param ed self instance
     *
     * Emitted when user has chosen next segment
     * And it is succesfully applied (it passes checks)
     */
    void nextSegmentChosen(StopEditor *ed);

public slots:
    /*!
     * \brief Popup next segment combo
     *
     * Popoup suggestions if there are multiple opptions available
     * If only one segment can be set as next segment, choose it automatically
     * and close editor.
     * This is done because QAbstractItemView::edit() does not let you pass additional arguments
     *
     * \sa nextSegmentChosen()
     */
    void popupSegmentCombo();

private slots:
    void onHelperSegmentChoosen();

private:
    QGridLayout *lay;

    StopEditingHelper *helper;

    QTimeEdit *arrEdit;
    QTimeEdit *depEdit;
    QSpinBox *mOutGateTrackSpin;

    bool m_closeOnSegmentChosen;
};

#endif // STOPEDITOR_H
