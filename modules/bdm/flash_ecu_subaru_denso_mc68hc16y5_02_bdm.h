#ifndef FLASH_ECU_UNBRICK_SUBARU_DENSO_MC68HC16Y5_02_H
#define FLASH_ECU_UNBRICK_SUBARU_DENSO_MC68HC16Y5_02_H

#include <QApplication>
#include <QByteArray>
#include <QCoreApplication>
#include <QDebug>
#include <QMainWindow>
#include <QSerialPort>
#include <QTime>
#include <QTimer>
#include <QWidget>
#include <QtCharts/QChartView>
#include <QLineSeries>

#include <kernelmemorymodels.h>
#include <file_actions.h>
#include <serial_port/serial_port_actions.h>
#include <ui_ecu_operations.h>

QT_BEGIN_NAMESPACE
namespace Ui
{
class EcuOperationsWindow;
}
QT_END_NAMESPACE

class FlashEcuSubaruDensoMC68HC16Y5_02_BDM : public QDialog
{
    Q_OBJECT

public:
    explicit FlashEcuSubaruDensoMC68HC16Y5_02_BDM(SerialPortActions *serial, FileActions::EcuCalDefStructure *ecuCalDef, QString cmd_type, QWidget *parent = nullptr);
    ~FlashEcuSubaruDensoMC68HC16Y5_02_BDM();

    void run();

signals:
    void external_logger(QString message);
    void external_logger(int value);
    void LOG_E(QString message, bool timestamp, bool linefeed);
    void LOG_W(QString message, bool timestamp, bool linefeed);
    void LOG_I(QString message, bool timestamp, bool linefeed);
    void LOG_D(QString message, bool timestamp, bool linefeed);

private:
    FileActions::EcuCalDefStructure *ecuCalDef;
    QString cmd_type;

#define STATUS_SUCCESS	0x00
#define STATUS_ERROR	0x01

    bool kill_process = false;

    int result;
    int mcu_type_index;

    uint16_t receive_timeout = 500;
    uint16_t serial_read_extra_short_timeout = 50;
    uint16_t serial_read_short_timeout = 200;
    uint16_t serial_read_medium_timeout = 500;
    uint16_t serial_read_long_timeout = 800;
    uint16_t serial_read_extra_long_timeout = 3000;

    uint32_t flashbytescount = 0;
    uint32_t flashbytesindex = 0;

    QString mcu_type_string;
    QString flash_method;
    QString kernel;

    void closeEvent(QCloseEvent *bar);

    int read_mem(uint32_t start_addr, uint32_t length);
    int write_mem();
    int flash_block(const uint8_t *newdata, const struct flashdev_t *fdt, unsigned blockno);

    QString parse_message_to_hex(QByteArray received);
    void set_progressbar_value(int value);
    void delay(int timeout);

    SerialPortActions *serial;
    Ui::EcuOperationsWindow *ui;

};


#endif // FLASH_ECU_UNBRICK_SUBARU_DENSO_MC68HC16Y5_02_H
