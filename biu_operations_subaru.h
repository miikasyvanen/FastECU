#ifndef BIUOPERATIONSSUBARU_H
#define BIUOPERATIONSSUBARU_H

#include <QApplication>
#include <QByteArray>
#include <QCoreApplication>
#include <QDebug>
#include <QMainWindow>
#include <QSerialPort>
#include <QTime>
#include <QTimer>
#include <QWidget>
#include <QDialog>

#include <serial_port_actions.h>

QT_BEGIN_NAMESPACE
namespace Ui
{
    class BiuOperationsSubaruWindow;
}
QT_END_NAMESPACE

class BiuOperationsSubaru : public QDialog
{
    Q_OBJECT

public:
    explicit BiuOperationsSubaru(SerialPortActions *serial, QWidget *parent = nullptr);
    ~BiuOperationsSubaru();

private:

    enum BiuCommands {
        CONNECT = 0x81,
        DISCONNECT = 0x82,
        DTC_READ = 0x18,
        DTC_CLEAR = 0x14,
        IN_OUT_SWITCHES = 0x50,
        LIGHTING_SWITCHES = 0x51,
        BIU_DATA = 0x40,
        CAN_DATA = 0x41,
        BIU_CUSTOM_TIME_TEMP = 0x52,
        DEALER_COUNTRY_OPTIONS = 0x53,
    };

    QStringList biu_messages = {    "Connect", "81",
                                    "Disconnect", "82",
                                    "DTC read", "18",
                                    "DTC clear", "14,40,00",
                                    "Input/output switches", "21,50",
                                    "Lighting related switches", "21,51",
                                    "BIU data", "21,40",
                                    "CAN data", "21,41",
                                    "BIU Customisable Times & Temps", "21,52",
                                    "Dealer / country options", "21,53",
                                    "Custom", "",
                               };

    QStringList biu_dtc_list = {    "B0100", "BIU internal error",
                                    "B0101", "Battery Power supply error",
                                    "B0102", "Battery Power backup error",
                                    "B0103", "IGN power error",
                                    "B0104", "ACC power error",
                                    "B0105", "Key interlock circuit short",
                                    "B0106", "Shift Lock circuit short",
                                    "B0201", "HS CAN off",
                                    "B0202", "HS CAN off",
                                    "B0211", "HS CAN EGI error",
                                    "B0212", "HS CAN TCM error",
                                    "B0213", "HS CAN VDC/ABS error",
                                    "B0214", "",
                                    "B0216", "",
                                    "B0217", "HS CAN EPS data error",
                                    "B0218", "",
                                    "B0221", "HS CAN ECM no data",
                                    "B0222", "HS CAN TCM no data",
                                    "B0223", "HS CAN VDC/ABS no data",
                                    "B0224", "",
                                    "B0226", "",
                                    "B0227", "HS CAN EPS no data",
                                    "B0228", "",
                                    "B0300", "LS CAN error",
                                    "B0301", "LS CAN malfunction",
                                    "B0302", "LS CAN off",
                                    "B0303", "Wake up line abnormal",
                                    "B0311", "LS CAN meter error",
                                    "B0313", "",
                                    "B0314", "",
                                    "B0321", "LS CAN meter no data",
                                    "B0323", "",
                                    "B0327", "LS CAN gateway no data",
                                    "B0401", "M collation bad",
                                    "B0402", "Immobilizer Key Collation bad",
                                    "B0403", "E/G request bad",
                                    "B0404", "Smart registration bad",
                                    "B0500", "Keyless UART comm error",
                               };


    void parse_biu_message(QByteArray message);
    uint8_t calculate_checksum(QByteArray output, bool dec_0x100);
    QString parse_message_to_hex(QByteArray received);
    int send_log_window_message(QString message, bool timestamp, bool linefeed);
    void delay(int timeout);

    SerialPortActions *serial;
    Ui::BiuOperationsSubaruWindow *ui;

private slots:
    void send_biu_msg();

signals:

};

#endif // BIUOPERATIONSSUBARU_H
