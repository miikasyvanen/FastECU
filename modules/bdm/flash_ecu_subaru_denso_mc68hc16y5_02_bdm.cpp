#include "flash_ecu_subaru_denso_mc68hc16y5_02_bdm.h"
#include "serial_port_actions.h"

FlashEcuSubaruDensoMC68HC16Y5_02_BDM::FlashEcuSubaruDensoMC68HC16Y5_02_BDM(SerialPortActions *serial, FileActions::EcuCalDefStructure *ecuCalDef, QString cmd_type, QWidget *parent)
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
        this->setWindowTitle("Read ROM from ECU");

    this->serial = serial;
}

void FlashEcuSubaruDensoMC68HC16Y5_02_BDM::run()
{
    this->show();

    int result = STATUS_ERROR;
    set_progressbar_value(0);

    bool ok = false;

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
        emit LOG_I("Read ROM from Subaru Denso MC68HC16 with BDM", true, true);
    }
    else if (cmd_type == "write")
    {
        emit LOG_I("Write ROM to Subaru Denso MC68HC16 with BDM", true, true);
    }

    // Set serial port
    serial->set_is_iso14230_connection(false);
    serial->set_is_can_connection(false);
    serial->set_is_iso15765_connection(false);
    serial->set_is_29_bit_id(false);
    serial->set_serial_port_baudrate("115200");
    // Open serial port
    serial->open_serial_port();

    int ret = QMessageBox::warning(this, tr("Connecting to ECU"),
                                   tr("Turn ignition ON and press OK to start initializing connection to ECU"),
                                   QMessageBox::Ok | QMessageBox::Cancel,
                                   QMessageBox::Ok);

    switch (ret)
    {
        case QMessageBox::Ok:
            emit LOG_I("Connecting to Subaru Denso MC68HC16 with BDM, please wait...", true, true);
            emit external_logger("Preparing, please wait...");
            if (cmd_type == "read")
            {
                emit external_logger("Reading ROM, please wait...");
                emit LOG_I("Reading ROM from Subaru Denso MC68HC16 with BDM", true, true);
                result = read_mem(flashdevices[mcu_type_index].fblocks[0].start, flashdevices[mcu_type_index].romsize);
                //result = read_mem(0x0, 0x1000);
            }
            if (cmd_type == "write")
            {
                emit external_logger("Writing ROM, please wait...");
                emit LOG_I("Writing ROM to Subaru Denso MC68HC16 with BDM", true, true);
                result = write_mem();
                //if (result == STATUS_SUCCESS)
                //    int read_result = read_mem(0x0, 0x100);
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
            qDebug() << "Operation canceled";
            this->close();
            break;
        default:
            QMessageBox::warning(this, tr("Connecting to ECU"), "Unknown operation selected!");
            qDebug() << "Unknown operation selected!";
            this->close();
            break;
    }
}

FlashEcuSubaruDensoMC68HC16Y5_02_BDM::~FlashEcuSubaruDensoMC68HC16Y5_02_BDM()
{
    delete ui;
}

void FlashEcuSubaruDensoMC68HC16Y5_02_BDM::closeEvent(QCloseEvent *bar)
{
    kill_process = true;
}

/*******************************************************
 *  Read ROM 16bit K-Line ECUs
 ******************************************************/
int FlashEcuSubaruDensoMC68HC16Y5_02_BDM::read_mem(uint32_t start_addr, uint32_t length)
{
    QElapsedTimer timer;
    QByteArray output;
    QByteArray received;
    QByteArray msg;
    QByteArray mapdata;

    uint32_t pagesize = 0;
    uint32_t cplen = 0;

    pagesize = 0x400;
    if (length < pagesize)
        pagesize = length;

    if (start_addr == 0 && length == 0)
    {
        start_addr = 0;
        length = 0x028000;
    }

    uint32_t skip_start = start_addr & (pagesize - 1); //if unaligned, we'll be receiving this many extra bytes
    uint32_t addr = start_addr - skip_start;
    uint32_t willget = (skip_start + length + pagesize - 1) & ~(pagesize - 1);
    uint32_t len_done = 0;  //total data written to file

    timer.start();

    // Clear receive buffer
    received = serial->read_serial_data(serial_read_short_timeout);
    set_progressbar_value(0);

    while (willget)
    {
        if (kill_process)
            return STATUS_ERROR;

        uint32_t numblocks = 1;
        unsigned curspeed = 0, tleft;
        uint32_t curblock = (addr / pagesize);
        float pleft = 0;
        unsigned long chrono;

        pleft = (float)(addr - start_addr) / (float)(length + 0x8000) * 100.0f;
        set_progressbar_value(pleft);

        if (addr >= flashdevices[mcu_type_index].rblocks->start && addr < (flashdevices[mcu_type_index].rblocks->start + flashdevices[mcu_type_index].rblocks->len))
        {
            received.clear();
            for (unsigned int j = flashdevices[mcu_type_index].rblocks->start; j < (flashdevices[mcu_type_index].rblocks->start + flashdevices[mcu_type_index].rblocks->len); j++)
            {
                received.append((uint8_t)0xff);
            }
            mapdata.append(received);

            addr = flashdevices[mcu_type_index].rblocks->start + flashdevices[mcu_type_index].rblocks->len;
        }

        QString start_address = QString("%1").arg(addr,8,16,QLatin1Char('0')).toUpper();
        QString block_len = QString("%1").arg(pagesize,8,16,QLatin1Char('0')).toUpper();
        msg = QString("rpmem 0x%1 0x%2").arg(start_address).arg(block_len).toUtf8();

        output.clear();
        output.append(msg);
        received = serial->write_serial_data(output);

        qDebug() << "Sent:" << output << parse_message_to_hex(output);
        received.clear();
        uint32_t loopcount = 0;
        while ((uint32_t)received.length() != pagesize && loopcount < 50)
        {
            received = serial->read_serial_data(serial_read_short_timeout);
            qDebug() << ".";
            loopcount++;
            delay(100);
        }

        if (received.length())
        {
            mapdata.append(received);
            qDebug() << "Received:" << parse_message_to_hex(received);
        }
        else
        {
            qDebug() << "ERROR IN DATA RECEIVE!";
            qDebug() << "Received:" << received << parse_message_to_hex(received);
            return STATUS_ERROR;
        }

        cplen = (numblocks * pagesize);

        chrono = timer.elapsed();
        timer.start();

        if (cplen > 0 && chrono > 0)
            curspeed = cplen * (1000.0f / chrono);

        if (!curspeed) {
            curspeed += 1;
        }

        tleft = (willget / curspeed) % 9999;
        tleft++;

        start_address = QString("%1").arg(addr,8,16,QLatin1Char('0')).toUpper();
        block_len = QString("%1").arg(pagesize,8,16,QLatin1Char('0')).toUpper();
        msg = QString("BDM read addr:  0x%1  length:  0x%2,  %3  B/s  %4 s").arg(start_address).arg(block_len).arg(curspeed, 6, 10, QLatin1Char(' ')).arg(tleft, 6, 10, QLatin1Char(' ')).toUtf8();
        emit LOG_I(msg, true, true);
        delay(1);

        len_done += cplen;
        addr += (numblocks * pagesize);
        willget -= pagesize;
    }

    ecuCalDef->FullRomData = mapdata;
    set_progressbar_value(100);

    return STATUS_SUCCESS;
}

int FlashEcuSubaruDensoMC68HC16Y5_02_BDM::write_mem()
{
    QByteArray output;
    QByteArray received;
    QByteArray msg;

    unsigned blockno;

    // Clear receive buffer
    received = serial->read_serial_data(serial_read_short_timeout);
    set_progressbar_value(0);

    flashbytescount = 0;
    flashbytesindex = 0;



    QFile file(kernel);
    // Check kernel file
    if (!file.open(QIODevice::ReadOnly ))
    {
        emit LOG_I("Unable to open kernel file for reading", true, true);
        return -1;
    }
    uint32_t file_len = file.size();
    uint32_t pl_len = file_len;
    QByteArray pl_encr = file.readAll();
    uint32_t len = pl_len;
    ecuCalDef->FullRomData.clear();
    ecuCalDef->FullRomData.append(pl_encr);
    QScopedArrayPointer<uint8_t> data_array(new uint8_t[pl_encr.length()]);

    while (ecuCalDef->FullRomData.length() % 0x20)
        ecuCalDef->FullRomData.append((uint8_t)0x00);

    qDebug() << "Kernel file: " << kernel;
    qDebug() << "Kernel length: " << ecuCalDef->FullRomData.length();

    flash_block(&data_array[0], &flashdevices[mcu_type_index], 0);

    // Change baudrate (kernel do this anyway)
    qDebug() << "Set SCIB baudrate to 62500 and enable it";
    QString cmd_wr_response = "ACK_CMD_WDMEM";
    QString wr_response = "ACK_WR";
    msg = QString("wdmem 0xFFC28 0x4").toUtf8();
    output.clear();
    output.append(msg);
    received = serial->write_serial_data(output);
    qDebug() << "Sent:" << output << parse_message_to_hex(output);
    received = serial->read_serial_data(serial_read_extra_long_timeout);
    qDebug() << "Received:" << received << parse_message_to_hex(received);
    if (received.length() < cmd_wr_response.length() || received != cmd_wr_response)
    {
        received.append(serial->read_serial_data(serial_read_long_timeout));
        qDebug() << "ERROR! Received:" << received << parse_message_to_hex(received);
        return STATUS_ERROR;
    }
    received = serial->read_serial_data(serial_read_long_timeout);
    qDebug() << "Received:" << parse_message_to_hex(received);

    output.clear();
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x0D);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x0C);
    received = serial->write_serial_data(output);
    qDebug() << "Sent:" << output << parse_message_to_hex(output);
    received = serial->read_serial_data(serial_read_long_timeout);
    qDebug() << "Received:" << received << parse_message_to_hex(received);
    if (received.length() < wr_response.length() || received != wr_response)
    {
        received.append(serial->read_serial_data(serial_read_long_timeout));
        qDebug() << "ERROR! Received:" << received << parse_message_to_hex(received);
        //delay(20);
        return STATUS_ERROR;
    }
    received = serial->read_serial_data(serial_read_short_timeout);
    qDebug() << "Received:" << parse_message_to_hex(received);

    qDebug() << "Set program counter and stack pointer";
    msg = QString("wpcsp").toUtf8();
    output.clear();
    output.append(msg);
    received = serial->write_serial_data(output);
    received = serial->read_serial_data(serial_read_long_timeout);
    qDebug() << "Received:" << received << parse_message_to_hex(received);

    received = serial->read_serial_data(serial_read_long_timeout);
    qDebug() << "Received:" << received << parse_message_to_hex(received);
/*
    msg = QString("rdmem 0x20000 0x600 hex").toUtf8();
    output.clear();
    output.append(msg);
    received = serial->write_serial_data(output);
    received = serial->read_serial_data(0x600, serial_read_extra_long_timeout);
    qDebug() << "Received:" << received << parse_message_to_hex(received);

    received = serial->read_serial_data(0x600, serial_read_extra_long_timeout);
    qDebug() << "Received:" << received << parse_message_to_hex(received);
*/
    qDebug() << "GO!!!";
    msg = QString("go").toUtf8();
    output.clear();
    output.append(msg);
    received = serial->write_serial_data(output);
    received = serial->read_serial_data(serial_read_long_timeout);
    qDebug() << "Received:" << received << parse_message_to_hex(received);

/*
    for (blockno = 0; blockno < flashdevices[mcu_type_index].numblocks; blockno++)
    {
        flashbytescount += flashdevices[mcu_type_index].fblocks[blockno].len;
    }

    for (blockno = 0; blockno < flashdevices[mcu_type_index].numblocks; blockno++)
    {
        if (kill_process)
            return STATUS_ERROR;

        if (flash_block(&data_array[flashdevices[mcu_type_index].fblocks->start], &flashdevices[mcu_type_index], blockno))
        {
            qDebug() << "ERROR in block write!";
            return STATUS_ERROR;
        }
        else
        {
            flashbytesindex += flashdevices[mcu_type_index].fblocks[blockno].len;
        }

    }
*/
    return STATUS_SUCCESS;
}

int FlashEcuSubaruDensoMC68HC16Y5_02_BDM::flash_block(const uint8_t *newdata, const struct flashdev_t *fdt, unsigned blockno)
{
    QByteArray output;
    QByteArray received;
    QByteArray msg;
    QElapsedTimer timer;
    unsigned long chrono;
    unsigned curspeed, tleft;
    uint32_t blocksize = 0x20;
    uint32_t start = fdt->rblocks[blockno].start;
    uint32_t len = ecuCalDef->FullRomData.length();
    //uint32_t start = fdt->fblocks[blockno].start;
    //uint32_t len = fdt->fblocks[blockno].len;
    uint32_t remain = len;
    uint32_t byteindex = flashbytesindex;

    QString cmd_wr_response = "ACK_CMD_WDMEM";
    //QString cmd_wr_response = "ACK_CMD_WPMEM";
    QString wr_response = "ACK_WR";
    //set_progressbar_value(0);

//    if (start >= flashdevices[mcu_type_index].rblocks->start && start < (flashdevices[mcu_type_index].rblocks->start + flashdevices[mcu_type_index].rblocks->len))
//        start = flashdevices[mcu_type_index].rblocks->start + flashdevices[mcu_type_index].rblocks->len;

    QString start_addr = QString("%1").arg((uint32_t)start,8,16,QLatin1Char('0')).toUpper();
    QString length = QString("%1").arg((uint32_t)len,8,16,QLatin1Char('0')).toUpper();
    QString block = QString("%1").arg((uint32_t)blockno,2,16,QLatin1Char('0')).toUpper();
    msg = QString("Writing block: " + block + " at address 0x" + start_addr).toUtf8();
    emit LOG_I(msg, true, true);
    msg = QString("wdmem 0x" + start_addr + " 0x" + length).toUtf8();
    //msg = QString("wdmem 0x" + start_addr + " 0x" + length).toUtf8();
    qDebug() << msg;

    output.clear();
    output.append(msg);
    received = serial->write_serial_data(output);
    qDebug() << "Sent:" << output << parse_message_to_hex(output);
    received = serial->read_serial_data(serial_read_extra_long_timeout);
    qDebug() << "Received:" << received << parse_message_to_hex(received);

    if (received.length() < cmd_wr_response.length() || received != cmd_wr_response)
    {
        received.append(serial->read_serial_data(serial_read_long_timeout));
        qDebug() << "ERROR! Received:" << received << parse_message_to_hex(received);
        return STATUS_ERROR;
    }

    timer.start();
    while (remain) {
        if (kill_process)
            return STATUS_ERROR;

        output.clear();
        for (uint32_t i = 0; i < blocksize; i++)
        {
            //output.append((uint8_t)0xFF);
            //output.append(ecuCalDef->FullRomData.at(start + i));
            output.append(ecuCalDef->FullRomData.at(byteindex + i));
        }
        received = serial->write_serial_data(output);
        //delay(20);
        qDebug() << "Sent:" << parse_message_to_hex(output);

        received = serial->read_serial_data(serial_read_long_timeout);
        qDebug() << "Received:" << received << parse_message_to_hex(received);
        //delay(20);

        if (received.length() < wr_response.length() || received != wr_response)
        {
            received.append(serial->read_serial_data(serial_read_long_timeout));
            qDebug() << "ERROR! Received:" << received << parse_message_to_hex(received);
            //delay(20);
            return STATUS_ERROR;
        }

        QString start_address = QString("%1").arg(start,8,16,QLatin1Char('0')).toUpper();
        msg = QString("Writing chunk @ 0x%1 (%2\% - %3 B/s, ~ %4 s)").arg(start_address).arg((unsigned) 100 * (len - remain) / len,1,10,QLatin1Char('0')).arg((uint32_t)curspeed,1,10,QLatin1Char('0')).arg(tleft,1,10,QLatin1Char('0')).toUtf8();
        emit LOG_I(msg, true, true);

        remain -= blocksize;
        start += blocksize;
        byteindex += blocksize;
        //src += blocksize;

        chrono = timer.elapsed();
        timer.start();

        if (!chrono) {
            chrono += 1;
        }
        curspeed = blocksize * (1000.0f / chrono);  //avg B/s
        if (!curspeed) {
            curspeed += 1;
        }

        tleft = ((float)flashbytescount - byteindex) / curspeed;  //s
        if (tleft > 9999) {
            tleft = 9999;
        }
        tleft++;

        float pleft = (float)byteindex / (float)len * 100.0f;
        set_progressbar_value(pleft);
    }
    emit LOG_I("Block write complete.", true, true);
    received = serial->read_serial_data(serial_read_short_timeout);
    set_progressbar_value(100);

    return STATUS_SUCCESS;
}

/*
 * Parse QByteArray to readable form
 *
 * @return parsed message
 */
QString FlashEcuSubaruDensoMC68HC16Y5_02_BDM::parse_message_to_hex(QByteArray received)
{
    QString msg;

    for (int i = 0; i < received.length(); i++)
    {
        msg.append(QString("%1 ").arg((uint8_t)received.at(i),2,16,QLatin1Char('0')).toUtf8());
    }

    return msg;
}

void FlashEcuSubaruDensoMC68HC16Y5_02_BDM::set_progressbar_value(int value)
{
    if (ui->progressbar)
        ui->progressbar->setValue(value);
    QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
}

void FlashEcuSubaruDensoMC68HC16Y5_02_BDM::set_progressbar_value_by_client(int value)
{
    if (ui->progressbar)
    {
        ui->progressbar->setValue(value);
    }
    QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
}

void FlashEcuSubaruDensoMC68HC16Y5_02_BDM::delay(int timeout)
{
    QTime dieTime = QTime::currentTime().addMSecs(timeout);
    while (QTime::currentTime() < dieTime)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 1);
}

