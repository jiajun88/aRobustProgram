#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDateTime>
#include <QString>
#include <QFile>
#include <QThread>

#include "uuv1.h"
#include "uuv2.h"
#include "uuv3.h"
#include "progressbarx.h"


QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

signals:
    void startConnectUUV1(unsigned short, QString);
    void startConnectUUV2(unsigned short, QString);
    void startConnectUUV3(unsigned short, QString);

    void readReadyOKUUV1(int, int);
    void readReadyOKUUV2(int, int);
    void readReadyOKUUV3(int, int);

    void disConnectUUV1();
    void disConnectUUV2();
    void disConnectUUV3();

private slots:
    void on_connectUUV1_clicked();
    void on_connectUUV2_clicked();
    void on_connectUUV3_clicked();

    void on_queryUUV1PkgNums_clicked();
    void on_queryUUV2PkgNums_clicked();
    void on_queryUUV3PkgNums_clicked();

    void on_readReadyOK1_clicked();
    void on_disconnectUUV1_clicked();
    void on_readReadyOK2_clicked();
    void on_disconnectUUV2_clicked();
    void on_readReadyOK3_clicked();
    void on_disconnectUUV3_clicked();

private:
    Ui::MainWindow *ui;

private:
    QFile* journalTxtUUV1;
    QFile* journalTxtUUV2;
    QFile* journalTxtUUV3;

    QThread* threadUUV1;
    QThread* threadUUV2;
    QThread* threadUUV3;
    UUV1* taskUUV1;
    UUV2* taskUUV2;
    UUV3* taskUUV3;

    progressBarX* taskProgressBar1;

private:
    void initTextLines();
    QString getCurrentTime();
    void journalRecord(QString& record, int uuvID);
    void createJournalFile();
    void closeJournalFile();
    void releaseSource();

    void createUUV1Thread();
    void createUUV2Thread();
    void createUUV3Thread();
    void processThreadSignals();
    void processThreadSignals2();
    void processThreadSignals3();

};
#endif // MAINWINDOW_H
