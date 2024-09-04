#include "remote_utility.h"

RemoteUtility::RemoteUtility(QString peerAddress,
                            QWebSocket *web_socket,
                            QObject *parent)
    : QObject{parent}
    , peerAddress(peerAddress)
    , webSocket(web_socket)
    , socket(new WebSocketIoDevice(webSocket, webSocket))
{
    if (peerAddress.startsWith("local:"))
    {
        startLocal();
    }
    else
    {
        startOverNetwok();
    }
    QObject::connect(remote_utility, &RemoteUtilityReplica::stateChanged,
                        this, &RemoteUtility::utilityRemoteStateChanged);
}

RemoteUtility::~RemoteUtility()
{}

void RemoteUtility::startLocal(void)
{
    QString p = peerAddress+remoteObjectNameUtility;
    node.connectToNode(QUrl(p));
    remote_utility = node.acquire<RemoteUtilityReplica>(remoteObjectNameUtility);
}

void RemoteUtility::startOverNetwok()
{
    node.setHeartbeatInterval(heartbeatInterval);
    remote_utility = node.acquire<RemoteUtilityReplica>(remoteObjectNameUtility);
    //Start node when Web Socket will be up
    //If it is already connected, connection could not be established
    QObject::connect(webSocket, &QWebSocket::connected, this, &RemoteUtility::websocket_connected);
}

void RemoteUtility::websocket_connected(void)
{
    node.addClientSideConnection(socket);
}

void RemoteUtility::waitForSource(void)
{
    //Don't wait for replication too long
    //This class is not very important
    remote_utility->waitForSource(10000);
}

bool RemoteUtility::send_log_window_message(QString message)
{
    return qtrohelper::slot_sync(remote_utility->send_log_window_message(message));
}

bool RemoteUtility::set_progressbar_value(int value)
{
    return qtrohelper::slot_sync(remote_utility->set_progressbar_value(value));
}

QRemoteObjectReplica::State RemoteUtility::state(void) const
{
    return remote_utility->state();
}

bool RemoteUtility::isValid(void)
{
    return remote_utility->state() == QRemoteObjectReplica::Valid;
}

void RemoteUtility::utilityRemoteStateChanged(QRemoteObjectReplica::State state, QRemoteObjectReplica::State oldState)
{
    if (state == QRemoteObjectReplica::Valid)
        qDebug() << "RemoteUtility remote connection established";
    else if (oldState == QRemoteObjectReplica::Valid)
        qDebug() << "RemoteUtility remote connection lost";
}
