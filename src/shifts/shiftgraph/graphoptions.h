#ifndef GRAPHOPTIONS_H
#define GRAPHOPTIONS_H

#include <QDialog>

#include "utils/types.h"

class QListView;
class QCheckBox;
class QPushButton;

class GraphOptions : public QDialog
{
    Q_OBJECT
public:
    explicit GraphOptions(QWidget *parent = nullptr);

    bool hideSameStation() const;
    void setHideSameStations(bool value);

private:
    QListView *view;

    QPushButton *selectAllBut;
    QPushButton *selectNoneBut;

    QCheckBox *stationCheck;
};

#endif // GRAPHOPTIONS_H
