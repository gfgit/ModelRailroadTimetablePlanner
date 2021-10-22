#ifndef LINEGRAPHSELECTIONWIDGET_H
#define LINEGRAPHSELECTIONWIDGET_H

#include <QWidget>

#include "utils/types.h"
#include "graph/linegraphtypes.h"

class QComboBox;
class QPushButton;
class CustomCompletionLineEdit;
class ISqlFKMatchModel;

/*!
 * \brief Widget to select railway station, segment or line
 *
 * Consist of a combobox and a line edit
 * The combo box selects the content type (\sa LineGraphType)
 * The line edit allows to choose which item of selected type
 * should be shown.
 */
class LineGraphSelectionWidget : public QWidget
{
    Q_OBJECT

public:
    explicit LineGraphSelectionWidget(QWidget *parent = nullptr);
    ~LineGraphSelectionWidget();

    LineGraphType getGraphType() const;
    void setGraphType(LineGraphType type);

    db_id getObjectId() const;
    void setObjectId(db_id objectId, const QString& name);

signals:
    void graphChanged(int type, db_id objectId);

private slots:
    void onTypeComboActivated(int index);
    void onCompletionDone();

private:
    void setupModel(LineGraphType type);

private:
    QComboBox *graphTypeCombo;
    CustomCompletionLineEdit *objectCombo;

    ISqlFKMatchModel *matchModel;

    db_id m_objectId;
    LineGraphType m_graphType;
};

#endif // LINEGRAPHSELECTIONWIDGET_H
