#ifndef J2534_LINUX_H
#define J2534_LINUX_H

#include <QByteArray>
#include <QComboBox>
#include <QCoreApplication>
#include <QDebug>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QTime>

#include <stdint.h>

#include "J2534_tactrix_win.h"

class J2534 : public QWidget
{
    Q_OBJECT

public:
    explicit J2534();
    ~J2534();

    const char *DLL_VERSION = "3.0.0";
    const char *API_VERSION = "04.04";

    bool serial_port_protocol_iso14230 = false;
    bool J2534_init_ok = false;

    bool is_serial_port_open();

    bool init() { return true; };
    void setDllName(const char* name) {}; // For Win/Linux compatibility only
    void getDllName(char* name) {}; // For Win/Linux compatibility only
    void debug(bool enable) { debugMode = enable; };

    long PassThruOpen(const void *pName, unsigned long *pDeviceID);
    long PassThruClose(unsigned long DeviceID);
    long PassThruConnect(unsigned long DeviceID, unsigned long ProtocolID, unsigned long Flags, unsigned long Baudrate, unsigned long *pChannelID);
    long PassThruDisconnect(unsigned long ChannelID);
    long PassThruReadMsgs(unsigned long ChannelID, PASSTHRU_MSG *pMsg, unsigned long *pNumMsgs, unsigned long Timeout);
    long PassThruWriteMsgs(unsigned long ChannelID, const PASSTHRU_MSG *pMsg, unsigned long *pNumMsgs, unsigned long Timeout);
    long PassThruStartPeriodicMsg(unsigned long ChannelID, const PASSTHRU_MSG *pMsg, unsigned long *pMsgID, unsigned long TimeInterval);
    long PassThruStopPeriodicMsg(unsigned long ChannelID, unsigned long MsgID);
    long PassThruStartMsgFilter(unsigned long ChannelID, unsigned long FilterType, const PASSTHRU_MSG *pMaskMsg, const PASSTHRU_MSG *pPatternMsg, const PASSTHRU_MSG *pFlowControlMsg, unsigned long *pMsgID);
    long PassThruStopMsgFilter(unsigned long ChannelID, unsigned long MsgID);
    long PassThruSetProgrammingVoltage(unsigned long DeviceID, unsigned long Pin, unsigned long Voltage);
    long PassThruReadVersion(char *pApiVersion, char *pDllVersion, char *pFirmwareVersion, unsigned long DeviceID);
    long PassThruGetLastError(char *pErrorDescription);
    long PassThruIoctl(unsigned long ChannelID, unsigned long IoctlID, const void *pInput, void *pOutput);

    QString open_serial_port(QString serial_port);
    void close_serial_port();
    QByteArray read_serial_data(uint32_t datalen, uint16_t timeout);
    int write_serial_data(QByteArray output);
    QByteArray write_serial_iso14230_data(QByteArray output);
    QString parseMessageToHex(QByteArray received);
    uint32_t parse_ts(const char *data);

private:
    bool debugMode;

    QString opened_serial_port;
    QString serial_port_baudrate = "4800";

    QSerialPort *serial = new QSerialPort();
    unsigned long periodic_msg_id;

    enum rx_msg_type {
        NORM_MSG,
        TX_DONE_MSG = 0x10,
        TX_LB_MSG = 0x20,
        RX_MSG_END_IND = 0x40,
        EXT_ADDR_MSG_END_IND = 0x44,
        LB_MSG_END_IND = 0x60,
        NORM_MSG_START_IND = 0x80,
        TX_LB_START_IND = 0xA0,
    };

    int is_valid_sconfig_param(SCONFIG s);
    void dump_sbyte_array(const SBYTE_ARRAY* s);
    void dump_sconfig_param(SCONFIG s);

    void delay(int n);

private slots:
    void handle_error(QSerialPort::SerialPortError error);

};

#endif // J2534_LINUX_H
