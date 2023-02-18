#ifndef BACKGROUNDRESULTWIDGET_H
#define BACKGROUNDRESULTWIDGET_H

#ifdef ENABLE_BACKGROUND_MANAGER

#include <QWidget>

class QTreeView;
class QProgressBar;
class QPushButton;

class IBackgroundChecker;

class BackgroundResultWidget : public QWidget
{
    Q_OBJECT
public:
    explicit BackgroundResultWidget(IBackgroundChecker *mgr_, QWidget *parent = nullptr);

protected:
    void timerEvent(QTimerEvent *e) override;

private slots:
    void startTask();
    void stopTask();
    void onTaskProgress(int val, int max);
    void taskFinished();
    void showContextMenu(const QPoint &pos);

private:
    friend class BackgroundResultPanel;

    IBackgroundChecker *mgr;
    QTreeView *view;
    QProgressBar *progressBar;
    QPushButton *startBut;
    QPushButton *stopBut;
    int timerId;
};

#endif // ENABLE_BACKGROUND_MANAGER

#endif // BACKGROUNDRESULTWIDGET_H
