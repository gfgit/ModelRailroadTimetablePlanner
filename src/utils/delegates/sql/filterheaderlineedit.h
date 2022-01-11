#ifndef FILTERHEADERLINEEDIT_H
#define FILTERHEADERLINEEDIT_H

#include <QLineEdit>

class FilterHeaderLineEdit : public QLineEdit
{
    Q_OBJECT
public:
    FilterHeaderLineEdit(int col, QWidget *parent = nullptr);
    ~FilterHeaderLineEdit();

    void updateTextWithoutEmitting(const QString& str);

    inline int getColumn() const { return m_column; }

    inline int allowNull() const { return m_allowNull; }
    void setAllowNull(bool val);

signals:
    void delayedTextChanged(FilterHeaderLineEdit *self, const QString& str);
    void tooltipRequested(FilterHeaderLineEdit *self, const QPoint& globalPos);

private slots:
    void startTextTimer();
    void stopTextTimer();
    void delayedTimerFinished();
    void setNULLFilter();

protected:
    bool event(QEvent *e) override;
    void timerEvent(QTimerEvent *e) override;
    void contextMenuEvent(QContextMenuEvent *e) override;

private:
    QAction *actionFilterNull;

    QString lastValue;
    int m_column;
    int m_textTimerId;
    bool m_allowNull;
};

#endif // FILTERHEADERLINEEDIT_H
