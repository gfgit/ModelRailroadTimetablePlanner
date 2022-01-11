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

signals:
    void delayedTextChanged(FilterHeaderLineEdit *self, const QString& str);

private slots:
    void startTextTimer();
    void stopTextTimer();
    void delayedTimerFinished();

protected:
    void timerEvent(QTimerEvent *e) override;

private:
    QString lastValue;
    int m_column;
    int m_textTimerId;
};

#endif // FILTERHEADERLINEEDIT_H
