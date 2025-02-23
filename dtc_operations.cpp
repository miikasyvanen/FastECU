#include "dtc_operations.h"

DtcOperations::DtcOperations(SerialPortActions *serial, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::DtcOperationsWindow)
{
    ui->setupUi(this);

    this->serial = serial;

    run();
    init_obd();
}

DtcOperations::~DtcOperations()
{
    delete ui;
}

void DtcOperations::closeEvent(QCloseEvent *event)
{
    kill_process = true;
}

void DtcOperations::run()
{
    int result = STATUS_ERROR;

    // Set serial port
    serial->set_is_iso14230_connection(true);
    serial->set_add_iso14230_header(true);
    serial->set_is_can_connection(false);
    serial->set_is_iso15765_connection(false);
    serial->set_is_29_bit_id(false);
    serial->set_can_speed("500000");
    serial->set_iso14230_startbyte(0xc0);
    serial->set_iso14230_tester_id(0xf1);
    serial->set_iso14230_target_id(0x33);
    serial->set_can_source_address(0x7e0);
    serial->set_can_destination_address(0x7e8);
    serial->set_iso15765_source_address(0x7e0);
    serial->set_iso15765_destination_address(0x7e8);
    // Open serial port
    serial->open_serial_port();
}

int DtcOperations::init_obd()
{
    QByteArray received;
    int result = STATUS_SUCCESS;

    // FIVE_BAUD_INIT
    emit LOG_D("Request SLOW INIT", true, true);
    result = serial->slow_init();
    if (result)
    {
        emit LOG_D("Request response", true, true);
        received = serial->read_serial_data(serial_read_timeout);

    }

    return STATUS_SUCCESS;
}
