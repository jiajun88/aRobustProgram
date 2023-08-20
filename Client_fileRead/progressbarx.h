#ifndef PROGRESSBARX_H
#define PROGRESSBARX_H

#include <QObject>
#include <QThread>
#include <QProgressBar>

class progressBarX : public QThread
{
    Q_OBJECT
public:
    explicit progressBarX(QProgressBar* pB, QProgressBar* pB2, QProgressBar* pB3, QObject *parent = nullptr);

protected:
    void run() override;

private:
    QProgressBar* progressBar;
    QProgressBar* progressBar2;
    QProgressBar* progressBar3;
//    int m_value = 0;

public:
    void pBValue(int);
    void pBValue2(int);
    void pBValue3(int);

signals:
    void setValue(int);
    void setValue2(int);
    void setValue3(int);
};

#endif // PROGRESSBARX_H
