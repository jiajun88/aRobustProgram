#include "uuv1.h"
#include <QHostAddress>
#include <QByteArray>
#include <QMetaObject>
#include <QDataStream>
#include <QDebug>
#include <QFile>
#include <QMessageBox>
#include <QVector>
#include <QtEndian>


UUV1::UUV1(QObject *parent)
    : QObject{parent}
{

}

// 负责和服务端连接/断开连接的【任务函数（一般是作为槽函数）】
void UUV1::connectionUUV1(unsigned short port, QString ip)
{
    m_tcp = new QTcpSocket; // 每点击一次申请连接按钮都会新建一个TCP连接
    m_tcp->connectToHost(QHostAddress(ip), port);

    reConnect = 1;

    // 发送一个信号告知主线程：客户端已经成功连接到了UUV1服务端（信号也可以作为槽函数!!!）
    connect(m_tcp, &QTcpSocket::connected, this, &UUV1::connectUUV1_OK);

    // 发送一个信号告知主线程：已经断开了TCP连接
    connect(m_tcp, &QTcpSocket::disconnected, [=](){
        m_tcp->close(); // 断开连接后，该TCP连接就应该马上释放资源
        m_tcp->deleteLater();

        emit UUV1Disconnect();
    });

    // 当监测到有数据可读时的相应处理
    connect(m_tcp, &QTcpSocket::readyRead, this, [=](){
        static int i = 1;
        if(reConnect == 1){ // 表示这是一个新连接
            reConnect = 0;
            i = 1;
        }
        if(i <= 1){
            QByteArray data = m_tcp->readAll();
            QString dataType = QString::fromUtf8(data).mid(0,12);
            qDebug() << dataType;
            if(dataType == "queryPkgNums"){
                QString queryRes = QString::fromUtf8(data);
                QVector<int> idxs_;
                QVector<int> suffixIdxs;
                for(int idx_ = 12; idx_ < queryRes.size(); ++idx_){
                    if(queryRes[idx_] == '-'){
                        idxs_.push_back(idx_);
                    }
                    if(queryRes[idx_] == ':'){
                        suffixIdxs.push_back(idx_);
                    }
                }
                QString pkgNums = queryRes.mid(12,idxs_[0] - 12);
                emit uuv1PkgNums(pkgNums);

                int pkgnums = pkgNums.toInt();
                for(int ii = 0; ii < pkgnums; ++ii){
                    QString size = queryRes.mid(idxs_[ii] + 1, suffixIdxs[ii] - idxs_[ii] - 1);
                    qint64 pkgSize = size.toLongLong();
                    qDebug() << "第" << ii + 1 << "个文件的大小为" << pkgSize;
                    pkgsSize.push_back(pkgSize);

                    if(ii < pkgnums - 1){
                        QString suffix = queryRes.mid(suffixIdxs[ii] + 1, idxs_[ii + 1] - suffixIdxs[ii] - 1);
                        pkgSuffixs.push_back(suffix);
                    }
                }
                QString suffix = queryRes.mid(suffixIdxs[pkgnums - 1] + 1, queryRes.size() - suffixIdxs[pkgnums - 1] - 1);
                pkgSuffixs.push_back(suffix);
            }
            i++;
        }else{
            // 解析对端发来的数据包
            dataReception2();

            if(idxPkg == m_endPkg){ // 接收完所有的数据包了
                emit dataReceptionCompleted();
                m_tcp->close();
                m_tcp->deleteLater();
            }
        }
    });
}

// 负责向UUV1服务端发送相关指令的任务函数
void UUV1::queryPkgNums()
{
    QString query = "queryPkgNums";
    m_tcp->write(query.toUtf8()); // 向客户端发送一条查询指令
}

void UUV1::readReadyOK(int startPkg, int endPkg)
{
    m_startPkg = startPkg;
    m_endPkg = endPkg;
    idxPkg = startPkg - 1;
    for(int i = startPkg - 1; i <= endPkg - 1; ++i){
        totalSize += pkgsSize[i];
    }

    // 打开第一个文件
    QString filePath = QString::number(startPkg) + '.' + pkgSuffixs[startPkg - 1];
    file = new QFile(filePath);
    file->open(QIODevice::WriteOnly | QIODevice::Truncate);
    qDebug() << "已打开文件：" << filePath;

    QString query = "readReadyOKK" + QString::number(startPkg) + '.' + QString::number(endPkg);
    m_tcp->write(query.toUtf8()); // 向对端发送一条准备下载数据包的指令
}


void UUV1::dataReception2()
{
    static qint64 receivedBytes = 0;
    QByteArray data = m_tcp->readAll();


    qint64 bytesWritten = file->write(data);
    qDebug() << "已成功写入" << bytesWritten << "字节";

    receivedBytes += bytesWritten;
    qDebug() << "总共写入了" << receivedBytes << "字节，传输文件的大小为" << pkgsSize[idxPkg] << "字节";

    downloadedBytes += bytesWritten;

    emit downloaded(downloadedBytes * 100 / totalSize);
    qDebug() << "totalSize = " << totalSize;
    qDebug() << "downloadedBytes = " << downloadedBytes;
    qDebug() << "downloadedBytes * 100 / totalSize = " << downloadedBytes * 100/ totalSize;

    if(receivedBytes == pkgsSize[idxPkg]){
        receivedBytes = 0;
        // 接收完一个了，向对端下达发下一个文件的指令
        QString instruction = "readReadyOKK" + QString::number(idxPkg + 2) + '.' + QString::number(m_endPkg);
        m_tcp->write(instruction.toUtf8());
        file->flush();
        file->close();

        if(idxPkg + 1 < m_endPkg){
            QString filePath = QString::number(idxPkg + 2) + '.' + pkgSuffixs[idxPkg + 1];
            file = new QFile(filePath); // 复用file指针
            file->open(QIODevice::WriteOnly | QIODevice::Truncate);
            qDebug() << "已打开文件：" << filePath;
        }

        if(idxPkg + 1 == m_endPkg){
            downloadedBytes = 0;
            totalSize = 0;
        }

        idxPkg++;
        emit receivedPkgs(idxPkg);
    }

//    if(idxPkg == m_endPkg){
//        idxPkg = m_startPkg - 1;
//    }

}

void UUV1::disConnectUUV1()
{
    // 此时就是清空缓冲区(直接关闭也行，反正都决定强行停止了，剩下的这些缓冲区几K的数据也肯定也不重要)，下载好的文件已经保存，只要保存正在下载的文件即可
    downloadedBytes = 0;
    totalSize = 0;
    m_tcp->close();
    m_tcp->deleteLater();
    if(file != nullptr && file->isOpen()){
        file->flush();
        file->close();
    }
}

//----------------------------------------------------------------------------------
//----------------------------------------------------------------------------------
//----------------------------------------------------------------------------------
//----------------------------------------------------------------------------------
void UUV1::dataReception()
{
    unsigned int totalBytes = 0; // 对端发送过来的一个数据包长度
    unsigned int recvBytes = 0;
    QByteArray block;
    static qint64 recivedBytes = 0;

    qDebug() << "数据缓冲区有" << m_tcp->bytesAvailable() << "字节的数据";

    // 判断有无数据
    if(m_tcp->bytesAvailable() == 0){
        return;
    }

    // 读TCP包的包头
    if(m_tcp->bytesAvailable() >= sizeof(int)){
        QByteArray head = m_tcp->read(sizeof(int));
        totalBytes = *(int*)(head.data());
        qDebug() << "此TCP包的大小为：" << totalBytes;
    }else{
        // TCP包竟然不足一个包头的长度，那肯定就是哪里出了问题
        emit errorTransmission();
        m_tcp->close();
        m_tcp->deleteLater();
        return;
    }

    // 读实际数据块的内容
    while(totalBytes - recvBytes > 0 && m_tcp->bytesAvailable()){
        block.append(m_tcp->read(totalBytes - recvBytes)); // 每次尽量一次性读取完一个TCP包，但一次读取不能超过一个TCP包
        recvBytes = block.size(); // 已经收到的数据大小
    }

    if(recvBytes == totalBytes){
        recivedBytes += totalBytes;
    }
    qDebug() << "recivedBytes = " << recivedBytes << "pkgsSize[idxPkg] = " << pkgsSize[idxPkg];

    if(recivedBytes == pkgsSize[idxPkg]){
        recivedBytes = 0;
        idxPkg++;
        emit receivedPkgs(idxPkg);
    }
    if(idxPkg == m_endPkg){
        idxPkg = m_startPkg - 1;
    }

    // 如果还有数据包，就继续读取下一个数据包
    qDebug() << "接收缓冲区还有多少数据：" << m_tcp->bytesAvailable();
        if(m_tcp->bytesAvailable() > 0){
        qDebug() << "开始读取第" + QString::number(idxPkg + 1) + "个文件的下一个TCP包...";
        dataReception();
        qDebug() << "退出递归了...";
    }
}



