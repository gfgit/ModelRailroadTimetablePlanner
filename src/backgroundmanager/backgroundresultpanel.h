#ifndef BACKGROUNDRESULTPANEL_H
#define BACKGROUNDRESULTPANEL_H

#ifdef ENABLE_BACKGROUND_MANAGER

#include <QTabWidget>

class IBackgroundChecker;

class BackgroundResultPanel : public QTabWidget
{
    Q_OBJECT
public:
    explicit BackgroundResultPanel(QWidget *parent = nullptr);

private slots:
    void addChecker(IBackgroundChecker *mgr);
    void removeChecker(IBackgroundChecker *mgr);
};

#endif // ENABLE_BACKGROUND_MANAGER

#endif // BACKGROUNDRESULTPANEL_H
