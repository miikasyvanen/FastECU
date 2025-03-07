#include "flash_ecu_subaru_hitachi_m32r_can.h"
#include "serial_port_actions.h"

FlashEcuSubaruHitachiM32rCan::FlashEcuSubaruHitachiM32rCan(SerialPortActions *serial, FileActions::EcuCalDefStructure *ecuCalDef, QString cmd_type, QWidget *parent)
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

void FlashEcuSubaruHitachiM32rCan::run()
{
    this->show();

    int result = STATUS_ERROR;

    //result = init_flash_subaru_hitachi_can();

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

    flash_method = ecuCalDef->FlashMethod;

    emit external_logger("Starting");

    if (cmd_type == "read")
    {
        emit LOG_I("Read memory with flashmethod '" + flash_method + "'", true, true);
    }
    else if (cmd_type == "write")
    {
        emit LOG_I("Write memory with flashmethod '" + flash_method + "'", true, true);
    }

    // Set serial port
    serial->set_is_iso14230_connection(false);
    serial->set_add_iso14230_header(false);
    serial->set_is_can_connection(false);
    serial->set_is_iso15765_connection(true);
    serial->set_is_29_bit_id(false);
    serial->set_can_speed("500000");
    serial->set_iso15765_source_address(0x7E0);
    serial->set_iso15765_destination_address(0x7E8);
    // Open serial port
    serial->open_serial_port();

    int ret = QMessageBox::warning(this, tr("Connecting to ECU"),
                                   tr("Turn ignition ON and press OK to start initializing connection to ECU"),
                                   QMessageBox::Ok | QMessageBox::Cancel,
                                   QMessageBox::Ok);

    switch (ret)
    {
        case QMessageBox::Ok:
            emit LOG_I("Connecting to Subaru ECU 512Kb Hitachi CAN bootloader, please wait...", true, true);
            result = connect_bootloader();

            if (result == STATUS_SUCCESS)
            {
                if (cmd_type == "read")
                {
                    emit external_logger("Reading ROM, please wait...");
                    emit LOG_I("Reading ROM from ECU Subaru using CAN", true, true);
                    result = read_mem(flashdevices[mcu_type_index].fblocks[0].start, flashdevices[mcu_type_index].romsize);
                }
                else if (cmd_type == "test_write" || cmd_type == "write")
                {
                    emit external_logger("Writing ROM, please wait...");
                    emit LOG_I("Writing ROM to Subaru Hitachi WA12212970WWW using CAN", true, true);
                    result = write_mem(test_write);
                }
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

FlashEcuSubaruHitachiM32rCan::~FlashEcuSubaruHitachiM32rCan()
{
    delete ui;
}

void FlashEcuSubaruHitachiM32rCan::closeEvent(QCloseEvent *event)
{
    kill_process = true;
}

/*
 * Connect to Subaru TCU Hitachi CAN bootloader
 *
 * @return success
 */
int FlashEcuSubaruHitachiM32rCan::connect_bootloader()
{
    QByteArray output;
    QByteArray received;
    QByteArray seed;
    QByteArray seed_key;
    QByteArray response;
    QString msg;

    if (!serial->is_serial_port_open())
    {
        emit LOG_I("ERROR: Serial port is not open.", true, true);
        return STATUS_ERROR;
    }

    emit LOG_I("Checking if OBK is already running...", true, true);

    output.clear();
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x07);
    output.append((uint8_t)0xE0);
    output.append((uint8_t)0xB7);
    serial->write_serial_data_echo_check(output);
    
    delay(50);
    received = serial->read_serial_data(serial_read_short_timeout);
    if (received.length() > 6)
    {
        if ((uint8_t)received.at(4) == 0x7F && (uint8_t)received.at(5) == 0xB7 && (uint8_t)received.at(6) == 0x13)
        {
            emit LOG_I("OBK is active!", true, true);
            
            kernel_alive = true;
            return STATUS_SUCCESS;
        }
    }
    emit LOG_I("OBK not active, initialising ECU...", true, true);

    emit LOG_I("Requesting ECU ID", true, true);
    output.clear();
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x07);
    output.append((uint8_t)0xE0);
    output.append((uint8_t)0xAA);

    serial->write_serial_data_echo_check(output);
    
    delay(50);
    received = serial->read_serial_data(serial_read_timeout);
    if (received.length() > 5)
    {
        if ((uint8_t)received.at(4) == 0xEA)
        {
            QByteArray response = received;
            response.remove(0, 8);
            response.remove(5, response.length()-5);
            
            QString ecuid;
            for (int i = 0; i < 5; i++)
                ecuid.append(QString("%1").arg((uint8_t)response.at(i),2,16,QLatin1Char('0')).toUpper());
            emit LOG_I("ECU ID: " + ecuid, true, true);
            if (cmd_type == "read")
                ecuCalDef->RomId = ecuid + "_";
        }
        else
        {
            emit LOG_E("Wrong response from ECU: " + FileActions::parse_nrc_message(received.mid(4, received.length()-1)), true, true);
            
        }
    }
    else
    {
        emit LOG_E("No valid response from ECU", true, true);
        
    }

    emit LOG_I("Requesting VIN", true, true);
    output.clear();
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x07);
    output.append((uint8_t)0xE0);
    output.append((uint8_t)0x09);
    output.append((uint8_t)0x02);
    serial->write_serial_data_echo_check(output);
    
    delay(50);
    received = serial->read_serial_data(serial_read_timeout);
    if (received.length() > 5)
    {
        if ((uint8_t)received.at(4) == 0x49 && (uint8_t)received.at(5) == 0x02)
        {
            QByteArray response = received;
            response.remove(0, 7);
            
            emit LOG_I("VIN: " + response, true, true);
        }
        else
        {
            emit LOG_E("Wrong response from ECU: " + FileActions::parse_nrc_message(received.mid(4, received.length()-1)), true, true);
            
        }
    }
    else
    {
        emit LOG_E("No valid response from ECU", true, true);
        
    }

    emit LOG_I("Requesting CAL ID...", true, true);
    output.clear();
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x07);
    output.append((uint8_t)0xE0);
    output.append((uint8_t)0x09);
    output.append((uint8_t)0x04);
    serial->write_serial_data_echo_check(output);
    
    delay(50);
    received = serial->read_serial_data(serial_read_timeout);
    if (received.length() > 5)
    {
        if ((uint8_t)received.at(4) == 0x49 && (uint8_t)received.at(5) == 0x04)
        {
            QByteArray response = received;
            response.remove(0, 7);
            
            emit LOG_I("CAL ID: " + response, true, true);
            if (cmd_type == "read")
                ecuCalDef->RomId.insert(0, QString(response) + "_");
        }
        else
        {
            emit LOG_E("Wrong response from ECU: " + FileActions::parse_nrc_message(received.mid(4, received.length()-1)), true, true);
            
        }
    }
    else
    {
        emit LOG_E("No valid response from ECU", true, true);
        
    }

    emit LOG_I("Requesting CVN", true, true);
    output.clear();
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x07);
    output.append((uint8_t)0xE0);
    output.append((uint8_t)0x09);
    output.append((uint8_t)0x06);
    serial->write_serial_data_echo_check(output);
    
    delay(50);
    received = serial->read_serial_data(serial_read_timeout);
    if (received.length() > 5)
    {
        if ((uint8_t)received.at(4) == 0x49 && (uint8_t)received.at(5) == 0x06)
        {
            QByteArray response = received;
            response.remove(0, 7);
            QString msg;
            msg.clear();
            for (int i = 0; i < response.length(); i++)
                msg.append(QString("%1").arg((uint8_t)response.at(i),2,16,QLatin1Char('0')).toUpper());
            
            emit LOG_I("CVN: " + msg, true, true);
        }
        else
        {
            emit LOG_E("Wrong response from ECU: " + FileActions::parse_nrc_message(received.mid(4, received.length()-1)), true, true);
            
        }
    }
    else
    {
        emit LOG_E("No valid response from ECU", true, true);
        
    }

    emit LOG_I("Initializing bootloader...", true, true);

    //On car session hack
    output.clear();
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x07);
    output.append((uint8_t)0xE0);
    output.append((uint8_t)0xA8);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0xD7);
    serial->write_serial_data_echo_check(output);
    
    delay(50);
    received = serial->read_serial_data(200);
    

    if (received.length() > 5)
    {
        if ((uint8_t)received.at(5) != 0xA0 && (uint8_t)received.at(6) != 0x20)
        {
            emit LOG_I("In Car Programming, Accessing...", true, true);
            delay(800);

            output.clear();
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x07);
            output.append((uint8_t)0xE0);
            output.append((uint8_t)0xA8);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x01);
            output.append((uint8_t)0x3B);
            serial->write_serial_data_echo_check(output);
            
            delay(50);
            received = serial->read_serial_data(200);
            

            output.clear();
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x07);
            output.append((uint8_t)0xDF);
            output.append((uint8_t)0x10);
            output.append((uint8_t)0x03);
            serial->write_serial_data_echo_check(output);
            
            delay(50);
            received = serial->read_serial_data(200);
            

            output.clear();
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x07);
            output.append((uint8_t)0xE1);
            output.append((uint8_t)0x04);
            serial->write_serial_data_echo_check(output);
            
            delay(50);
            received = serial->read_serial_data(200);
            

            output.clear();
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x07);
            output.append((uint8_t)0xB0);
            output.append((uint8_t)0x10);
            output.append((uint8_t)0x03);
            serial->write_serial_data_echo_check(output);
            
            delay(50);
            received = serial->read_serial_data(200);
            

            output.clear();
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x07);
            output.append((uint8_t)0xB0);
            output.append((uint8_t)0x85);
            output.append((uint8_t)0x02);
            serial->write_serial_data_echo_check(output);
            
            delay(50);
            received = serial->read_serial_data(200);
            

            output.clear();
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x07);
            output.append((uint8_t)0xDF);
            output.append((uint8_t)0x85);
            output.append((uint8_t)0x02);
            serial->write_serial_data_echo_check(output);
            
            delay(50);
            received = serial->read_serial_data(200);
            

            output.clear();
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x07);
            output.append((uint8_t)0xB0);
            output.append((uint8_t)0x85);
            output.append((uint8_t)0x02);
            serial->write_serial_data_echo_check(output);
            
            delay(50);
            received = serial->read_serial_data(200);
            

            output.clear();
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x07);
            output.append((uint8_t)0xDF);
            output.append((uint8_t)0x85);
            output.append((uint8_t)0x02);
            serial->write_serial_data_echo_check(output);
            
            delay(50);
            received = serial->read_serial_data(200);
            

            output.clear();
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x07);
            output.append((uint8_t)0xDF);
            output.append((uint8_t)0x28);
            output.append((uint8_t)0x03);
            output.append((uint8_t)0x01);
            serial->write_serial_data_echo_check(output);
            
            delay(50);
            received = serial->read_serial_data(200);
            

            emit LOG_I("Sending seed request...", true, true);
            output.clear();
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x07);
            output.append((uint8_t)0xE0);
            output.append((uint8_t)0x27);
            output.append((uint8_t)0x01);
            serial->write_serial_data_echo_check(output);
            
            delay(200);
            received = serial->read_serial_data(200);
            
            if (received.length() > 5)
            {
                if ((uint8_t)received.at(4) != 0x67 || (uint8_t)received.at(5) != 0x01)
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
            emit LOG_I("Seed request ok", true, true);

            seed.clear();
            seed.append(received.at(6));
            seed.append(received.at(7));
            seed.append(received.at(8));
            seed.append(received.at(9));

            msg = parse_message_to_hex(seed);
            emit LOG_I("Received seed: " + msg, true, true);

            seed_key = generate_seed_key(seed);

            msg = parse_message_to_hex(seed_key);
            emit LOG_I("Calculated seed key: " + msg, true, true);

            emit LOG_I("Sending seed key to ECU", true, true);
            output.clear();
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x07);
            output.append((uint8_t)0xE0);
            output.append((uint8_t)0x27);
            output.append((uint8_t)0x02);
            output.append(seed_key);
            serial->write_serial_data_echo_check(output);
            
            delay(200);
            received = serial->read_serial_data(200);
            
            if (received.length() > 5)
            {
                if ((uint8_t)received.at(4) != 0x67 || (uint8_t)received.at(5) != 0x02)
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
            emit LOG_I("Seed key ok", true, true);

            output.clear();
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x07);
            output.append((uint8_t)0xE0);
            output.append((uint8_t)0xA8);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0xD5);
            serial->write_serial_data_echo_check(output);
            
            delay(50);
            received = serial->read_serial_data(200);
            

            output.clear();
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x07);
            output.append((uint8_t)0xE0);
            output.append((uint8_t)0xA8);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x01);
            output.append((uint8_t)0x3B);
            serial->write_serial_data_echo_check(output);
            
            delay(50);
            received = serial->read_serial_data(200);
            

            output.clear();
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x07);
            output.append((uint8_t)0xE0);
            output.append((uint8_t)0xA8);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x1C);
            serial->write_serial_data_echo_check(output);
            
            delay(50);
            received = serial->read_serial_data(200);
            

            output.clear();
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x07);
            output.append((uint8_t)0xE0);
            output.append((uint8_t)0xA8);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x0E);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x0F);
            serial->write_serial_data_echo_check(output);
            
            delay(50);
            received = serial->read_serial_data(200);
            

            output.clear();
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x07);
            output.append((uint8_t)0xE0);
            output.append((uint8_t)0x10);
            output.append((uint8_t)0x02);
            serial->write_serial_data_echo_check(output);
            
            delay(50);
            received = serial->read_serial_data(200);
            

            emit LOG_I("Checking if jump successful and kernel alive...", true, true);
            output.clear();
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x07);
            output.append((uint8_t)0xE0);
            output.append((uint8_t)0x34);
            output.append((uint8_t)0x04);
            output.append((uint8_t)0x33);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x08);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x00);
            serial->write_serial_data_echo_check(output);
            
            delay(200);
            received = serial->read_serial_data(200);
            
            if (received.length() > 7)
            {
                if ((uint8_t)received.at(4) != 0x74 || (uint8_t)received.at(5) != 0x20 || (uint8_t)received.at(6) != 0x01 || (uint8_t)received.at(7) != 0x04)
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
        }
        else
        {
            emit LOG_I("Bench Programming, Accessing...", true, true);
            delay(800);

            output.clear();
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x07);
            output.append((uint8_t)0xE0);
            output.append((uint8_t)0x10);
            output.append((uint8_t)0x43);
            serial->write_serial_data_echo_check(output);
            
            delay(200);
            received = serial->read_serial_data(200);
            
            if (received.length() > 5)
            {
                if ((uint8_t)received.at(4) != 0x50 || (uint8_t)received.at(5) != 0x43)
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

            emit LOG_I("Starting seed request...", true, true);

            output.clear();
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x07);
            output.append((uint8_t)0xE0);
            output.append((uint8_t)0x27);
            output.append((uint8_t)0x01);
            serial->write_serial_data_echo_check(output);
            
            delay(200);
            received = serial->read_serial_data(200);
            
            if (received.length() > 5)
            {
                if ((uint8_t)received.at(4) != 0x67 || (uint8_t)received.at(5) != 0x01)
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
            emit LOG_I("Seed request ok", true, true);

            seed.clear();
            seed.append(received.at(6));
            seed.append(received.at(7));
            seed.append(received.at(8));
            seed.append(received.at(9));

            msg = parse_message_to_hex(seed);
            emit LOG_I("Received seed: " + msg, true, true);

            seed_key = generate_seed_key(seed);

            msg = parse_message_to_hex(seed_key);
            emit LOG_I("Calculated seed key: " + msg, true, true);

            emit LOG_I("Sending seed key to ECU...", true, true);
            output.clear();
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x07);
            output.append((uint8_t)0xE0);
            output.append((uint8_t)0x27);
            output.append((uint8_t)0x02);
            output.append(seed_key);
            serial->write_serial_data_echo_check(output);
            
            delay(200);
            received = serial->read_serial_data(200);
            
            if (received.length() > 5)
            {
                if ((uint8_t)received.at(4) != 0x67 || (uint8_t)received.at(5) != 0x02)
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
            emit LOG_I("Seed key ok", true, true);

            emit LOG_I("Jumping to onboad kernel...", true, true);
            output.clear();
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x07);
            output.append((uint8_t)0xE0);
            output.append((uint8_t)0x10);
            output.append((uint8_t)0x42);
            serial->write_serial_data_echo_check(output);
            
            delay(200);
            received = serial->read_serial_data(200);
            
            if (received.length() > 5)
            {
                if ((uint8_t)received.at(4) != 0x50 || (uint8_t)received.at(5) != 0x42)
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
            emit LOG_I("Jump to kernel ok", true, true);

            emit LOG_I("Checking if jump successful and kernel alive...", true, true);
            output.clear();
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x07);
            output.append((uint8_t)0xE0);
            output.append((uint8_t)0x34);
            output.append((uint8_t)0x04);
            output.append((uint8_t)0x33);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x08);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x00);
            serial->write_serial_data_echo_check(output);
            
            delay(200);
            received = serial->read_serial_data(200);
            
            if (received.length() > 7)
            {
                if ((uint8_t)received.at(4) != 0x74 || (uint8_t)received.at(5) != 0x20 || (uint8_t)received.at(6) != 0x01 || (uint8_t)received.at(7) != 0x04)
                {
                    emit LOG_I("Kernel not verified to be running!", true, true);
                    return STATUS_ERROR;
                }
            }
            else
            {
                emit LOG_I("Kernel not verified to be running!", true, true);
                return STATUS_ERROR;
            }
        }
        emit LOG_I("ECU inittialised, continue...", true, true);

        kernel_alive = true;
        return STATUS_SUCCESS;
    }
    return STATUS_ERROR;
}

/*
 * Read memory from Subaru TCU Hitachi CAN bootloader
 *
 * @return success
 */
int FlashEcuSubaruHitachiM32rCan::read_mem(uint32_t start_addr, uint32_t length)
{
    QElapsedTimer timer;
    QByteArray output;
    QByteArray received;
    QByteArray msg;
    QByteArray pagedata;
    QByteArray mapdata;
    uint32_t cplen = 0;
    uint32_t timeout = 0;

    uint32_t pagesize = 0x100;

    start_addr = 0x0;
    length = 0x80000;    // hack for testing

    uint32_t skip_start = start_addr & (pagesize - 1); //if unaligned, we'll be receiving this many extra bytes
    uint32_t addr = start_addr - skip_start;
    uint32_t willget = (skip_start + length + pagesize - 1) & ~(pagesize - 1);
    uint32_t len_done = 0;  //total data written to file

    emit LOG_I("Settting dump start & length...", true, true);

    output.clear();
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x07);
    output.append((uint8_t)0xE0);
    output.append((uint8_t)0x35);
    output.append((uint8_t)0x04);
    output.append((uint8_t)0x33);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x08);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    serial->write_serial_data_echo_check(output);
    
    delay(200);
    received = serial->read_serial_data(200);
    
    if (received.length() > 7)
    {
        if ((uint8_t)received.at(4) != 0x75 || (uint8_t)received.at(5) != 0x20 || (uint8_t)received.at(6) != 0x01 || (uint8_t)received.at(7) != 0x01)
        {
            emit LOG_E("Wrong response from ECU: " + FileActions::parse_nrc_message(received.mid(4, received.length()-1)), true, true);
            
        }
    }
    else
    {
        emit LOG_E("No valid response from ECU", true, true);
        
    }

    emit LOG_I("Start reading ROM, please wait...", true, true);

    // send 0xB7 command to kernel to dump from ROM
    output.clear();
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x07);
    output.append((uint8_t)0xE0);
    output.append((uint8_t)0xB7);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);

    set_progressbar_value(0);

    mapdata.clear();

    while (willget)
    {
        if (kill_process)
            return STATUS_ERROR;

        uint32_t numblocks = 1;
        unsigned curspeed = 0, tleft;
        float pleft = 0;
        unsigned long chrono;

        pleft = (float)(addr - start_addr) / (float)length * 100.0f;
        set_progressbar_value(pleft);

        //length = 7FFF0;

        output[5] = (uint8_t)((addr >> 16) & 0xFF);
        output[6] = (uint8_t)((addr >> 8) & 0xFF);
        output[7] = (uint8_t)(addr & 0xFF);
        serial->write_serial_data_echo_check(output);
        
        received = serial->read_serial_data(serial_read_short_timeout);
        
        if (received.length() > 4)
        {
            if ((uint8_t)received.at(4) != 0xF7)
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
        pagedata.clear();
        received.remove(0, 5);
        received = decrypt_payload(received, received.length());
        //emit LOG_D("Decrypted response: " + parse_message_to_hex(received), true, true);
        mapdata.append(received);

        // don't count skipped first bytes //
        cplen = (numblocks * pagesize) - skip_start; //this is the actual # of valid bytes in buf[]
        skip_start = 0;

        chrono = timer.elapsed();
        timer.start();

        if (cplen > 0 && chrono > 0)
            curspeed = cplen * (1000.0f / chrono);

        if (!curspeed) {
            curspeed += 1;
        }

        tleft = (willget / curspeed) % 9999;
        tleft++;

        QString start_address = QString("%1").arg(addr,8,16,QLatin1Char('0')).toUpper();
        QString block_len = QString("%1").arg(pagesize,8,16,QLatin1Char('0')).toUpper();
        msg = QString("Kernel read addr: 0x%1 length: 0x%2, %3 B/s %4 s").arg(start_address).arg(block_len).arg(curspeed, 6, 10, QLatin1Char(' ')).arg(tleft, 6, 10, QLatin1Char(' ')).toUtf8();
        emit LOG_I(msg, true, true);
        //delay(1);

        // and drop extra bytes at the end //
        uint32_t extrabytes = (cplen + len_done);   //hypothetical new length
        if (extrabytes > length) {
            cplen -= (extrabytes - length);
            //thus, (len_done + cplen) will not exceed len
        }

        // increment addr, len, etc //
        len_done += cplen;
        addr += (numblocks * pagesize);
        willget -= (numblocks * pagesize);
    }

    emit LOG_I("ROM read complete", true, true);

    QString mapdata_size = QString("0x%1").arg(mapdata.length(),8,16,QLatin1Char('0')).toUpper();
    emit LOG_I("Mapdata size: " + mapdata_size, true, true);

    emit LOG_I("Sending stop command...", true, true);

    output.clear();
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x07);
    output.append((uint8_t)0xE0);
    output.append((uint8_t)0x37);
    serial->write_serial_data_echo_check(output);
    
    delay(200);
    received = serial->read_serial_data(200);
    
    if (received.length() > 4)
    {
        if ((uint8_t)received.at(4) != 0x77)
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

    //mapdata = decrypt_payload(mapdata, mapdata.length());

    ecuCalDef->FullRomData = mapdata;
    set_progressbar_value(100);

    return STATUS_SUCCESS;
}

/*
 * Write memory to Subaru Hitachi CAN 32bit TCUs, on board kernel
 *
 * @return success
 */

int FlashEcuSubaruHitachiM32rCan::write_mem(bool test_write)
{
    QByteArray filedata;

    filedata = ecuCalDef->FullRomData;

    QScopedArrayPointer<uint8_t> data_array(new uint8_t[filedata.length()]);
    //uint8_t* data_array = new uint8_t[filedata.length()];

    int block_modified[16] = {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1};   // assume blocks after 0x8000 are modified

    unsigned bcnt;  // 13 blocks in M32R_512kB, but kernelmemorymodels.h has 11. Of these 11, the first 3 are not flashed by OBK
    unsigned blockno;

    //encrypt the data
    filedata = encrypt_payload(filedata, filedata.length());

    for (int i = 0; i < filedata.length(); i++)
    {
        data_array[i] = filedata.at(i);
    }

    bcnt = 0;
    emit LOG_I("Blocks to flash: ", true, false);
    for (blockno = 0; blockno < flashdevices[mcu_type_index].numblocks; blockno++) {
        if (block_modified[blockno]) {
            emit LOG_I(QString::number(blockno) + ", ", false, false);
            bcnt += 1;
        }
    }
    emit LOG_I(" (total: " + QString::number(bcnt) + ")", false, true);

    if (bcnt)
    {
        emit LOG_I("--- Erasing ECU flash memory ---", true, true);
        if (erase_memory())
        {
            emit LOG_I("--- Erasing did not complete successfully ---", true, true);
            return STATUS_ERROR;
        }

        flashbytescount = 0;
        flashbytesindex = 0;

        for (blockno = 0; blockno < flashdevices[mcu_type_index].numblocks; blockno++)
        {
            if (block_modified[blockno])
            {
                flashbytescount += flashdevices[mcu_type_index].fblocks[blockno].len;
            }
        }

        emit LOG_I("--- Start writing ROM file to ECU flash memory ---", true, true);
        for (blockno = 0; blockno < flashdevices[mcu_type_index].numblocks; blockno++)  // hack so that only 1 flash loop done for the entire ROM above 0x8000
        {
            if (block_modified[blockno])
            {
                if (reflash_block(&data_array[flashdevices[mcu_type_index].fblocks->start], &flashdevices[mcu_type_index], blockno, test_write))
                {
                    emit LOG_I("Block " + QString::number(blockno) + " reflash failed.", true, true);
                    return STATUS_ERROR;
                }
                else
                {
                    flashbytesindex += flashdevices[mcu_type_index].fblocks[blockno].len;
                    emit LOG_I("Block " + QString::number(blockno) + " reflash complete.", true, true);
                }
            }
        }

    }
    else
    {
        emit LOG_I("*** No blocks require flash! ***", true, true);
    }

    return STATUS_SUCCESS;
}

/*
 * Upload kernel to Subaru Denso CAN (iso15765) 32bit ECUs
 *
 * @return success
 */
int FlashEcuSubaruHitachiM32rCan::reflash_block(const uint8_t *newdata, const struct flashdev_t *fdt, unsigned blockno, bool test_write)
{

    int errval;

    uint32_t start_address, end_addr;
    uint32_t pl_len;
    uint16_t maxblocks;
    uint16_t blockctr;
    uint32_t blockaddr;

    uint32_t pagesize = 0x100;

    QByteArray output;
    QByteArray received;
    QByteArray msg;

    set_progressbar_value(0);

    if (blockno >= fdt->numblocks) {
        emit LOG_D("block " + QString::number(blockno) + " out of range !", true, true);
        return -1;
    }

    start_address = fdt->fblocks[blockno].start;
    pl_len = fdt->fblocks[blockno].len;
    maxblocks = pl_len / 256;
    end_addr = (start_address + (maxblocks * 256)) & 0xFFFFFFFF;
    uint32_t data_len = end_addr - start_address;

    QString start_addr = QString("%1").arg((uint32_t)start_address,8,16,QLatin1Char('0')).toUpper();
    QString length = QString("%1").arg((uint32_t)pl_len,8,16,QLatin1Char('0')).toUpper();
    msg = QString("Flash block addr: 0x" + start_addr + " len: 0x" + length).toUtf8();
    emit LOG_I(msg, true, true);

    emit LOG_I("Setting flash start & length...", true, true);

    output.clear();
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x07);
    output.append((uint8_t)0xE0);
    output.append((uint8_t)0x34);
    output.append((uint8_t)0x04);
    output.append((uint8_t)0x33);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x08);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    serial->write_serial_data_echo_check(output);
    
    delay(200);
    received = serial->read_serial_data(200);
    
    if (received.length() > 4)
    {
        if ((uint8_t)received.at(4) != 0x74)
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

    int data_bytes_sent = 0;
    for (blockctr = 0; blockctr < maxblocks; blockctr++)
    {
        if (kill_process)
            return 0;

        blockaddr = start_address + blockctr * 256;
        output.clear();
        output.append((uint8_t)0x00);
        output.append((uint8_t)0x00);
        output.append((uint8_t)0x07);
        output.append((uint8_t)0xE0);
        output.append((uint8_t)0xB6);
        output.append((uint8_t)(blockaddr >> 16) & 0xFF);
        output.append((uint8_t)(blockaddr >> 8) & 0xFF);
        output.append((uint8_t)blockaddr & 0xFF);
        
        for (int i = 0; i < 256; i++)
        {
            output.append((uint8_t)(newdata[i + blockaddr] & 0xFF));
            data_bytes_sent++;
        }
        data_len -= 256;

        serial->write_serial_data_echo_check(output);
        received = serial->read_serial_data(receive_timeout);
        
        if (received.length() > 4)
        {
            if ((uint8_t)received.at(4) != 0xF6)
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

        float pleft = (float)blockctr / (float)maxblocks * 100;
        set_progressbar_value(pleft);

    }

    set_progressbar_value(100);

    emit LOG_I("Closing out Flashing of this block...", true, true);
    output.clear();
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x07);
    output.append((uint8_t)0xE0);
    output.append((uint8_t)0x37);

    uint8_t try_count = 0;
    bool connected = false;
    while (try_count < 6 && connected == false)
    {
        serial->write_serial_data_echo_check(output);
        
        delay(200);
        received = serial->read_serial_data(200);
        if (received.length() > 4)
        {
            if ((uint8_t)received.at(4) != 0x77)
            {
                emit LOG_I("." + parse_message_to_hex(received), false, false);
            }
            else if ((uint8_t)received.at(4) == 0x77)
            {
                connected = true;
                emit LOG_I("", false, true);
                emit LOG_I("Closed succesfully: " + parse_message_to_hex(received), true, true);
            }
        }
        try_count++;
    }

    delay(100);

    emit LOG_I("Verifying checksum...", true, true);
    output.clear();
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x07);
    output.append((uint8_t)0xE0);
    output.append((uint8_t)0x31);
    output.append((uint8_t)0x01);
    output.append((uint8_t)0x02);
    output.append((uint8_t)0x02);
    output.append((uint8_t)0x01);
    serial->write_serial_data_echo_check(output);
    
    delay(1000);
    received = serial->read_serial_data(500);
    emit LOG_I(QString::number(try_count) + ": 0x31 response: " + parse_message_to_hex(received), true, true);
    if (received.length() != 7)
    {
        emit LOG_E("Wrong response from ECU: " + FileActions::parse_nrc_message(received.mid(4, received.length()-1)), true, true);
        
        emit LOG_E("Checksum not verified", true, true);
        return STATUS_ERROR;
    }
    else
    {
        if ((uint8_t)received.at(4) != 0x7F || (uint8_t)received.at(5) != 0x31 || (uint8_t)received.at(6) != 0x78)
        {
            emit LOG_E("Wrong response from ECU: " + FileActions::parse_nrc_message(received.mid(4, received.length()-1)), true, true);
            
            emit LOG_E("Checksum not verified", true, true);
            return STATUS_ERROR;
        }
        else
        {
            delay(200);
            received = serial->read_serial_data(500);
            //emit LOG_I(QString::number(try_count) + ": 0x31 response: " + parse_message_to_hex(received), true, true);

            if (received.length() > 6)
            {
                if ((uint8_t)received.at(4) != 0x71 || (uint8_t)received.at(5) != 0x01 || (uint8_t)received.at(6) != 0x02)
                {
                    emit LOG_E("ROM checksum error", true, true);
                    
                    return STATUS_ERROR;
                }
            }
            else
            {
                emit LOG_E("Wrong response from ECU: " + FileActions::parse_nrc_message(received.mid(4, received.length()-1)), true, true);
                
                return STATUS_ERROR;
            }
        }
        emit LOG_I("Checksum verified", true, true);
    }

    return STATUS_SUCCESS;
}

/*
 * Erase Subaru Hitachi TCU CAN (iso15765)
 *
 * @return success
 */
int FlashEcuSubaruHitachiM32rCan::erase_memory()
{
    QByteArray output;
    QByteArray received;

    bool connected = false;
    int try_count = 0;

    if (!serial->is_serial_port_open())
    {
        emit LOG_I("ERROR: Serial port is not open.", true, true);
        return STATUS_ERROR;
    }

    emit LOG_I("Erasing ECU ROM...", true, true);

    output.clear();
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x07);
    output.append((uint8_t)0xE0);
    output.append((uint8_t)0x31);
    output.append((uint8_t)0x01);
    output.append((uint8_t)0x02);
    output.append((uint8_t)0x01);
    output.append((uint8_t)0x0f);
    output.append((uint8_t)0xff);
    output.append((uint8_t)0xff);
    output.append((uint8_t)0xff);
    serial->write_serial_data_echo_check(output);
    
    delay(200);
    received = serial->read_serial_data(200);
    
    while (try_count < 20 && connected == false)
    {
        received = serial->read_serial_data(500);
        if (received.length() > 6)
        {
            if ((uint8_t)received.at(4) != 0x71 || (uint8_t)received.at(5) != 0x01 || (uint8_t)received.at(6) != 0x02)
            {
                emit LOG_I(".", false, false);
            }
            else if ((uint8_t)received.at(4) == 0x71 && (uint8_t)received.at(5) == 0x01 && (uint8_t)received.at(6) == 0x02)
            {
                connected = true;
                emit LOG_I("", false, true);
                
            }
        }
        else
        {
            emit LOG_I(".", false, false);
        }
        delay(500);
        try_count++;
    }
    if (!connected)
    {
        emit LOG_E("Flash area erase failed: " + parse_message_to_hex(received), true, true);
        
        return STATUS_ERROR;
    }

    emit LOG_I("Flash erased! Starting flash write, do not power off!", true, true);

    return STATUS_SUCCESS;
}


/*
 * Generate tcu hitachi can seed key from received seed bytes
 *
 * @return seed key (4 bytes)
 */
QByteArray FlashEcuSubaruHitachiM32rCan::generate_seed_key(QByteArray requested_seed)
{
    QByteArray key;

    const uint16_t keytogenerateindex[]={
        0x90A1, 0x2F92, 0xDE3C, 0xCDC0,
        0x1A99, 0x437C, 0xF91B, 0xDB57,
        0x96BA, 0xDE10, 0xFCAF, 0x3F31,
        0xF47F, 0x0BB6, 0x16E9, 0x4645
    };

    const uint8_t indextransformation[]={
        0x5, 0x6, 0x7, 0x1, 0x9, 0xC, 0xD, 0x8,
        0xA, 0xD, 0x2, 0xB, 0xF, 0x4, 0x0, 0x3,
        0xB, 0x4, 0x6, 0x0, 0xF, 0x2, 0xD, 0x9,
        0x5, 0xC, 0x1, 0xA, 0x3, 0xD, 0xE, 0x8
    };

    key = calculate_seed_key(requested_seed, keytogenerateindex, indextransformation);

    return key;
}

/*
 * Calculate subaru tcu hitachi seed key from received seed bytes
 *
 * @return seed key (4 bytes)
 */
QByteArray FlashEcuSubaruHitachiM32rCan::calculate_seed_key(QByteArray requested_seed, const uint16_t *keytogenerateindex, const uint8_t *indextransformation)
{
    QByteArray key;

    uint32_t seed, index;
    uint16_t wordtogenerateindex, wordtobeencrypted, encryptionkey;
    int ki, n;

    seed = (requested_seed.at(0) << 24) & 0xFF000000;
    seed += (requested_seed.at(1) << 16) & 0x00FF0000;
    seed += (requested_seed.at(2) << 8) & 0x0000FF00;
    seed += requested_seed.at(3) & 0x000000FF;
    //seed = reconst_32(seed8);

    for (ki = 15; ki >= 0; ki--) {

        wordtogenerateindex = seed;
        wordtobeencrypted = seed >> 16;
        index = wordtogenerateindex ^ keytogenerateindex[ki];
        index += index << 16;
        encryptionkey = 0;

        for (n = 0; n < 4; n++) {
            encryptionkey += indextransformation[(index >> (n * 4)) & 0x1F] << (n * 4);
        }

        encryptionkey = (encryptionkey >> 3) + (encryptionkey << 13);
        seed = (encryptionkey ^ wordtobeencrypted) + (wordtogenerateindex << 16);
    }

    seed = (seed >> 16) + (seed << 16);

    key.clear();
    key.append((uint8_t)(seed >> 24));
    key.append((uint8_t)(seed >> 16));
    key.append((uint8_t)(seed >> 8));
    key.append((uint8_t)seed);

    //write_32b(seed, key);

    return key;
}
/*
 * Encrypt upload data
 *
 * @return encrypted data
 */

QByteArray FlashEcuSubaruHitachiM32rCan::encrypt_payload(QByteArray buf, uint32_t len)
{
    QByteArray encrypted;

    const uint16_t keytogenerateindex[]={
        //        0xF50E, 0x973C, 0x77F4, 0x14CA
        0x14CA, 0x77F4, 0x973C, 0xF50E
    };

    const uint8_t indextransformation[]={
        0x5, 0x6, 0x7, 0x1, 0x9, 0xC, 0xD, 0x8,
        0xA, 0xD, 0x2, 0xB, 0xF, 0x4, 0x0, 0x3,
        0xB, 0x4, 0x6, 0x0, 0xF, 0x2, 0xD, 0x9,
        0x5, 0xC, 0x1, 0xA, 0x3, 0xD, 0xE, 0x8
    };

    encrypted = calculate_payload(buf, len, keytogenerateindex, indextransformation);

    return encrypted;
}

QByteArray FlashEcuSubaruHitachiM32rCan::decrypt_payload(QByteArray buf, uint32_t len)
{
    QByteArray decrypt;

    const uint16_t keytogenerateindex[]={
        //        0x14CA, 0x77F4, 0x973C, 0xF50E
        0xF50E, 0x973C, 0x77F4, 0x14CA

    };

    const uint8_t indextransformation[]={
        0x5, 0x6, 0x7, 0x1, 0x9, 0xC, 0xD, 0x8,
        0xA, 0xD, 0x2, 0xB, 0xF, 0x4, 0x0, 0x3,
        0xB, 0x4, 0x6, 0x0, 0xF, 0x2, 0xD, 0x9,
        0x5, 0xC, 0x1, 0xA, 0x3, 0xD, 0xE, 0x8
    };

    decrypt = calculate_payload(buf, len, keytogenerateindex, indextransformation);

    return decrypt;
}

QByteArray FlashEcuSubaruHitachiM32rCan::calculate_payload(QByteArray buf, uint32_t len, const uint16_t *keytogenerateindex, const uint8_t *indextransformation)
{
    QByteArray encrypted;
    uint32_t datatoencrypt32, index;
    uint16_t wordtogenerateindex, wordtobeencrypted, encryptionkey;
    int ki, n;

    if (!buf.length() || !len) {
        return NULL;
    }

    encrypted.clear();

    len &= ~3;
    for (uint32_t i = 0; i < len; i += 4) {
        datatoencrypt32 = ((buf.at(i) << 24) & 0xFF000000) | ((buf.at(i + 1) << 16) & 0xFF0000) | ((buf.at(i + 2) << 8) & 0xFF00) | (buf.at(i + 3) & 0xFF);

        for (ki = 0; ki < 4; ki++) {

            wordtogenerateindex = datatoencrypt32;
            wordtobeencrypted = datatoencrypt32 >> 16;
            index = wordtogenerateindex ^ keytogenerateindex[ki];
            index += index << 16;
            encryptionkey = 0;

            for (n = 0; n < 4; n++) {
                encryptionkey += indextransformation[(index >> (n * 4)) & 0x1F] << (n * 4);
            }

            encryptionkey = (encryptionkey >> 3) + (encryptionkey << 13);
            datatoencrypt32 = (encryptionkey ^ wordtobeencrypted) + (wordtogenerateindex << 16);
        }

        datatoencrypt32 = (datatoencrypt32 >> 16) + (datatoencrypt32 << 16);

        encrypted.append((datatoencrypt32 >> 24) & 0xFF);
        encrypted.append((datatoencrypt32 >> 16) & 0xFF);
        encrypted.append((datatoencrypt32 >> 8) & 0xFF);
        encrypted.append(datatoencrypt32 & 0xFF);
        //encrypted.append(sub_encrypt(tempbuf));
    }
    return encrypted;
}

/*
 * Parse QByteArray to readable form
 *
 * @return parsed message
 */
QString FlashEcuSubaruHitachiM32rCan::parse_message_to_hex(QByteArray received)
{
    QString msg;

    for (int i = 0; i < received.length(); i++)
    {
        msg.append(QString("%1 ").arg((uint8_t)received.at(i),2,16,QLatin1Char('0')).toUtf8());
    }

    return msg;
}

void FlashEcuSubaruHitachiM32rCan::set_progressbar_value(int value)
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

void FlashEcuSubaruHitachiM32rCan::delay(int timeout)
{
    QTime dieTime = QTime::currentTime().addMSecs(timeout);
    while (QTime::currentTime() < dieTime)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 1);
}
