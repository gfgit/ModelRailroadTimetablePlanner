#ifndef SQLCONSOLE_H
#define SQLCONSOLE_H

#ifdef ENABLE_USER_QUERY

#include <QDialog>

class SQLViewer;
class QPlainTextEdit;
class QSpinBox;
class QTimer;

class SQLConsole : public QDialog
{
    Q_OBJECT
public:
    explicit SQLConsole(QWidget *parent = nullptr);
    ~SQLConsole();

    void setInterval(int secs);

public slots:
    bool executeQuery();

private slots:
    void onIntervalChangedUser();

    void timedExec();
private:
    QPlainTextEdit *edit;
    SQLViewer *viewer;
    QSpinBox *intervalSpin;
    QTimer *timer;
};

#endif // ENABLE_USER_QUERY

#endif // SQLCONSOLE_H
