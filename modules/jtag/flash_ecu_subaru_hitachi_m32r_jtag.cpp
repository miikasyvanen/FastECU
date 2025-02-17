#include "flash_ecu_subaru_hitachi_m32r_jtag.h"

FlashEcuSubaruHitachiM32rJtag::FlashEcuSubaruHitachiM32rJtag(SerialPortActions *serial, FileActions::EcuCalDefStructure *ecuCalDef, QString cmd_type, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::EcuOperationsWindow)
    , ecuCalDef(ecuCalDef)
    , cmd_type(cmd_type)
{
    ui->setupUi(this);

    if (cmd_type == "test_write")
        this->setWindowTitle("Test write ROM " + ecuCalDef->FileName + " to ECU");
    else if (cmd_type == "write")
        this->setWindowTitle("Write ROM " + ecuCalDef->FileName + " to ECU");
    else if (cmd_type == "read")
        this->setWindowTitle("Read ROM from TCU");

    this->serial = serial;
}

void FlashEcuSubaruHitachiM32rJtag::run()
{
    this->show();

    int result = STATUS_ERROR;
    set_progressbar_value(0);

    mcu_type_string = ecuCalDef->McuType;
    mcu_type_index = 0;

    while (flashdevices[mcu_type_index].name != 0)
    {
        if (flashdevices[mcu_type_index].name == mcu_type_string)
            break;
        mcu_type_index++;
    }
    QString mcu_name = flashdevices[mcu_type_index].name;
    emit LOG_D("MCU type: " + mcu_name + " " + mcu_type_string + " and index: " + QString::number(mcu_type_index), true, true);

    kernel = ecuCalDef->Kernel;
    flash_method = ecuCalDef->FlashMethod;

    emit external_logger("Starting");

    if (cmd_type == "read")
    {
        emit LOG_I("Read memory with flashmethod '" + flash_method + "' and kernel '" + ecuCalDef->Kernel + "'", true, true);
    }
    else if (cmd_type == "test_write")
    {
        test_write = true;
        emit LOG_I("Test write memory with flashmethod '" + flash_method + "' and kernel '" + ecuCalDef->Kernel + "'", true, true);
    }
    else if (cmd_type == "write")
    {
        test_write = false;
        emit LOG_I("Write memory with flashmethod '" + flash_method + "' and kernel '" + ecuCalDef->Kernel + "'", true, true);
    }

    serial->set_add_iso14230_header(false);
    serial->set_is_iso14230_connection(false);
    serial->set_is_can_connection(false);
    serial->set_is_iso15765_connection(false);
    serial->set_is_29_bit_id(false);
    serial->set_serial_port_baudrate("4800");
    // Open serial port
    serial->open_serial_port();

    int ret = QMessageBox::warning(this, tr("Connecting to ECU"),
                                   tr("Turn ignition ON and press OK to start initializing connection to ECU"),
                                   QMessageBox::Ok | QMessageBox::Cancel,
                                   QMessageBox::Ok);

    switch (ret)
    {
        case QMessageBox::Ok:

            init_jtag();

            if (cmd_type == "read")
            {
                emit external_logger("Reading ROM, please wait...");
                emit LOG_I("Reading ROM from ECU using K-Line", true, true);
                result = read_mem(flashdevices[mcu_type_index].fblocks[0].start, flashdevices[mcu_type_index].romsize);
            }
            else
            {
                emit external_logger("Writing ROM, please wait...");
                emit LOG_I("Writing ROM to ECU Subaru Hitachi using K-Line", true, true);
                //result = write_mem_subaru_denso_can_02_32bit(test_write);
                result = write_mem(test_write);
            }
            emit external_logger("Finished");

            if (result == STATUS_SUCCESS)
            {
                QMessageBox::information(this, tr("ECU Operation"), "ECU operation was succesful, press OK to exit");
                this->close();
            }
            else
            {
                QMessageBox::warning(this, tr("ECU Operation"), "ECU operation failed, press OK to exit and try again");
            }
            break;
        case QMessageBox::Cancel:
            emit LOG_D("Operation canceled", true, true);
            this->close();
            break;
        default:
            QMessageBox::warning(this, tr("Connecting to ECU"), "Unknown operation selected!");
            emit LOG_D("Unknown operation selected!", true, true);
            this->close();
            break;
    }

}

FlashEcuSubaruHitachiM32rJtag::~FlashEcuSubaruHitachiM32rJtag()
{
    delete ui;
}

void FlashEcuSubaruHitachiM32rJtag::closeEvent(QCloseEvent *event)
{
    kill_process = true;
}

int FlashEcuSubaruHitachiM32rJtag::init_jtag()
{
    hard_reset_jtag();
    read_idcode();
    read_usercode();
    //set_rtdenb();

    read_tool_rom_code();

    return STATUS_SUCCESS;
}

int FlashEcuSubaruHitachiM32rJtag::read_mem(uint32_t start_addr, uint32_t length)
{


    return STATUS_SUCCESS;
}

int FlashEcuSubaruHitachiM32rJtag::write_mem(bool test_write)
{


    return STATUS_SUCCESS;
}

void FlashEcuSubaruHitachiM32rJtag::hard_reset_jtag()
{
    QByteArray output;
    QString msg;

    emit LOG_D("Hard reset JTAG", true, true);

    msg.clear();
    msg.append("80"); // Hard reset (TRST)
    msg.append("0D"); // Additional CR
    output = msg.toUtf8();
    serial->write_serial_data(output);
    emit LOG_D("Sent: " + parse_message_to_hex(output), true, true);
    delay(10);
}

void FlashEcuSubaruHitachiM32rJtag::read_idcode()
{
    emit LOG_I("IDCODE:", true, true);
    write_jtag_ir("IDCODE", IDCODE);
    read_jtag_dr("ID CODE");
}

void FlashEcuSubaruHitachiM32rJtag::read_usercode()
{
    emit LOG_I("USERCODE:", true, true);
    write_jtag_ir("USERCODE", USERCODE);
    read_jtag_dr("USERCODE");
}

void FlashEcuSubaruHitachiM32rJtag::set_rtdenb()
{
    emit LOG_I("RTDENB:", true, true);
    write_jtag_ir("RTDENB", RTDENB);
    write_jtag_dr("RTDENB", "00000001");
    read_jtag_dr("RTDENB");
}

/*************************************
 *
 *  Reading of Tool-ROM setting
 *  verification Code
 *
 * **********************************/
void FlashEcuSubaruHitachiM32rJtag::read_tool_rom_code()
{
    QByteArray response;

    emit LOG_I("Tool-ROM setting verification code:", true, true);
    for (int i = 0; i < 4; i++)
    {
        emit LOG_I("Set instruction: SETH R1, #0xFF00", true, true);
        write_jtag_ir("MON_CODE", MON_CODE);
        write_jtag_dr("MON_CODE", "d1c0ff00");
        //write_jtag_dr("MON_CODE", "d1c00000");
        emit LOG_I("Set instruction: LD R0, @(0x3FFC,R1=>DAT_FF003FFC)", true, true);
        write_jtag_ir("MON_CODE", MON_CODE);
        write_jtag_dr("MON_CODE", "a0c13ffc");
        //write_jtag_dr("MON_CODE", "a0c10000");
        emit LOG_I("Set instruction: ST R0, R4 | NOP", true, true);
        write_jtag_ir("MON_CODE", MON_CODE);
        write_jtag_dr("MON_CODE", "2044f000");
    }
    emit LOG_I("Set instruction: BRA DAT_FFFF9000", true, true);
    write_jtag_ir("MON_CODE", MON_CODE);
    write_jtag_dr("MON_CODE", "7ff4");
    emit LOG_I("Set MON_ACCESS -> MNSTART (bit0)", true, true);
    write_jtag_ir("MON_ACCESS", MON_ACCESS);
    write_jtag_dr("MON_ACCESS", "00000001");

    delay(1);
    emit LOG_I("Check MON_ACCESS -> MNCDBUSY (bit3)", true, true);
    write_jtag_ir("MON_ACCESS", MON_ACCESS);
    response = read_jtag_dr("MON_ACCESS"); // MNCDBUSY

    delay(1);
    emit LOG_I("Set MON_ACCESS -> MNSTART (bit0)", true, true);
    write_jtag_ir("MON_ACCESS", MON_ACCESS);
    write_jtag_dr("MON_ACCESS", "00000000");

    //emit LOG_I("Check MON_PARAM", true, true);
    //write_jtag_ir("MON_PARAM", MON_PARAM);
    //response = read_jtag_dr("MON_PARAM");
    emit LOG_I("Check MON_DATA", true, true);
    write_jtag_ir("MON_DATA", MON_DATA);
    response = read_jtag_dr("MON_DATA");
    emit LOG_I("Check MON_DATA", true, true);
    write_jtag_ir("MON_DATA", MON_DATA);
    response = read_jtag_dr("MON_DATA");
    emit LOG_I("Check MON_DATA", true, true);
    write_jtag_ir("MON_DATA", MON_DATA);
    response = read_jtag_dr("MON_DATA");
    emit LOG_I("Check MON_DATA", true, true);
    write_jtag_ir("MON_DATA", MON_DATA);
    response = read_jtag_dr("MON_DATA");
    emit LOG_I("Check MON_DATA", true, true);
    write_jtag_ir("MON_DATA", MON_DATA);
    response = read_jtag_dr("MON_DATA");

}






/*************************************
 *
 * JTAG functions to write and read
 * IR and DR registers
 *
 ************************************/
void FlashEcuSubaruHitachiM32rJtag::write_jtag_ir(QString desc, QString code)
{
    QByteArray output;
    QByteArray response;
    QString msg;
    QString endbit = code.at(0);
    bool ok = false;
    bool is_endbit = endbit.toInt(&ok, 16) & 0x2;

    emit LOG_D("Write instruction opcode " + desc + " to shift-ir", true, true);

    msg.clear();
    //msg.append("80"); // Hard reset (TRST)
    msg.append("4b051f"); // Reset TAP state machine
    msg.append("4b0303"); // Move to Shift-IR
    msg.append("3b04"); // Set instruction (first 5 bits)
    msg.append(code);
    if (is_endbit)
        msg.append("7b0001"); // If last bit == 1, set TMS and TDI bits
    else
        msg.append("6b0001"); // If last bit == 0, set only TMS bit
    if (is_endbit)
        emit LOG_I("IS ENDBIT", true, true);
    msg.append("4b0101"); // Move to Run/Idle
    //msg.append("4b0700"); // Idle 8 clockcycles
    msg.append("0D"); // Additional CR
    output = msg.toUtf8();
    serial->write_serial_data(output);
    emit LOG_D("Sent: " + parse_message_to_hex(output), true, true);

    response = read_response();
    emit LOG_I("Response: " + parse_message_to_hex(response), true, true);
}

QByteArray FlashEcuSubaruHitachiM32rJtag::read_jtag_dr(QString desc)
{
    QByteArray output;
    QByteArray response;
    QString msg;

    emit LOG_D("Read data from " + desc + " shift-dr", true, true);

    msg.append("4b0700"); // Idle 8 clockcycles
    msg.append("4b0201"); // Move to Shift-DR
    msg.append("6b1f80000000"); // Read 32 bits
    msg.append("4b0101"); // Move to Run/Idle
    msg.append("0D"); // Additional CR
    output = msg.toUtf8();
    serial->write_serial_data(output);
    emit LOG_D("Sent: " + parse_message_to_hex(output), true, true);

    response = read_response();
    if (response.at(0) == 0x7f)
        emit LOG_I("ERROR: " + parse_message_to_hex(response), true, true);
    else
        emit LOG_I("Response: " + parse_message_to_hex(response), true, true);

    return response;
}

QByteArray FlashEcuSubaruHitachiM32rJtag::write_jtag_dr(QString desc, QString data)
{
    QByteArray output;
    QByteArray response;
    QString msg;
    QString endbit = data.at(0);
    bool ok = false;
    bool is_endbit = endbit.toInt(&ok, 16) & 0x8;
    //QString endbit = data.at(data.length()-1);
    //data.remove(data.length()-1, 1);

    emit LOG_D("Write data '" + data + "' to " + desc + " shift-dr", true, true);

    msg.append("4b0700"); // Idle 8 clockcycles
    msg.append("4b0201"); // Move to Shift-DR
    msg.append("3b1e"); // Set 31 bits
    msg.append(data);   // Append data-1 bit
    if (is_endbit)
        msg.append("7b0001"); // If last bit == 1, set TMS and TDI bits
    else
        msg.append("6b0001"); // If last bit == 0, set only TMS bit
    if (is_endbit)
        emit LOG_I("IS ENDBIT", true, true);
    msg.append("4b0101"); // Move to Run/Idle
    msg.append("0D"); // Additional CR
    output = msg.toUtf8();
    serial->write_serial_data(output);
    emit LOG_D("Sent: " + parse_message_to_hex(output), true, true);

    response = read_response();
    emit LOG_I("Response: " + parse_message_to_hex(response), true, true);

    return response;
}

QByteArray FlashEcuSubaruHitachiM32rJtag::read_response()
{
    QByteArray received;
    QByteArray response;
    QByteArray response_inverted;

    received.clear();
    do
    {
        received = serial->read_serial_data(1, serial_read_short_timeout);
        if (!received.length())
            break;
        if (received.length())
            response = received.mid(4, (uint8_t)received.at(3));
        //emit LOG_D("Response: " + parse_message_to_hex(received), true, true);
    } while (received.length());

    for (int i = 0; i < response.length(); i++)
    {
        response_inverted.append(response.at(response.length() - 1 - i));
    }

    return response_inverted;
}











/*
 * Parse QByteArray to readable form
 *
 * @return parsed message
 */
QString FlashEcuSubaruHitachiM32rJtag::parse_message_to_hex(QByteArray received)
{
    QString msg;

    for (int i = 0; i < received.length(); i++)
    {
        msg.append(QString("%1 ").arg((uint8_t)received.at(i),2,16,QLatin1Char('0')).toUtf8());
    }

    return msg;
}

void FlashEcuSubaruHitachiM32rJtag::set_progressbar_value(int value)
{
    bool valueChanged = true;
    if (ui->progressbar)
    {
        valueChanged = ui->progressbar->value() != value;
        ui->progressbar->setValue(value);
    }
    if (valueChanged)
        emit external_logger(value);
    QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
}

void FlashEcuSubaruHitachiM32rJtag::delay(int timeout)
{
    QTime dieTime = QTime::currentTime().addMSecs(timeout);
    while (QTime::currentTime() < dieTime)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 1);
}
