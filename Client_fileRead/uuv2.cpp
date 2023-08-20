#include "uuv2.h"
#include <QHostAddress>
#include <QByteArray>
#include <QMetaObject>
#include <QDataStream>
#include <QDebug>
#include <QFile>
#include <QMessageBox>
#include <QVector>
#include <QtEndian>


UUV2::UUV2(QObject *parent)
    : QObject{parent}
{

}

// 负责和服务端连接/断开连接的【任务函数（一般是作为槽函数）】
void UUV2::connectionUUV2(unsigned short port, QString ip)
{
    m_tcp = new QTcpSocket; // 每点击一次申请连接按钮都会新建一个TCP连接
    m_tcp->connectToHost(QHostAddress(ip), port);

    reConnect = 1;

    // 发送一个信号告知主线程：客户端已经成功连接到了UUV2服务端（信号也可以作为槽函数!!!）
    connect(m_tcp, &QTcpSocket::connected, this, &UUV2::connectUUV2_OK);

    // 发送一个信号告知主线程：已经断开了TCP连接
    connect(m_tcp, &QTcpSocket::disconnected, [=](){
        m_tcp->close(); // 断开连接后，该TCP连接就应该马上释放资源
        m_tcp->deleteLater();

        emit UUV2Disconnect();
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
                emit uuv2PkgNums(pkgNums);

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

// 负责向UUV2服务端发送相关指令的任务函数
void UUV2::queryPkgNums()
{
    QString query = "queryPkgNums";
    m_tcp->write(query.toUtf8()); // 向客户端发送一条查询指令
}

void UUV2::readReadyOK(int startPkg, int endPkg)
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


void UUV2::dataReception2()
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

void UUV2::disConnectUUV2()
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



