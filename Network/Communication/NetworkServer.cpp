#include <QDebug>
#include <QHostAddress>
#include <QList>
#include <QNetworkInterface>
#include <QString>
#include <QTcpServer>

#include "NetworkServer.hpp"

namespace Network
{
NetworkServer::NetworkServer(int port, QObject* parent):
    QObject{parent}
{
    m_tcpServer = new QTcpServer(this);


    while(!m_tcpServer->listen(QHostAddress::Any, port))
    {
        port++;
    }

    QList<QHostAddress> ipAddressesList = QNetworkInterface::allAddresses();

    // use the first non-localhost IPv4 address
    for(int i = 0; i < ipAddressesList.size(); ++i)
    {
        if(ipAddressesList.at(i) != QHostAddress::LocalHost &&
                ipAddressesList.at(i).toIPv4Address())
        {
            m_localAddress = ipAddressesList.at(i).toString();
            break;
        }
    }

    // if we did not find one, use IPv4 localhost
    if(m_localAddress.isEmpty())
    {
        m_localAddress = QHostAddress(QHostAddress::LocalHost).toString();
    }

    m_localPort = m_tcpServer->serverPort();
    qDebug() << "Server: " << m_localAddress << ":" << m_localPort;

    connect(m_tcpServer, &QTcpServer::newConnection,
            this, [=] ()
    {
        emit newSocket(m_tcpServer->nextPendingConnection());
    });
}

int NetworkServer::port() const
{
    return m_tcpServer->serverPort();
}
}
