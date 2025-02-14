#ifndef HEXCOMMANDER_H
#define HEXCOMMANDER_H

#include <QApplication>
#include <QButtonGroup>
#include <QByteArray>
#include <QCoreApplication>
#include <QDebug>
#include <QDialog>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QMessageBox>
#include <QPushButton>
#include <QSerialPort>
#include <QSpacerItem>
#include <QTextEdit>
#include <QTime>
#include <QTimer>
#include <QWidget>

#include <serial_port/serial_port_actions.h>
#include <ui_data_terminal.h>

QT_BEGIN_NAMESPACE
namespace Ui
{
class DataTerminal;
}
QT_END_NAMESPACE

class HexCommander : public QDialog
{
    Q_OBJECT

signals:
    void LOG_E(QString message, bool timestamp, bool linefeed);
    void LOG_W(QString message, bool timestamp, bool linefeed);
    void LOG_I(QString message, bool timestamp, bool linefeed);
    void LOG_D(QString message, bool timestamp, bool linefeed);

public:
    explicit HexCommander(SerialPortActions *serial, QWidget *parent = nullptr);
    ~HexCommander();


private:
    uint16_t receive_timeout = 500;
    uint16_t serial_read_extra_short_timeout = 50;
    uint16_t serial_read_short_timeout = 200;
    uint16_t serial_read_medium_timeout = 400;
    uint16_t serial_read_long_timeout = 800;
    uint16_t serial_read_extra_long_timeout = 3000;

    QVBoxLayout *vBoxLayout;

    uint8_t calculate_checksum(QByteArray output, bool dec_0x100);
    QByteArray add_ssm_header(QByteArray output, uint8_t tester_id, uint8_t target_id, bool dec_0x100);
    QString parse_message_to_hex(QByteArray received);
    void delay(int timeout);

    SerialPortActions *serial;
    Ui::DataTerminal *ui;

signals:

private slots:
    void protocolTypeChanged(int);
    void listenInterface();
    void sendToInterface();

};

#endif // HEXCOMMANDER_H
