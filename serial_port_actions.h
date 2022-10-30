#ifndef SERIAL_PORT_ACTIONS_H
#define SERIAL_PORT_ACTIONS_H

#include <QCoreApplication>
#include <QByteArray>
#include <QComboBox>
#include <QDebug>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QTime>
#include <QTimer>
#include <QWidget>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//#include <functional>
//#include <iostream>
//#include <algorithm>
using namespace std;

#if defined(Q_OS_LINUX)
    #include "J2534_linux.h"
#elif defined(Q_OS_WIN32)
    #include "J2534_win.h"
#endif

class SerialPortActions : public QWidget
{
    Q_OBJECT

public:
    explicit SerialPortActions();
    ~SerialPortActions();

    bool serialPortAvailable = false;
    bool serialport_protocol_14230 = false;
    bool setRequestToSend = true;
    bool setDataTerminalReady = true;
    bool is_can_connection = false;
    bool is_iso15765_connection = false;

    bool use_openport2_adapter = false;

    int requestToSendEnabled = 0;
    int requestToSendDisabled = 1;
    int dataTerminalEnabled = 0;
    int dataTerminalDisabled = 1;

    QByteArray ssm_receive_header_start = { "\x80\xf0\x10" };

    QString openedSerialPort;
    QString subaru_02_16bit_bootloader_baudrate = "9600";
    QString subaru_04_16bit_bootloader_baudrate = "15625";
    QString subaru_02_32bit_bootloader_baudrate = "9600";
    QString subaru_04_32bit_bootloader_baudrate = "";
    QString subaru_05_32bit_bootloader_baudrate = "";

    QString subaru_02_16bit_kernel_baudrate = "39473";
    QString subaru_04_16bit_kernel_baudrate = "39473";
    QString subaru_02_32bit_kernel_baudrate = "62500";
    QString subaru_04_32bit_kernel_baudrate = "62500";
    QString subaru_05_32bit_kernel_baudrate = "62500";

    QString serial_port_baudrate = "4800";
    QString serial_port_linux = "/dev/ttyUSB0";
    QString serial_port_windows = "COM67";
    QString serial_port;
    QString serial_port_prefix;
    QString serial_port_prefix_linux = "/dev/";
    QString serial_port_prefix_win;
    QSerialPort *serial = new QSerialPort();

    bool is_serial_port_open();
    int change_port_speed(QString portSpeed);
    int set_lec_lines(int lec1, int lec2);
    int pulse_lec_1_line(int timeout);
    int pulse_lec_2_line(int timeout);

    void reset_connection();

    //QStringList check_win_serial_ports();
    QByteArray read_serial_data(uint32_t datalen, unsigned long timeout);
    QByteArray write_serial_data(QByteArray output);
    QByteArray write_serial_data_echo_check(QByteArray output);

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
    long PassThruReadVersion(char *pApiVersion,char *pDllVersion,char *pFirmwareVersion,unsigned long DeviceID);
    long PassThruGetLastError(char *pErrorDescription);
    long PassThruIoctl(unsigned long ChannelID, unsigned long IoctlID, const void *pInput, void *pOutput);

    int init_j2534_connection();
    int create_j2534_can_connection();
    int set_j2534_can_connection_filters();
    int set_j2534_can_timings();
    int create_j2534_iso9141_connection();
    int set_j2534_iso9141_connection_filters();
    int set_j2534_iso9141_timings();

private:
    #define STATUS_SUCCESS							0x00
    #define STATUS_ERROR							0x01

    J2534 *j2534;

    unsigned int baudrate = 4800;
    unsigned long devID;
    unsigned long chanID;
    unsigned long flags;
    unsigned int parity = NO_PARITY;
    unsigned int timeout = 20;

    bool ssm_init_ok = false;

    void close_j2534_serial_port();
    bool get_serial_num(char* serial);
    void dump_msg(PASSTHRU_MSG* msg);
    void reportJ2534Error();

#ifdef Q_OS_LINUX
    unsigned int protocol = ISO9141;

#elif defined(Q_OS_WIN32)
    unsigned int protocol = ISO9141;

#endif

    bool J2534_init_ok = false;
    bool J2534_open_ok = false;
    bool J2534_connect_ok = false;
    bool J2534_get_version_ok = false;
    bool J2534_timing_ok = false;
    bool J2534_filters_ok = false;

    int line_end_check_1_toggled(int state);
    int line_end_check_2_toggled(int state);

    QByteArray write_serial_iso14230_data(QByteArray output);
    int write_j2534_data(QByteArray output);
    QByteArray read_j2534_data(unsigned long timeout);

public slots:
    QStringList check_serial_ports();
    QString open_serial_port(QStringList serial_port);

private slots:

    void close_serial_port();
    void handle_error(QSerialPort::SerialPortError error);
    void delay(int n);

};

#endif // SERIAL_PORT_ACTIONS_H
