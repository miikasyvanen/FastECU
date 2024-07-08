#ifndef ECUOPERATIONSISO14230_H
#define ECUOPERATIONSISO14230_H

#include <QMessageBox>
#include <QProgressBar>
#include <QTextEdit>
#include <QVBoxLayout>

#include <file_actions.h>
#include <serial_port/serial_port_actions.h>

class EcuOperationsIso14230 : public QWidget
{
    Q_OBJECT

public:
    explicit EcuOperationsIso14230(QWidget *ui, SerialPortActions *serial);
    ~EcuOperationsIso14230();

    #define STATUS_SUCCESS							0x00
    #define STATUS_ERROR							0x01

    bool kill_process = false;

    int fast_init(QByteArray output);

    QByteArray request_timings();
    QByteArray request_service1_pids();
    QByteArray request_service9_pids();
    QString request_vin();
    QStringList request_cal_id();
    QString request_cvn();
    QString request_ecu_name();
    QByteArray read_data(QByteArray header, QByteArray payload);

private:
    SerialPortActions *serial;
    FileActions *fileActions;
    QWidget *flash_window;

    QString parse_message_to_hex(QByteArray received);
    void set_progressbar_value(int value);
    void delay(int timeout);

};

#endif // ECUOPERATIONSISO14230_H
