#include "flash_tcu_subaru_denso_sh705x_can.h"

FlashTcuSubaruDensoSH705xCan::FlashTcuSubaruDensoSH705xCan(SerialPortActions *serial, FileActions::EcuCalDefStructure *ecuCalDef, QString cmd_type, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::EcuOperationsWindow)
    , ecuCalDef(ecuCalDef)
    , cmd_type(cmd_type)
{
    // TCU 0xFFFF3000 0x2000

    ui->setupUi(this);

    if (cmd_type == "test_write")
        this->setWindowTitle("Test write ROM " + ecuCalDef->FileName + " to TCU");
    else if (cmd_type == "write")
        this->setWindowTitle("Write ROM " + ecuCalDef->FileName + " to TCU");
    else if (cmd_type == "read")
        this->setWindowTitle("Read ROM from TCU");

    this->serial = serial;
}

void FlashTcuSubaruDensoSH705xCan::run()
{
    this->show();

    int result = STATUS_ERROR;
    set_progressbar_value(0);
    int tcuAction = 1;
    bool ok = false;

    //result = init_flash_denso_subarucan();

    mcu_type_string = ecuCalDef->McuType;
    mcu_type_index = 0;

    while (flashdevices[mcu_type_index].name != 0)
    {
        if (flashdevices[mcu_type_index].name == mcu_type_string)
            break;
        mcu_type_index++;
    }
    QString mcu_name = flashdevices[mcu_type_index].name;
    emit LOG_D("MCU type: " + mcu_name + " " + mcu_type_string + " and index: " + mcu_type_index, true, true);

    kernel = ecuCalDef->Kernel;
    flash_method = ecuCalDef->FlashMethod;

    emit external_logger("Starting");

    if (cmd_type == "read")
    {
        QMessageBox msgBox;
        msgBox.setText("Choose which option");
        msgBox.setInformativeText("Perform TCU ROM Dump, Relearn, Read Parmeter or Set Parameter?");
        QPushButton* pButtonDump = msgBox.addButton("Dump", QMessageBox::YesRole);
        QPushButton* pButtonRelearn = msgBox.addButton("Relearn", QMessageBox::YesRole);
        QPushButton* pButtonReadParam = msgBox.addButton("Read Param", QMessageBox::YesRole);
        QPushButton* pButtonSetParam = msgBox.addButton("Set Param", QMessageBox::YesRole);

        msgBox.exec();

        if (msgBox.clickedButton() == (QAbstractButton *)pButtonDump)
        {
            emit LOG_I("Read memory with flashmethod '" + flash_method + "' and kernel '" + ecuCalDef->Kernel + "'", true, true);
            tcuAction = 1;
        }
        else if (msgBox.clickedButton() == (QAbstractButton *)pButtonRelearn)
        {
            emit LOG_I("Attempting TCU relearn", true, true);
            tcuAction = 2;
        }
        else if (msgBox.clickedButton() == (QAbstractButton *)pButtonReadParam)
        {
            emit LOG_I("Attempting to read TCU parameters", true, true);
            tcuAction = 3;
        }
        else if (msgBox.clickedButton() == (QAbstractButton *)pButtonSetParam)
        {
            emit LOG_I("Attempting to set TCU parameters", true, true);
            tcuAction = 4;
        }
        else
            emit LOG_I("No option selected", true, true);


        /*
        QMessageBox msgBox;
        msgBox.setText("Choose which option");
        msgBox.setInformativeText("Proceed with TCU Read (Yes) or TCU Relearn (No)?");
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        tcuRead = msgBox.exec();

        if (tcuRead == QMessageBox::Yes) emit LOG_I("Read memory with flashmethod '" + flash_method + "' and kernel '" + ecuCalDef->Kernel + "'", true, true);
        else emit LOG_I("Attempting TCU relearn", true, true);
        //qDebug() << "Read memory with flashmethod" << flash_method << "and kernel" << ecuCalDef->Kernel;
        */
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

    // Set serial port
    serial->set_is_iso14230_connection(false);
    serial->set_is_can_connection(false);
    serial->set_is_iso15765_connection(true);
    serial->set_is_29_bit_id(false);
    serial->set_can_speed("500000");
    serial->set_iso15765_source_address(0x7E1);
    serial->set_iso15765_destination_address(0x7E9);
    // Open serial port
    serial->open_serial_port();

    int ret = QMessageBox::warning(this, tr("Connecting to TCU"),
                                   tr("Turn ignition ON and press OK to start initializing connection to TCU"),
                                   QMessageBox::Ok | QMessageBox::Cancel,
                                   QMessageBox::Ok);

    switch (ret)
    {
        case QMessageBox::Ok:
            if (tcuAction == 1)
            {
                emit LOG_I("Connecting to Subaru Denso CAN bootloader, please wait...", true, true);
                result = connect_bootloader();

                if (result == STATUS_SUCCESS && !kernel_alive)
                {
                    emit external_logger("Preparing, please wait...");
                    emit LOG_I("Initializing Subaru Denso CAN kernel upload, please wait...", true, true);
                    result = upload_kernel(kernel, ecuCalDef->KernelStartAddr.toUInt(&ok, 16));
                }
                if (result == STATUS_SUCCESS)
                {
                    if (cmd_type == "read")
                    {
                        emit external_logger("Reading ROM, please wait...");
                        emit LOG_I("Reading ROM from Subaru Denso using CAN", true, true);
                        result = read_mem(flashdevices[mcu_type_index].fblocks[0].start, flashdevices[mcu_type_index].romsize);
                    }
                    else if (cmd_type == "test_write" || cmd_type == "write")
                    {
                        emit external_logger("Writing ROM, please wait...");
                        emit LOG_I("Writing ROM to Subaru Denso using CAN (beta status)", true, true);
                        result = write_mem(test_write);
                    }
                }
            }
            else if (tcuAction == 2)
            {
                emit LOG_I("Commencing TCU relearn process, please wait...", true, true);
                result = tcu_relearn_subaru_ssm();

            }
            else if (tcuAction == 3)
            {
                emit LOG_I("Commencing to read TCU parameters, please wait...", true, true);
                result = tcu_readparam_subaru_ssm();
            }
            else if (tcuAction == 4)
            {
                emit LOG_I("Commencing to set TCU parameters, please wait...", true, true);
                result = tcu_setparam_subaru_ssm();
            }

            emit external_logger("Finished");

            if (result == STATUS_SUCCESS)
            {
                QMessageBox::information(this, tr("TCU Operation"), "TCU operation was succesful, press OK to exit");
                this->close();
            }
            else
            {
                QMessageBox::warning(this, tr("TCU Operation"), "TCU operation failed, press OK to exit and try again");
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

FlashTcuSubaruDensoSH705xCan::~FlashTcuSubaruDensoSH705xCan()
{
    delete ui;
}

void FlashTcuSubaruDensoSH705xCan::closeEvent(QCloseEvent *event)
{
    kill_process = true;
}

/*
 * Set SSM params for Denso CAN (iso15765) 32bit ECUs
 *
 * @return success
 */
int FlashTcuSubaruDensoSH705xCan::tcu_setparam_subaru_ssm()
{
    QByteArray output;
    QByteArray received;
    QByteArray msg;
    bool responseOK;
    int try_count;

    // CAN 0xb8 command is disabled, so switch to K-Line comms

    emit LOG_I("Changing to K-Line comms...", true, true);
    serial->reset_connection();
    serial->set_is_can_connection(false);
    serial->set_is_iso15765_connection(false);
    serial->set_is_iso14230_connection(true);
    serial->open_serial_port();
    serial->change_port_speed("4800");
    serial->set_add_iso14230_header(false);
    tester_id = 0xF0;
    target_id = 0x18;

    if (!serial->is_serial_port_open())
    {
        emit LOG_I("ERROR: Serial port is not open.", true, true);
        return STATUS_ERROR;
    }
    emit LOG_I("Change to K-Line comms successful...", true, true);

    bool ok;
    int correction_1to2 = QInputDialog::getInt(this, "Enter SSM Parameter Data", "1->2 Pressure Correction (DC):", 0, 0, 255, 1, &ok);
    if (!ok) return STATUS_ERROR;
    int correction_2to3 = QInputDialog::getInt(this, "Enter SSM Parameter Data", "2->3 Pressure Correction (HLRC):", 0, 0, 255, 1, &ok);
    if (!ok) return STATUS_ERROR;
    int correction_3to4 = QInputDialog::getInt(this, "Enter SSM Parameter Data", "3->4 Pressure Correction (IC):", 0, 0, 255, 1, &ok);
    if (!ok) return STATUS_ERROR;
    int correction_4to5 = QInputDialog::getInt(this, "Enter SSM Parameter Data", "4->5 Pressure Correction (FB):", 0, 0, 255, 1, &ok);
    if (!ok) return STATUS_ERROR;
    int correction_fwdb = QInputDialog::getInt(this, "Enter SSM Parameter Data", "Fwd Brake Pressure Correction:", 0, 0, 255, 1, &ok);
    if (!ok) return STATUS_ERROR;
    int correction_4wd = QInputDialog::getInt(this, "Enter SSM Parameter Data", "4WD Pressure Correction:", 0, 0, 255, 1, &ok);
    if (!ok) return STATUS_ERROR;
    int correction_pl = QInputDialog::getInt(this, "Enter SSM Parameter Data", "Line Pressure Correction:", 0, 0, 255, 1, &ok);
    if (!ok) return STATUS_ERROR;
    int temp_basis = QInputDialog::getInt(this, "Enter SSM Parameter Data", "Temp Basis for Corrections:", 0, 0, 255, 1, &ok);
    if (!ok) return STATUS_ERROR;
    int torque_correction_awd = QInputDialog::getInt(this, "Enter SSM Parameter Data", "AWD Torque Correction:", 0, 0, 65535, 1, &ok);
    if (!ok) return STATUS_ERROR;

    emit LOG_I("Setting TCU parameters...", true, true);

    output.append((uint8_t)0xB8);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x01);
    output.append((uint8_t)0x6c);  // 0x16c - IC Correction, 3->4
    output.append((uint8_t)(correction_3to4 & 0xff));
    serial->write_serial_data_echo_check(add_ssm_header(output, tester_id, target_id, false));
    emit LOG_D("Sent: " + parse_message_to_hex(add_ssm_header(output, tester_id, target_id, false)), true, true);
    received = serial->read_serial_data(8, receive_timeout);
    emit LOG_D("Response:" + parse_message_to_hex(received), true, true);
    if ((uint8_t)received[4] != 0xF8)
        return STATUS_ERROR;

    output[3] = ((uint8_t)0x6d);  // 0x16d - HLRC Correction, 2->3
    output[4] = ((uint8_t)(correction_2to3 & 0xff));
    serial->write_serial_data_echo_check(add_ssm_header(output, tester_id, target_id, false));
    emit LOG_D("Sent: " + parse_message_to_hex(add_ssm_header(output, tester_id, target_id, false)), true, true);
    received = serial->read_serial_data(8, receive_timeout);
    emit LOG_D("Response:" + parse_message_to_hex(received), true, true);
    if ((uint8_t)received[4] != 0xF8)
        return STATUS_ERROR;

    output[3] = ((uint8_t)0x6e);  // 0x16e - DC Correction, 1->2
    output[4] = ((uint8_t)(correction_1to2 & 0xff));
    serial->write_serial_data_echo_check(add_ssm_header(output, tester_id, target_id, false));
    emit LOG_D("Sent: " + parse_message_to_hex(add_ssm_header(output, tester_id, target_id, false)), true, true);
    received = serial->read_serial_data(8, receive_timeout);
    emit LOG_D("Response:" + parse_message_to_hex(received), true, true);
    if ((uint8_t)received[4] != 0xF8)
        return STATUS_ERROR;

    output[3] = ((uint8_t)0x6f);  // 0x16f - FB correction
    output[4] = ((uint8_t)(correction_4to5 & 0xff));
    serial->write_serial_data_echo_check(add_ssm_header(output, tester_id, target_id, false));
    emit LOG_D("Sent: " + parse_message_to_hex(add_ssm_header(output, tester_id, target_id, false)), true, true);
    received = serial->read_serial_data(8, receive_timeout);
    emit LOG_D("Response:" + parse_message_to_hex(received), true, true);
    if ((uint8_t)received[4] != 0xF8)
        return STATUS_ERROR;

    output[3] = ((uint8_t)0x70);  // 0x170 - AWD Clutch Torque High
    output[4] = ((uint8_t)((torque_correction_awd >> 8) & 0xff));
    serial->write_serial_data_echo_check(add_ssm_header(output, tester_id, target_id, false));
    emit LOG_D("Sent: " + parse_message_to_hex(add_ssm_header(output, tester_id, target_id, false)), true, true);
    received = serial->read_serial_data(8, receive_timeout);
    emit LOG_D("Response:" + parse_message_to_hex(received), true, true);
    if ((uint8_t)received[4] != 0xF8)
        return STATUS_ERROR;

    output[3] = ((uint8_t)0x71);  // 0x171 - AWD Clutch Torque Low
    output[4] = ((uint8_t)(torque_correction_awd & 0xff));
    serial->write_serial_data_echo_check(add_ssm_header(output, tester_id, target_id, false));
    emit LOG_D("Sent: " + parse_message_to_hex(add_ssm_header(output, tester_id, target_id, false)), true, true);
    received = serial->read_serial_data(8, receive_timeout);
    emit LOG_D("Response:" + parse_message_to_hex(received), true, true);
    if ((uint8_t)received[4] != 0xF8)
        return STATUS_ERROR;

    output[3] = ((uint8_t)0xbc);  // 0x1bc - FwdB correction
    output[4] = ((uint8_t)(correction_fwdb & 0xff));
    serial->write_serial_data_echo_check(add_ssm_header(output, tester_id, target_id, false));
    emit LOG_D("Sent: " + parse_message_to_hex(add_ssm_header(output, tester_id, target_id, false)), true, true);
    received = serial->read_serial_data(8, receive_timeout);
    emit LOG_D("Response:" + parse_message_to_hex(received), true, true);
    if ((uint8_t)received[4] != 0xF8)
        return STATUS_ERROR;

    output[3] = ((uint8_t)0xbd);  // 0x1bd - 4WD correction
    output[4] = ((uint8_t)(correction_4wd & 0xff));
    serial->write_serial_data_echo_check(add_ssm_header(output, tester_id, target_id, false));
    emit LOG_D("Sent: " + parse_message_to_hex(add_ssm_header(output, tester_id, target_id, false)), true, true);
    received = serial->read_serial_data(8, receive_timeout);
    emit LOG_D("Response:" + parse_message_to_hex(received), true, true);
    if ((uint8_t)received[4] != 0xF8)
        return STATUS_ERROR;

    output[3] = ((uint8_t)0xbe);  // 0x1be - PL correction
    output[4] = ((uint8_t)(correction_pl & 0xff));
    serial->write_serial_data_echo_check(add_ssm_header(output, tester_id, target_id, false));
    emit LOG_D("Sent: " + parse_message_to_hex(add_ssm_header(output, tester_id, target_id, false)), true, true);
    received = serial->read_serial_data(8, receive_timeout);
    emit LOG_D("Response:" + parse_message_to_hex(received), true, true);
    if ((uint8_t)received[4] != 0xF8)
        return STATUS_ERROR;

    output[3] = ((uint8_t)0xbf);  // 0x1bf - Temp basis
    output[4] = ((uint8_t)(temp_basis & 0xff));
    serial->write_serial_data_echo_check(add_ssm_header(output, tester_id, target_id, false));
    emit LOG_D("Sent: " + parse_message_to_hex(add_ssm_header(output, tester_id, target_id, false)), true, true);
    received = serial->read_serial_data(8, receive_timeout);
    emit LOG_D("Response:" + parse_message_to_hex(received), true, true);
    if ((uint8_t)received[4] != 0xF8)
        return STATUS_ERROR;

    emit LOG_I("Saving to TCU...", true, true);
    output[2] = ((uint8_t)0x00);
    output[3] = ((uint8_t)0xec);  // 0xec
    output[4] = ((uint8_t)0x55);
    serial->write_serial_data_echo_check(add_ssm_header(output, tester_id, target_id, false));
    emit LOG_D("Sent: " + parse_message_to_hex(add_ssm_header(output, tester_id, target_id, false)), true, true);
    received = serial->read_serial_data(8, receive_timeout);
    emit LOG_D("Response:" + parse_message_to_hex(received), true, true);
    if ((uint8_t)received[4] != 0xF8)
        return STATUS_ERROR;

    output[4] = ((uint8_t)0xaa);
    serial->write_serial_data_echo_check(add_ssm_header(output, tester_id, target_id, false));
    emit LOG_D("Sent: " + parse_message_to_hex(add_ssm_header(output, tester_id, target_id, false)), true, true);
    received = serial->read_serial_data(8, receive_timeout);
    emit LOG_D("Response:" + parse_message_to_hex(received), true, true);
    if ((uint8_t)received[4] != 0xF8)
        return STATUS_ERROR;

    emit LOG_I("Changing back to CAN comms...", true, true);
    // Set serial port
    serial->reset_connection();
    serial->set_is_iso14230_connection(false);
    serial->set_is_can_connection(false);
    serial->set_is_iso15765_connection(true);
    serial->set_is_29_bit_id(false);
    serial->set_can_speed("500000");
    serial->set_iso15765_source_address(0x7E1);
    serial->set_iso15765_destination_address(0x7E9);
    // Open serial port
    serial->open_serial_port();

    return STATUS_SUCCESS;

}

/*
 * Read SSM params from Subaru Denso CAN (iso15765) 32bit ECUs
 *
 * @return success
 */
int FlashTcuSubaruDensoSH705xCan::tcu_readparam_subaru_ssm()
{
    QByteArray output;
    QByteArray received;
    QByteArray msg;
    bool responseOK;
    int try_count;

    if (!serial->is_serial_port_open())
    {
        emit LOG_I("ERROR: Serial port is not open.", true, true);
        return STATUS_ERROR;
    }

    serial->set_add_iso14230_header(false);

    emit LOG_I("Reading TCU parameters...", true, true);
    output.clear();
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x07);
    output.append((uint8_t)0xE1);
    output.append((uint8_t)0xA8);
    output.append((uint8_t)0x00);  // one time only
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x01);
    output.append((uint8_t)0x6c);  // 0x16c
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x01);
    output.append((uint8_t)0x6d);  // 0x16d
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x01);
    output.append((uint8_t)0x6e);  // 0x16e
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x01);
    output.append((uint8_t)0x6f);  // 0x16f
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x01);
    output.append((uint8_t)0x70);  // 0x170
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x01);
    output.append((uint8_t)0x71);  // 0x171
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x01);
    output.append((uint8_t)0xbc);  // 0x1bc
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x01);
    output.append((uint8_t)0xbd);  // 0x1bd
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x01);
    output.append((uint8_t)0xbe);  // 0x1be
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x01);
    output.append((uint8_t)0xbf);  // 0x1bf

    try_count = 0;
    responseOK = false;
    while (try_count < 6 && responseOK == false)
    {
        serial->write_serial_data_echo_check(output);
        emit LOG_D("Sent: " + parse_message_to_hex(output), true, true);
        //delay(50);
        received = serial->read_serial_data(20, 200);
        if (received != "")
        {
            responseOK = true;
            emit LOG_I(QString::number(try_count) + ": 0xA8 response: " + parse_message_to_hex(received), true, true);
        }
        try_count++;
        //delay(try_timeout);
    }

    if (!responseOK)
        return STATUS_ERROR;
    if ((uint8_t)received[4] != 0xE8)
        return STATUS_ERROR;

    uint16_t torque_correction_awd = ((uint8_t)received[9]) << 8 + (uint8_t)received[10];

    emit LOG_I("Input Clutch Pressure Correction (raw byte): " + QString::number((uint8_t)received[5]), true, true);
    emit LOG_I("High Low Reverse Clutch Pressure Correction (raw byte): " + QString::number((uint8_t)received[6]), true, true);
    emit LOG_I("Direct Clutch Pressure Correction (raw byte): " + QString::number((uint8_t)received[7]), true, true);
    emit LOG_I("Front Brake Pressure Correction (raw byte): " + QString::number((uint8_t)received[8]), true, true);
    emit LOG_I("Correction of AWD Clutch Torque (raw word): " + QString::number((uint16_t)torque_correction_awd), true, true);
    emit LOG_I("Forward Brake Pressure Correction (raw byte): " + QString::number((uint8_t)received[11]), true, true);
    emit LOG_I("4WD Pressure Correction (raw byte): " + QString::number((uint8_t)received[12]), true, true);
    emit LOG_I("Line Pressure Correction (raw byte): " + QString::number((uint8_t)received[13]), true, true);
    emit LOG_I("Temperature basis for above Pressure Corrections (raw byte): " + QString::number((uint8_t)received[14]), true, true);

    return STATUS_SUCCESS;
}
/*
 * TCU relearn for Subaru Denso CAN (iso15765) bootloader 32bit ECUs
 *
 * @return success
 */
int FlashTcuSubaruDensoSH705xCan::tcu_relearn_subaru_ssm()
{
    QByteArray output;
    QByteArray received;
    QByteArray msg;
    bool responseOK;
    int try_count;

    if (!serial->is_serial_port_open())
    {
        emit LOG_I("ERROR: Serial port is not open.", true, true);
        return STATUS_ERROR;
    }

    serial->set_add_iso14230_header(false);

    QMessageBox::information(this, tr("TCU Relearn"), "Engine must be at operating temperature. Car must be off the ground! Start with Engine off, Ignition on, stick in P, press OK to continue");

    emit LOG_I("Initialising TCU relearn, step 1...", true, true);
    output.clear();
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x07);
    output.append((uint8_t)0xE1);
    output.append((uint8_t)0xB8);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x01);
    output.append((uint8_t)0xFC);
    output.append((uint8_t)0x01);
    try_count = 0;
    responseOK = false;
    while (try_count < 6 && responseOK == false)
    {
        serial->write_serial_data_echo_check(output);
        emit LOG_D("Sent: " + parse_message_to_hex(output), true, true);
        //delay(50);
        received = serial->read_serial_data(20, 200);
        if ((received != "") && ((uint8_t)received[4] == 0xF8))
        {
            responseOK = true;
            emit LOG_I(QString::number(try_count) + ": 0xB8 response: " + parse_message_to_hex(received), true, true);
        }
        try_count++;
        //delay(try_timeout);
    }
    //if (!responseOK)
    //    return STATUS_ERROR;
    //if ((uint8_t)received[4] != 0xF8)
    //    return STATUS_ERROR;

    emit LOG_I("Initialising TCU relearn, step 2......", true, true);
    output[7] = ((uint8_t)0xFD);
    output[8] = ((uint8_t)0x09);
    try_count = 0;
    responseOK = false;
    while (try_count < 6 && responseOK == false)
    {
        serial->write_serial_data_echo_check(output);
        emit LOG_D("Sent: " + parse_message_to_hex(output), true, true);
        //delay(50);
        received = serial->read_serial_data(20, 200);
        if (received != "")
        {
            responseOK = true;
            emit LOG_I(QString::number(try_count) + ": 0xB8 response: " + parse_message_to_hex(received), true, true);
        }
        try_count++;
        //delay(try_timeout);
    }
    //if (!responseOK)
    //    return STATUS_ERROR;
    //if ((uint8_t)received[4] != 0xF8)
    //    return STATUS_ERROR;

    QMessageBox::information(this, tr("TCU Relearn"), "Start Engine, let revs settle, move stick into D, fully press brake, press OK to continue");

    emit LOG_I("Tracking relearn status......", true, true);
    output[4] = ((uint8_t)0xA8);
    output[5] = ((uint8_t)0x00);
    output[6] = ((uint8_t)0x00);
    output[7] = ((uint8_t)0x01);
    output[8] = ((uint8_t)0xFC);
    output[9] = ((uint8_t)0x00);
    output[10] = ((uint8_t)0x01);
    output[11] = ((uint8_t)0xFD);
    try_count = 0;
    responseOK = false;
    while (try_count < 200 && responseOK == false)
    {
        serial->write_serial_data_echo_check(output);
        emit LOG_D("Sent: " + parse_message_to_hex(output), true, true);
        //delay(50);
        received = serial->read_serial_data(20, 200);
        if (received != "")
        {
            //responseOK = true;
            emit LOG_I(QString::number(try_count) + ": 0xA8 response: " + parse_message_to_hex(received), true, true);
        }
        try_count++;
        //delay(try_timeout);
    }
    //if (!responseOK)
    //    return STATUS_ERROR;
    //if ((uint8_t)received[4] != 0xE8)
    //    return STATUS_ERROR;

    return STATUS_ERROR;
}

/*
 * Connect to Subaru Denso CAN (iso15765) bootloader 32bit ECUs
 *
 * @return success
 */
int FlashTcuSubaruDensoSH705xCan::connect_bootloader()
{
    QByteArray output;
    QByteArray received;
    QByteArray seed;
    QByteArray seed_key;
    QByteArray msg;

    if (!serial->is_serial_port_open())
    {
        emit LOG_I("ERROR: Serial port is not open.", true, true);
        return STATUS_ERROR;
    }

    serial->set_add_iso14230_header(false);

    //if (connect_bootloader_start_countdown(bootloader_start_countdown))
    //    return STATUS_ERROR;

    emit LOG_I("Checking if kernel is already running...", true, true);

    // Check if kernel already alive
    output.clear();
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x07);
    output.append((uint8_t)0xE1);
    output.append((uint8_t)(SUB_DENSOCAN_START_COMM & 0xFF));
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    serial->write_serial_data_echo_check(output);
    delay(200);
    received = serial->read_serial_data(20, 10);
    if ((uint8_t)received.at(0) == 0x7F && (uint8_t)received.at(2) == 0x34)
    {
        emit LOG_I("Kernel already running", true, true);

        kernel_alive = true;
        return STATUS_SUCCESS;
    }

    emit LOG_I("Requesting ECU ID", true, true);
    output.clear();
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x07);
    output.append((uint8_t)0xE1);
    output.append((uint8_t)0xAA);

    serial->write_serial_data_echo_check(output);
    emit LOG_D("Sent: " + parse_message_to_hex(output), true, true);
    delay(50);
    received = serial->read_serial_data(20, serial_read_timeout);
    if (received.length() > 5)
    {
        if ((uint8_t)received.at(4) == 0xEA)
        {
            QByteArray response = received;
            response.remove(0, 8);
            response.remove(5, response.length()-5);
            emit LOG_D("Response: " + parse_message_to_hex(received), true, true);
            QString ecuid;
            for (int i = 0; i < 5; i++)
                ecuid.append(QString("%1").arg((uint8_t)response.at(i),2,16,QLatin1Char('0')).toUpper());
            emit LOG_I("ECU ID: " + ecuid, true, true);
            if (cmd_type == "read")
                ecuCalDef->RomId = ecuid + "_";
        }
        else
        {
            emit LOG_E("Wrong response from TCU: " + fileActions.parse_nrc_message(received.remove(0, 4)), true, true);
            emit LOG_D("Response: " + parse_message_to_hex(received), true, true);
        }
    }
    else
    {
        emit LOG_E("No valid response from ECU", true, true);
        emit LOG_D("Response: " + parse_message_to_hex(received), true, true);
    }

    emit LOG_I("Requesting CAL ID", true, true);
    output.clear();
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x07);
    output.append((uint8_t)0xE1);
    output.append((uint8_t)0x09);
    output.append((uint8_t)0x04);
    serial->write_serial_data_echo_check(output);
    emit LOG_D("Sent: " + parse_message_to_hex(output), true, true);
    delay(50);
    received = serial->read_serial_data(20, serial_read_timeout);
    if (received.length() > 5)
    {
        if ((uint8_t)received.at(4) == 0x49 && (uint8_t)received.at(5) == 0x04)
        {
            QByteArray response = received;
            response.remove(0, 7);
            emit LOG_D("Response: " + parse_message_to_hex(received), true, true);
            emit LOG_I("CAL ID: " + response, true, true);
            if (cmd_type == "read")
                ecuCalDef->RomId.insert(0, QString(response) + "_");
        }
        else
        {
            emit LOG_E("Wrong response from TCU: " + fileActions.parse_nrc_message(received.remove(0, 4)), true, true);
            emit LOG_D("Response: " + parse_message_to_hex(received), true, true);
        }
    }
    else
    {
        emit LOG_E("No valid response from ECU", true, true);
        emit LOG_D("Response: " + parse_message_to_hex(received), true, true);
    }

    emit LOG_I("Requesting session mode", true, true);
    output.clear();
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x07);
    output.append((uint8_t)0xE1);
    output.append((uint8_t)0x10);
    output.append((uint8_t)0x03);
    serial->write_serial_data_echo_check(output);
    emit LOG_D("Sent: " + parse_message_to_hex(output), true, true);
    delay(50);
    received = serial->read_serial_data(20, serial_read_timeout);
    if (received.length() > 5)
    {
        if ((uint8_t)received.at(4) != 0x50 && (uint8_t)received.at(5) != 0x03)
        {
            emit LOG_E("Wrong response from TCU: " + fileActions.parse_nrc_message(received.remove(0, 4)), true, true);
            emit LOG_D("Response: " + parse_message_to_hex(received), true, true);
            return STATUS_ERROR;
        }
    }
    else
    {
        emit LOG_E("No valid response from ECU", true, true);
        emit LOG_D("Response: " + parse_message_to_hex(received), true, true);
        return STATUS_ERROR;
    }

    output.clear();
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x07);
    output.append((uint8_t)0xE1);
    output.append((uint8_t)0x27);
    output.append((uint8_t)0x01);

    serial->write_serial_data_echo_check(output);
    emit LOG_D("Sent: " + parse_message_to_hex(output), true, true);
    delay(50);
    received = serial->read_serial_data(20, serial_read_timeout);
    if (received.length() > 5)
    {
        if ((uint8_t)received.at(4) != 0x67 || (uint8_t)received.at(5) != 0x01)
        {
            emit LOG_E("Wrong response from TCU: " + fileActions.parse_nrc_message(received.remove(0, 4)), true, true);
            emit LOG_D("Response: " + parse_message_to_hex(received), true, true);
            return STATUS_ERROR;
        }
    }
    else
    {
        emit LOG_E("No valid response from ECU", true, true);
        emit LOG_D("Response: " + parse_message_to_hex(received), true, true);
        return STATUS_ERROR;
    }

    emit LOG_I("Seed request ok", true, true);
    emit LOG_D("Response: " + parse_message_to_hex(received), true, true);

    seed.clear();
    seed.append(received.at(6));
    seed.append(received.at(7));
    seed.append(received.at(8));
    seed.append(received.at(9));

    seed_key = generate_seed_key(seed);

    emit LOG_I("Sending seed key", true, true);
    output.clear();
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x07);
    output.append((uint8_t)0xE1);
    output.append((uint8_t)0x27);
    output.append((uint8_t)0x02);
    output.append(seed_key);

    serial->write_serial_data_echo_check(output);
    emit LOG_D("Sent: " + parse_message_to_hex(output), true, true);
    delay(50);
    received = serial->read_serial_data(20, serial_read_timeout);
    if (received.length() > 5)
    {
        if ((uint8_t)received.at(4) != 0x67 || (uint8_t)received.at(5) != 0x02)
        {
            emit LOG_E("Wrong response from TCU: " + fileActions.parse_nrc_message(received.remove(0, 4)), true, true);
            emit LOG_D("Response: " + parse_message_to_hex(received), true, true);
            return STATUS_ERROR;
        }
    }
    else
    {
        emit LOG_E("No valid response from ECU", true, true);
        emit LOG_D("Response: " + parse_message_to_hex(received), true, true);
        return STATUS_ERROR;
    }

    emit LOG_I("Seed key ok", true, true);
    emit LOG_D("Response: " + parse_message_to_hex(received), true, true);

    output.clear();
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x07);
    output.append((uint8_t)0xE1);
    output.append((uint8_t)0x10);
    output.append((uint8_t)0x02);
    serial->write_serial_data_echo_check(output);
    emit LOG_D("Sent: " + parse_message_to_hex(output), true, true);
    delay(50);
    received = serial->read_serial_data(20, serial_read_timeout);
    if (received.length() > 5)
    {
        if ((uint8_t)received.at(4) == 0x50 && ((uint8_t)received.at(5) == 0x02 || (uint8_t)received.at(5) == 0x42))
        {
            QByteArray response = received;
            response.remove(0, 6);
            QString msg;
            msg.clear();
            for (int i = 0; i < response.length(); i++)
                msg.append(QString("%1").arg((uint8_t)response.at(i),2,16,QLatin1Char('0')).toUpper());
            emit LOG_D("Response: " + parse_message_to_hex(received), true, true);
        }
        else
        {
            emit LOG_E("Wrong response from TCU: " + fileActions.parse_nrc_message(received.remove(0, 4)), true, true);
            emit LOG_D("Response: " + parse_message_to_hex(received), true, true);
            return STATUS_ERROR;
        }
    }
    else
    {
        emit LOG_E("No valid response from ECU", true, true);
        emit LOG_D("Response: " + parse_message_to_hex(received), true, true);
        return STATUS_ERROR;
    }
    emit LOG_I("Succesfully set to programming session", true, true);

    return STATUS_SUCCESS;
}

/*
 * Upload kernel to Subaru Denso CAN (iso15765) 32bit ECUs
 *
 * @return success
 */
int FlashTcuSubaruDensoSH705xCan::upload_kernel(QString kernel, uint32_t kernel_start_addr)
{
    QFile file(kernel);

    QByteArray output;
    QByteArray received;
    QByteArray pl_encr;
    uint32_t file_len = 0;
    uint32_t pl_len = 0;
    uint32_t start_address = 0;
    uint32_t end_addr = 0;
    uint32_t chk_sum = 0;
    uint32_t blockaddr = 0;
    uint16_t blockno = 0;
    uint16_t maxblocks = 0;

    start_address = kernel_start_addr;
    emit LOG_D("Start address to upload kernel: 0x" + QString::number(start_address, 16), true, true);

    if (!serial->is_serial_port_open())
    {
        emit LOG_E("ERROR: Serial port is not open.", true, true);
        return STATUS_ERROR;
    }

    serial->set_add_iso14230_header(false);

    // Check kernel file
    if (!file.open(QIODevice::ReadOnly))
    {
        emit LOG_E("Unable to open kernel file for reading", true, true);
        return STATUS_ERROR;
    }
    file_len = file.size();
    pl_len = (file_len + 3) & ~3;
    pl_encr = file.readAll();
    maxblocks = pl_len / 128;
    if((pl_len % 128) != 0)
        maxblocks++;
    end_addr = (start_address + (maxblocks * 128)) & 0xFFFFFFFF;
    uint32_t data_len = end_addr - start_address;
    while ((uint32_t)pl_encr.length() < data_len)
        pl_encr.append((uint8_t)0x00);
    pl_encr.remove(pl_encr.length() - 4, 4);
    chk_sum = 0;
    for (int i = 0; i < pl_encr.length(); i+=4)
        chk_sum += ((pl_encr.at(i) << 24) & 0xFF000000) | ((pl_encr.at(i + 1) << 16) & 0xFF0000) | ((pl_encr.at(i + 2) << 8) & 0xFF00) | ((pl_encr.at(i + 3)) & 0xFF);
    chk_sum = 0x5aa5a55a - chk_sum;

    pl_encr.append((uint8_t)((chk_sum >> 24) & 0xFF));
    pl_encr.append((uint8_t)((chk_sum >> 16) & 0xFF));
    pl_encr.append((uint8_t)((chk_sum >> 8) & 0xFF));
    pl_encr.append((uint8_t)(chk_sum & 0xFF));
    pl_encr = encrypt_payload(pl_encr, pl_encr.length());
    //pl_encr = decrypt_payload(pl_encr, pl_encr.length());

    set_progressbar_value(0);

    emit LOG_I("Initialize kernel upload", true, true);
    output.clear();
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x07);
    output.append((uint8_t)0xE1);
    output.append((uint8_t)0x34);
    output.append((uint8_t)0x04);
    output.append((uint8_t)0x33);
    output.append((uint8_t)((start_address >> 16) & 0xFF));
    output.append((uint8_t)((start_address >> 8) & 0xFF));
    output.append((uint8_t)(start_address & 0xFF));
    output.append((uint8_t)((data_len >> 16) & 0xFF));
    output.append((uint8_t)((data_len >> 8) & 0xFF));
    output.append((uint8_t)(data_len & 0xFF));

    serial->write_serial_data_echo_check(output);
    emit LOG_D("Sent: " + parse_message_to_hex(output), true, true);
    delay(50);
    received = serial->read_serial_data(20, 10);
    if (received.length() > 5)
    {
        if ((uint8_t)received.at(4) == 0x74 && (uint8_t)received.at(5) == 0x20)
        {
            emit LOG_D("Response: " + parse_message_to_hex(received), true, true);
        }
        else
        {
            emit LOG_E("Wrong response from TCU: " + fileActions.parse_nrc_message(received.remove(0, 4)), true, true);
            emit LOG_D("Response: " + parse_message_to_hex(received), true, true);
            return STATUS_ERROR;
        }
    }
    else
    {
        emit LOG_E("No valid response from ECU", true, true);
        emit LOG_D("Response: " + parse_message_to_hex(received), true, true);
        return STATUS_ERROR;
    }

    emit LOG_I("Uploading kernel, please wait...", true, true);
    for (blockno = 0; blockno <= maxblocks; blockno++)
    {
        if (kill_process)
            return 0;

        blockaddr = start_address + blockno * 128;
        output.clear();
        output.append((uint8_t)0x00);
        output.append((uint8_t)0x00);
        output.append((uint8_t)0x07);
        output.append((uint8_t)0xE1);
        output.append((uint8_t)0xB6);
        output.append((uint8_t)(blockaddr >> 16) & 0xFF);
        output.append((uint8_t)(blockaddr >> 8) & 0xFF);
        output.append((uint8_t)blockaddr & 0xFF);

        if (blockno == maxblocks)
        {
            for (uint32_t i = 0; i < data_len; i++)
            {
                output.append(pl_encr.at(i + blockno * 128));
            }
        }
        else
        {
            for (int i = 0; i < 128; i++)
            {
                output.append(pl_encr.at(i + blockno * 128));
            }
            data_len -= 128;
        }

        serial->write_serial_data_echo_check(output);
        received = serial->read_serial_data(5, receive_timeout);

        float pleft = (float)blockno / (float)maxblocks * 100;
        set_progressbar_value(pleft);
    }

    emit LOG_I("Kernel uploaded, starting...", true, true);

    output.clear();
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x07);
    output.append((uint8_t)0xE1);
    output.append((uint8_t)0x37);

    serial->write_serial_data_echo_check(output);
    emit LOG_D("Sent: " + parse_message_to_hex(output), true, true);
    delay(50);
    received = serial->read_serial_data(20, serial_read_timeout);
    if (received.length() > 4)
    {
        if ((uint8_t)received.at(4) != 0x77)
        {
            emit LOG_E("Wrong response from TCU: " + fileActions.parse_nrc_message(received.remove(0, 4)), true, true);
            emit LOG_D("Response: " + parse_message_to_hex(received), true, true);
            return STATUS_ERROR;
        }
    }
    else
    {
        emit LOG_E("No valid response from ECU", true, true);
        emit LOG_D("Response: " + parse_message_to_hex(received), true, true);
        return STATUS_ERROR;
    }
    emit LOG_D("Response: " + parse_message_to_hex(received), true, true);

    output.clear();
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x07);
    output.append((uint8_t)0xE1);
    output.append((uint8_t)0x31);
    output.append((uint8_t)0x01);
    output.append((uint8_t)0x02);
    output.append((uint8_t)0x02);
    output.append((uint8_t)0x02);

    serial->write_serial_data_echo_check(output);
    emit LOG_D("Sent: " + parse_message_to_hex(output), true, true);
    delay(50);
    received = serial->read_serial_data(20, serial_read_timeout);
    if (received.length() > 4)
    {
        if ((uint8_t)received.at(4) != 0x31)
        {
            emit LOG_E("Wrong response from TCU: " + fileActions.parse_nrc_message(received.remove(0, 4)), true, true);
            emit LOG_D("Response: " + parse_message_to_hex(received), true, true);
            return STATUS_ERROR;
        }
    }
    else
    {
        emit LOG_E("No valid response from ECU", true, true);
        emit LOG_D("Response: " + parse_message_to_hex(received), true, true);
        return STATUS_ERROR;
    }
    emit LOG_D("Response: " + parse_message_to_hex(received), true, true);

    set_progressbar_value(100);

    emit LOG_I("Kernel started, initializing...", true, true);

    //switch to simple CAN comms for kernel (kernel has no iso15765 support)
    serial->reset_connection();
    serial->set_is_iso14230_connection(false);
    serial->set_is_can_connection(true);
    serial->set_is_iso15765_connection(false);
    serial->set_is_29_bit_id(false);
    serial->set_can_speed("500000");
    serial->set_can_source_address(0x7e1);
    serial->set_can_destination_address(0x7e9);
    // Open serial port
    serial->open_serial_port();

    delay(500);

    emit LOG_I("Requesting kernel ID", true, true);

    received.clear();
    received = request_kernel_id();
    if (received == "")
        return STATUS_ERROR;

    emit LOG_I("Kernel ID: " + received, true, true);


    return STATUS_SUCCESS;
}



/*
 * Read memory from Subaru Denso CAN (iso15765) 32bit TCUs, nisprog kernel
 *
 * @return success
 */
int FlashTcuSubaruDensoSH705xCan::read_mem(uint32_t start_addr, uint32_t length)
{
    QElapsedTimer timer;
    QByteArray output;
    QByteArray received;
    QByteArray msg;
    QByteArray pagedata;
    QByteArray mapdata;
    uint32_t cplen = 0;
    uint32_t timeout = 0;

    uint32_t pagesize = 0x400;

    uint32_t skip_start = start_addr & (pagesize - 1); //if unaligned, we'll be receiving this many extra bytes
    uint32_t addr = start_addr - skip_start;
    uint32_t willget = (skip_start + length + pagesize - 1) & ~(pagesize - 1);
    uint32_t len_done = 0;  //total data written to file

    emit LOG_I("Start reading ROM, please wait..." + received, true, true);

    // send 0xD8 command to kernel to dump the chunk from ROM
    output.clear();
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x07);
    output.append((uint8_t)0xE1);
    output.append((uint8_t)SUB_DENSOCAN_START_COMM);
    output.append((uint8_t)(SID_CAN_DUMP_ROM + 0x06));
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
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

        //uint32_t curblock = (addr / pagesize);


        pleft = (float)(addr - start_addr) / (float)length * 100.0f;
        set_progressbar_value(pleft);

        output[6] = (uint8_t)((pagesize >> 24) & 0xFF);
        output[7] = (uint8_t)((pagesize >> 16) & 0xFF);
        output[8] = (uint8_t)((pagesize >> 8) & 0xFF);
        output[9] = (uint8_t)((addr >> 24) & 0xFF);
        output[10] = (uint8_t)((addr >> 16) & 0xFF);
        output[11] = (uint8_t)((addr >> 8) & 0xFF);
        serial->write_serial_data_echo_check(output);
        received = serial->read_serial_data(1, 10);

        if ((uint8_t)received.at(0) != SUB_DENSOCAN_START_COMM || ((uint8_t)received.at(1) & 0xF8) != SID_CAN_DUMP_ROM)
        {
            emit LOG_I("Page data request failed!", true, true);
            emit LOG_I("Received msg: " + parse_message_to_hex(received), true, true);
            return STATUS_ERROR;
        }

        timeout = 0;
        pagedata.clear();

        while ((uint32_t)pagedata.length() < pagesize && timeout < 1000)
        {
            received = serial->read_serial_data(1, 50);
            pagedata.append(received, 8);
            timeout++;
        }
        if (timeout >= 1000)
        {
            emit LOG_I("Page data timeout!", true, true);
        }

        mapdata.append(pagedata);

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
        msg = QString("Kernel read addr:  0x%1  length:  0x%2,  %3  B/s  %4 s remaining").arg(start_address).arg(block_len).arg(curspeed, 6, 10, QLatin1Char(' ')).arg(tleft, 6, 10, QLatin1Char(' ')).toUtf8();
        emit LOG_I(msg, true, true);
        delay(1);

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

    emit LOG_I("ROM read ready" + received, true, true);

    ecuCalDef->FullRomData = mapdata;
    set_progressbar_value(100);

    return STATUS_SUCCESS;
}

/*
 * Write memory to Subaru Denso CAN (iso15765) 32bit ECUs, nisprog kernel
 *
 * @return success
 */
int FlashTcuSubaruDensoSH705xCan::write_mem(bool test_write)
{
    QByteArray filedata;

    filedata = ecuCalDef->FullRomData;

    QScopedArrayPointer<uint8_t> data_array(new uint8_t[filedata.length()]);

    int block_modified[16] = {0};

    unsigned bcnt = 0;
    unsigned blockno;

    for (int i = 0; i < filedata.length(); i++)
    {
        data_array[i] = filedata.at(i);
    }

    emit LOG_I("--- comparing ECU flash memory pages to image file ---", true, true);
    emit LOG_I("blk\t\tstart\tlen\tecu crc\timg crc\tsame?", true, true);

    if (get_changed_blocks(&data_array[0], block_modified))
    {
        emit LOG_I("Error in ROM compare", true, true);
        return STATUS_ERROR;
    }

    bcnt = 0;
    emit LOG_I("Different blocks : ", true, false);
    for (blockno = 0; blockno < flashdevices[mcu_type_index].numblocks; blockno++) {
        if (block_modified[blockno]) {
            emit LOG_I(QString::number(blockno) + ", ", false, false);
            bcnt += 1;
        }
    }
    emit LOG_I(" (total: " + QString::number(bcnt) + ")", false, true);

    if (bcnt)
    {
        flashbytescount = 0;
        flashbytesindex = 0;

        for (blockno = 0; blockno < flashdevices[mcu_type_index].numblocks; blockno++)
        {
            if (block_modified[blockno])
            {
                flashbytescount += flashdevices[mcu_type_index].fblocks[blockno].len;
            }
        }

        emit LOG_I("--- start writing ROM file to ECU flash memory ---", true, true);
        for (blockno = 0; blockno < flashdevices[mcu_type_index].numblocks; blockno++)
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

        emit LOG_I("--- comparing ECU flash memory pages to image file after reflash ---", true, true);
        emit LOG_I("blk\t\tstart\tlen\tecu crc\timg crc\tsame?", true, true);

        if (get_changed_blocks(&data_array[0], block_modified))
        {
            emit LOG_I("Error in ROM compare", true, true);
            return STATUS_ERROR;
        }

        bcnt = 0;
        emit LOG_I("Different blocks : ", true, false);
        for (blockno = 0; blockno < flashdevices[mcu_type_index].numblocks; blockno++) {
            if (block_modified[blockno])
            {
                emit LOG_I(QString::number(blockno) + ", ", false, false);
                bcnt += 1;
            }
        }
        emit LOG_I(" (total: " + QString::number(bcnt) + ")", false, true);
        if (!test_write)
        {
            if (bcnt)
            {
                emit LOG_I("*** ERROR IN FLASH PROCESS ***", true, true);
                emit LOG_I("Don't power off your ECU, kernel is still running and you can try flashing again!", true, true);
            }
        }
        else
            emit LOG_I("*** Test write PASS, it's ok to perform actual write! ***", true, true);
    }
    else
    {
        emit LOG_I("*** Compare results no difference between ROM and ECU data, no flashing needed! ***", true, true);
    }

    return STATUS_SUCCESS;
}

/*
 * Compare ROM 32bit (iso15765) CAN ECUs
 *
 * @return
 */
int FlashTcuSubaruDensoSH705xCan::get_changed_blocks(const uint8_t *src, int *modified)
{
    unsigned blockno;
    QByteArray msg;

    for (blockno = 0; blockno < flashdevices[mcu_type_index].numblocks; blockno++) {
        if (kill_process)
            return STATUS_ERROR;

        uint32_t bs, blen;
        bs = flashdevices[mcu_type_index].fblocks[blockno].start;
        blen = flashdevices[mcu_type_index].fblocks[blockno].len;

        QString block_no = QString("%1").arg((uint8_t)blockno,2,10,QLatin1Char('0')).toUpper();
        QString block_start = QString("%1").arg((uint32_t)bs,8,16,QLatin1Char('0')).toUpper();
        QString block_length = QString("%1").arg((uint32_t)blen,8,16,QLatin1Char('0')).toUpper();

        msg = QString("FB" + block_no + "\t0x" + block_start + "\t0x" + block_length).toUtf8();
        emit LOG_I(msg, true, false);
        // do CRC comparison with ECU //
        if (check_romcrc(&src[bs], bs, blen, &modified[blockno])) {
            return -1;
        }
    }
    return 0;
}

/*
 * ROM CRC 32bit CAN (iso15765) ECUs
 *
 * @return
 */
int FlashTcuSubaruDensoSH705xCan::check_romcrc(const uint8_t *src, uint32_t start_addr, uint32_t len, int *modified)
{
    QByteArray output;
    QByteArray received;
    QByteArray msg;
    QByteArray pagedata;
    uint16_t chunko;
    uint32_t pagesize = ROMCRC_ITERSIZE_CAN;
    uint32_t byte_index = 0;

    len = (len + ROMCRC_LENMASK_CAN) & ~ROMCRC_LENMASK_CAN;

    chunko = start_addr / ROMCRC_CHUNKSIZE_CAN;

    output.clear();
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x07);
    output.append((uint8_t)0xE1);
    output.append((uint8_t)SUB_DENSOCAN_START_COMM);
    output.append((uint8_t)(SID_CAN_CONF_CKS + 0x06));
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);

    //request format : <SID_CONF> <SID_CONF_CKS1> <CNH> <CNL> <CRC0H> <CRC0L> ...<CRC3H> <CRC3L>
    //verify if <CRCH:CRCL> hash is valid for n*256B chunk of the ROM (starting at <CNH:CNL> * 256)
    for (; len > 0; len -= ROMCRC_ITERSIZE_CAN, chunko += ROMCRC_NUMCHUNKS_CAN) {
        if (kill_process)
            return STATUS_ERROR;

        output[6] = (uint8_t)((pagesize >> 24) & 0xFF);
        output[7] = (uint8_t)((pagesize >> 16) & 0xFF);
        output[8] = (uint8_t)((pagesize >> 8) & 0xFF);
        output[9] = (uint8_t)((start_addr >> 24) & 0xFF);
        output[10] = (uint8_t)((start_addr >> 16) & 0xFF);
        output[11] = (uint8_t)((start_addr >> 8) & 0xFF);
        start_addr += pagesize;

        received = serial->write_serial_data_echo_check(output);
        received = serial->read_serial_data(1, serial_read_short_timeout);

        uint16_t chk_sum = 0;
        for (uint32_t j = 0; j < pagesize; j++) {
            pagedata[j] = src[(byte_index * pagesize) + j];
            chk_sum += (pagedata[j] & 0xFF);
            chk_sum = ((chk_sum >> 8) & 0xFF) + (chk_sum & 0xFF);
        }
        byte_index++;

        if ((uint8_t)received.at(0) != SUB_DENSOCAN_START_COMM || ((uint8_t)received.at(1) & 0xF8) != SID_CAN_CONF_CKS || chk_sum == (uint8_t)received.at(2))
            continue;

        emit LOG_I("\tNO", false, true);

        //confirmed bad CRC, we can exit
        *modified = 1;

        return 0;
    }   //for

    emit LOG_I("\tYES", false, true);
    *modified = 0;
    serial->read_serial_data(100, serial_read_short_timeout);
    return 0;
}

/*
 * Reflash ROM 32bit CAN (iso15765) ECUs
 *
 * @return success
 */
int FlashTcuSubaruDensoSH705xCan::reflash_block(const uint8_t *newdata, const struct flashdev_t *fdt, unsigned blockno, bool test_write)
{
    int errval;

    uint32_t block_start;
    uint32_t block_len;

    QByteArray output;
    QByteArray received;
    QByteArray msg;

    set_progressbar_value(0);

    if (blockno >= fdt->numblocks) {
        emit LOG_I("block " + QString::number(blockno) + " out of range !", true, true);
        return -1;
    }

    block_start = fdt->fblocks[blockno].start;
    block_len = fdt->fblocks[blockno].len;

    QString start_addr = QString("%1").arg((uint32_t)block_start,8,16,QLatin1Char('0')).toUpper();
    QString length = QString("%1").arg((uint32_t)block_len,8,16,QLatin1Char('0')).toUpper();
    msg = QString("Flash block addr: 0x" + start_addr + " len: 0x" + length).toUtf8();
    emit LOG_I(msg, true, true);

    output.clear();
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x07);
    output.append((uint8_t)0xE1);
    output.append((uint8_t)SUB_DENSOCAN_START_COMM);
    output.append((uint8_t)(SID_CAN_FLASH + 0x01));
    if (test_write)
        output.append((uint8_t)SID_CAN_FL_PROTECT);
    else
        output.append((uint8_t)SID_CAN_FL_UNPROTECT);
    output.append((uint8_t)(0x00));
    output.append((uint8_t)(0x00));
    output.append((uint8_t)(0x00));
    output.append((uint8_t)(0x00));
    output.append((uint8_t)(0x00));
    received = serial->write_serial_data_echo_check(output);
    qDebug() << parse_message_to_hex(output);
    qDebug() << "0xE0 message sent to kernel to initialize erasing / flashing microcodes";
    delay(200);
    received = serial->read_serial_data(3, serial_read_short_timeout);
    qDebug() << parse_message_to_hex(received);

    if((uint8_t)received.at(0) != SUB_DENSOCAN_START_COMM || ((uint8_t)received.at(1) & 0xF8) != SID_CAN_FLASH)
    {
        qDebug() << "Initialize of erasing / flashing microcodes failed!";
        return STATUS_ERROR;
    }


    int num_128_byte_blocks = (block_len >> 7) & 0xFFFFFFFF;

    qDebug() << "Proceeding to attempt erase and flash of block number: " << blockno;
    output[5] = (uint8_t)(SID_CAN_FL_EB + 0x06);
    output[6] = (uint8_t)(blockno & 0xFF);
    output[7] = (uint8_t)((block_start >> 24) & 0xFF);
    output[8] = (uint8_t)((block_start >> 16) & 0xFF);
    output[9] = (uint8_t)((block_start >> 8) & 0xFF);
    output[10] = (uint8_t)((num_128_byte_blocks >> 8) & 0xFF);
    output[11] = (uint8_t)(num_128_byte_blocks & 0xFF);
    received = serial->write_serial_data_echo_check(output);
    qDebug() << parse_message_to_hex(output);
    //emit LOG_I("0xF0 message sent to kernel to erase block number: " + QString::number(blockno), true, true);
    qDebug() << "0xF0 message sent to kernel to erase block number: " << blockno;
    delay(500);

    QTime dieTime = QTime::currentTime().addMSecs(serial_read_extra_long_timeout);
    while ((uint32_t)received.length() < 3 && (QTime::currentTime() < dieTime))
    {
        received = serial->read_serial_data(3, serial_read_short_timeout);
        QCoreApplication::processEvents(QEventLoop::AllEvents, 1);
        delay(100);
    }

    //emit LOG_I(parse_message_to_hex(received), true, true);
    qDebug() << parse_message_to_hex(received);

    if((uint8_t)received.at(0) != SUB_DENSOCAN_START_COMM || ((uint8_t)received.at(1) & 0xF8) != SID_CAN_FL_EB)
    {
        qDebug() << "Not ready for 128byte block writing";
        return STATUS_ERROR;
    }

    errval = flash_block(newdata, block_start, block_len);
    if (errval) {
        emit LOG_I("Reflash error! Do not panic, do not reset the ECU immediately. The kernel is most likely still running and receiving commands!", true, true);
        return STATUS_ERROR;
    }

    set_progressbar_value(100);

    return STATUS_SUCCESS;
}

/*
 * Flash block 32bit CAN (iso15765) ECUs
 *
 * @return success
 */
int FlashTcuSubaruDensoSH705xCan::flash_block(const uint8_t *src, uint32_t start, uint32_t len)
{
    QByteArray output;
    QByteArray received;
    QByteArray msg;

    uint32_t remain = len;
    uint32_t block_start = start;
    uint32_t block_len = len;
    uint32_t byteindex = flashbytesindex;
    uint16_t chk_sum;
    uint8_t blocksize = 128;

    QElapsedTimer timer;

    unsigned long chrono;
    unsigned curspeed, tleft;

    int num_128_byte_blocks = (block_len >> 7) & 0xFFFFFFFF;
    int byte_index = block_start & 0xFFFFFFFF;

    qDebug() << "flashbytesindex" << flashbytesindex;
    qDebug() << "flashbytescount" << flashbytescount;

    output.clear();
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x07);
    output.append((uint8_t)0xE1);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);

    for (int i = 0; i < num_128_byte_blocks; i++)
    {
        if (kill_process)
            return STATUS_ERROR;

        chk_sum = 0;
        for (int j = 0; j < 16; j++)
        {
            // send 16 lots of 8 byte pure data messages to load and flash the new block (16 x 8 bytes = 128 bytes)
            for (int k = 0; k < 8; k++){
                output[k + 4] = (uint8_t)(src[byte_index + k] & 0xFF);
                chk_sum += (output[k + 4] & 0xFF);
                chk_sum = ((chk_sum >> 8) & 0xFF) + (chk_sum & 0xFF);
            }
            byte_index += 8;
            received = serial->write_serial_data_echo_check(output);
        }

        output[4] = (uint8_t)SUB_DENSOCAN_START_COMM;
        output[5] = (uint8_t)(SID_CAN_FL_WB + 0x03);
        output[6] = (uint8_t)((i >> 8) & 0xFF);
        output[7] = (uint8_t)(i & 0xFF);
        output[8] = (uint8_t)(chk_sum & 0xFF);
        received = serial->write_serial_data_echo_check(output);

        received = serial->read_serial_data(3, serial_read_long_timeout);
        if((uint8_t)received.at(0) != SUB_DENSOCAN_START_COMM || ((uint8_t)received.at(1) & 0xF8) != SID_CAN_FL_WB)
        {
            qDebug() << "Flashing of 128 byte block unsuccessful, stopping";
            qDebug() << hex << num_128_byte_blocks << "/" << (i & 0xFFFF);
            //return STATUS_ERROR;
        }
        else
        {
            //qDebug() << "Flashing of 128 byte block successful, proceeding to next 128 byte block";
            //qDebug() << hex << num_128_byte_blocks << "/" << (i & 0xFFFF);
        }

        remain -= blocksize;
        start += blocksize;
        byteindex += blocksize;

        chrono = timer.elapsed();
        timer.start();

        if (!chrono) {
            chrono += 1;
        }
        curspeed = blocksize * (1000.0f / chrono);  //avg B/s
        if (!curspeed) {
            curspeed += 1;
        }

        tleft = remain / curspeed;  //s
        if (tleft > 9999) {
            tleft = 9999;
        }
        tleft++;

        float pleft = (float)byteindex / (float)flashbytescount * 100.0f;
        set_progressbar_value(pleft);

        QString start_address = QString("%1").arg(start,8,16,QLatin1Char('0'));
        msg = QString("Writing chunk @ 0x%1 (%2\% - %3 B/s, ~ %4 s remaining)").arg(start_address).arg((unsigned) 100 * (len - remain) / len,1,10,QLatin1Char('0')).arg((uint32_t)curspeed,1,10,QLatin1Char('0')).arg(tleft,1,10,QLatin1Char('0')).toUtf8();
        emit LOG_I(msg, true, true);

    }

    return STATUS_SUCCESS;
}

/*
 * 8bit checksum
 *
 * @return
 */
uint8_t FlashTcuSubaruDensoSH705xCan::cks_add8(QByteArray chksum_data, unsigned len)
{
    uint16_t sum = 0;
    uint8_t data[chksum_data.length()];
/*
    for (int i = 0; i < chksum_data.length(); i++)
    {
        data[i] = chksum_data.at(i);
    }
*/
    for (unsigned i = 0; i < len; i++) {
        sum += (uint8_t)chksum_data.at(i);//data[i];
        if (sum & 0x100) {
            sum += 1;
        }
        sum = (uint8_t) sum;
    }
    return sum;
}

/*
 * CRC16 implementation adapted from Lammert Bies
 *
 * @return
 */
#define NPK_CRC16   0xBAAD  //koopman, 2048bits (256B)
static bool crc_tab16_init = 0;
static uint16_t crc_tab16[256];
void FlashTcuSubaruDensoSH705xCan::init_crc16_tab(void)
{

    uint32_t i, j;
    uint16_t crc, c;

    for (i=0; i<256; i++) {
        crc = 0;
        c   = (uint16_t) i;

        for (j=0; j<8; j++) {
            if ( (crc ^ c) & 0x0001 ) {
                crc = ( crc >> 1 ) ^ NPK_CRC16;
            } else {
                crc =   crc >> 1;
            }
            c = c >> 1;
        }
        crc_tab16[i] = crc;
    }

    crc_tab16_init = 1;

}

uint16_t FlashTcuSubaruDensoSH705xCan::crc16(const uint8_t *data, uint32_t siz)
{
    uint16_t crc;

    if (!crc_tab16_init) {
        init_crc16_tab();
    }

    crc = 0;

    while (siz > 0) {
        uint16_t tmp;
        uint8_t nextval;

        nextval = *data++;
        tmp =  crc ^ nextval;
        crc = (crc >> 8) ^ crc_tab16[ tmp & 0xff ];
        siz -= 1;
    }
    return crc;
}




















/*
 * Generate denso can seed key from received seed bytes
 *
 * @return seed key (4 bytes)
 */
QByteArray FlashTcuSubaruDensoSH705xCan::generate_seed_key(QByteArray requested_seed)
{
    QByteArray key;

    const uint16_t keytogenerateindex_1[]={
        0x78B1, 0x4625, 0x201C, 0x9EA5,
        0xAD6B, 0x35F4, 0xFD21, 0x5E71,
        0xB046, 0x7F4A, 0x4B75, 0x93F9,
        0x1895, 0x8961, 0x3ECC, 0x862B
    };

    const uint8_t indextransformation[]={
        0x5, 0x6, 0x7, 0x1, 0x9, 0xC, 0xD, 0x8,
        0xA, 0xD, 0x2, 0xB, 0xF, 0x4, 0x0, 0x3,
        0xB, 0x4, 0x6, 0x0, 0xF, 0x2, 0xD, 0x9,
        0x5, 0xC, 0x1, 0xA, 0x3, 0xD, 0xE, 0x8
    };

    key = calculate_seed_key(requested_seed, keytogenerateindex_1, indextransformation);

    return key;
}


/*
 * Calculate denso seed key from received seed bytes
 *
 * @return seed key (4 bytes)
 */
QByteArray FlashTcuSubaruDensoSH705xCan::calculate_seed_key(QByteArray requested_seed, const uint16_t *keytogenerateindex, const uint8_t *indextransformation)
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
QByteArray FlashTcuSubaruDensoSH705xCan::encrypt_payload(QByteArray buf, uint32_t len)
{
    QByteArray encrypted;

    uint16_t keytogenerateindex[]={
        0xC85B, 0x32C0, 0xE282, 0x92A0
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

QByteArray FlashTcuSubaruDensoSH705xCan::decrypt_payload(QByteArray buf, uint32_t len)
{
    QByteArray decrypt;

    uint16_t keytogenerateindex[]={
        0x92A0, 0xE282, 0x32C0, 0xC85B
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

QByteArray FlashTcuSubaruDensoSH705xCan::calculate_payload(QByteArray buf, uint32_t len, const uint16_t *keytogenerateindex, const uint8_t *indextransformation)
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
 * Request kernel id
 *
 * @return kernel id
 */
QByteArray FlashTcuSubaruDensoSH705xCan::request_kernel_id()
{
    QByteArray output;
    QByteArray received;
    QByteArray msg;
    QByteArray kernelid;

    request_denso_kernel_id = true;

    output.clear();

    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x07);
    output.append((uint8_t)0xE1);
    output.append((uint8_t)SUB_DENSOCAN_START_COMM);
    output.append((uint8_t)0xA0);

    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);

    int loop = 0;
    while (loop < 5 && received == "")
    {
        received = serial->write_serial_data_echo_check(output);
        emit LOG_I("Kernel ID request: " + parse_message_to_hex(output), true, true);
        delay(100);
        kernelid.clear();

        received = serial->read_serial_data(100, serial_read_long_timeout);
        emit LOG_I("Kernel ID response: " + parse_message_to_hex(received), true, true);
        received.remove(0, 2);
        kernelid = received;

        while (received != "")
        {
            received = serial->read_serial_data(1, serial_read_short_timeout);
            received.remove(0, 2);
            kernelid.append(received);
        }
        loop++;
    }
    request_denso_kernel_id = false;

    return kernelid;
}



















/*
 * Add SSM header to message
 *
 * @return parsed message
 */
QByteArray FlashTcuSubaruDensoSH705xCan::add_ssm_header(QByteArray output, uint8_t tester_id, uint8_t target_id, bool dec_0x100)
{
    uint8_t length = output.length();

    output.insert(0, (uint8_t)0x80);
    output.insert(1, target_id & 0xFF);
    output.insert(2, tester_id & 0xFF);
    output.insert(3, length);

    output.append(calculate_checksum(output, dec_0x100));

    //emit LOG_I("Send: " + parse_message_to_hex(output), true, true);
    return output;
}

/*
 * Calculate SSM checksum to message
 *
 * @return 8-bit checksum
 */
uint8_t FlashTcuSubaruDensoSH705xCan::calculate_checksum(QByteArray output, bool dec_0x100)
{
    uint8_t checksum = 0;

    for (uint16_t i = 0; i < output.length(); i++)
        checksum += (uint8_t)output.at(i);

    if (dec_0x100)
        checksum = (uint8_t) (0x100 - checksum);

    return checksum;
}

/*
 * Parse QByteArray to readable form
 *
 * @return parsed message
 */
QString FlashTcuSubaruDensoSH705xCan::parse_message_to_hex(QByteArray received)
{
    QString msg;

    for (int i = 0; i < received.length(); i++)
    {
        msg.append(QString("%1 ").arg((uint8_t)received.at(i),2,16,QLatin1Char('0')).toUtf8());
    }

    return msg;
}

void FlashTcuSubaruDensoSH705xCan::set_progressbar_value(int value)
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

void FlashTcuSubaruDensoSH705xCan::delay(int timeout)
{
    QTime dieTime = QTime::currentTime().addMSecs(timeout);
    while (QTime::currentTime() < dieTime)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 1);
}

