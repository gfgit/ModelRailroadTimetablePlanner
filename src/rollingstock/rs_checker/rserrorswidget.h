#ifndef RSERRORSWIDGET_H
#define RSERRORSWIDGET_H

#ifdef ENABLE_RS_CHECKER

#include <QWidget>

class QTreeView;
class QProgressBar;
class QPushButton;

class RsErrorsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit RsErrorsWidget(QWidget *parent = nullptr);

protected:
    void timerEvent(QTimerEvent *event);

private slots:
    void startTask();
    void stopTask();
    void taskProgressMax(int max);
    void taskFinished();
    void showContextMenu(const QPoint &pos);

private:
    QTreeView *view;
    QProgressBar *progressBar;
    QPushButton *startBut;
    QPushButton *stopBut;
    int timerId;
};

#endif // ENABLE_RS_CHECKER

#endif // RSERRORSWIDGET_H
