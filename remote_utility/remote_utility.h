#ifndef REMOTE_UTILITY_H
#define REMOTE_UTILITY_H

#include <QObject>
#include "rep_remote_utility_replica.h"
#include "websocketiodevice.h"
#include "qtrohelper.hpp"

class RemoteUtility : public QObject
{
    Q_OBJECT
public:
    explicit RemoteUtility(QString peerAddress,
                           QWebSocket *web_socket,
                           QObject *parent = nullptr);
    ~RemoteUtility();

    QRemoteObjectReplica::State state(void) const;
    bool isValid(void);

public slots:
    bool send_log_window_message(QString message);
    bool set_progressbar_value(int value);
    void ping(QString message);
    void websocket_connected(void);
    void waitForSource(void);

signals:

private:
    QString peerAddress;
    RemoteUtilityReplica *remote_utility;
    const QString remoteObjectNameUtility = "FastECU_Utility";
    const int heartbeatInterval = 5000;
    QWebSocket *webSocket;
    WebSocketIoDevice *socket;
    QRemoteObjectNode node;
    int keepalive_interval = 10000;
    QTimer *keepalive_timer;
    void start_keepalive(void);
    void stop_keepalive(void);
    void startRemote(void);
    void startOverNetwok(void);
    void startLocal(void);

private slots:
    void utilityRemoteStateChanged(QRemoteObjectReplica::State state, QRemoteObjectReplica::State oldState);

};

#endif // REMOTE_UTILITY_H
