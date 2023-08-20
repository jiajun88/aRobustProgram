#include "progressbarx.h"
#include <QDebug>

progressBarX::progressBarX(QProgressBar* pB, QProgressBar* pB2, QProgressBar* pB3, QObject *parent)
    : QThread{parent}
{
    progressBar = pB;
    progressBar2 = pB2;
    progressBar3 = pB3;
}

void progressBarX::run()
{
    connect(this, &progressBarX::setValue, this, [=](int value){
        qDebug() << "value = " << value;
        progressBar->setValue(value);
    });
    connect(this, &progressBarX::setValue2, this, [=](int value){
        qDebug() << "value = " << value;
        progressBar2->setValue(value);
    });
    connect(this, &progressBarX::setValue3, this, [=](int value){
        qDebug() << "value = " << value;
        progressBar3->setValue(value);
    });

    exec();
}

void progressBarX::pBValue(int value)
{
    emit setValue(value);
}

void progressBarX::pBValue2(int value)
{
    emit setValue2(value);
}

void progressBarX::pBValue3(int value)
{
    emit setValue3(value);
}
