#ifndef KMSPINBOX_H
#define KMSPINBOX_H

#include <QSpinBox>

class KmSpinBox : public QSpinBox
{
    Q_OBJECT
public:
    explicit KmSpinBox(QWidget *parent = nullptr);

    enum Sections
    {
        KmSection = 0,
        MetersSection
    };

    void stepBy(int steps) override;

protected:
    QValidator::State validate(QString &input, int &pos) const override;
    int valueFromText(const QString &text) const override;
    QString textFromValue(int val) const override;
    void fixup(QString &str) const override;

    bool focusNextPrevChild(bool next) override;
    void focusInEvent(QFocusEvent *e) override;
    void keyPressEvent(QKeyEvent *e) override;

private slots:
    void cursorPosChanged(int oldPos, int newPos);

private:
    void setCurrentSection(int section);
    QString stripped(const QString &t, int *pos) const;

private:
    int currentSection;
};

#endif // KMSPINBOX_H
