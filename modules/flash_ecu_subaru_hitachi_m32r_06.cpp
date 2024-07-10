#include "flash_ecu_subaru_hitachi_m32r_06.h"

FlashEcuSubaruHitachiM32R_06::FlashEcuSubaruHitachiM32R_06(SerialPortActions *serial, FileActions::EcuCalDefStructure *ecuCalDef, QString cmd_type, QWidget *parent)
    : QDialog(parent),
      ui(new Ui::EcuOperationsWindow)
{
    ui->setupUi(this);

    if (cmd_type == "test_write")
        this->setWindowTitle("Test write ROM " + ecuCalDef->FileName + " to ECU");
    else if (cmd_type == "write")
        this->setWindowTitle("Write ROM " + ecuCalDef->FileName + " to ECU");
    else if (cmd_type == "read")
        this->setWindowTitle("Read ROM from ECU");

    this->serial = serial;
    this->show();

    int result = STATUS_ERROR;

    result = init_flash_subaru_hitachi(ecuCalDef, cmd_type);

    if (result == STATUS_SUCCESS)
    {
        QMessageBox::information(this, tr("ECU Operation"), "ECU operation was succesful, press OK to exit");
        this->close();
    }
    else
    {
        QMessageBox::warning(this, tr("ECU Operation"), "ECU operation failed, press OK to exit and try again");
    }
}

FlashEcuSubaruHitachiM32R_06::~FlashEcuSubaruHitachiM32R_06()
{

}

void FlashEcuSubaruHitachiM32R_06::closeEvent(QCloseEvent *event)
{
    kill_process = true;
}

int FlashEcuSubaruHitachiM32R_06::init_flash_subaru_hitachi(FileActions::EcuCalDefStructure *ecuCalDef, QString cmd_type)
{
    mcu_type_string = ecuCalDef->McuType;
    mcu_type_index = 0;

    while (flashdevices[mcu_type_index].name != 0)
    {
        if (flashdevices[mcu_type_index].name == mcu_type_string)
            break;
        mcu_type_index++;
    }
    QString mcu_name = flashdevices[mcu_type_index].name;
    //send_log_window_message("MCU type: " + mcu_name + " and index: " + mcu_type_index, true, true);
    qDebug() << "MCU type:" << mcu_name << mcu_type_string << "and index:" << mcu_type_index;

    int result = STATUS_ERROR;

    flash_method = ecuCalDef->FlashMethod;

    if (cmd_type == "read")
    {
        send_log_window_message("Read memory with flashmethod '" + flash_method + "' and kernel '" + ecuCalDef->Kernel + "'", true, true);
        //qDebug() << "Read memory with flashmethod" << flash_method << "and kernel" << ecuCalDef->Kernel;
    }
    else if (cmd_type == "write")
    {
        send_log_window_message("Write memory with flashmethod '" + flash_method + "' and kernel '" + ecuCalDef->Kernel + "'", true, true);
        //qDebug() << "Write memory with flashmethod" << flash_method << "and kernel" << ecuCalDef->Kernel;
    }

    // Set serial port
    serial->set_is_iso14230_connection(true);
    serial->set_is_can_connection(false);
    serial->set_is_iso15765_connection(false);
    serial->set_is_29_bit_id(false);
    tester_id = 0xF0;
    target_id = 0x10;
    // Open serial port
    serial->open_serial_port();

    QMessageBox::information(this, tr("Connecting to ECU"), "Turn ignition ON and press OK to start initializing connection");
    //QMessageBox::information(this, tr("Connecting to ECU"), "Press OK to start countdown!");

    if (cmd_type == "read")
    {
        send_log_window_message("Reading ROM from Subaru Hitachi WA12212970WWW using K-Line", true, true);
        result = read_mem_subaru_hitachi(ecuCalDef, flashdevices[mcu_type_index].fblocks[0].start, flashdevices[mcu_type_index].romsize);
    }
    else if (cmd_type == "test_write" || cmd_type == "write")
    {
        send_log_window_message("Writing ROM to Subaru Hitachi WA12212970WWW using K-Line", true, true);
        result = write_mem_subaru_hitachi(ecuCalDef, test_write);
    }
    return result;

}

/*
 * Read memory from Subaru Hitachi K-Line 32bit ECUs
 *
 * @return success
 */
int FlashEcuSubaruHitachiM32R_06::read_mem_subaru_hitachi(FileActions::EcuCalDefStructure *ecuCalDef, uint32_t start_addr, uint32_t length)
{

    return STATUS_ERROR;
}

/*
 * Write memory to Subaru Hitachi WA12212970WWW K-Line 32bit ECUs
 *
 * @return success
 */
int FlashEcuSubaruHitachiM32R_06::write_mem_subaru_hitachi(FileActions::EcuCalDefStructure *ecuCalDef, bool test_write)
{

    return STATUS_ERROR;
}


















/*
 * Add SSM header to message
 *
 * @return parsed message
 */
QByteArray FlashEcuSubaruHitachiM32R_06::add_ssm_header(QByteArray output, uint8_t tester_id, uint8_t target_id, bool dec_0x100)
{
    uint8_t length = output.length();

    output.insert(0, (uint8_t)0x80);
    output.insert(1, target_id & 0xFF);
    output.insert(2, tester_id & 0xFF);
    output.insert(3, length);

    output.append(calculate_checksum(output, dec_0x100));

    //send_log_window_message("Send: " + parse_message_to_hex(output), true, true);
    //qDebug () << "Send:" << parse_message_to_hex(output);
    return output;
}

/*
 * Calculate SSM checksum to message
 *
 * @return 8-bit checksum
 */
uint8_t FlashEcuSubaruHitachiM32R_06::calculate_checksum(QByteArray output, bool dec_0x100)
{
    uint8_t checksum = 0;

    for (uint16_t i = 0; i < output.length(); i++)
        checksum += (uint8_t)output.at(i);

    if (dec_0x100)
        checksum = (uint8_t) (0x100 - checksum);

    return checksum;
}

/*
 * Countdown prior power on
 *
 * @return
 */
int FlashEcuSubaruHitachiM32R_06::connect_bootloader_start_countdown(int timeout)
{
    for (int i = timeout; i > 0; i--)
    {
        if (kill_process)
            break;
        send_log_window_message("Starting in " + QString::number(i), true, true);
        //qDebug() << "Countdown:" << i;
        delay(1000);
    }
    if (!kill_process)
    {
        send_log_window_message("Initializing connection, please wait...", true, true);
        delay(1500);
        return STATUS_SUCCESS;
    }

    return STATUS_ERROR;
}

/*
 * Parse QByteArray to readable form
 *
 * @return parsed message
 */
QString FlashEcuSubaruHitachiM32R_06::parse_message_to_hex(QByteArray received)
{
    QString msg;

    for (int i = 0; i < received.length(); i++)
    {
        msg.append(QString("%1 ").arg((uint8_t)received.at(i),2,16,QLatin1Char('0')).toUtf8());
    }

    return msg;
}

/*
 * Output text to log window
 *
 * @return
 */
int FlashEcuSubaruHitachiM32R_06::send_log_window_message(QString message, bool timestamp, bool linefeed)
{
    QDateTime dateTime = dateTime.currentDateTime();
    QString dateTimeString = dateTime.toString("[yyyy-MM-dd hh':'mm':'ss'.'zzz']  ");

    if (timestamp)
        message = dateTimeString + message;
    if (linefeed)
        message = message + "\n";

    QTextEdit* textedit = this->findChild<QTextEdit*>("text_edit");
    if (textedit)
    {
        ui->text_edit->insertPlainText(message);
        ui->text_edit->ensureCursorVisible();

        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);

        return STATUS_SUCCESS;
    }

    return STATUS_ERROR;
}

void FlashEcuSubaruHitachiM32R_06::set_progressbar_value(int value)
{
    if (ui->progressbar)
        ui->progressbar->setValue(value);
    QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
}

void FlashEcuSubaruHitachiM32R_06::delay(int timeout)
{
    QTime dieTime = QTime::currentTime().addMSecs(timeout);
    while (QTime::currentTime() < dieTime)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 1);
}
