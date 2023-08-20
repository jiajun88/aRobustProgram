#ifndef UUV1_H
#define UUV1_H

#include <QObject>
#include <QTcpSocket>
#include <QString>
#include <QFile>
#include <QByteArray>
#include <QVector>

class UUV1 : public QObject
{
    Q_OBJECT
public:
    explicit UUV1(QObject *parent = nullptr);

public:
    void connectionUUV1(unsigned short port, QString ip);
    void queryPkgNums();
    void readReadyOK(int startPkg, int endPkg);
    void dataReception();
    void dataReception2();
    void disConnectUUV1();


private:
    QTcpSocket* m_tcp;
    int reConnect = 0; // 表示重新连接的一个标志位
    int idxPkg = 0; // 表示第几个数据包
    int m_startPkg = 1;
    int m_endPkg = 1;
    qint64 totalSize = 0;
    qint64 downloadedBytes = 0;
    QFile* file = nullptr;

    QVector<qint64> pkgsSize; // 各数据包大小
    QVector<QString> pkgSuffixs; // 各文件包后缀名
signals:
    void connectUUV1_OK();
    void UUV1Disconnect();

    void uuv1PkgNums(QString );
    void startReceiveData();
    void dataReceptionCompleted();
    void errorTransmission();
    void receivedPkgs(int);
    void totalPkgSize(qint64);
    void downloaded(int);
};

#endif // UUV1_H
