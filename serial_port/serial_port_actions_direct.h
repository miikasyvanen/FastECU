#ifndef SERIAL_PORT_ACTIONS_DIRECT_H
#define SERIAL_PORT_ACTIONS_DIRECT_H

#include <QCoreApplication>
#include <QByteArray>
#include <QComboBox>
#include <QDebug>
#include <QElapsedTimer>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QDateTime>
#include <QTime>
#include <QTimer>
#include <QWidget>
#include <QSettings>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <thread>
#include <chrono>

#if defined Q_OS_UNIX
#include "J2534_linux.h"
#elif defined Q_OS_WIN32
#include "J2534_win.h"
#endif

class SerialPortActionsDirect : public QObject
{
    Q_OBJECT

public:
    explicit SerialPortActionsDirect(QObject *parent=nullptr);
    ~SerialPortActionsDirect();

    bool serialPortAvailable = false;
    bool setRequestToSend = true;
    bool setDataTerminalReady = true;

    bool add_iso14230_header = false;
    bool is_iso14230_connection = false;
    bool is_can_connection = false;
    bool is_iso15765_connection = false;
    bool is_29_bit_id = false;

    bool use_openport2_adapter = false;

    int requestToSendEnabled = 0;
    int requestToSendDisabled = 1;
    int dataTerminalEnabled = 0;
    int dataTerminalDisabled = 1;

    int echo_check_timout = 5000;

    uint8_t iso14230_startbyte = 0;
    uint8_t iso14230_tester_id = 0;
    uint8_t iso14230_target_id = 0;

    QByteArray ssm_receive_header_start = { "\x80\xf0\x10" };

    QStringList serial_port_list;
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

    QString can_speed = "500000";

    uint8_t serial_port_parity = (uint8_t)QSerialPort::NoParity;
    QString serial_port_baudrate = "4800";
    QString serial_port_linux = "/dev/ttyUSB0";
    QString serial_port_windows = "COM67";
    QString serial_port;
    QString serial_port_prefix;
    QString serial_port_prefix_linux = "/dev/";
    QString serial_port_prefix_win;

    uint32_t can_source_address = 0;
    uint32_t can_destination_address = 0;
    uint32_t iso15765_source_address = 0;
    uint32_t iso15765_destination_address = 0;

    bool is_serial_port_open();
    int change_port_speed(QString portSpeed);
    int fast_init(QByteArray output);
    int set_lec_lines(int lec1, int lec2);
    int pulse_lec_1_line(int timeout);
    int pulse_lec_2_line(int timeout);

    void reset_connection();

    QByteArray read_serial_data(uint32_t datalen, uint16_t timeout);
    QByteArray write_serial_data(QByteArray output);
    QByteArray write_serial_data_echo_check(QByteArray output);

    int clear_rx_buffer();
    int clear_tx_buffer();

    int send_periodic_j2534_data(QByteArray output, int timeout);
    int stop_periodic_j2534_data();

    QStringList check_serial_ports();
    QString open_serial_port();

private:
#ifndef ARRAYSIZE
#define ARRAYSIZE(A) (sizeof(A)/sizeof((A)[0]))
#endif

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

    int set_j2534_ioctl(unsigned long parameter, int value);
    int init_j2534_connection();
    int set_j2534_can();
    int unset_j2534_can();
    int set_j2534_can_filters();
    //    int set_j2534_stmin_tx();
    int set_j2534_can_timings();
    int set_j2534_iso9141();
    int set_j2534_iso9141_filters();
    int set_j2534_iso9141_timings();
    unsigned long msgID = 0;

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

#define STATUS_SUCCESS							0x00
#define STATUS_ERROR							0x01

    J2534 *j2534;
    QSerialPort *serial;

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

#if defined Q_OS_UNIX
    unsigned int protocol = ISO9141;
#elif defined Q_OS_WIN32
    unsigned int protocol = ISO9141;
#endif

    bool J2534_init_ok = false;
    bool J2534_open_ok = false;
    bool J2534_connect_ok = false;
    bool J2534_get_version_ok = false;
    bool J2534_timing_ok = false;
    bool J2534_filters_ok = false;

    bool J2534_is_denso_dsti = false;

    int line_end_check_1_toggled(int state);
    int line_end_check_2_toggled(int state);
#ifdef Q_OS_WIN32
    QMap<QString, QString> installed_drivers;
#endif

    QByteArray add_packet_header(QByteArray output);
    int write_j2534_data(QByteArray output);
    QByteArray read_j2534_data(unsigned long timeout);
    QString parse_message_to_hex(QByteArray received);

    /*public slots:
    QStringList check_serial_ports();
    QString open_serial_port();*/

#ifdef WIN32
    QMap<QString, QString> getAllJ2534DriversNames();
#endif
    QStringList check_j2534_devices(QMap<QString, QString> installed_drivers);

private slots:

    void close_serial_port();
    void handle_error(QSerialPort::SerialPortError error);
    void accurate_delay(int timeout);
    void fast_delay(int timeout);
    void delay(int timeout);

};

#endif // SERIAL_PORT_ACTIONS_DIRECT_H
