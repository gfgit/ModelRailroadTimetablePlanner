#ifndef LINEGRAPHTOOLBAR_H
#define LINEGRAPHTOOLBAR_H

#include <QWidget>

#include "utils/types.h"

class QComboBox;
class CustomCompletionLineEdit;

class LineGraphScene;
class ISqlFKMatchModel;

class LineGraphToolbar : public QWidget
{
    Q_OBJECT
public:
    explicit LineGraphToolbar(QWidget *parent = nullptr);
    ~LineGraphToolbar();

    void setScene(LineGraphScene *scene);

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

    ISqlFKMatchModel *matchModel;
};

#endif // LINEGRAPHTOOLBAR_H
