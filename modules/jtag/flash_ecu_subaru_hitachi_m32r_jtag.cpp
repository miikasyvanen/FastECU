#include "flash_ecu_subaru_hitachi_m32r_jtag.h"
#include "serial_port_actions.h"

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
    tester_id = 0xF0;
    target_id = 0x10;
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
    //delay(100);
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
    QByteArray received;

    emit LOG_D("Hard reset JTAG", true, true);
    output.clear();
    output.append((uint8_t)0x80);
    output = add_header(output);
    serial->write_serial_data(output);
    emit LOG_I("Sent: " + parse_message_to_hex(output), true, true);
    delay(10);
    received = serial->read_serial_data(serial_read_short_timeout);
    emit LOG_I("Response: " + parse_message_to_hex(received), true, true);
}

int FlashEcuSubaruHitachiM32rJtag::read_idcode()
{
    QByteArray output;
    QByteArray received;

    emit LOG_D("Read ID code", true, true);
    output.clear();
    output.append((uint8_t)SUB_KERNEL_ID & 0xFF);
    output = add_header(output);
    serial->write_serial_data(output);
    emit LOG_I("Sent: " + parse_message_to_hex(output), true, true);
    delay(10);
    received = serial->read_serial_data(serial_read_short_timeout);
    if (received.length() > 4)
    {
        if ((uint8_t)received.at(0) != ((SUB_KERNEL_START_COMM >> 8) & 0xFF) || (uint8_t)received.at(1) != (SUB_KERNEL_START_COMM & 0xFF) || (uint8_t)received.at(4) != (SUB_KERNEL_ID | 0x40) || (uint8_t)received.at(8) != SUB_KERNEL_JTAG_IR_ACK)
        {
            emit LOG_E("Wrong response from ECU: " + FileActions::parse_nrc_message(received.mid(4, received.length()-1)), true, true);
            
            return STATUS_ERROR;
        }
    }
    else
    {
        emit LOG_E("No valid response from ECU", true, true);
        
        return STATUS_ERROR;
    }
    emit LOG_I("Device ID request ok", true, true);
    

    QByteArray response = received.mid(9, 4);
    uint32_t dev_id;
    QString version = 0;
    QString part_number = 0;
    QString manufacturer_id = 0;

    dev_id = (uint8_t)response.at(0) << 24;
    dev_id += (uint8_t)response.at(1) << 16;
    dev_id += (uint8_t)response.at(2) << 8;
    dev_id += (uint8_t)response.at(3);
    version = QString::number((dev_id & 0xf0000000) >> 28, 16);
    part_number = QString::number((dev_id & 0x0ffff000) >> 12, 16);
    manufacturer_id = QString::number((dev_id & 0x00000ffe) >> 1, 16);
    emit LOG_I("Device ID: 0x" + QString::number(dev_id, 16), true, true);
    emit LOG_I("Version: 0x" + version, true, true);
    emit LOG_I("Part number: 0x" + part_number, true, true);
    emit LOG_I("Manufacturer ID: 0x" + manufacturer_id, true, true);

    return STATUS_SUCCESS;
}

int FlashEcuSubaruHitachiM32rJtag::read_usercode()
{
    QByteArray output;
    QByteArray received;

    emit LOG_D("Read user code", true, true);
    output.clear();
    output.append((uint8_t)SUB_KERNEL_READ_USERCODE & 0xFF);
    output = add_header(output);
    serial->write_serial_data(output);
    emit LOG_I("Sent: " + parse_message_to_hex(output), true, true);
    delay(10);
    received = serial->read_serial_data(serial_read_short_timeout);
    if (received.length() > 4)
    {
        if ((uint8_t)received.at(0) != ((SUB_KERNEL_START_COMM >> 8) & 0xFF) || (uint8_t)received.at(1) != (SUB_KERNEL_START_COMM & 0xFF) || (uint8_t)received.at(4) != (SUB_KERNEL_READ_USERCODE | 0x40) || (uint8_t)received.at(8) != SUB_KERNEL_JTAG_IR_ACK)
        {
            emit LOG_E("Wrong response from ECU: " + FileActions::parse_nrc_message(received.mid(4, received.length()-1)), true, true);
            
            return STATUS_ERROR;
        }
    }
    else
    {
        emit LOG_E("No valid response from ECU", true, true);
        
        return STATUS_ERROR;
    }
    emit LOG_I("User code request ok", true, true);
    

    QByteArray response = received.mid(9, 4);
    uint32_t dev_id;
    QString rom = 0;
    QString isa = 0;
    QString sdi = 0;

    dev_id = (uint8_t)response.at(0) << 24;
    dev_id += (uint8_t)response.at(1) << 16;
    dev_id += (uint8_t)response.at(2) << 8;
    dev_id += (uint8_t)response.at(3);
    rom = QString::number((dev_id & 0x00000f00) >> 8, 16);
    isa = QString::number((dev_id & 0x000000f0) >> 4, 16);
    sdi = QString::number(dev_id & 0x0000000f, 16);
    emit LOG_I("User code: 0x" + QString::number(dev_id, 16), true, true);
    emit LOG_I("ROM: 0x" + rom, true, true);
    emit LOG_I("ISA: 0x" + isa, true, true);
    emit LOG_I("SDI: 0x" + sdi, true, true);
    

    return STATUS_SUCCESS;
}

int FlashEcuSubaruHitachiM32rJtag::read_tool_rom_code()
{
    QByteArray output;
    QByteArray received;

    emit LOG_I("Tool-ROM setting verification code:", true, true);

    output.clear();
    output.append((uint8_t)SUB_KERNEL_JTAG_COMMAND & 0xFF);
    output.append((uint8_t)SUB_KERNEL_IR_EXTEST & 0xFF);
    output.append((uint8_t)SUB_KERNEL_SUB_CMD_READ & 0xFF);
    output = add_header(output);
    serial->write_serial_data(output);
    emit LOG_I("Sent: " + parse_message_to_hex(output), true, true);
    delay(10);
    received = serial->read_serial_data(serial_read_short_timeout);
    if (received.length() > 4)
    {
        if ((uint8_t)received.at(0) != ((SUB_KERNEL_START_COMM >> 8) & 0xFF) || (uint8_t)received.at(1) != (SUB_KERNEL_START_COMM & 0xFF) || (uint8_t)received.at(4) != (SUB_KERNEL_JTAG_COMMAND | 0x40) || (uint8_t)received.at(8) != SUB_KERNEL_JTAG_IR_ACK)
        {
            emit LOG_E("Wrong response from ECU: " + FileActions::parse_nrc_message(received.mid(4, received.length()-1)), true, true);
            
            return STATUS_ERROR;
        }
    }
    else
    {
        emit LOG_E("No valid response from ECU", true, true);
        
        return STATUS_ERROR;
    }
    emit LOG_I("IR EXTEST transfer ok", true, true);
    

    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 3; j++)
        {
            output.clear();
            output.append((uint8_t)SUB_KERNEL_JTAG_COMMAND & 0xFF);
            output.append((uint8_t)SUB_KERNEL_MON_CODE & 0xFF);
            output.append((uint8_t)SUB_KERNEL_SUB_CMD_WRITE & 0xFF);
            output.append((uint8_t)inst_tool_rom_code[j*4+0]);
            output.append((uint8_t)inst_tool_rom_code[j*4+1]);
            output.append((uint8_t)inst_tool_rom_code[j*4+2]);
            output.append((uint8_t)inst_tool_rom_code[j*4+3]);
            output = add_header(output);
            serial->write_serial_data(output);
            emit LOG_I("Sent: " + parse_message_to_hex(output), true, true);
            delay(10);
            received = serial->read_serial_data(serial_read_short_timeout);
            if (received.length() > 4)
            {
                if ((uint8_t)received.at(0) != ((SUB_KERNEL_START_COMM >> 8) & 0xFF) || (uint8_t)received.at(1) != (SUB_KERNEL_START_COMM & 0xFF) || (uint8_t)received.at(4) != (SUB_KERNEL_JTAG_COMMAND | 0x40) || (uint8_t)received.at(8) != SUB_KERNEL_JTAG_IR_ACK)
                {
                    emit LOG_E("Wrong response from ECU: " + FileActions::parse_nrc_message(received.mid(4, received.length()-1)), true, true);
                    
                    return STATUS_ERROR;
                }
            }
            else
            {
                emit LOG_E("No valid response from ECU", true, true);
                
                return STATUS_ERROR;
            }
            emit LOG_I("MON_CODE transfer ok", true, true);
            
        }

    }
    output.clear();
    output.append((uint8_t)SUB_KERNEL_JTAG_COMMAND & 0xFF);
    output.append((uint8_t)SUB_KERNEL_MON_CODE & 0xFF);
    output.append((uint8_t)SUB_KERNEL_SUB_CMD_WRITE & 0xFF);
    output.append((uint8_t)inst_tool_rom_code[3*4+0]);
    output.append((uint8_t)inst_tool_rom_code[3*4+1]);
    output.append((uint8_t)inst_tool_rom_code[3*4+2]);
    output.append((uint8_t)inst_tool_rom_code[3*4+3]);
    output = add_header(output);
    serial->write_serial_data(output);
    emit LOG_I("Sent: " + parse_message_to_hex(output), true, true);
    delay(10);
    received = serial->read_serial_data(serial_read_short_timeout);
    if (received.length() > 4)
    {
        if ((uint8_t)received.at(0) != ((SUB_KERNEL_START_COMM >> 8) & 0xFF) || (uint8_t)received.at(1) != (SUB_KERNEL_START_COMM & 0xFF) || (uint8_t)received.at(4) != (SUB_KERNEL_JTAG_COMMAND | 0x40) || (uint8_t)received.at(8) != SUB_KERNEL_JTAG_IR_ACK)
        {
            emit LOG_E("Wrong response from ECU: " + FileActions::parse_nrc_message(received.mid(4, received.length()-1)), true, true);
            
            return STATUS_ERROR;
        }
    }
    else
    {
        emit LOG_E("No valid response from ECU", true, true);
        
        return STATUS_ERROR;
    }
    emit LOG_I("MON_CODE transfer ok", true, true);
    

    output.clear();
    output.append((uint8_t)SUB_KERNEL_JTAG_COMMAND & 0xFF);
    output.append((uint8_t)SUB_KERNEL_MON_ACCESS & 0xFF);
    output.append((uint8_t)SUB_KERNEL_SUB_CMD_WRITE & 0xFF);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x01);
    output = add_header(output);
    serial->write_serial_data(output);
    emit LOG_I("Sent: " + parse_message_to_hex(output), true, true);
    delay(10);
    received = serial->read_serial_data(serial_read_short_timeout);
    if (received.length() > 4)
    {
        if ((uint8_t)received.at(0) != ((SUB_KERNEL_START_COMM >> 8) & 0xFF) || (uint8_t)received.at(1) != (SUB_KERNEL_START_COMM & 0xFF) || (uint8_t)received.at(4) != (SUB_KERNEL_JTAG_COMMAND | 0x40) || (uint8_t)received.at(8) != SUB_KERNEL_JTAG_IR_ACK)
        {
            emit LOG_E("Wrong response from ECU: " + FileActions::parse_nrc_message(received.mid(4, received.length()-1)), true, true);
            
            return STATUS_ERROR;
        }
    }
    else
    {
        emit LOG_E("No valid response from ECU", true, true);
        
        return STATUS_ERROR;
    }
    emit LOG_I("MON_ACCESS transfer ok", true, true);
    

    for (int i = 0; i < 5; i++)
    {
        output.clear();
        output.append((uint8_t)SUB_KERNEL_JTAG_COMMAND & 0xFF);
        output.append((uint8_t)SUB_KERNEL_MON_DATA & 0xFF);
        output.append((uint8_t)SUB_KERNEL_SUB_CMD_READ & 0xFF);
        output = add_header(output);
        serial->write_serial_data(output);
        emit LOG_I("Sent: " + parse_message_to_hex(output), true, true);
        delay(10);
        received = serial->read_serial_data(serial_read_short_timeout);
        if (received.length() > 4)
        {
            if ((uint8_t)received.at(0) != ((SUB_KERNEL_START_COMM >> 8) & 0xFF) || (uint8_t)received.at(1) != (SUB_KERNEL_START_COMM & 0xFF) || (uint8_t)received.at(4) != (SUB_KERNEL_JTAG_COMMAND | 0x40) || (uint8_t)received.at(8) != SUB_KERNEL_JTAG_IR_ACK)
            {
                emit LOG_E("Wrong response from ECU: " + FileActions::parse_nrc_message(received.mid(4, received.length()-1)), true, true);
                
                return STATUS_ERROR;
            }
        }
        else
        {
            emit LOG_E("No valid response from ECU", true, true);
            
            return STATUS_ERROR;
        }
        emit LOG_I("MON_DATA transfer ok", true, true);
        

        delay(100);
    }

    return STATUS_SUCCESS;
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
/*
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
*/





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
        received = serial->read_serial_data(serial_read_short_timeout);
        if (!received.length())
            break;
        if (received.length())
            response = received.mid(4, (uint8_t)received.at(3));
        //
    } while (received.length());

    for (int i = 0; i < response.length(); i++)
    {
        response_inverted.append(response.at(response.length() - 1 - i));
    }

    return response_inverted;
}










/*
 * Add SSM header to message
 *
 * @return parsed message
 */
QByteArray FlashEcuSubaruHitachiM32rJtag::add_header(QByteArray output)
{
    QByteArray received;
    QByteArray msg;

    uint8_t length = output.length();

    output.insert(0, (uint8_t)0xbe);
    output.insert(1, (uint8_t)0xef);
    output.insert(2, length << 8);
    output.insert(3, length);

    output.append(calculate_checksum(output));

    //
    return output;
}

/*
 * Calculate SSM checksum to message
 *
 * @return 8-bit checksum
 */
uint8_t FlashEcuSubaruHitachiM32rJtag::calculate_checksum(QByteArray output)
{
    uint8_t checksum = 0;

    for (uint16_t i = 0; i < output.length(); i++)
        checksum += (uint8_t)output.at(i);

    return checksum;
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
