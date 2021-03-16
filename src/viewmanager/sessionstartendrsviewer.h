#ifndef SESSIONSTARTENDRSVIEWER_H
#define SESSIONSTARTENDRSVIEWER_H

#include <QWidget>

class QTreeView;
class QComboBox;
class SessionStartEndModel;

class SessionStartEndRSViewer : public QWidget
{
    Q_OBJECT
public:
    explicit SessionStartEndRSViewer(QWidget *parent = nullptr);

private slots:
    void orderChanged();
    void modeChanged();

    void exportSheet();
private:
    SessionStartEndModel *model;

    QTreeView *view;

    QComboBox *modeCombo;
    QComboBox *orderCombo;
};

#endif // SESSIONSTARTENDRSVIEWER_H
