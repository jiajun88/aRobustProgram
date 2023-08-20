#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QMessageBox>
#include <QByteArray>
#include <QIODevice>
#include <QThread>

/*
 * 每个子线程需要做2件事（都在子线程中进行）：
 *      1、基于IP和端口进行和服务器的连接（服务器传输文件完毕后自动断开连接，或者客户端主动申请断开与服务器的连接，即使此时还在传输数据）
 *      2、连接成功后，选择想要接收的包范围，点击确定，开始接收
 * 主界面需要一直维护进度条
 */

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    setWindowTitle("阵列数据多线程下载测试程序");

    // 创建日志文件
    createJournalFile();

    // 初始化一些TextLine
    initTextLines();

    // 创建负责TCP连接的子线程对象
    createUUV1Thread();
    createUUV2Thread();
    createUUV3Thread();

    // 创建负责更新进度条的子线程对象
    taskProgressBar1 = new progressBarX(ui->progressBar1, ui->progressBar2, ui->progressBar3);
    taskProgressBar1->start();

    // 处理负责TCP连接的子线程发回来的信号
    processThreadSignals();
    processThreadSignals2();
    processThreadSignals3();

    // 处理子线程与子线程之间的信号和槽
    connect(taskUUV1, &UUV1::downloaded, taskProgressBar1, &progressBarX::pBValue);
    connect(taskUUV2, &UUV2::downloaded, taskProgressBar1, &progressBarX::pBValue2);
    connect(taskUUV3, &UUV3::downloaded, taskProgressBar1, &progressBarX::pBValue3);
}

MainWindow::~MainWindow()
{
    // 关闭日志文件
    closeJournalFile();

    // 释放堆区资源
    releaseSource();

    delete ui;
}

void MainWindow::initTextLines()
{
    ui->IP1->setText("127.0.0.1");
    ui->IP2->setText("127.0.0.1");
    ui->IP3->setText("127.0.0.1");
    ui->Port1->setText("6666"); // 为了方便在本机上测试，在IP地址相同的情况下使用不同端口以模拟不同的服务器
    ui->Port2->setText("8888");
    ui->Port3->setText("9999");

    ui->queryUUV1PkgNums->setDisabled(true);
    ui->queryUUV2PkgNums->setDisabled(true);
    ui->queryUUV3PkgNums->setDisabled(true);

    ui->pkgsUUV1->setText("0");
    ui->pkgsUUV2->setText("0");
    ui->pkgsUUV3->setText("0");
    ui->pkgsUUV1->setReadOnly(true);
    ui->pkgsUUV2->setReadOnly(true);
    ui->pkgsUUV3->setReadOnly(true);

    ui->readUUV1PkgStart->setDisabled(true);
    ui->readUUV2PkgStart->setDisabled(true);
    ui->readUUV3PkgStart->setDisabled(true);
    ui->readUUV1PkgEnd->setDisabled(true);
    ui->readUUV2PkgEnd->setDisabled(true);
    ui->readUUV3PkgEnd->setDisabled(true);

    ui->readReadyOK1->setDisabled(true);
    ui->readReadyOK2->setDisabled(true);
    ui->readReadyOK3->setDisabled(true);

    ui->infoWindow1->setReadOnly(true);
    ui->infoWindow2->setReadOnly(true);
    ui->infoWindow3->setReadOnly(true);
}

QString MainWindow::getCurrentTime()
{
    return QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
}

void MainWindow::journalRecord(QString &record, int uuvID)
{
    QString currentTime = getCurrentTime();
    switch(uuvID){
    case 1: ui->infoWindow1->append(currentTime);
            journalTxtUUV1->write((currentTime + "\n").toUtf8());
            ui->infoWindow1->append(record);
            journalTxtUUV1->write((record + "\n").toUtf8());
            break;
        case 2: ui->infoWindow2->append(currentTime);
            journalTxtUUV2->write((currentTime + "\n").toUtf8());
            ui->infoWindow2->append(record);
            journalTxtUUV2->write((record + "\n").toUtf8());
            break;
        case 3: ui->infoWindow3->append(currentTime);
            journalTxtUUV3->write((currentTime + "\n").toUtf8());
            ui->infoWindow3->append(record);
            journalTxtUUV3->write((record + "\n").toUtf8());
            break;
        }
}

void MainWindow::createJournalFile()
{
    QString txtName = QDateTime::currentDateTime().toString("yyyy-MM-dd");
    journalTxtUUV1 = new QFile(txtName + "-UUV1日志文件.txt");
    journalTxtUUV1->open(QIODevice::ReadWrite | QIODevice::Text | QIODevice::Append);

    journalTxtUUV2 = new QFile(txtName + "-UUV2日志文件.txt");
    journalTxtUUV2->open(QIODevice::ReadWrite | QIODevice::Text | QIODevice::Append);

    journalTxtUUV3 = new QFile(txtName + "-UUV3日志文件.txt");
    journalTxtUUV3->open(QIODevice::ReadWrite | QIODevice::Text | QIODevice::Append);
}

void MainWindow::closeJournalFile()
{
    journalTxtUUV1->flush();
    journalTxtUUV1->close();
    journalTxtUUV2->flush();
    journalTxtUUV2->close();
    journalTxtUUV3->flush();
    journalTxtUUV3->close();
}

//---------------------------------------------------------------------------------------------------------------------------
void MainWindow::createUUV1Thread()
{
    // 创建子线程对象
    threadUUV1 = new QThread;

    // 创建任务对象
    taskUUV1 = new UUV1;

    // 将任务对象交给子线程
    taskUUV1->moveToThread(threadUUV1);

    // 指定运行哪个任务函数，但是任务函数实际执行的时机是由信号决定的，而不是线程一启动就立马运行任务函数，因此这种线程使用方式对线程是何时开启的不敏感
    // 当主线程点击“连接UUV1”按钮时，就会发射一个startConnectUUV1信号，这个信号在这里通过connect函数指定被UUV1任务类的connectionUUV1槽函数接收，用于处理与UUV1的连接问题
    connect(this, &MainWindow::startConnectUUV1, taskUUV1, &UUV1::connectionUUV1);
    connect(ui->queryUUV1PkgNums, &QPushButton::clicked, taskUUV1, &UUV1::queryPkgNums);
    connect(this, &MainWindow::readReadyOKUUV1, taskUUV1, &UUV1::readReadyOK);
    connect(this, &MainWindow::disConnectUUV1, taskUUV1, &UUV1::disConnectUUV1);

    // 启动UUV1相关的线程
    threadUUV1->start();
}

void MainWindow::createUUV2Thread()
{
    // 创建子线程对象
    threadUUV2 = new QThread;

    // 创建任务对象
    taskUUV2 = new UUV2;

    // 将任务对象交给子线程
    taskUUV2->moveToThread(threadUUV2);

    connect(this, &MainWindow::startConnectUUV2, taskUUV2, &UUV2::connectionUUV2);
    connect(ui->queryUUV2PkgNums, &QPushButton::clicked, taskUUV2, &UUV2::queryPkgNums);
    connect(this, &MainWindow::readReadyOKUUV2, taskUUV2, &UUV2::readReadyOK);
    connect(this, &MainWindow::disConnectUUV2, taskUUV2, &UUV2::disConnectUUV2);

    // 启动UUV2相关的线程
    threadUUV2->start();
}

void MainWindow::createUUV3Thread()
{
    // 创建子线程对象
    threadUUV3 = new QThread;

    // 创建任务对象
    taskUUV3 = new UUV3;

    // 将任务对象交给子线程
    taskUUV3->moveToThread(threadUUV3);

    connect(this, &MainWindow::startConnectUUV3, taskUUV3, &UUV3::connectionUUV3);
    connect(ui->queryUUV3PkgNums, &QPushButton::clicked, taskUUV3, &UUV3::queryPkgNums);
    connect(this, &MainWindow::readReadyOKUUV3, taskUUV3, &UUV3::readReadyOK);
    connect(this, &MainWindow::disConnectUUV3, taskUUV3, &UUV3::disConnectUUV3);

    // 启动UUV3相关的线程
    threadUUV3->start();
}

//------------------------------------------------------处理子线程发射回来的信号---------------------------------------------------------------------
// 处理子线程发射回来的信号
void MainWindow::processThreadSignals()
{
    connect(taskUUV1, &UUV1::connectUUV1_OK, this, [=](){
        QString record = "已成功与UUV1建立了TCP连接！";
        journalRecord(record, 1);

        ui->connectUUV1->setDisabled(true);
        ui->disconnectUUV1->setEnabled(true);
        ui->queryUUV1PkgNums->setEnabled(true);
        ui->readUUV1PkgStart->setEnabled(true);
        ui->readUUV1PkgEnd->setEnabled(true);
    });
    connect(taskUUV1, &UUV1::UUV1Disconnect, this, [=](){
        QString record = "已断开TCP连接！";
        journalRecord(record, 1);

        ui->disconnectUUV1->setDisabled(true);
        ui->queryUUV1PkgNums->setDisabled(true);
        ui->connectUUV1->setEnabled(true);
        ui->readUUV1PkgStart->setDisabled(true);
        ui->readUUV1PkgEnd->setDisabled(true);
        ui->readReadyOK1->setDisabled(true);

        // 【注意】虽然已经断开连接了，但此时还不必急着释放相关线程的资源（但是TCP连接的资源一定需要释放），因为下一次连接还需要用到
    });

    connect(taskUUV1, &UUV1::uuv1PkgNums, this, [=](QString pkgNums){
        QString record = "UUV1总包数为" + pkgNums;
        journalRecord(record, 1);

        ui->pkgsUUV1->setText(pkgNums);
        ui->queryUUV1PkgNums->setDisabled(true);
        ui->readUUV1PkgStart->setEnabled(true);
        ui->readUUV1PkgEnd->setEnabled(true);
        ui->readReadyOK1->setEnabled(true);
    });

    connect(taskUUV1, &UUV1::startReceiveData, this, [=](){
        QString record = "对端开始发送数据，本端准备接收..." ;
        journalRecord(record, 1);
        ui->readReadyOK1->setDisabled(true);

    });

    connect(taskUUV1, &UUV1::dataReceptionCompleted, this, [=](){
        QString record = "已接收完所有数据！" ;
        journalRecord(record, 1);

        ui->readReadyOK1->setDisabled(true);
    });

//    connect(taskUUV1, &UUV1::downloaded, ui->progressBar1, &QProgressBar::setValue);
//    connect(taskUUV1, &UUV1::downloaded, this, [=](int percent){
//        ui->progressBar1->setValue(percent);
//        QString record = "数据条怎么还不更新...";
//        journalRecord(record, 1);
//    });


    connect(taskUUV1, &UUV1::errorTransmission, this, [=](){
        QString record = "数据传输发生错误！";
        journalRecord(record, 1);

        // 既然发生错误了，那就重新开始下载
        ui->connectUUV1->setEnabled(true);
        ui->disconnectUUV1->setDisabled(true);
    });

    connect(taskUUV1, &UUV1::receivedPkgs, this, [=](int idxPkg){
        QString record = "已接收到第" + QString::number(idxPkg) + "个数据包";
        journalRecord(record, 1);
        ui->readReadyOK1->setDisabled(true);
    });
}

// 处理子线程2发射回来的信号
void MainWindow::processThreadSignals2()
{
    connect(taskUUV2, &UUV2::connectUUV2_OK, this, [=](){
        QString record = "已成功与UUV2建立了TCP连接！";
        journalRecord(record, 2);

        ui->connectUUV2->setDisabled(true);
        ui->disconnectUUV2->setEnabled(true);
        ui->queryUUV2PkgNums->setEnabled(true);
        ui->readUUV2PkgStart->setEnabled(true);
        ui->readUUV2PkgEnd->setEnabled(true);
    });
    connect(taskUUV2, &UUV2::UUV2Disconnect, this, [=](){
        QString record = "已断开TCP连接！";
        journalRecord(record, 2);

        ui->disconnectUUV2->setDisabled(true);
        ui->queryUUV2PkgNums->setDisabled(true);
        ui->connectUUV2->setEnabled(true);
        ui->readUUV2PkgStart->setDisabled(true);
        ui->readUUV2PkgEnd->setDisabled(true);
        ui->readReadyOK2->setDisabled(true);

        // 【注意】虽然已经断开连接了，但此时还不必急着释放相关线程的资源（但是TCP连接的资源一定需要释放），因为下一次连接还需要用到
    });

    connect(taskUUV2, &UUV2::uuv2PkgNums, this, [=](QString pkgNums){
        QString record = "UUV2总包数为" + pkgNums;
                         journalRecord(record, 2);

        ui->pkgsUUV2->setText(pkgNums);
        ui->queryUUV2PkgNums->setDisabled(true);
        ui->readUUV2PkgStart->setEnabled(true);
        ui->readUUV2PkgEnd->setEnabled(true);
        ui->readReadyOK2->setEnabled(true);
    });

    connect(taskUUV2, &UUV2::startReceiveData, this, [=](){
        QString record = "对端开始发送数据，本端准备接收..." ;
        journalRecord(record, 2);
        ui->readReadyOK2->setDisabled(true);
    });

    connect(taskUUV2, &UUV2::dataReceptionCompleted, this, [=](){
        QString record = "已接收完所有数据！" ;
        journalRecord(record, 2);

        ui->readReadyOK2->setDisabled(true);
    });

    //    connect(taskUUV2, &UUV2::downloaded, ui->progressBar2, &QProgressBar::setValue);
    //    connect(taskUUV2, &UUV2::downloaded, this, [=](int percent){
    //        ui->progressBar2->setValue(percent);
    //        QString record = "数据条怎么还不更新...";
    //        journalRecord(record, 2);
    //    });


    connect(taskUUV2, &UUV2::errorTransmission, this, [=](){
        QString record = "数据传输发生错误！";
        journalRecord(record, 2);

        // 既然发生错误了，那就重新开始下载
        ui->connectUUV2->setEnabled(true);
        ui->disconnectUUV2->setDisabled(true);
    });

    connect(taskUUV2, &UUV2::receivedPkgs, this, [=](int idxPkg){
        QString record = "已接收到第" + QString::number(idxPkg) + "个数据包";
        journalRecord(record, 2);
        ui->readReadyOK2->setDisabled(true);
    });
}

// 处理子线程3发射回来的信号
void MainWindow::processThreadSignals3()
{
    connect(taskUUV3, &UUV3::connectUUV3_OK, this, [=](){
        QString record = "已成功与UUV3建立了TCP连接！";
        journalRecord(record, 3);

        ui->connectUUV3->setDisabled(true);
        ui->disconnectUUV3->setEnabled(true);
        ui->queryUUV3PkgNums->setEnabled(true);
        ui->readUUV3PkgStart->setEnabled(true);
        ui->readUUV3PkgEnd->setEnabled(true);
    });
    connect(taskUUV3, &UUV3::UUV3Disconnect, this, [=](){
        QString record = "已断开TCP连接！";
        journalRecord(record, 3);

        ui->disconnectUUV3->setDisabled(true);
        ui->queryUUV3PkgNums->setDisabled(true);
        ui->connectUUV3->setEnabled(true);
        ui->readUUV3PkgStart->setDisabled(true);
        ui->readUUV3PkgEnd->setDisabled(true);
        ui->readReadyOK3->setDisabled(true);

        // 【注意】虽然已经断开连接了，但此时还不必急着释放相关线程的资源（但是TCP连接的资源一定需要释放），因为下一次连接还需要用到
    });

    connect(taskUUV3, &UUV3::uuv3PkgNums, this, [=](QString pkgNums){
        QString record = "UUV3总包数为" + pkgNums;
        journalRecord(record, 3);

        ui->pkgsUUV3->setText(pkgNums);
        ui->queryUUV3PkgNums->setDisabled(true);
        ui->readUUV3PkgStart->setEnabled(true);
        ui->readUUV3PkgEnd->setEnabled(true);
        ui->readReadyOK3->setEnabled(true);
    });

    connect(taskUUV3, &UUV3::startReceiveData, this, [=](){
        QString record = "对端开始发送数据，本端准备接收..." ;
        journalRecord(record, 3);
        ui->readReadyOK3->setDisabled(true);
    });

    connect(taskUUV3, &UUV3::dataReceptionCompleted, this, [=](){
        QString record = "已接收完所有数据！" ;
        journalRecord(record, 3);

        ui->readReadyOK3->setDisabled(true);
    });

    //    connect(taskUUV3, &UUV3::downloaded, ui->progressBar3, &QProgressBar::setValue);
    //    connect(taskUUV3, &UUV3::downloaded, this, [=](int percent){
    //        ui->progressBar3->setValue(percent);
    //        QString record = "数据条怎么还不更新...";
    //        journalRecord(record, 3);
    //    });


    connect(taskUUV3, &UUV3::errorTransmission, this, [=](){
        QString record = "数据传输发生错误！";
        journalRecord(record, 3);

        // 既然发生错误了，那就重新开始下载
        ui->connectUUV3->setEnabled(true);
        ui->disconnectUUV3->setDisabled(true);
    });

    connect(taskUUV3, &UUV3::receivedPkgs, this, [=](int idxPkg){
        QString record = "已接收到第" + QString::number(idxPkg) + "个数据包";
        journalRecord(record, 3);
        ui->readReadyOK3->setDisabled(true);
    });
}

//---------------------------------------------------------------------------------------------------------------------------
void MainWindow::releaseSource()
{
    taskUUV1->deleteLater();
    threadUUV1->quit();
    threadUUV1->wait();
    threadUUV1->deleteLater();

    taskUUV2->deleteLater();
    threadUUV2->quit();
    threadUUV2->wait();
    threadUUV2->deleteLater();

    taskUUV3->deleteLater();
    threadUUV3->quit();
    threadUUV3->wait();
    threadUUV3->deleteLater();
}

//---------------------------------------------------------------------------------------------------------------------------
void MainWindow::on_connectUUV1_clicked()
{
    QString ip = ui->IP1->text();
    unsigned short port = ui->Port1->text().toUShort();
    if(ip != "127.0.0.1" || ip.size() == 0 || port != 6666){
        QMessageBox::warning(this,"警告","请填写正确的IP地址或端口号！");
        QString record = "警告！请填写正确的IP地址或端口号！(errIP: " + ip + " errPort: " + ui->Port1->text() + ")";
        journalRecord(record, 1); // 日志记录
    }else{
        //ui->connectUUV1->setDisabled(true);
        ui->IP1->setReadOnly(true);
        ui->Port1->setReadOnly(true);
        ui->progressBar1->setValue(0);

        QString record = "正在连接UUV1...";
        journalRecord(record, 1); // 日志记录

        // 主线程发送开始连接的信号，它并不负责实际的连接
        emit startConnectUUV1(port, ip); // 发射带有UUV1的IP地址和端口的信号
    }
}

void MainWindow::on_connectUUV2_clicked()
{
    QString ip = ui->IP2->text();
    unsigned short port = ui->Port2->text().toUShort();
    if(ip != "127.0.0.1" || ip.size() == 0 || port != 8888){
        QMessageBox::warning(this,"警告","请填写正确的IP地址或端口号！");
        QString record = "警告！请填写正确的IP地址或端口号！(errIP: " + ip + " errPort: " + ui->Port2->text() + ")";
        journalRecord(record, 2); // 日志记录
    }else{
        //ui->connectUUV2->setDisabled(true);
        ui->IP2->setReadOnly(true);
        ui->Port2->setReadOnly(true);
        ui->progressBar2->setValue(0);

        QString record = "正在连接UUV2...";
        journalRecord(record, 2); // 日志记录

        // 主线程发送开始连接的信号，它并不负责实际的连接
        emit startConnectUUV2(port, ip); // 发射带有UUV2的IP地址和端口的信号
    }
}

void MainWindow::on_connectUUV3_clicked()
{
    QString ip = ui->IP3->text();
    unsigned short port = ui->Port3->text().toUShort();
    if(ip != "127.0.0.1" || ip.size() == 0 || port != 9999){
        QMessageBox::warning(this,"警告","请填写正确的IP地址或端口号！");
        QString record = "警告！请填写正确的IP地址或端口号！(errIP: " + ip + " errPort: " + ui->Port3->text() + ")";
        journalRecord(record, 3); // 日志记录
    }else{
        //ui->connectUUV3->setDisabled(true);
        ui->IP3->setReadOnly(true);
        ui->Port3->setReadOnly(true);
        ui->progressBar3->setValue(0);

        QString record = "正在连接UUV3...";
        journalRecord(record, 3); // 日志记录

        // 主线程发送开始连接的信号，它并不负责实际的连接
        emit startConnectUUV3(port, ip); // 发射带有UUV3的IP地址和端口的信号
    }
}

//---------------------------------------------------------------------------------------------------------------------------
void MainWindow::on_queryUUV1PkgNums_clicked()
{
    ui->queryUUV1PkgNums->setDisabled(true); // 为了避免还未查询到（UUV1未返回数据）就又点了一次或多次查询

    QString record = "正在查询UUV1总包数...";
    journalRecord(record, 1); // 日志记录
}

void MainWindow::on_queryUUV2PkgNums_clicked()
{
    ui->queryUUV2PkgNums->setDisabled(true); // 为了避免还未查询到（UUV2未返回数据）就又点了一次或多次查询

    QString record = "正在查询UUV2总包数...";
    journalRecord(record, 2); // 日志记录
}

void MainWindow::on_queryUUV3PkgNums_clicked()
{
    ui->queryUUV3PkgNums->setDisabled(true); // 为了避免还未查询到（UUV3未返回数据）就又点了一次或多次查询

    QString record = "正在查询UUV3总包数...";
    journalRecord(record, 3); // 日志记录
}

//---------------------------------------------------------------------------------------------------------------------------
void MainWindow::on_readReadyOK1_clicked()
{
    ui->readReadyOK1->setDisabled(true);
    int startPkg = ui->readUUV1PkgStart->text().toInt();
    int endPkg = ui->readUUV1PkgEnd->text().toInt();
    int pkgNums = ui->pkgsUUV1->text().toInt();
    if(startPkg < 1 || startPkg > endPkg || startPkg > pkgNums || endPkg > pkgNums){
        QMessageBox::warning(this, "警告", "请输入合理的数据包索引！");
        QString record = "警告！请填写合理的数据包索引！(PkgNums:" + ui->pkgsUUV1->text() + ",errStartPkg:" + ui->readUUV1PkgStart->text() + ",errEndPkg:" + ui->readUUV1PkgEnd->text() + ")";
        journalRecord(record, 1); // 日志记录
        return;
    }

    emit readReadyOKUUV1(startPkg, endPkg); // 通知子线程向UUV1发送下载数据包的指令

    QString record = "正在请求下载UUV1的数据包...（PkgNums:" + ui->pkgsUUV1->text() + ",startPkg:" + ui->readUUV1PkgStart->text() + ",endPkg:" + ui->readUUV1PkgEnd->text() + ")";
    journalRecord(record, 1);
    ui->queryUUV1PkgNums->setDisabled(true);
}

void MainWindow::on_readReadyOK2_clicked()
{
    int startPkg = ui->readUUV2PkgStart->text().toInt();
    int endPkg = ui->readUUV2PkgEnd->text().toInt();
    int pkgNums = ui->pkgsUUV2->text().toInt();
    if(startPkg < 1 || startPkg > endPkg || startPkg > pkgNums || endPkg > pkgNums){
        QMessageBox::warning(this, "警告", "请输入合理的数据包索引！");
        QString record = "警告！请填写合理的数据包索引！(PkgNums:" + ui->pkgsUUV2->text() + ",errStartPkg:" + ui->readUUV2PkgStart->text() + ",errEndPkg:" + ui->readUUV2PkgEnd->text() + ")";
        journalRecord(record, 2); // 日志记录
        return;
    }

    emit readReadyOKUUV2(startPkg, endPkg); // 通知子线程向UUV2发送下载数据包的指令

    QString record = "正在请求下载UUV2的数据包...（PkgNums:" + ui->pkgsUUV2->text() + ",startPkg:" + ui->readUUV2PkgStart->text() + ",endPkg:" + ui->readUUV2PkgEnd->text() + ")";
    journalRecord(record, 2);
    ui->readReadyOK2->setDisabled(true);
    ui->queryUUV2PkgNums->setDisabled(true);
}

void MainWindow::on_readReadyOK3_clicked()
{
    int startPkg = ui->readUUV3PkgStart->text().toInt();
    int endPkg = ui->readUUV3PkgEnd->text().toInt();
    int pkgNums = ui->pkgsUUV3->text().toInt();
    if(startPkg < 1 || startPkg > endPkg || startPkg > pkgNums || endPkg > pkgNums){
        QMessageBox::warning(this, "警告", "请输入合理的数据包索引！");
        QString record = "警告！请填写合理的数据包索引！(PkgNums:" + ui->pkgsUUV3->text() + ",errStartPkg:" + ui->readUUV3PkgStart->text() + ",errEndPkg:" + ui->readUUV3PkgEnd->text() + ")";
        journalRecord(record, 3); // 日志记录
        return;
    }

    emit readReadyOKUUV3(startPkg, endPkg); // 通知子线程向UUV3发送下载数据包的指令

    QString record = "正在请求下载UUV3的数据包...（PkgNums:" + ui->pkgsUUV3->text() + ",startPkg:" + ui->readUUV3PkgStart->text() + ",endPkg:" + ui->readUUV3PkgEnd->text() + ")";
    journalRecord(record, 3);
    ui->readReadyOK3->setDisabled(true);
    ui->queryUUV3PkgNums->setDisabled(true);
}

//---------------------------------------------------------------------------------------------------------------------------
void MainWindow::on_disconnectUUV1_clicked()
{
    ui->connectUUV1->setEnabled(true);
    ui->disconnectUUV1->setDisabled(true);

    QString record = "已主动与服务端断开TCP连接！";
    journalRecord(record, 1);
    emit disConnectUUV1();
}

void MainWindow::on_disconnectUUV2_clicked()
{
    ui->connectUUV2->setEnabled(true);
    ui->disconnectUUV2->setDisabled(true);

    QString record = "已主动与服务端断开TCP连接！";
    journalRecord(record, 2);
    emit disConnectUUV2();
}

void MainWindow::on_disconnectUUV3_clicked()
{
    ui->connectUUV3->setEnabled(true);
    ui->disconnectUUV3->setDisabled(true);

    QString record = "已主动与服务端断开TCP连接！";
    journalRecord(record, 3);
    emit disConnectUUV3();
}

