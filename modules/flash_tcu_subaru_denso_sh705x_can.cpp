#include "flash_tcu_subaru_denso_sh705x_can.h"

FlashTcuSubaruDensoSH705xCan::FlashTcuSubaruDensoSH705xCan(SerialPortActions *serial, FileActions::EcuCalDefStructure *ecuCalDef, QString cmd_type, QWidget *parent)
    : QDialog(parent),
      ui(new Ui::EcuOperationsWindow)
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
    this->show();

    int result = STATUS_ERROR;
    set_progressbar_value(0);

    result = init_flash_denso_subarucan(ecuCalDef, cmd_type);

    if (result == STATUS_SUCCESS)
    {
        QMessageBox::information(this, tr("TCU Operation"), "TCU operation was succesful, press OK to exit");
        this->close();
    }
    else
    {
        QMessageBox::warning(this, tr("TCU Operation"), "TCU operation failed, press OK to exit and try again");
    }
}

FlashTcuSubaruDensoSH705xCan::~FlashTcuSubaruDensoSH705xCan()
{

}

void FlashTcuSubaruDensoSH705xCan::closeEvent(QCloseEvent *event)
{
    kill_process = true;
}

int FlashTcuSubaruDensoSH705xCan::init_flash_denso_subarucan(FileActions::EcuCalDefStructure *ecuCalDef, QString cmd_type)
{
    bool ok = false;
    //int tcuRead;
    int tcuAction = 1;

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

    kernel = ecuCalDef->Kernel;
    flash_method = ecuCalDef->FlashMethod;

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
            send_log_window_message("Read memory with flashmethod '" + flash_method + "' and kernel '" + ecuCalDef->Kernel + "'", true, true);
            tcuAction = 1;
        }
        else if (msgBox.clickedButton() == (QAbstractButton *)pButtonRelearn)
        {
            send_log_window_message("Attempting TCU relearn", true, true);
            tcuAction = 2;
        }
        else if (msgBox.clickedButton() == (QAbstractButton *)pButtonReadParam)
        {
            send_log_window_message("Attempting to read TCU parameters", true, true);
            tcuAction = 3;
        }
        else if (msgBox.clickedButton() == (QAbstractButton *)pButtonSetParam)
        {
            send_log_window_message("Attempting to set TCU parameters", true, true);
            tcuAction = 4;
        }
        else
            send_log_window_message("No option selected", true, true);


        /*
        QMessageBox msgBox;
        msgBox.setText("Choose which option");
        msgBox.setInformativeText("Proceed with TCU Read (Yes) or TCU Relearn (No)?");
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        tcuRead = msgBox.exec();

        if (tcuRead == QMessageBox::Yes) send_log_window_message("Read memory with flashmethod '" + flash_method + "' and kernel '" + ecuCalDef->Kernel + "'", true, true);
        else send_log_window_message("Attempting TCU relearn", true, true);
        //qDebug() << "Read memory with flashmethod" << flash_method << "and kernel" << ecuCalDef->Kernel;
        */
    }
    else if (cmd_type == "test_write")
    {
        test_write = true;
        send_log_window_message("Test write memory with flashmethod '" + flash_method + "' and kernel '" + ecuCalDef->Kernel + "'", true, true);
        //qDebug() << "Test write memory with flashmethod" << flash_method << "and kernel" << ecuCalDef->Kernel;
    }
    else if (cmd_type == "write")
    {
        test_write = false;
        send_log_window_message("Write memory with flashmethod '" + flash_method + "' and kernel '" + ecuCalDef->Kernel + "'", true, true);
        //qDebug() << "Write memory with flashmethod" << flash_method << "and kernel" << ecuCalDef->Kernel;
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

    QMessageBox::information(this, tr("Connecting to TCU"), "Turn ignition ON and press OK to start initializing connection");
    //QMessageBox::information(this, tr("Connecting to TCU"), "Press OK to start countdown!");

    //if (tcuRead == QMessageBox::Yes)
    if (tcuAction == 1)
    {
        send_log_window_message("Connecting to Subaru Denso CAN bootloader, please wait...", true, true);
        result = connect_bootloader_subaru_denso_subarucan();

        if (result == STATUS_SUCCESS && !kernel_alive)
        {
            send_log_window_message("Initializing Subaru Denso CAN kernel upload, please wait...", true, true);
            result = upload_kernel_subaru_denso_subarucan(kernel, ecuCalDef->KernelStartAddr.toUInt(&ok, 16));
        }
        if (result == STATUS_SUCCESS)
        {
            if (cmd_type == "read")
            {
                send_log_window_message("Reading ROM from Subaru Denso using CAN", true, true);
                result = read_mem_subaru_denso_subarucan(ecuCalDef, flashdevices[mcu_type_index].fblocks[0].start, flashdevices[mcu_type_index].romsize);
            }
            else if (cmd_type == "test_write" || cmd_type == "write")
            {
                send_log_window_message("Writing ROM to Subaru Denso using CAN (beta status)", true, true);
                result = write_mem_subaru_denso_subarucan(ecuCalDef, test_write);
            }
        }
    }
    else if (tcuAction == 2)
    {
        send_log_window_message("Commencing TCU relearn process, please wait...", true, true);
        result = tcu_relearn_subaru_ssm();

    }
    else if (tcuAction == 3)
    {
        send_log_window_message("Commencing to read TCU parameters, please wait...", true, true);
        result = tcu_readparam_subaru_ssm();
    }
    else if (tcuAction == 4)
    {
        send_log_window_message("Commencing to set TCU parameters, please wait...", true, true);
        result = tcu_setparam_subaru_ssm();
    }

    return result;
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

    send_log_window_message("Changing to K-Line comms...", true, true);
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
        send_log_window_message("ERROR: Serial port is not open.", true, true);
        return STATUS_ERROR;
    }
    send_log_window_message("Change to K-Line comms successful...", true, true);

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

    send_log_window_message("Setting TCU parameters...", true, true);

    output.append((uint8_t)0xB8);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x01);
    output.append((uint8_t)0x6c);  // 0x16c - IC Correction, 3->4
    output.append((uint8_t)(correction_3to4 & 0xff));
    serial->write_serial_data_echo_check(add_ssm_header(output, tester_id, target_id, false));
    send_log_window_message("Sent: " + parse_message_to_hex(add_ssm_header(output, tester_id, target_id, false)), true, true);
    received = serial->read_serial_data(8, receive_timeout);
    send_log_window_message("Received:" + parse_message_to_hex(received), true, true);
    if ((uint8_t)received[4] != 0xF8)
        return STATUS_ERROR;

    output[3] = ((uint8_t)0x6d);  // 0x16d - HLRC Correction, 2->3
    output[4] = ((uint8_t)(correction_2to3 & 0xff));
    serial->write_serial_data_echo_check(add_ssm_header(output, tester_id, target_id, false));
    send_log_window_message("Sent: " + parse_message_to_hex(add_ssm_header(output, tester_id, target_id, false)), true, true);
    received = serial->read_serial_data(8, receive_timeout);
    send_log_window_message("Received:" + parse_message_to_hex(received), true, true);
    if ((uint8_t)received[4] != 0xF8)
        return STATUS_ERROR;

    output[3] = ((uint8_t)0x6e);  // 0x16e - DC Correction, 1->2
    output[4] = ((uint8_t)(correction_1to2 & 0xff));
    serial->write_serial_data_echo_check(add_ssm_header(output, tester_id, target_id, false));
    send_log_window_message("Sent: " + parse_message_to_hex(add_ssm_header(output, tester_id, target_id, false)), true, true);
    received = serial->read_serial_data(8, receive_timeout);
    send_log_window_message("Received:" + parse_message_to_hex(received), true, true);
    if ((uint8_t)received[4] != 0xF8)
        return STATUS_ERROR;

    output[3] = ((uint8_t)0x6f);  // 0x16f - FB correction
    output[4] = ((uint8_t)(correction_4to5 & 0xff));
    serial->write_serial_data_echo_check(add_ssm_header(output, tester_id, target_id, false));
    send_log_window_message("Sent: " + parse_message_to_hex(add_ssm_header(output, tester_id, target_id, false)), true, true);
    received = serial->read_serial_data(8, receive_timeout);
    send_log_window_message("Received:" + parse_message_to_hex(received), true, true);
    if ((uint8_t)received[4] != 0xF8)
        return STATUS_ERROR;

    output[3] = ((uint8_t)0x70);  // 0x170 - AWD Clutch Torque High
    output[4] = ((uint8_t)((torque_correction_awd >> 8) & 0xff));
    serial->write_serial_data_echo_check(add_ssm_header(output, tester_id, target_id, false));
    send_log_window_message("Sent: " + parse_message_to_hex(add_ssm_header(output, tester_id, target_id, false)), true, true);
    received = serial->read_serial_data(8, receive_timeout);
    send_log_window_message("Received:" + parse_message_to_hex(received), true, true);
    if ((uint8_t)received[4] != 0xF8)
        return STATUS_ERROR;

    output[3] = ((uint8_t)0x71);  // 0x171 - AWD Clutch Torque Low
    output[4] = ((uint8_t)(torque_correction_awd & 0xff));
    serial->write_serial_data_echo_check(add_ssm_header(output, tester_id, target_id, false));
    send_log_window_message("Sent: " + parse_message_to_hex(add_ssm_header(output, tester_id, target_id, false)), true, true);
    received = serial->read_serial_data(8, receive_timeout);
    send_log_window_message("Received:" + parse_message_to_hex(received), true, true);
    if ((uint8_t)received[4] != 0xF8)
        return STATUS_ERROR;

    output[3] = ((uint8_t)0xbc);  // 0x1bc - FwdB correction
    output[4] = ((uint8_t)(correction_fwdb & 0xff));
    serial->write_serial_data_echo_check(add_ssm_header(output, tester_id, target_id, false));
    send_log_window_message("Sent: " + parse_message_to_hex(add_ssm_header(output, tester_id, target_id, false)), true, true);
    received = serial->read_serial_data(8, receive_timeout);
    send_log_window_message("Received:" + parse_message_to_hex(received), true, true);
    if ((uint8_t)received[4] != 0xF8)
        return STATUS_ERROR;

    output[3] = ((uint8_t)0xbd);  // 0x1bd - 4WD correction
    output[4] = ((uint8_t)(correction_4wd & 0xff));
    serial->write_serial_data_echo_check(add_ssm_header(output, tester_id, target_id, false));
    send_log_window_message("Sent: " + parse_message_to_hex(add_ssm_header(output, tester_id, target_id, false)), true, true);
    received = serial->read_serial_data(8, receive_timeout);
    send_log_window_message("Received:" + parse_message_to_hex(received), true, true);
    if ((uint8_t)received[4] != 0xF8)
        return STATUS_ERROR;

    output[3] = ((uint8_t)0xbe);  // 0x1be - PL correction
    output[4] = ((uint8_t)(correction_pl & 0xff));
    serial->write_serial_data_echo_check(add_ssm_header(output, tester_id, target_id, false));
    send_log_window_message("Sent: " + parse_message_to_hex(add_ssm_header(output, tester_id, target_id, false)), true, true);
    received = serial->read_serial_data(8, receive_timeout);
    send_log_window_message("Received:" + parse_message_to_hex(received), true, true);
    if ((uint8_t)received[4] != 0xF8)
        return STATUS_ERROR;

    output[3] = ((uint8_t)0xbf);  // 0x1bf - Temp basis
    output[4] = ((uint8_t)(temp_basis & 0xff));
    serial->write_serial_data_echo_check(add_ssm_header(output, tester_id, target_id, false));
    send_log_window_message("Sent: " + parse_message_to_hex(add_ssm_header(output, tester_id, target_id, false)), true, true);
    received = serial->read_serial_data(8, receive_timeout);
    send_log_window_message("Received:" + parse_message_to_hex(received), true, true);
    if ((uint8_t)received[4] != 0xF8)
        return STATUS_ERROR;

    send_log_window_message("Saving to TCU...", true, true);
    output[2] = ((uint8_t)0x00);
    output[3] = ((uint8_t)0xec);  // 0xec
    output[4] = ((uint8_t)0x55);
    serial->write_serial_data_echo_check(add_ssm_header(output, tester_id, target_id, false));
    send_log_window_message("Sent: " + parse_message_to_hex(add_ssm_header(output, tester_id, target_id, false)), true, true);
    received = serial->read_serial_data(8, receive_timeout);
    send_log_window_message("Received:" + parse_message_to_hex(received), true, true);
    if ((uint8_t)received[4] != 0xF8)
        return STATUS_ERROR;

    output[4] = ((uint8_t)0xaa);
    serial->write_serial_data_echo_check(add_ssm_header(output, tester_id, target_id, false));
    send_log_window_message("Sent: " + parse_message_to_hex(add_ssm_header(output, tester_id, target_id, false)), true, true);
    received = serial->read_serial_data(8, receive_timeout);
    send_log_window_message("Received:" + parse_message_to_hex(received), true, true);
    if ((uint8_t)received[4] != 0xF8)
        return STATUS_ERROR;

    send_log_window_message("Changing back to CAN comms...", true, true);
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
        send_log_window_message("ERROR: Serial port is not open.", true, true);
        return STATUS_ERROR;
    }

    serial->set_add_iso14230_header(false);

    send_log_window_message("Reading TCU parameters...", true, true);
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
        send_log_window_message("Sent: " + parse_message_to_hex(output), true, true);
        //delay(50);
        received = serial->read_serial_data(20, 200);
        if (received != "")
        {
            responseOK = true;
            send_log_window_message(QString::number(try_count) + ": 0xA8 response: " + parse_message_to_hex(received), true, true);
        }
        try_count++;
        //delay(try_timeout);
    }

    if (!responseOK)
        return STATUS_ERROR;
    if ((uint8_t)received[4] != 0xE8)
        return STATUS_ERROR;

    uint16_t torque_correction_awd = ((uint8_t)received[9]) << 8 + (uint8_t)received[10];

    send_log_window_message("Input Clutch Pressure Correction (raw byte): " + QString::number((uint8_t)received[5]), true, true);
    send_log_window_message("High Low Reverse Clutch Pressure Correction (raw byte): " + QString::number((uint8_t)received[6]), true, true);
    send_log_window_message("Direct Clutch Pressure Correction (raw byte): " + QString::number((uint8_t)received[7]), true, true);
    send_log_window_message("Front Brake Pressure Correction (raw byte): " + QString::number((uint8_t)received[8]), true, true);
    send_log_window_message("Correction of AWD Clutch Torque (raw word): " + QString::number((uint16_t)torque_correction_awd), true, true);
    send_log_window_message("Forward Brake Pressure Correction (raw byte): " + QString::number((uint8_t)received[11]), true, true);
    send_log_window_message("4WD Pressure Correction (raw byte): " + QString::number((uint8_t)received[12]), true, true);
    send_log_window_message("Line Pressure Correction (raw byte): " + QString::number((uint8_t)received[13]), true, true);
    send_log_window_message("Temperature basis for above Pressure Corrections (raw byte): " + QString::number((uint8_t)received[14]), true, true);

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
        send_log_window_message("ERROR: Serial port is not open.", true, true);
        return STATUS_ERROR;
    }

    serial->set_add_iso14230_header(false);

    QMessageBox::information(this, tr("TCU Relearn"), "Engine must be at operating temperature. Car must be off the ground! Start with Engine off, Ignition on, stick in P, press OK to continue");

    send_log_window_message("Initialising TCU relearn, step 1...", true, true);
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
        send_log_window_message("Sent: " + parse_message_to_hex(output), true, true);
        //delay(50);
        received = serial->read_serial_data(20, 200);
        if ((received != "") && (received[4] == 0xF8))
        {
            responseOK = true;
            send_log_window_message(QString::number(try_count) + ": 0xB8 response: " + parse_message_to_hex(received), true, true);
        }
        try_count++;
        //delay(try_timeout);
    }
    //if (!responseOK)
    //    return STATUS_ERROR;
    //if ((uint8_t)received[4] != 0xF8)
    //    return STATUS_ERROR;

    send_log_window_message("Initialising TCU relearn, step 2......", true, true);
    output[7] = ((uint8_t)0xFD);
    output[8] = ((uint8_t)0x09);
    try_count = 0;
    responseOK = false;
    while (try_count < 6 && responseOK == false)
    {
        serial->write_serial_data_echo_check(output);
        send_log_window_message("Sent: " + parse_message_to_hex(output), true, true);
        //delay(50);
        received = serial->read_serial_data(20, 200);
        if (received != "")
        {
            responseOK = true;
            send_log_window_message(QString::number(try_count) + ": 0xB8 response: " + parse_message_to_hex(received), true, true);
        }
        try_count++;
        //delay(try_timeout);
    }
    //if (!responseOK)
    //    return STATUS_ERROR;
    //if ((uint8_t)received[4] != 0xF8)
    //    return STATUS_ERROR;

    QMessageBox::information(this, tr("TCU Relearn"), "Start Engine, let revs settle, move stick into D, fully press brake, press OK to continue");

    send_log_window_message("Tracking relearn status......", true, true);
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
        send_log_window_message("Sent: " + parse_message_to_hex(output), true, true);
        //delay(50);
        received = serial->read_serial_data(20, 200);
        if (received != "")
        {
            //responseOK = true;
            send_log_window_message(QString::number(try_count) + ": 0xA8 response: " + parse_message_to_hex(received), true, true);
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
int FlashTcuSubaruDensoSH705xCan::connect_bootloader_subaru_denso_subarucan()
{
    QByteArray output;
    QByteArray received;
    QByteArray seed;
    QByteArray seed_key;
    QByteArray msg;

    if (!serial->is_serial_port_open())
    {
        send_log_window_message("ERROR: Serial port is not open.", true, true);
        return STATUS_ERROR;
    }

    serial->set_add_iso14230_header(false);

    //if (connect_bootloader_start_countdown(bootloader_start_countdown))
    //    return STATUS_ERROR;

    send_log_window_message("Checking if kernel is already running...", true, true);
    qDebug() << "Checking if kernel is already running...";

    // Check if kernel already alive
    output.clear();
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x07);
    output.append((uint8_t)0xE1);
    output.append((uint8_t)(SID_START_COMM_CAN & 0xFF));
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
        send_log_window_message("Kernel already running", true, true);

        kernel_alive = true;
        return STATUS_SUCCESS;
    }

    send_log_window_message("Trying TCU Init...", true, true);
    qDebug() << "Trying TCU Init...";

    output.clear();
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x07);
    output.append((uint8_t)0xE1);
    output.append((uint8_t)0xAA);

    bool connected = false;
    int try_count = 0;

    while (try_count < 6 && connected == false)
    {
        serial->write_serial_data_echo_check(output);
        send_log_window_message("Sent: " + parse_message_to_hex(output), true, true);
        qDebug() << "Sent:" << parse_message_to_hex(output);
        //delay(50);
        received = serial->read_serial_data(20, 200);
        if (received != "")
        {
            connected = true;
            send_log_window_message(QString::number(try_count) + ": 0xAA response: " + parse_message_to_hex(received), true, true);
            qDebug() << try_count << ": 0xAA response:" << parse_message_to_hex(received) << received;
        }
        try_count++;
        //delay(try_timeout);
    }

    QByteArray response = received;
    response.remove(0, 8);
    QString calid;
    for (int i = 0; i < 5; i++)
        calid.append(QString("%1").arg((uint8_t)response.at(i),2,16,QLatin1Char('0')).toUpper());
    send_log_window_message("Init Success: CAL ID = " + calid, true, true);

    connected = false;
    try_count = 0;

    send_log_window_message("Trying 0x09 0x04...", true, true);
    qDebug() << "Trying 0x09 0x04...";

    output[4] = (uint8_t)0x09;
    output[5] = (uint8_t)0x04;

    while (try_count < 6 && connected == false)
    {
        serial->write_serial_data_echo_check(output);
        send_log_window_message("Sent: " + parse_message_to_hex(output), true, true);
        qDebug() << "Sent:" << parse_message_to_hex(output);
        //delay(50);
        received = serial->read_serial_data(20, 200);
        if (received != "")
        {
            connected = true;
            send_log_window_message(QString::number(try_count) + ": 0x09 0x04 response: " + parse_message_to_hex(received), true, true);
            qDebug() << try_count << ": 0x09 0x04 response:" << parse_message_to_hex(received) << received;
        }
        try_count++;
        //delay(try_timeout);
    }

    response = received;
    response.remove(0, 7);
    response.remove(8, 8);
    QString tcuid = QString::fromUtf8(response);
    send_log_window_message("Init Success: TCU ID = " + tcuid, true, true);

    send_log_window_message("Initializing bootloader...", true, true);
    qDebug() << "Initializing bootloader...";

    connected = false;
    try_count = 0;
    output[4] = ((uint8_t)0x10);
    output[5] = ((uint8_t)0x03);
    while (try_count < 6 && connected == false)
    {
        serial->write_serial_data_echo_check(output);
        send_log_window_message("Sent: " + parse_message_to_hex(output), true, true);
        qDebug() << "Sent:" << parse_message_to_hex(output);
        //delay(50);
        received = serial->read_serial_data(20, 10);
        if (received != "")
        {
            connected = true;
            send_log_window_message(QString::number(try_count) + ": 0x10 0x03 response: " + parse_message_to_hex(received), true, true);
            qDebug() << try_count << ": 0x10 0x03 response:" << parse_message_to_hex(received);
        }
        try_count++;
        //delay(try_timeout);
    }
    if (received == "" || (uint8_t)received.at(4) != 0x50 || (uint8_t)received.at(5) != 0x03)
        return STATUS_ERROR;

    /*
    connected = false;
    try_count = 0;
    output[5] = ((uint8_t)0x43);
    while (try_count < 6 && connected == false)
    {
        serial->write_serial_data_echo_check(output);
        send_log_window_message("Sent: " + parse_message_to_hex(output), true, true);
        qDebug() << "Sent:" << parse_message_to_hex(output);
        //delay(50);
        received = serial->read_serial_data(20, 10);
        if (received != "")
        {
            connected = true;
            send_log_window_message(QString::number(try_count) + ": 0x10 0x43 response: " + parse_message_to_hex(received), true, true);
            qDebug() << try_count << ": 0x10 0x43 response:" << parse_message_to_hex(received);
        }
        try_count++;
        delay(try_timeout);
    }
    if (received == "" || (uint8_t)received.at(4) != 0x50 || (uint8_t)received.at(5) != 0x43)
        return STATUS_ERROR;
    */

    connected = false;
    try_count = 0;
    output[4] = ((uint8_t)0x27);
    output[5] = ((uint8_t)0x01);
    while (try_count < 6 && connected == false)
    {
        serial->write_serial_data_echo_check(output);
        send_log_window_message("Sent: " + parse_message_to_hex(output), true, true);
        qDebug() << "Sent:" << parse_message_to_hex(output);
        //delay(50);
        received = serial->read_serial_data(20, 10);

        if (received != "")
        {
            connected = true;
            send_log_window_message(QString::number(try_count) + ": 0x27 0x01 response: " + parse_message_to_hex(received), true, true);
            qDebug() << try_count << ": 0x27 0x01 response:" << parse_message_to_hex(received);
        }
        try_count++;
        //delay(try_timeout);
    }
    if (received == "" || (uint8_t)received.at(4) != 0x67 || (uint8_t)received.at(5) != 0x01)
        return STATUS_ERROR;

    send_log_window_message("Seed request ok", true, true);
    qDebug() << "Seed request ok";

    seed.append(received.at(6));
    seed.append(received.at(7));
    seed.append(received.at(8));
    seed.append(received.at(9));

    seed_key = subaru_denso_generate_can_seed_key(seed);
    send_log_window_message("Calculated seed key: " + parse_message_to_hex(seed_key), true, true);
    qDebug() << "Calculated seed key:" << parse_message_to_hex(seed_key);
    send_log_window_message("Sending seed key", true, true);
    qDebug() << "Sending seed key";

    connected = false;
    try_count = 0;
    output[4] = ((uint8_t)0x27);
    output[5] = ((uint8_t)0x02);
    output.append(seed_key);
    while (try_count < 6 && connected == false)
    {
        serial->write_serial_data_echo_check(output);
        send_log_window_message("Sent: " + parse_message_to_hex(output), true, true);
        qDebug() << "Sent:" << parse_message_to_hex(output);
        //delay(50);
        received = serial->read_serial_data(20, 10);
        if (received != "")
        {
            connected = true;
            send_log_window_message(QString::number(try_count) + ": 0x27 0x02 response: " + parse_message_to_hex(received), true, true);
            qDebug() << try_count << ": 0x27 0x02 response:" << parse_message_to_hex(received);
        }
        try_count++;
        //delay(try_timeout);
    }
    if (received == "" || (uint8_t)received.at(4) != 0x67 || (uint8_t)received.at(5) != 0x02)
        return STATUS_ERROR;

    send_log_window_message("Seed key ok", true, true);
    qDebug() << "Seed key ok";


    connected = false;
    try_count = 0;
    output.clear();
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x07);
    output.append((uint8_t)0xE1);
    output.append((uint8_t)0x10);
    output.append((uint8_t)0x02);
    while (try_count < 6 && connected == false)
    {
        serial->write_serial_data_echo_check(output);
        send_log_window_message("Sent: " + parse_message_to_hex(output), true, true);
        qDebug() << "Sent:" << parse_message_to_hex(output);
        //delay(50);
        received = serial->read_serial_data(20, 10);
        if (received != "")
        {
            connected = true;
            send_log_window_message(QString::number(try_count) + ": 0x10 0x02 response: " + parse_message_to_hex(received), true, true);
            qDebug() << try_count << ": 0x10 0x02 response:" << parse_message_to_hex(received);
        }
        try_count++;
        //delay(try_timeout);
    }
    if (received == "" || (uint8_t)received.at(4) != 0x50)
        return STATUS_ERROR;

    return STATUS_SUCCESS;
}

/*
 * Upload kernel to Subaru Denso CAN (iso15765) 32bit ECUs
 *
 * @return success
 */
int FlashTcuSubaruDensoSH705xCan::upload_kernel_subaru_denso_subarucan(QString kernel, uint32_t kernel_start_addr)
{
    QFile file(kernel);

    QByteArray output;
    QByteArray payload;
    QByteArray received;
    QByteArray msg;
    QByteArray pl_encr;
    uint32_t file_len = 0;
    uint32_t pl_len = 0;
    uint32_t start_address = 0;
    uint32_t end_addr = 0;
    QByteArray cks_bypass;
    uint32_t chk_sum = 0;
    uint32_t blockaddr = 0;
    uint16_t blockno = 0;
    uint16_t maxblocks = 0;

    QString mcu_name;

    start_address = kernel_start_addr;//flashdevices[mcu_type_index].kblocks->start;
    qDebug() << "Start address to upload kernel:" << hex << start_address;

    if (!serial->is_serial_port_open())
    {
        send_log_window_message("ERROR: Serial port is not open.", true, true);
        return STATUS_ERROR;
    }

    serial->set_add_iso14230_header(false);

    // Check kernel file
    if (!file.open(QIODevice::ReadOnly ))
    {
        send_log_window_message("Unable to open kernel file for reading", true, true);
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
    pl_encr = subaru_denso_encrypt_32bit_payload(pl_encr, pl_encr.length());
    //pl_encr = subaru_denso_decrypt_32bit_payload(pl_encr, pl_encr.length());
    //send_log_window_message("Kernel data: " + parse_message_to_hex(pl_encr), true, true);
    //qDebug() << "Kernel checksum" << hex << chk_sum;

    set_progressbar_value(0);

    bool connected = false;
    int try_count = 0;
    int try_timeout = 250;

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
    while (try_count < 6 && connected == false)
    {
        serial->write_serial_data_echo_check(output);
        send_log_window_message("Sent: " + parse_message_to_hex(output), true, true);
        qDebug() << "Sent:" << parse_message_to_hex(output);
        //delay(50);
        received = serial->read_serial_data(20, 10);
        if (received != "")
        {
            connected = true;
            send_log_window_message(QString::number(try_count) + ": 0x34 0x04 response: " + parse_message_to_hex(received), true, true);
            qDebug() << try_count << ": 0x34 0x04 response:" << parse_message_to_hex(received);
        }
        try_count++;
        //delay(try_timeout);
    }
    if (received == "" || (uint8_t)received.at(4) != 0x74)
        return STATUS_ERROR;

    int data_bytes_sent = 0;
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
        //qDebug() << "Data header:" << parse_message_to_hex(output);

        if (blockno == maxblocks)
        {
            for (uint32_t i = 0; i < data_len; i++)
            {
                output.append(pl_encr.at(i + blockno * 128));
                data_bytes_sent++;
            }
        }
        else
        {
            for (int i = 0; i < 128; i++)
            {
                output.append(pl_encr.at(i + blockno * 128));
                data_bytes_sent++;
            }
            data_len -= 128;
        }

        serial->write_serial_data_echo_check(output);
        send_log_window_message("Sent: " + parse_message_to_hex(output), true, true);
        //qDebug() << "Kernel data:" << parse_message_to_hex(output);
        //delay(20);
        received = serial->read_serial_data(5, receive_timeout);

        float pleft = (float)blockno / (float)maxblocks * 100;
        set_progressbar_value(pleft);
    }
    qDebug() << "Data bytes sent:" << hex << data_bytes_sent;

    connected = false;
    try_count = 0;
    output.clear();
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x07);
    output.append((uint8_t)0xE1);
    output.append((uint8_t)0x37);

    while (try_count < 6 && connected == false)
    {
        serial->write_serial_data_echo_check(output);
        send_log_window_message("Sent: " + parse_message_to_hex(output), true, true);
        qDebug() << "Sent:" << parse_message_to_hex(output);
        //delay(50);
        received = serial->read_serial_data(20, 10);
        if (received != "")
        {
            connected = true;
            send_log_window_message(QString::number(try_count) + ": 0x37 response: " + parse_message_to_hex(received), true, true);
            qDebug() << try_count << ": 0x37 response:" << parse_message_to_hex(received);
        }
        try_count++;
        //delay(try_timeout);
    }
    if (received == "" || (uint8_t)received.at(4) != 0x77)
        return STATUS_ERROR;

    connected = false;
    try_count = 0;
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

    while (try_count < 6 && connected == false)
    {
        serial->write_serial_data_echo_check(output);
        send_log_window_message("Sent: " + parse_message_to_hex(output), true, true);
        qDebug() << "Sent:" << parse_message_to_hex(output);
        //delay(50);
        received = serial->read_serial_data(20, 10);
        if (received != "")
        {
            connected = true;
            send_log_window_message(QString::number(try_count) + ": 0x31 response: " + parse_message_to_hex(received), true, true);
            qDebug() << try_count << ": 0x31 response:" << parse_message_to_hex(received);
        }
        try_count++;
        //delay(try_timeout);
    }
    if (received == "" || (uint8_t)received.at(4) != 0x71)
        return STATUS_ERROR;

    set_progressbar_value(100);

    send_log_window_message("Kernel started, initializing...", true, true);
    qDebug() << "Kernel started, initializing...";

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

    /*
    serial->set_is_can_connection(true);
    serial->set_is_iso15765_connection(false);
    serial->set_is_29_bit_id(true);
    serial->set_can_source_address(0x7E1);
    serial->set_can_destination_address(0x7E9);

    delay(100);
    */

    send_log_window_message("Requesting kernel ID", true, true);
    qDebug() << "Requesting kernel ID";

    received.clear();
    received = request_kernel_id();
    if (received == "")
        return STATUS_ERROR;

    send_log_window_message("Kernel ID: " + received, true, true);
    qDebug() << "Kernel ID:" << parse_message_to_hex(received);


    return STATUS_SUCCESS;
}



/*
 * Read memory from Subaru Denso CAN (iso15765) 32bit TCUs, nisprog kernel
 *
 * @return success
 */
int FlashTcuSubaruDensoSH705xCan::read_mem_subaru_denso_subarucan(FileActions::EcuCalDefStructure *ecuCalDef, uint32_t start_addr, uint32_t length)
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

    send_log_window_message("Start reading ROM, please wait..." + received, true, true);
    qDebug() << "Start reading ROM, please wait...";

    // send 0xD8 command to kernel to dump the chunk from ROM
    output.clear();
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x07);
    output.append((uint8_t)0xE1);
    output.append((uint8_t)SID_START_COMM_CAN);
    output.append((uint8_t)(SID_DUMP_ROM_CAN + 0x06));
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

        //length = 256;

        output[6] = (uint8_t)((pagesize >> 24) & 0xFF);
        output[7] = (uint8_t)((pagesize >> 16) & 0xFF);
        output[8] = (uint8_t)((pagesize >> 8) & 0xFF);
        output[9] = (uint8_t)((addr >> 24) & 0xFF);
        output[10] = (uint8_t)((addr >> 16) & 0xFF);
        output[11] = (uint8_t)((addr >> 8) & 0xFF);
        serial->write_serial_data_echo_check(output);
        //qDebug() << "0xD8 message sent to kernel initiate dump";
        //delay(100);
        received = serial->read_serial_data(1, 10);
        //qDebug() << "Response to 0xD8 (dump mem) message:" << parse_message_to_hex(received);

        if ((uint8_t)received.at(0) != SID_START_COMM_CAN || ((uint8_t)received.at(1) & 0xF8) != SID_DUMP_ROM_CAN)
        {
            send_log_window_message("Page data request failed!", true, true);
            send_log_window_message("Received msg: " + parse_message_to_hex(received), true, true);
            //return STATUS_ERROR;
        }

        timeout = 0;
        pagedata.clear();

        while ((uint32_t)pagedata.length() < pagesize && timeout < 1000)
        {
            received = serial->read_serial_data(1, 50);
            pagedata.append(received, 8);
            timeout++;
            //qDebug() << parse_message_to_hex(received);
        }
        if (timeout >= 1000)
        {
            send_log_window_message("Page data timeout!", true, true);
            //return STATUS_ERROR;
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
        send_log_window_message(msg, true, true);
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

    send_log_window_message("ROM read ready" + received, true, true);
    qDebug() << "ROM read ready";

    ecuCalDef->FullRomData = mapdata;
    set_progressbar_value(100);

    return STATUS_SUCCESS;
}

/*
 * Write memory to Subaru Denso CAN (iso15765) 32bit ECUs, nisprog kernel
 *
 * @return success
 */
int FlashTcuSubaruDensoSH705xCan::write_mem_subaru_denso_subarucan(FileActions::EcuCalDefStructure *ecuCalDef, bool test_write)
{
    QByteArray filedata;

    filedata = ecuCalDef->FullRomData;

    uint8_t data_array[filedata.length()];

    int block_modified[16] = {0};

    unsigned bcnt = 0;
    unsigned blockno;

    for (int i = 0; i < filedata.length(); i++)
    {
        data_array[i] = filedata.at(i);
    }

    send_log_window_message("--- comparing ECU flash memory pages to image file ---", true, true);
    send_log_window_message("seg\tstart\tlen\tsame?", true, true);

    if (get_changed_blocks_denso_subarucan(data_array, block_modified))
    {
        send_log_window_message("Error in ROM compare", true, true);
        return STATUS_ERROR;
    }

    bcnt = 0;
    send_log_window_message("Different blocks : ", true, false);
    for (blockno = 0; blockno < flashdevices[mcu_type_index].numblocks; blockno++) {
        if (block_modified[blockno]) {
            send_log_window_message(QString::number(blockno) + ", ", false, false);
            bcnt += 1;
        }
    }
    send_log_window_message(" (total: " + QString::number(bcnt) + ")", false, true);

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

        send_log_window_message("--- start writing ROM file to ECU flash memory ---", true, true);
        for (blockno = 0; blockno < flashdevices[mcu_type_index].numblocks; blockno++)
        {
            if (block_modified[blockno])
            {
                if (reflash_block_denso_subarucan(&data_array[flashdevices[mcu_type_index].fblocks->start], &flashdevices[mcu_type_index], blockno, test_write))
                {
                    send_log_window_message("Block " + QString::number(blockno) + " reflash failed.", true, true);
                    return STATUS_ERROR;
                }
                else
                {
                    flashbytesindex += flashdevices[mcu_type_index].fblocks[blockno].len;
                    send_log_window_message("Block " + QString::number(blockno) + " reflash complete.", true, true);
                }
            }
        }

        send_log_window_message("--- comparing ECU flash memory pages to image file after reflash ---", true, true);
        send_log_window_message("seg\tstart\tlen\tsame?", true, true);

        if (get_changed_blocks_denso_subarucan(data_array, block_modified))
        {
            send_log_window_message("Error in ROM compare", true, true);
            return STATUS_ERROR;
        }

        bcnt = 0;
        send_log_window_message("Different blocks : ", true, false);
        for (blockno = 0; blockno < flashdevices[mcu_type_index].numblocks; blockno++) {
            if (block_modified[blockno])
            {
                send_log_window_message(QString::number(blockno) + ", ", false, false);
                bcnt += 1;
            }
        }
        send_log_window_message(" (total: " + QString::number(bcnt) + ")", false, true);
        if (!test_write)
        {
            if (bcnt)
            {
                send_log_window_message("*** ERROR IN FLASH PROCESS ***", true, true);
                send_log_window_message("Don't power off your ECU, kernel is still running and you can try flashing again!", true, true);
            }
        }
        else
            send_log_window_message("*** Test write PASS, it's ok to perform actual write! ***", true, true);
    }
    else
    {
        send_log_window_message("*** Compare results no difference between ROM and ECU data, no flashing needed! ***", true, true);
    }

    return STATUS_SUCCESS;
}

/*
 * Compare ROM 32bit (iso15765) CAN ECUs
 *
 * @return
 */
int FlashTcuSubaruDensoSH705xCan::get_changed_blocks_denso_subarucan(const uint8_t *src, int *modified)
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
        //qDebug() << msg;
        send_log_window_message(msg, true, false);
        // do CRC comparison with ECU //
        if (check_romcrc_denso_subarucan(&src[bs], bs, blen, &modified[blockno])) {
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
int FlashTcuSubaruDensoSH705xCan::check_romcrc_denso_subarucan(const uint8_t *src, uint32_t start_addr, uint32_t len, int *modified)
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
    output.append((uint8_t)SID_START_COMM_CAN);
    output.append((uint8_t)(SID_CONF_CKS1_CAN + 0x06));
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
        //qDebug() << "Send req:" << parse_message_to_hex(output);
        start_addr += pagesize;

        received = serial->write_serial_data_echo_check(output);
        //delay(100);
        received = serial->read_serial_data(1, serial_read_short_timeout);
        //received.remove(0, 4);
        //qDebug() << "Received:" << parse_message_to_hex(received);

        uint16_t chk_sum = 0;
        for (uint32_t j = 0; j < pagesize; j++) {
            pagedata[j] = src[(byte_index * pagesize) + j];
            chk_sum += (pagedata[j] & 0xFF);
            chk_sum = ((chk_sum >> 8) & 0xFF) + (chk_sum & 0xFF);
        }
        byte_index++;

        //qDebug() << "Checksums: File =" << hex << chk_sum << "ROM =" << hex << (uint8_t)received.at(2);
        if ((uint8_t)received.at(0) != SID_START_COMM_CAN || ((uint8_t)received.at(1) & 0xF8) != SID_CONF_CKS1_CAN || chk_sum == (uint8_t)received.at(2))
            continue;

        send_log_window_message("\tNO", false, true);

        //confirmed bad CRC, we can exit
        *modified = 1;

        return 0;
    }   //for

    send_log_window_message("\tYES", false, true);
    *modified = 0;
    serial->read_serial_data(100, serial_read_short_timeout);
    return 0;
}

/*
 * Reflash ROM 32bit CAN (iso15765) ECUs
 *
 * @return success
 */
int FlashTcuSubaruDensoSH705xCan::reflash_block_denso_subarucan(const uint8_t *newdata, const struct flashdev_t *fdt, unsigned blockno, bool test_write)
{
    int errval;

    uint32_t block_start;
    uint32_t block_len;

    QByteArray output;
    QByteArray received;
    QByteArray msg;

    set_progressbar_value(0);

    if (blockno >= fdt->numblocks) {
        send_log_window_message("block " + QString::number(blockno) + " out of range !", true, true);
        return -1;
    }

    block_start = fdt->fblocks[blockno].start;
    block_len = fdt->fblocks[blockno].len;

    QString start_addr = QString("%1").arg((uint32_t)block_start,8,16,QLatin1Char('0')).toUpper();
    QString length = QString("%1").arg((uint32_t)block_len,8,16,QLatin1Char('0')).toUpper();
    msg = QString("Flash block addr: 0x" + start_addr + " len: 0x" + length).toUtf8();
    send_log_window_message(msg, true, true);

    output.clear();
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x07);
    output.append((uint8_t)0xE1);
    output.append((uint8_t)SID_START_COMM_CAN);
    output.append((uint8_t)(SID_FLASH_CAN + 0x01));
    if (test_write)
        output.append((uint8_t)SIDFL_PROTECT_CAN);
    else
        output.append((uint8_t)SIDFL_UNPROTECT_CAN);
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

    if((uint8_t)received.at(0) != SID_START_COMM_CAN || ((uint8_t)received.at(1) & 0xF8) != SID_FLASH_CAN)
    {
        qDebug() << "Initialize of erasing / flashing microcodes failed!";
        return STATUS_ERROR;
    }


    int num_128_byte_blocks = (block_len >> 7) & 0xFFFFFFFF;

    qDebug() << "Proceeding to attempt erase and flash of block number: " << blockno;
    output[5] = (uint8_t)(SIDFL_EB_CAN + 0x06);
    output[6] = (uint8_t)(blockno & 0xFF);
    output[7] = (uint8_t)((block_start >> 24) & 0xFF);
    output[8] = (uint8_t)((block_start >> 16) & 0xFF);
    output[9] = (uint8_t)((block_start >> 8) & 0xFF);
    output[10] = (uint8_t)((num_128_byte_blocks >> 8) & 0xFF);
    output[11] = (uint8_t)(num_128_byte_blocks & 0xFF);
    received = serial->write_serial_data_echo_check(output);
    qDebug() << parse_message_to_hex(output);
    //send_log_window_message("0xF0 message sent to kernel to erase block number: " + QString::number(blockno), true, true);
    qDebug() << "0xF0 message sent to kernel to erase block number: " << blockno;
    delay(500);

    QTime dieTime = QTime::currentTime().addMSecs(serial_read_extra_long_timeout);
    while ((uint32_t)received.length() < 3 && (QTime::currentTime() < dieTime))
    {
        received = serial->read_serial_data(3, serial_read_short_timeout);
        QCoreApplication::processEvents(QEventLoop::AllEvents, 1);
        delay(100);
    }

    //send_log_window_message(parse_message_to_hex(received), true, true);
    qDebug() << parse_message_to_hex(received);

    if((uint8_t)received.at(0) != SID_START_COMM_CAN || ((uint8_t)received.at(1) & 0xF8) != SIDFL_EB_CAN)
    {
        qDebug() << "Not ready for 128byte block writing";
        return STATUS_ERROR;
    }

    errval = flash_block_denso_subarucan(newdata, block_start, block_len);
    if (errval) {
        send_log_window_message("Reflash error! Do not panic, do not reset the ECU immediately. The kernel is most likely still running and receiving commands!", true, true);
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
int FlashTcuSubaruDensoSH705xCan::flash_block_denso_subarucan(const uint8_t *src, uint32_t start, uint32_t len)
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

        output[4] = (uint8_t)SID_START_COMM_CAN;
        output[5] = (uint8_t)(SIDFL_WB_CAN + 0x03);
        output[6] = (uint8_t)((i >> 8) & 0xFF);
        output[7] = (uint8_t)(i & 0xFF);
        output[8] = (uint8_t)(chk_sum & 0xFF);
        received = serial->write_serial_data_echo_check(output);

        received = serial->read_serial_data(3, serial_read_long_timeout);
        if((uint8_t)received.at(0) != SID_START_COMM_CAN || ((uint8_t)received.at(1) & 0xF8) != SIDFL_WB_CAN)
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
        msg = QString("writing chunk @ 0x%1 (%2\% - %3 B/s, ~ %4 s remaining)").arg(start_address).arg((unsigned) 100 * (len - remain) / len,1,10,QLatin1Char('0')).arg((uint32_t)curspeed,1,10,QLatin1Char('0')).arg(tleft,1,10,QLatin1Char('0')).toUtf8();
        send_log_window_message(msg, true, true);

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
QByteArray FlashTcuSubaruDensoSH705xCan::subaru_denso_generate_can_seed_key(QByteArray requested_seed)
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

    key = subaru_denso_calculate_seed_key(requested_seed, keytogenerateindex_1, indextransformation);

    return key;
}


/*
 * Calculate denso seed key from received seed bytes
 *
 * @return seed key (4 bytes)
 */
QByteArray FlashTcuSubaruDensoSH705xCan::subaru_denso_calculate_seed_key(QByteArray requested_seed, const uint16_t *keytogenerateindex, const uint8_t *indextransformation)
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
QByteArray FlashTcuSubaruDensoSH705xCan::subaru_denso_encrypt_32bit_payload(QByteArray buf, uint32_t len)
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

    encrypted = subaru_denso_calculate_32bit_payload(buf, len, keytogenerateindex, indextransformation);

    return encrypted;
}

QByteArray FlashTcuSubaruDensoSH705xCan::subaru_denso_decrypt_32bit_payload(QByteArray buf, uint32_t len)
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

    decrypt = subaru_denso_calculate_32bit_payload(buf, len, keytogenerateindex, indextransformation);

    return decrypt;
}

QByteArray FlashTcuSubaruDensoSH705xCan::subaru_denso_calculate_32bit_payload(QByteArray buf, uint32_t len, const uint16_t *keytogenerateindex, const uint8_t *indextransformation)
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
/*
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x0F);
    output.append((uint8_t)0xFF);
    output.append((uint8_t)0xFE);
*/
    output.append((uint8_t)SID_START_COMM_CAN);
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
        send_log_window_message("Kernel ID request: " + parse_message_to_hex(output), true, true);
        qDebug() << "Kernel ID request:" << parse_message_to_hex(output);
        delay(100);
        kernelid.clear();

        received = serial->read_serial_data(100, serial_read_long_timeout);
        send_log_window_message("Kernel ID response: " + parse_message_to_hex(received), true, true);
        qDebug() << "Kernel ID response:" << parse_message_to_hex(received);
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

    //send_log_window_message("Send: " + parse_message_to_hex(output), true, true);
    //qDebug () << "Send:" << parse_message_to_hex(output);
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
 * Countdown prior power on
 *
 * @return
 */
int FlashTcuSubaruDensoSH705xCan::connect_bootloader_start_countdown(int timeout)
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
QString FlashTcuSubaruDensoSH705xCan::parse_message_to_hex(QByteArray received)
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
int FlashTcuSubaruDensoSH705xCan::send_log_window_message(QString message, bool timestamp, bool linefeed)
{
    QDateTime dateTime = dateTime.currentDateTime();
    QString dateTimeString = dateTime.toString("[yyyy-MM-dd hh':'mm':'ss'.'zzz']  ");

    if (timestamp)
        message = dateTimeString + message;
    if (linefeed)
        message = message + "\n";

    QString filename = "log.txt";
    QFile file(filename);
    //QFileInfo fileInfo(file.fileName());
    //QString file_name_str = fileInfo.fileName();

    if (!file.open(QIODevice::WriteOnly | QIODevice::Append ))
    {
        //qDebug() << "Unable to open file for writing";
        QMessageBox::warning(this, tr("Ecu calibration file"), "Unable to open file for writing");
        return NULL;
    }

    file.write(message.toUtf8());
    file.close();

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

void FlashTcuSubaruDensoSH705xCan::set_progressbar_value(int value)
{
    if (ui->progressbar)
        ui->progressbar->setValue(value);
    QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
}

void FlashTcuSubaruDensoSH705xCan::delay(int timeout)
{
    QTime dieTime = QTime::currentTime().addMSecs(timeout);
    while (QTime::currentTime() < dieTime)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 1);
}

