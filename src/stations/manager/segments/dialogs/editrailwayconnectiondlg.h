#ifndef EDITRAILWAYCONNECTIONDLG_H
#define EDITRAILWAYCONNECTIONDLG_H

#include <QDialog>

class RailwaySegmentConnectionsModel;

class SpinBoxEditorFactory;
class QTableView;

class EditRailwayConnectionDlg : public QDialog
{
    Q_OBJECT
public:
    EditRailwayConnectionDlg(RailwaySegmentConnectionsModel *m, QWidget *parent = nullptr);
    ~EditRailwayConnectionDlg();

    virtual void done(int res) override;

    void setReadOnly(bool readOnly);

private slots:
    void onModelError(const QString& msg);
    void addTrackConn();
    void removeSelectedTrackConn();
    void addDefaultConnections();

private:
    RailwaySegmentConnectionsModel *model;

    QTableView *view;
    SpinBoxEditorFactory *fromTrackFactory;
    SpinBoxEditorFactory *toTrackFactory;
};

#endif // EDITRAILWAYCONNECTIONDLG_H
