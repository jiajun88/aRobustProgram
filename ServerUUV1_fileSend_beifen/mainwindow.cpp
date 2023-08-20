#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QString>
#include <QThread>
#include <QDebug>
#include <QDir>
#include <QStringList>
#include <QDataStream>
#include <QtEndian>
#include <QFileInfo>

// 模拟发送比较简单，只管发送就可以了，使用主线程就可以完成
// 但是如果在发数据的过程中还想完成其他的操作，那就需要使用多线程了

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    initWidgets();

    // 客户端的指令集（设计为：前面12个字符是命令，后面是携带的参数信息，以'.'隔开）
    QString instruction1 = "queryPkgNums";
    QString instruction2 = "readReadyOKK";
    m_instructions[instruction1] = 1;
    m_instructions[instruction2] = 1;

    // 实例化服务器对象（本质上是进行监听的套接字对象）
    m_s = new QTcpServer(this); // 可以指定父对象以自动析构，当然也可以放到对应的析构函数中手动销毁

    // 等待客户端连接，每次有新连接可用时QTcpServer对象都会发出 newConnection 信号
    // 新连接可用，说明已经经历了三次握手，在这时QTcpServer对象才会发送 newConnection 信号
    /*
        当我们调用 QTcpServer 对象的 listen 函数时，它会由操作系统自动完成套接字绑定和监听的过程。
        当有客户端尝试连接到服务器的端口时，操作系统会自动执行 TCP 的三次握手过程，并触发 QTcpServer::newConnection 信号来通知我们有新的客户端连接已建立。
        因此，整个 TCP 的三次握手是由操作系统底层进行管理的，而在代码中我们只需关注服务器的监听和接受新连接的过程。
     */
    connect(m_s, &QTcpServer::newConnection, this, [=](){
        // 得到一个用于通信（读、写）的套接字实例对象
        // nextPendingConnection()：当客户端尝试连接服务器时，连接请求会被放置在一个队列中（SYN队列）。nextPendingConnection() 函数允许您获取该队列中的下一个连接。
        m_tcp = m_s->nextPendingConnection();
        if(m_tcp == nullptr){
            QMessageBox::warning(this, "警告", "无可用的通信套接字！");
        }
        connect(m_tcp, &QTcpSocket::readyRead, this, [=](){ // 此处只接收指令并返回相应的结果，并且指令都较短，远小于MTU，因此能够一次性传输完毕（当然也可以做一次性没传输完毕的处理）
            QByteArray instruction = m_tcp->readAll();
            QString INS = QString::fromUtf8(instruction);
            QString ins = INS.mid(0,12);
            qDebug() << "接收到的命令是" + ins;
            if(m_instructions.count(ins)){
                if(ins == "queryPkgNums"){
                    //m_tcp->write(QString::number(100).toUtf8()); // 返回查询结果
                    QString folderPath = ui->folderPath->text();
                    QDir dir(folderPath);
                    QStringList files = dir.entryList(QDir::Files); // 获取文件列表
                    qDebug() << "Number of files in the folder:" << files.size();
                    ins += QString::number(files.size());
                    for(int idxf = 0; idxf < files.size(); ++idxf){
                        QString filePath = folderPath + '/' + files[idxf];
                        QFileInfo fileInfo(filePath);
                        qint64 fileSize = fileInfo.size(); // 获取文件大小，返回的是字节数
                        QString fileSuffix = fileInfo.suffix(); // 获取文件后缀名
                        ins = ins + '-' + QString::number(fileSize) + ':' + fileSuffix;
                    }

                    qDebug() << ins;
                    m_tcp->write(ins.toUtf8()); // 返回查询结果
                }
                if(ins == "readReadyOKK"){
                    int idxp = 12;
                    for(; idxp < INS.size(); ++idxp){
                        if(INS[idxp] == '.'){
                            break;
                        }
                    }
                    QString startPkg = INS.mid(12, idxp - 12);
                    QString endPkg = INS.mid(idxp + 1, INS.size() - idxp - 1);
                    qDebug() << startPkg << " " << endPkg;


                    // 循环发送每个文件（实际上每次只发送一个）
                    int start = startPkg.toUInt(), end = endPkg.toInt();
                    QString folderPath = ui->folderPath->text();
                    QDir dir(folderPath);
                    QStringList files = dir.entryList(QDir::Files); // 获取文件列表

                    for(int i = start - 1; i < end; ++i){
                        QString filePath = folderPath + '/' + files[i];
                        QFile file(filePath);
                        if(file.open(QIODevice::ReadOnly)){
                            qDebug() << "开始发送：" + filePath;
                            while(!file.atEnd()){
                                QByteArray temp = file.read(4096); // 尽量一帧一帧读取数据，一帧长4096字节

                                // 添加包头
//                                int len = temp.size();
//                                QByteArray data((char*)&len, 4);
//                                data.append(temp);

                                // 发送数据
                                m_tcp->write(temp);
                                // 控制发送速度
                                QThread::msleep(20);
                            }
                            qDebug() <<  "该文件已发送完毕，等待发送下一个文件..." ;
                            break; // 每次只发送一个文件
                        }
                    }
                }
            }
        });

    });


}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::initWidgets()
{
    setWindowTitle("UUV1服务端");
    ui->IP->setReadOnly(true);
    ui->Port->setReadOnly(true);
    ui->startListen->setDisabled(true); // 选择文件夹后才能启动监听
}

void MainWindow::on_selectFolder_clicked()
{
    QString folderPath = QFileDialog::getExistingDirectory(nullptr, "Open Folder", "", QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (!folderPath.isEmpty()) {
        qDebug() << "Selected folder:" << folderPath;
    } else {
        qDebug() << "No folder selected.";
        QMessageBox::warning(this, "警告", "选择的文件夹路径不能为空！");
        return;
    }

    ui->folderPath->setText(folderPath);
    ui->startListen->setEnabled(true);
}



void MainWindow::on_startListen_clicked()
{
    unsigned short port = ui->Port->text().toUShort();
    /*
        address：该参数指定服务器应该监听传入连接的 IP 地址。默认情况下，它设置为 QHostAddress::Any，这意味着服务器将在所有可用的网络接口上监听传入连接。您还可以指定特定的 IP 地址，以便服务器仅在该特定接口上监听传入连接。
        port：该参数指定服务器应该监听传入连接的端口号。默认情况下，它设置为 0，这允许操作系统自动选择一个可用的端口。如果您希望将服务器绑定到特定的端口号，可以在这里提供所需的端口号。
     */
    // 开始监听指定的 IP 地址和端口上的传入连接
    m_s->listen(QHostAddress::Any, port);

    ui->startListen->setDisabled(true);
}




