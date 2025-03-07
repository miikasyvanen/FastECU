#include "flash_ecu_subaru_unisia_jecs.h"
#include "serial_port_actions.h"

FlashEcuSubaruUnisiaJecs::FlashEcuSubaruUnisiaJecs(SerialPortActions *serial, FileActions::EcuCalDefStructure *ecuCalDef, QString cmd_type, QWidget *parent)
    : QDialog(parent)
    , ecuCalDef(ecuCalDef)
    , cmd_type(cmd_type)
    , ui(new Ui::EcuOperationsWindow)
{
    ui->setupUi(this);

    if (cmd_type == "test_write")
        this->setWindowTitle("Test write ROM " + ecuCalDef->FileName + " to ECU");
    else if (cmd_type == "write")
        this->setWindowTitle("Write ROM " + ecuCalDef->FileName + " to ECU");
    else if (cmd_type == "read")
        this->setWindowTitle("Read ROM from ECU");

    this->serial = serial;
}

void FlashEcuSubaruUnisiaJecs::run()
{
    this->show();

    int result = STATUS_ERROR;
    set_progressbar_value(0);

    //result = init_flash_denso_kline_04(ecuCalDef, cmd_type);

    bool ok = false;

    mcu_type_string = ecuCalDef->McuType;
    mcu_type_index = 0;

    while (flashdevices[mcu_type_index].name != 0)
    {
        if (flashdevices[mcu_type_index].name == mcu_type_string)
            break;
        mcu_type_index++;
    }
    if (flashdevices[mcu_type_index].name != mcu_type_string)
    {
        QMessageBox::warning(this, tr("ECU Operation"), "ECU operation failed, selected MCU does not exists!");
        return;
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
    else if (cmd_type == "write")
    {
        uint32_t chk_start = 0x1000;
        uint32_t chk_end = flashdevices[mcu_type_index].romsize;
        uint32_t chk_sum = 0;
        uint32_t chk_xor = 0;
        uint32_t chk_sum_rom = ecuCalDef->FullRomData.at(chk_start + 6);
        uint32_t chk_xor_rom = ecuCalDef->FullRomData.at(chk_start + 7);
        for (uint32_t i = chk_start; i < chk_end; i++)
        {
            if (i < (chk_start + 6) || i > (chk_start + 7))
            {
                chk_sum += ecuCalDef->FullRomData.at(i);
                chk_xor ^= ecuCalDef->FullRomData.at(i);
            }
        }
        QByteArray msg;
        emit LOG_I("Checksums:", true, true);
        msg = QString("SUM: 0x%1 | 0x%2").arg((chk_sum & 0xff), 2, 16, QLatin1Char(' ')).arg((chk_sum_rom & 0xff), 2, 16, QLatin1Char(' ')).toUtf8();
        emit LOG_I(msg, true, true);
        msg = QString("XOR: 0x%1 | 0x%2").arg((chk_xor & 0xff), 2, 16, QLatin1Char(' ')).arg((chk_xor_rom & 0xff), 2, 16, QLatin1Char(' ')).toUtf8();
        emit LOG_I(msg, true, true);

        return;
    }

    // Set serial port
    serial->set_is_iso14230_connection(false);
    serial->set_is_can_connection(false);
    serial->set_is_iso15765_connection(false);
    serial->set_is_29_bit_id(false);
    serial->set_serial_port_baudrate("1953");
    serial->set_serial_port_parity(QSerialPort::EvenParity);
    // Open serial port
    serial->open_serial_port();

    int ret = QMessageBox::warning(this, tr("Connecting to ECU"),
                                   tr("Turn ignition ON and press OK to start initializing connection to ECU"),
                                   QMessageBox::Ok | QMessageBox::Cancel,
                                   QMessageBox::Ok);

    switch (ret)
    {
        case QMessageBox::Ok:
            if (cmd_type == "read")
            {
                emit external_logger("Reading ROM, please wait...");
                emit LOG_I("Reading ROM from Subaru Unisia Jecs using SSM cable", true, true);
                result = read_mem(flashdevices[mcu_type_index].fblocks[0].start, flashdevices[mcu_type_index].romsize);
            }
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
            LOG_D("Operation canceled", true, true);
            this->close();
            break;
        default:
            QMessageBox::warning(this, tr("Connecting to ECU"), "Unknown operation selected!");
            LOG_D("Unknown operation selected!", true, true);
            this->close();
            break;
        }


}

FlashEcuSubaruUnisiaJecs::~FlashEcuSubaruUnisiaJecs()
{
    delete ui;
}

void FlashEcuSubaruUnisiaJecs::closeEvent(QCloseEvent *bar)
{
    kill_process = true;
}


/*
 * Read memory from Subaru Denso K-Line 32bit ECUs, nisprog kernel
 *
 * @return success
 */
int FlashEcuSubaruUnisiaJecs::read_mem(uint32_t start_addr, uint32_t length)
{
    QElapsedTimer timer;
    QByteArray output;
    QByteArray received;
    QByteArray received_flush;
    QByteArray msg;
    QByteArray mapdata;

    uint32_t addr = start_addr;
    uint32_t willget = length;
    uint32_t len_done = 0;  //total data written to file
    uint32_t cplen = 0;
    uint32_t timeout = 0;

    bool byte_received = false;
    bool bytes_synced = false;

    #define NP10_MAXBLKS    32   //# of blocks to request per loop. Too high might flood us

    emit  LOG_I("Set ECU to read mode", true, true);

    output.append((uint8_t)0x78);
    output.append((uint8_t)0x12);
    output.append((uint8_t)0x34);
    output.append((uint8_t)0x00);
    serial->write_serial_data_echo_check(output);
    delay(500);
    received_flush = serial->read_serial_data(1000);

    timer.start();
    set_progressbar_value(0);

    start_addr = 0;
    addr = start_addr;
    length = 0x20;
    willget = length;
    mapdata.clear();
    mapdata.fill('\x00', start_addr + length);

    received.clear();
    emit LOG_I("Start reading, please wait...", true, true);
    while (willget)
    {
        if (kill_process)
            return STATUS_ERROR;

        uint32_t numblocks = 0;
        float pleft = 0;
        unsigned long chrono;
        unsigned curspeed = 0;
        unsigned tleft = 0;

        numblocks = willget / 32;

        if (numblocks > NP10_MAXBLKS)
            numblocks = NP10_MAXBLKS;

        uint32_t pagesize = numblocks * 32;

        pleft = (float)(addr - start_addr) / (float)length * 100.0f;
        set_progressbar_value(pleft);

        output.clear();
        output.append((uint8_t)0x78);
        output.append((uint8_t)((addr >> 8) & 0xFF));
        output.append((uint8_t)(addr & 0xFF));
        output.append((uint8_t)(0x00 & 0xFF));

        emit LOG_D("Write data: " + parse_message_to_hex(output), true, true);
        serial->write_serial_data(output);
        delay(45);

        byte_received = false;
        QTime dieTime;
        uint8_t loop_count = 0;

        received.clear();
        dieTime = QTime::currentTime().addMSecs(500);
        while (!byte_received)
        {
            if (kill_process)
                return STATUS_ERROR;

            if (QTime::currentTime() > dieTime)
            {
                timeout++;
                dieTime = QTime::currentTime().addMSecs(500);
                serial->write_serial_data(output);
            }

            received.append(serial->read_serial_data(5));
            while (received.length() >= 3)
            {
                if (received.at(0) == output.at(1) && received.at(1) == output.at(2))// && received.at(3) == output.at(1) && received.at(4) == output.at(2))
                {
                    byte_received = true;
                    bytes_synced = true;
                    received.remove(0, 2);
                    mapdata.replace(addr, 1, received);
                    received.remove(0, 1);
                }
                else
                {
                    if (bytes_synced)
                        received.remove(0, 3);
                    else
                        received.remove(0, 1);
                }
                loop_count++;
            }
            if (loop_count > 10)
                bytes_synced = false;
            QCoreApplication::processEvents(QEventLoop::AllEvents, 1);
        }
        received_flush = serial->read_serial_data(5);

        cplen = 1;

        chrono = timer.elapsed();

        if (cplen > 0 && chrono > 0)
            curspeed = cplen * (1000.0f / chrono);

        if (!curspeed) {
            curspeed += 1;
        }

        tleft = (willget / curspeed) % 9999;
        tleft++;

        timer.start();

        QString start_address = QString("%1").arg(addr,8,16,QLatin1Char('0')).toUpper();
        QString block_len = QString("%1").arg(pagesize,8,16,QLatin1Char('0')).toUpper();
        msg = QString("Kernel read addr:  0x%1  length:  0x%2,  %3  B/s  %4 s").arg(start_address).arg(block_len).arg(curspeed, 6, 10, QLatin1Char(' ')).arg(tleft, 6, 10, QLatin1Char(' ')).toUtf8();
        emit LOG_I(msg, true, true);
        delay(1);

        addr++;
        willget--;
    }
    emit LOG_I("Reading finished.", true, true);
    qDebug() << "Timeouts:" << timeout;
    qDebug() << "Mapdata length:" << mapdata.length();
    ecuCalDef->FullRomData = mapdata;
    set_progressbar_value(100);

    return STATUS_SUCCESS;
}



/*
 * Parse QByteArray to readable form
 *
 * @return parsed message
 */
QString FlashEcuSubaruUnisiaJecs::parse_message_to_hex(QByteArray received)
{
    QString msg;

    for (int i = 0; i < received.length(); i++)
    {
        msg.append(QString("%1 ").arg((uint8_t)received.at(i),2,16,QLatin1Char('0')).toUtf8());
    }

    return msg;
}

void FlashEcuSubaruUnisiaJecs::set_progressbar_value(int value)
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

void FlashEcuSubaruUnisiaJecs::delay(int timeout)
{
    QTime dieTime = QTime::currentTime().addMSecs(timeout);
    while (QTime::currentTime() < dieTime)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 1);
}
