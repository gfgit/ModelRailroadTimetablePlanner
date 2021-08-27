#ifndef LINEGRAPHTOOLBAR_H
#define LINEGRAPHTOOLBAR_H

#include <QWidget>

#include "utils/types.h"

class QComboBox;
class QPushButton;
class CustomCompletionLineEdit;

class LineGraphScene;
class ISqlFKMatchModel;

/*!
 * \brief Toolbar to select railway stations or lines
 *
 * Consist of a combobox and a line edit
 * The combo box selects the content type (\sa LineGraphType)
 * The line edit allows to choose which item of selected type
 * should be shown.
 */
class LineGraphToolbar : public QWidget
{
    Q_OBJECT
public:
    explicit LineGraphToolbar(QWidget *parent = nullptr);
    ~LineGraphToolbar();

    void setScene(LineGraphScene *scene);

signals:
    void requestRedraw();

public slots:
    void resetToolbarToScene();

private slots:
    void onGraphChanged(int type, db_id objectId);
    void onTypeComboActivated(int index);
    void onCompletionDone();
    void onSceneDestroyed();

private:
    void setupModel(int type);

private:
    LineGraphScene *m_scene;

    QComboBox *graphTypeCombo;
    CustomCompletionLineEdit *objectCombo;
    QPushButton *redrawBut;

    ISqlFKMatchModel *matchModel;

    int oldGraphType;
};

#endif // LINEGRAPHTOOLBAR_H
