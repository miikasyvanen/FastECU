#include "biu_operations_subaru.h"
#include <ui_biu_operations_subaru.h>

BiuOperationsSubaru::BiuOperationsSubaru(SerialPortActions *serial, QWidget *parent)
    : QDialog(parent),
      ui(new Ui::BiuOperationsSubaruWindow)
{
    ui->setupUi(this);

    ui->progressbar->hide();

    this->serial = serial;

    biuOpsSubaruSwitchesIo = nullptr;
    biuOpsSubaruSwitchesLighting = nullptr;
    biuOpsSubaruSwitchesOptions = nullptr;
    biuOpsSubaruDataDtcs = nullptr;
    biuOpsSubaruDataBiu = nullptr;
    biuOpsSubaruDataCan = nullptr;
    biuOpsSubaruDataTt = nullptr;
    biuOpsSubaruDataVdcabs = nullptr;
    biuOpsSubaruDataDest = nullptr;
    biuOpsSubaruDataFactory = nullptr;

    counter = 0;
    biu_tt_result = new QByteArray();
    biu_option_result = new QByteArray();
    switch_result = new QStringList();
    data_result = new QStringList();
    keep_alive_timer = new QTimer(this);
    keep_alive_timer->setInterval(1000);
    current_command = NO_COMMAND;
    connection_state = NOT_CONNECTED;

    for (int i = 0; i < biu_messages.length(); i+=2)
    {
        ui->msg_combo_box->addItem(biu_messages.at(i));
    }

    connect(ui->send_msg, SIGNAL(clicked(bool)), this, SLOT(parse_biu_cmd()));
    connect(keep_alive_timer, SIGNAL(timeout()), this, SLOT(keep_alive()));

}

BiuOperationsSubaru::~BiuOperationsSubaru()
{
    keep_alive_timer->stop();
    delete ui;
    delete keep_alive_timer;
    delete biu_tt_result;
    delete biu_option_result;
    delete switch_result;
    delete data_result;
    delete biuOpsSubaruSwitchesIo;
    delete biuOpsSubaruSwitchesLighting;
    delete biuOpsSubaruSwitchesOptions;
    delete biuOpsSubaruDataDtcs;
    delete biuOpsSubaruDataBiu;
    delete biuOpsSubaruDataCan;
    delete biuOpsSubaruDataTt;
    delete biuOpsSubaruDataVdcabs;
    delete biuOpsSubaruDataDest;
    delete biuOpsSubaruDataFactory;
    delete biuOpsSubaruInput1;
    delete biuOpsSubaruInput2;
}

BiuOpsSubaruSwitches* BiuOperationsSubaru::update_biu_ops_subaru_switches_window(BiuOpsSubaruSwitches *biuOpsSubaruSwitches)
{
    if (biuOpsSubaruSwitches == nullptr)
    {
        biuOpsSubaruSwitches = new BiuOpsSubaruSwitches(switch_result);
        biuOpsSubaruSwitches->show();
    }
    else
    {
        if (!biuOpsSubaruSwitches->isVisible()) biuOpsSubaruSwitches->show();
        biuOpsSubaruSwitches->update_switch_results(switch_result);
    }

    return biuOpsSubaruSwitches;

}

BiuOpsSubaruData* BiuOperationsSubaru::update_biu_ops_subaru_data_window(BiuOpsSubaruData *biuOpsSubaruData)
{
    if (biuOpsSubaruData == nullptr)
    {
        biuOpsSubaruData = new BiuOpsSubaruData(data_result);
        biuOpsSubaruData->show();
    }
    else
    {
        if (!biuOpsSubaruData->isVisible()) biuOpsSubaruData->show();
        biuOpsSubaruData->update_data_results(data_result);
    }

    return biuOpsSubaruData;

}

void BiuOperationsSubaru::close_results_windows()
{
    if (biuOpsSubaruSwitchesIo != nullptr) biuOpsSubaruSwitchesIo->hide();
    if (biuOpsSubaruSwitchesLighting != nullptr) biuOpsSubaruSwitchesLighting->hide();
    if (biuOpsSubaruSwitchesOptions != nullptr) biuOpsSubaruSwitchesOptions->hide();
    if (biuOpsSubaruDataDtcs != nullptr) biuOpsSubaruDataDtcs->hide();
    if (biuOpsSubaruDataBiu != nullptr) biuOpsSubaruDataBiu->hide();
    if (biuOpsSubaruDataCan != nullptr) biuOpsSubaruDataCan->hide();
    if (biuOpsSubaruDataTt != nullptr) biuOpsSubaruDataTt->hide();
    if (biuOpsSubaruDataVdcabs != nullptr) biuOpsSubaruDataVdcabs->hide();
    if (biuOpsSubaruDataDest != nullptr) biuOpsSubaruDataDest->hide();
    if (biuOpsSubaruDataFactory != nullptr) biuOpsSubaruDataFactory->hide();
}

void BiuOperationsSubaru::closeEvent(QCloseEvent *event)
{
    qDebug() << "Closing BIU log window";
    keep_alive_timer->stop();
    if (biuOpsSubaruSwitchesIo != nullptr) biuOpsSubaruSwitchesIo->close();
    if (biuOpsSubaruSwitchesLighting != nullptr) biuOpsSubaruSwitchesLighting->close();
    if (biuOpsSubaruSwitchesOptions != nullptr) biuOpsSubaruSwitchesOptions->close();
    if (biuOpsSubaruDataDtcs != nullptr) biuOpsSubaruDataDtcs->close();
    if (biuOpsSubaruDataBiu != nullptr) biuOpsSubaruDataBiu->close();
    if (biuOpsSubaruDataCan != nullptr) biuOpsSubaruDataCan->close();
    if (biuOpsSubaruDataTt != nullptr) biuOpsSubaruDataTt->close();
    if (biuOpsSubaruDataVdcabs != nullptr) biuOpsSubaruDataVdcabs->close();
    if (biuOpsSubaruDataDest != nullptr) biuOpsSubaruDataDest->close();
    if (biuOpsSubaruDataFactory != nullptr) biuOpsSubaruDataFactory->close();

}

void BiuOperationsSubaru::keep_alive()
{
    if (current_command == TESTER_PRESENT)
    {
        output.clear();
        output.append((uint8_t)0x81);
        output.append((uint8_t)0x40);
        output.append((uint8_t)0xf0);
        output.append((uint8_t)0x3E);
        output.append((uint8_t)0xEF);
    }

    send_biu_msg();
}

void BiuOperationsSubaru::parse_biu_cmd()
{

    QString selected_item_text = ui->msg_combo_box->currentText();
    QStringList selected_item_msg;

    bool cmd_ready = true;
    bool ok = false;

    if (selected_item_text != "Custom")
    {
        for (int i = 0; i < biu_messages.length(); i+=2)
        {
            if (selected_item_text == biu_messages.at(i))
                selected_item_msg = biu_messages.at(i + 1).split(",");
        }
    }
    else
    {
        if (!ui->msg_line_edit->text().isEmpty())
            selected_item_msg = ui->msg_line_edit->text().split(",");
    }

    cmd.clear();
    for (int i = 0; i < selected_item_msg.length(); i++) cmd.append(selected_item_msg.at(i).toUInt(&ok, 16));

    if (selected_item_text == "SET:  Times & Temps")
    {
        if (biu_tt_result->length() > 0)
        {
            //send_log_window_message("TT selected", true, true);
            biuOpsSubaruInput1 = new BiuOpsSubaruInput1(biu_tt_result);
            connect(biuOpsSubaruInput1, SIGNAL(send_biu_setting1(QByteArray)), this, SLOT(prepare_biu_set_cmd(QByteArray)));
            biuOpsSubaruInput1->show();
        }
        else
            send_log_window_message("Read data before attempting change", true, true);

        cmd_ready = false;

    }

    if (selected_item_text == "SET:  Car options")
    {
        if (biu_option_result->length() > 0)
        {
            biuOpsSubaruInput2 = new BiuOpsSubaruInput2(&biu_option_names, biu_option_result);
            connect(biuOpsSubaruInput2, SIGNAL(send_biu_setting2(QByteArray)), this, SLOT(prepare_biu_set_cmd(QByteArray)));
            biuOpsSubaruInput2->show();
        }
        else
            send_log_window_message("Read data before attempting change", true, true);

        cmd_ready = false;

    }

    if (cmd_ready)
    {
        current_command = cmd[0];
        if (current_command  == INFO_REQUEST) current_command = cmd[1];

        prepare_biu_msg();
    }
    else
        current_command = TESTER_PRESENT;

}

void BiuOperationsSubaru::prepare_biu_set_cmd(QByteArray cmd_settings)
{
    for (int i = 0; i < cmd_settings.length(); i++) cmd[2 + i] = cmd_settings.at(i);

    current_command = cmd[0];
    if (current_command == WRITE_DATA) current_command = cmd[1];

    prepare_biu_msg();

}

void BiuOperationsSubaru::prepare_biu_msg()
{
    output.clear();

    output.append((uint8_t)0x80);
    output.append((uint8_t)0x40);
    output.append((uint8_t)0xf0);

    for (int i = 0; i < cmd.length(); i++) output.append(cmd.at(i));

    output[0] = output[0] | (output.length() - 3);
    uint8_t chk_sum = calculate_checksum(output, false);
    output.append((uint8_t) chk_sum);

    send_biu_msg();
}

void BiuOperationsSubaru::send_biu_msg()
{

    keep_alive_timer->stop();

    QByteArray received;

    if (connection_state == NOT_CONNECTED && current_command != CONNECT)
    {
        send_log_window_message("Not connected, can't send command", true, true);
        return;
    }

    send_log_window_message("Send msg: " + parse_message_to_hex(output), true, true);

    if (connection_state == NOT_CONNECTED && current_command == CONNECT)
    {
        serial->fast_init(output);
        //delay(100);

    }
    else
        received = serial->write_serial_data_echo_check(output);

    received = serial->read_serial_data(100, 250);

    /*
    received.clear();
    switch (current_command)
    {
        case CONNECT:
            received.append((uint8_t)0x83);
            received.append((uint8_t)0xf0);
            received.append((uint8_t)0x40);
            received.append((uint8_t)0xc1);
            received.append((uint8_t)0xe9);
            received.append((uint8_t)0x8f);
            received.append((uint8_t)0xec);
            break;
        case DISCONNECT:
            received.append((uint8_t)0x81);
            received.append((uint8_t)0xf0);
            received.append((uint8_t)0x40);
            received.append((uint8_t)0xc2);
            received.append((uint8_t)0x73);
            break;
        case DTC_READ:
            received.append((uint8_t)0x85);
            received.append((uint8_t)0xf0);
            received.append((uint8_t)0x40);
            received.append((uint8_t)0x58);
            received.append((uint8_t)0x01);
            received.append((uint8_t)0x82);
            received.append((uint8_t)0x21);
            received.append((uint8_t)0x00);
            received.append((uint8_t)0xB1);
            break;
        case DTC_CLEAR:
            received.append((uint8_t)0x83);
            received.append((uint8_t)0xf0);
            received.append((uint8_t)0x40);
            received.append((uint8_t)0x54);
            received.append((uint8_t)0x40);
            received.append((uint8_t)0x00);
            received.append((uint8_t)0x47);
            break;
        case TESTER_PRESENT:
            received.append((uint8_t)0x81);
            received.append((uint8_t)0xf0);
            received.append((uint8_t)0x40);
            received.append((uint8_t)0x7e);
            received.append((uint8_t)0x2f);
            break;
        case IN_OUT_SWITCHES:
            received.append((uint8_t)0x92);
            received.append((uint8_t)0xf0);
            received.append((uint8_t)0x40);
            received.append((uint8_t)0x61);
            received.append((uint8_t)0x50);
            if (counter == 0) received.append((uint8_t)0x01);
            else received.append((uint8_t)0x00);
            if (counter == 0) received.append((uint8_t)0x01);
            else received.append((uint8_t)0x00);
            if (counter == 0) received.append((uint8_t)0x01);
            else received.append((uint8_t)0x00);
            received.append((uint8_t)0x40);
            received.append((uint8_t)0x04);
            received.append((uint8_t)0x00);
            received.append((uint8_t)0x08);
            received.append((uint8_t)0x49);
            received.append((uint8_t)0x00);
            received.append((uint8_t)0x01);
            received.append((uint8_t)0x00);
            received.append((uint8_t)0x40);
            received.append((uint8_t)0x2C);
            received.append((uint8_t)0x00);
            received.append((uint8_t)0x00);
            received.append((uint8_t)0x80);
            if (counter == 0) received.append((uint8_t)0xF8);
            else received.append((uint8_t)0xF5);
            counter = counter ^ 1;
            break;
        case LIGHTING_SWITCHES:
            received.append((uint8_t)0x85);
            received.append((uint8_t)0xf0);
            received.append((uint8_t)0x40);
            received.append((uint8_t)0x61);
            received.append((uint8_t)0x51);
            received.append((uint8_t)0xFF);
            received.append((uint8_t)0xFF);
            received.append((uint8_t)0xFF);
            received.append((uint8_t)0x64);
            break;
        case BIU_DATA:
            received.append((uint8_t)0x8E);
            received.append((uint8_t)0xf0);
            received.append((uint8_t)0x40);
            received.append((uint8_t)0x61);
            received.append((uint8_t)0x40);
            received.append((uint8_t)0x8E);
            received.append((uint8_t)0x8E);
            if (counter == 0) received.append((uint8_t)0x8E);
            else received.append((uint8_t)0x8A);
            if (counter == 0) received.append((uint8_t)0x8A);
            else received.append((uint8_t)0x8E);
            received.append((uint8_t)0xF9);
            received.append((uint8_t)0xFA);
            received.append((uint8_t)0x48);
            received.append((uint8_t)0x7C);
            received.append((uint8_t)0x51);
            received.append((uint8_t)0x58);
            received.append((uint8_t)0xF0);
            received.append((uint8_t)0x03);
            received.append((uint8_t)0xE6);
            counter = counter ^ 1;
            break;
        case CAN_DATA:
            received.append((uint8_t)0x8F);
            received.append((uint8_t)0xf0);
            received.append((uint8_t)0x40);
            received.append((uint8_t)0x61);
            received.append((uint8_t)0x41);
            received.append((uint8_t)0x00);
            received.append((uint8_t)0x00);
            if (counter == 0) received.append((uint8_t)0xFF);
            else received.append((uint8_t)0x3F);
            if (counter == 0) received.append((uint8_t)0x3F);
            else received.append((uint8_t)0xFF);
            received.append((uint8_t)0x00);
            received.append((uint8_t)0xC0);
            received.append((uint8_t)0x56);
            received.append((uint8_t)0x00);
            received.append((uint8_t)0x00);
            received.append((uint8_t)0x51);
            received.append((uint8_t)0xFD);
            received.append((uint8_t)0x00);
            received.append((uint8_t)0x0F);
            received.append((uint8_t)0x12);
            counter = counter ^ 1;
            break;
        case TIME_TEMP_READ:
            received.append((uint8_t)0x85);
            received.append((uint8_t)0xf0);
            received.append((uint8_t)0x40);
            received.append((uint8_t)0x61);
            received.append((uint8_t)0x52);
            received.append((uint8_t)0x02);
            received.append((uint8_t)0x03);
            received.append((uint8_t)0x01);
            received.append((uint8_t)0x6E);
            break;
        case OPTIONS_READ:
            received.append((uint8_t)0x86);
            received.append((uint8_t)0xf0);
            received.append((uint8_t)0x40);
            received.append((uint8_t)0x61);
            received.append((uint8_t)0x53);
            received.append((uint8_t)0x30);
            received.append((uint8_t)0x16);
            received.append((uint8_t)0x10);
            received.append((uint8_t)0x02);
            received.append((uint8_t)0xC2);
            break;
        case VDC_ABS_CONDITION:
            received.append((uint8_t)0x83);
            received.append((uint8_t)0xf0);
            received.append((uint8_t)0x40);
            received.append((uint8_t)0x61);
            received.append((uint8_t)0x60);
            received.append((uint8_t)0x00);
            received.append((uint8_t)0x74);
            break;
        case DEST_TOUCH_STATUS:
            received.append((uint8_t)0x84);
            received.append((uint8_t)0xf0);
            received.append((uint8_t)0x40);
            received.append((uint8_t)0x61);
            received.append((uint8_t)0x61);
            received.append((uint8_t)0x08);
            received.append((uint8_t)0x00);
            received.append((uint8_t)0x7E);
            break;
        case FACTORY_STATUS:
            received.append((uint8_t)0x83);
            received.append((uint8_t)0xf0);
            received.append((uint8_t)0x40);
            received.append((uint8_t)0x61);
            received.append((uint8_t)0x54);
            received.append((uint8_t)0x00);
            received.append((uint8_t)0x68);
            break;
        case TIME_TEMP_WRITE:
            received.append((uint8_t)0x82);
            received.append((uint8_t)0xf0);
            received.append((uint8_t)0x40);
            received.append((uint8_t)0x7B);
            received.append((uint8_t)0x8A);
            received.append((uint8_t)0xB7);
            break;
        case OPTIONS_WRITE:
            received.append((uint8_t)0x82);
            received.append((uint8_t)0xf0);
            received.append((uint8_t)0x40);
            received.append((uint8_t)0x7B);
            received.append((uint8_t)0x8C);
            received.append((uint8_t)0xB9);
            break;

        default:
            break;
    }
    */

    send_log_window_message("Received msg: " + parse_message_to_hex(received), true, true);
    parse_biu_message(received);

    if (connection_state == CONNECTED) keep_alive_timer->start();
}

uint8_t BiuOperationsSubaru::calculate_checksum(QByteArray out, bool exclude_last_byte)
{
    uint8_t checksum = 0;
    int len = out.length();
    if (exclude_last_byte) len--;

    for (uint16_t i = 0; i < len; i++) checksum += (uint8_t)out.at(i);

    return checksum;
}

void BiuOperationsSubaru::parse_biu_message(QByteArray message)
{
    uint8_t chk_sum;

    chk_sum = calculate_checksum(message, true);

    if (!message.length())
    {
        send_log_window_message("Invalid message received: zero length", true, true);
        return;
    }

    if (((uint8_t)message.at(0) & 0x80) != 0x80 || (uint8_t)message.at(1) != 0xf0 || (uint8_t)message.at(2) != 0x40)
    {
        send_log_window_message("Invalid message received: invalid header", true, true);
        return;
    }

    if (((uint8_t)message.at(0) & 0x7F) != (uint8_t)message.length() - 4)
    {
        send_log_window_message("Invalid message received: invalid length", true, true);
        return;
    }

    if (chk_sum != (uint8_t)message.at(message.length() - 1))
    {
        send_log_window_message("Invalid message received: invalid checksum", true, true);
        return;
    }

    if ((uint8_t)message.at(3) == (CONNECT + 0x40))
    {
        /*
         * index: 0    1    2    3    4    5    6
         * cmd:   fm+l dest src  0x81 cksm
         * rsp:   fm+l dest src  rply 0xK1 0xK2 cksm
         */

        send_log_window_message("Connection to BIU successful", true, true);
        connection_state = CONNECTED;
        current_command = TESTER_PRESENT;
    }
    else if ((uint8_t)message.at(3) == (DISCONNECT + 0x40))
    {
        /*
         * index: 0    1    2    3    4    5    6
         * cmd:   fm+l dest src  0x82 cksm
         * rsp:   fm+l dest src  rply cksm
         */

        send_log_window_message("Disconnection from BIU successful", true, true);
        connection_state = NOT_CONNECTED;
        current_command = NO_COMMAND;
        close_results_windows();
    }
    else if ((uint8_t)message.at(3) == (DTC_READ + 0x40))
    {
        /*
         * index: 0    1    2    3    4    5    6
         * cmd:   fm+l dest src  0x18 cksm
         * rsp:   fm+l dest src  rply 0xD1 0xD2 0xD3 cksm
         */

        //QStringList dtc_result;
        QString dtc_code;
        QString byte;

        current_command = TESTER_PRESENT;

        int index = 5;

        data_result->clear();

        if (message.length() >= (index + 4))
        {
            //send_log_window_message("BIU DTC list:", true, true);

            while (index < (message.length() - 1))
            {
                byte = QString("%1").arg(message.at(index) & 0x0f,2,16,QLatin1Char('0'));
                dtc_code = "B" + byte;
                index++;
                byte = QString("%1").arg(message.at(index) & 0xff,2,16,QLatin1Char('0'));
                dtc_code.append(byte);
                index++;
                for (int i = 0; i < biu_dtc_list.length(); i+=2)
                {
                    if (dtc_code == biu_dtc_list.at(i))
                        dtc_code.append(" - " + biu_dtc_list.at(i + 1));
                }
                if (message.at(index) == 0)
                    dtc_code.append(" (pending)");
                else
                    dtc_code.append(" (stored)");
                index++;

                if (!dtc_code.isEmpty())
                {
                    //send_log_window_message(dtc_code, true, true);
                    data_result->append(dtc_code);
                }
            }

        }
        else
        {
            data_result->append("No BIU DTC found");
            send_log_window_message("No BIU DTC found", true, true);
        }

        biuOpsSubaruDataDtcs = update_biu_ops_subaru_data_window(biuOpsSubaruDataDtcs);

    }
    else if ((uint8_t)message.at(3) == (DTC_CLEAR + 0x40))
    {
        /*
         * index: 0    1    2    3    4    5    6
         * cmd:   fm+l dest src  0x14 0x40 0x00 cksm
         * rsp:   fm+l dest src  rply 0x40 0x00 cksm
         */

        current_command = TESTER_PRESENT;

        send_log_window_message("BIU DTCs successfully cleared", true, true);

    }
    else if ((uint8_t)message.at(3) == (INFO_REQUEST + 0x40) && (uint8_t)message.at(4) == IN_OUT_SWITCHES)
    {
        /*
         * index: 0    1    2    3    4    5    6    7
         * cmd:   fm+l dest src  0x21 0x50 cksm
         * rsp:   fm+l dest src  rply 0x50 0xB1 0xB2 0xBn cksm
         */

        int index = 5;
        int i;

        switch_result->clear();

        if (message.length() >= (index + 2))
        {
            for (index = 5; index < (message.length() - 1); index++)
            {
                int bit_mask = 1;
                for (int bit_counter = 0; bit_counter < 8; bit_counter++)
                {
                    i = ((index - 5) * 8) + bit_counter;
                    switch_result->append(biu_switch_names.at(i));
                    if ((uint8_t)message.at(index) & bit_mask) switch_result->append("ON");
                    else switch_result->append("OFF");
                    //send_log_window_message(switch_result->at(2 * i) + switch_result->at(2 * i + 1), true, true);
                    bit_mask = bit_mask << 1;
                }
            }

        }

        biuOpsSubaruSwitchesIo = update_biu_ops_subaru_switches_window(biuOpsSubaruSwitchesIo);

    }
    else if ((uint8_t)message.at(3) == (INFO_REQUEST + 0x40) && (uint8_t)message.at(4) == LIGHTING_SWITCHES)
    {
        /*
         * index: 0    1    2    3    4    5    6    7
         * cmd:   fm+l dest src  0x21 0x51 cksm
         * rsp:   fm+l dest src  rply 0x51 0xB1 0xB2 0xBn cksm
         */

        int index = 5;
        int i;

        switch_result->clear();

        if (message.length() >= (index + 2))
        {
            for (index = 5; index < (message.length() - 1); index++)
            {
                int bit_mask = 1;
                for (int bit_counter = 0; bit_counter < 8; bit_counter++)
                {
                    i = ((index - 5) * 8) + bit_counter;
                    switch_result->append(biu_lightsw_names.at(i));
                    if ((uint8_t)message.at(index) & bit_mask) switch_result->append("ON");
                    else switch_result->append("OFF");
                    //send_log_window_message(switch_result, true, true);
                    bit_mask = bit_mask << 1;
                }
            }

        }

        biuOpsSubaruSwitchesLighting = update_biu_ops_subaru_switches_window(biuOpsSubaruSwitchesLighting);
    }
    else if ((uint8_t)message.at(3) == (INFO_REQUEST + 0x40) && (uint8_t)message.at(4) == BIU_DATA)
    {
        /*
         * index: 0    1    2    3    4    5    6    7
         * cmd:   fm+l dest src  0x21 0x40 cksm
         * rsp:   fm+l dest src  rply 0x40 0xB1 0xB2 0xBn cksm
         */

        float calc_result;
        QString biu_data_result;
        int index = 5;

        data_result->clear();

        if (message.length() >= (index + 2))
        {
            for (index = 5; index < (message.length() - 1); index++)
            {
                biu_data_result = biu_data_names.at((index - 5) * 2);
                calc_result = ((uint8_t)message.at(index) * biu_data_factors[(index - 5) * 2]) + biu_data_factors[(index - 5) * 2 + 1];
                biu_data_result.append(QString("%1 ").arg(calc_result));
                biu_data_result.append(biu_data_names.at((index - 5) * 2 + 1));
                data_result->append(biu_data_result);
                //send_log_window_message(data_result, true, true);
            }

        }

        biuOpsSubaruDataBiu = update_biu_ops_subaru_data_window(biuOpsSubaruDataBiu);

    }
    else if ((uint8_t)message.at(3) == (INFO_REQUEST + 0x40) && (uint8_t)message.at(4) == CAN_DATA)
    {
        /*
         * index: 0    1    2    3    4    5    6    7
         * cmd:   fm+l dest src  0x21 0x41 cksm
         * rsp:   fm+l dest src  rply 0x41 0xB1 0xB2 0xBn cksm
         */

        QString can_data_result;
        float calc_result;

        int item = 0;

        data_result->clear();

        if (message.length() >= 7)
        {

            // front wheel speed
            can_data_result = can_data_names.at(item * 2);
            calc_result = ((uint8_t)message.at(6) << 8) | (uint8_t)message.at(5);
            calc_result = (calc_result * can_data_factors[item * 2]) + can_data_factors[item * 2 + 1];
            can_data_result.append(QString("%1 ").arg(calc_result));
            can_data_result.append(can_data_names.at(item * 2 + 1));
            data_result->append(can_data_result);
            //send_log_window_message(can_data_result, true, true);

            // VDC/ABS latest f-code
            item++;
            can_data_result = can_data_names.at(item * 2);
            can_data_result.append(QString("%1 ").arg((uint8_t)message.at(8),2,16,QLatin1Char('0')));
            can_data_result.append(QString("%1 ").arg((uint8_t)message.at(7),2,16,QLatin1Char('0')));
            can_data_result.append(can_data_names.at(item * 2 + 1));
            data_result->append(can_data_result);
            //send_log_window_message(can_data_result, true, true);

            // Blower fan steps
            item++;
            can_data_result = can_data_names.at(item * 2);
            calc_result = (uint8_t)message.at(9);
            calc_result = (calc_result * can_data_factors[item * 2]) + can_data_factors[item * 2 + 1];
            can_data_result.append(QString("%1 ").arg(calc_result));
            can_data_result.append(can_data_names.at(item * 2 + 1));
            data_result->append(can_data_result);
            //send_log_window_message(can_data_result, true, true);

            // Fuel level resistance
            item++;
            can_data_result = can_data_names.at(item * 2);
            calc_result = ((uint8_t)message.at(11) << 8) | (uint8_t)message.at(10);
            calc_result = (calc_result * can_data_factors[item * 2]) + can_data_factors[item * 2 + 1];
            can_data_result.append(QString("%1 ").arg(calc_result));
            can_data_result.append(can_data_names.at(item * 2 + 1));
            data_result->append(can_data_result);
            //send_log_window_message(can_data_result, true, true);

            // Fuel consumption
            item++;
            can_data_result = can_data_names.at(item * 2);
            calc_result = ((uint8_t)message.at(13) << 8) | (uint8_t)message.at(12);
            calc_result = (calc_result * can_data_factors[item * 2]) + can_data_factors[item * 2 + 1];
            can_data_result.append(QString("%1 ").arg(calc_result));
            can_data_result.append(can_data_names.at(item * 2 + 1));
            data_result->append(can_data_result);
            //send_log_window_message(can_data_result, true, true);

            // engine coolant temp
            item++;
            can_data_result = can_data_names.at(item * 2);
            calc_result = (uint8_t)message.at(14);
            calc_result = (calc_result * can_data_factors[item * 2]) + can_data_factors[item * 2 + 1];
            can_data_result.append(QString("%1 ").arg(calc_result));
            can_data_result.append(can_data_names.at(item * 2 + 1));
            data_result->append(can_data_result);
            //send_log_window_message(can_data_result, true, true);

            // g-force
            item++;
            can_data_result = can_data_names.at(item * 2);
            calc_result = (uint8_t)message.at(15);
            calc_result = (calc_result * can_data_factors[item * 2]) + can_data_factors[item * 2 + 1];
            can_data_result.append(QString("%1 ").arg(calc_result));
            can_data_result.append(can_data_names.at(item * 2 + 1));
            data_result->append(can_data_result);
            //send_log_window_message(can_data_result, true, true);

            // sport shift
            item++;
            can_data_result = can_data_names.at(item * 2);
            calc_result = (uint8_t)message.at(16);
            calc_result = (calc_result * can_data_factors[item * 2]) + can_data_factors[item * 2 + 1];
            can_data_result.append(QString("%1 ").arg(calc_result));
            can_data_result.append(can_data_names.at(item * 2 + 1));
            data_result->append(can_data_result);
            //send_log_window_message(can_data_result, true, true);

            // shift position
            item++;
            can_data_result = can_data_names.at(item * 2);
            calc_result = (uint8_t)message.at(17);
            calc_result = (calc_result * can_data_factors[item * 2]) + can_data_factors[item * 2 + 1];
            can_data_result.append(QString("%1 ").arg(calc_result));
            can_data_result.append(can_data_names.at(item * 2 + 1));
            data_result->append(can_data_result);
            //send_log_window_message(can_data_result, true, true);

        }

        biuOpsSubaruDataCan = update_biu_ops_subaru_data_window(biuOpsSubaruDataCan);

    }
    else if ((uint8_t)message.at(3) == (INFO_REQUEST + 0x40) && (uint8_t)message.at(4) == TIME_TEMP_READ)
    {
        /*
         * index: 0    1    2    3    4    5    6    7
         * cmd:   fm+l dest src  0x21 0x52 cksm
         * rsp:   fm+l dest src  rply 0x52 0xB1 0xB2 0xBn cksm
         */

        current_command = TESTER_PRESENT;
        QString temp;
        float calc_result;

        data_result->clear();

        if (message.length() >= 7)
        {

            // room lamp off delay time
            temp = biu_tt_names.at(0);
            biu_tt_result->append((uint8_t)message.at(5) & 0x03);
            calc_result = (uint8_t)message.at(5) & 0x03;
            if (calc_result == 0) temp.append("Normal");
            else if (calc_result == 0x01) temp.append("OFF");
            else if (calc_result == 0x02) temp.append("Short");
            else if (calc_result == 0x03) temp.append("Long");
            temp.append(biu_tt_names.at(1));
            data_result->append(temp);
            //send_log_window_message(biu_tt_result, true, true);

            // auto-lock time
            temp = biu_tt_names.at(2);
            biu_tt_result->append((uint8_t)message.at(6) & 0x07);
            calc_result = ((uint8_t)message.at(6) & 0x07) * 10;
            temp.append(QString("%1 ").arg(calc_result));
            temp.append(biu_tt_names.at(3));
            data_result->append(temp);
            //send_log_window_message(biu_tt_result, true, true);

            // outside temp offset
            if (message.length() == 9)
            {
                temp = biu_tt_names.at(4);
                biu_tt_result->append((uint8_t)message.at(7) & 0x0F);
                calc_result = (((((uint8_t)message.at(7) & 0x0F) + 4) & 0x0F) - 4) * 0.5;
                temp.append(QString("%1 ").arg(calc_result));
                temp.append(biu_tt_names.at(5));
                data_result->append(temp);
                //send_log_window_message(biu_tt_result, true, true);
            }
        }

        biuOpsSubaruDataTt = update_biu_ops_subaru_data_window(biuOpsSubaruDataTt);

    }
    else if ((uint8_t)message.at(3) == (INFO_REQUEST + 0x40) && (uint8_t)message.at(4) == OPTIONS_READ)
    {
        /*
         * index: 0    1    2    3    4    5    6    7
         * cmd:   fm+l dest src  0x21 0x53 cksm
         * rsp:   fm+l dest src  rply 0x53 0xB1 0xB2 0xBn cksm
         */

        //QString biu_option_result;

        current_command = TESTER_PRESENT;
        switch_result->clear();

        int index = 5;
        int i;

        if (message.length() >= (index + 2))
        {
            for (index = 5; index < (message.length() - 1); index++)
            {
                int bit_mask = 1;
                for (int bit_counter = 0; bit_counter < 8; bit_counter++)
                {
                    i = ((index - 5) * 8) + bit_counter;
                    switch_result->append(biu_option_names.at(i * 3));
                    if ((uint8_t)message.at(index) & bit_mask) switch_result->append(biu_option_names.at(i * 3 + 1));
                    else switch_result->append(biu_option_names.at(i * 3 + 2));
                    //send_log_window_message(switch_result->at(2 * i) + switch_result->at(2 * i + 1), true, true);
                    bit_mask = bit_mask << 1;
                }

                biu_option_result->append((uint8_t)message.at(index));
            }

        }

        biuOpsSubaruSwitchesOptions = update_biu_ops_subaru_switches_window(biuOpsSubaruSwitchesOptions);

    }
    else if ((uint8_t)message.at(3) == (INFO_REQUEST + 0x40) && (uint8_t)message.at(4) == VDC_ABS_CONDITION)
    {
        /*
         * index: 0    1    2    3    4    5    6    7
         * cmd:   fm+l dest src  0x21 0x60 cksm
         * rsp:   fm+l dest src  rply 0x60 0xB1 cksm
         */

        current_command = TESTER_PRESENT;
        data_result->clear();

        int condition = (uint8_t)message.at(5) & 0x07;
        data_result->append("VDC/ABS Condition: " + QString::number(condition));
        //send_log_window_message(data_result, true, true);

        biuOpsSubaruDataVdcabs = update_biu_ops_subaru_data_window(biuOpsSubaruDataVdcabs);

    }
    else if ((uint8_t)message.at(3) == (INFO_REQUEST + 0x40) && (uint8_t)message.at(4) == DEST_TOUCH_STATUS)
    {
        /*
         * index: 0    1    2    3    4    5    6    7
         * cmd:   fm+l dest src  0x21 0x61 cksm
         * rsp:   fm+l dest src  rply 0x61 0xB1 0xB2 cksm
         */

        int condition;
        current_command = TESTER_PRESENT;
        data_result->clear();

        condition = (uint8_t)message.at(5) & 0x0F;
        data_result->append("Destination:    " + QString::number(condition));
        condition = (uint8_t)message.at(6) & 0x3F;
        data_result->append("Touchscreen SW: " + QString::number(condition));
        //send_log_window_message(data_result, true, true);

        biuOpsSubaruDataDest = update_biu_ops_subaru_data_window(biuOpsSubaruDataDest);

    }
    else if ((uint8_t)message.at(3) == (INFO_REQUEST + 0x40) && (uint8_t)message.at(4) == FACTORY_STATUS)
    {
        /*
         * index: 0    1    2    3    4    5    6    7
         * cmd:   fm+l dest src  0x21 0x54 cksm
         * rsp:   fm+l dest src  rply 0x54 0xB1 cksm
         */

        QString setting;
        current_command = TESTER_PRESENT;
        data_result->clear();

        if ((uint8_t)message.at(5) & 0x01) setting = "Factory";
        else setting = "Market";
        data_result->append("Factory Initial Setting: " + setting);
        //send_log_window_message(data_result, true, true);

        biuOpsSubaruDataFactory = update_biu_ops_subaru_data_window(biuOpsSubaruDataFactory);

    }
    else if ((uint8_t)message.at(3) == (WRITE_DATA + 0x40) && (uint8_t)message.at(4) == TIME_TEMP_WRITE)
    {
        /*
         * index: 0    1    2    3    4    5    6    7
         * cmd:   fm+l dest src  0x3E 0x8A 0xD1 0xD2 0xD3 cksm
         * rsp:   fm+l dest src  rply 0x8A cksm
         */

        current_command = TESTER_PRESENT;

        send_log_window_message("Setting change successful", true, true);
        biu_tt_result->clear();
        biuOpsSubaruInput1->close();

    }
    else if ((uint8_t)message.at(3) == (WRITE_DATA + 0x40) && (uint8_t)message.at(4) == OPTIONS_WRITE)
    {
        /*
         * index: 0    1    2    3    4    5    6    7
         * cmd:   fm+l dest src  0x3E 0x8C 0xD1 0xD2 0xD3 cksm
         * rsp:   fm+l dest src  rply 0x8C cksm
         */

        current_command = TESTER_PRESENT;

        send_log_window_message("Setting change successful", true, true);
        biu_option_result->clear();
        biuOpsSubaruInput2->close();

    }
    else if ((uint8_t)message.at(3) == 0x7F)
    {
        send_log_window_message("Error response received from BIU", true, true);
    }

}

QString BiuOperationsSubaru::parse_message_to_hex(QByteArray received)
{
    QByteArray msg;

    for (unsigned long i = 0; i < received.length(); i++)
    {
        msg.append(QString("%1 ").arg((uint8_t)received.at(i),2,16,QLatin1Char('0')).toUtf8());
    }

    return msg;
}

int BiuOperationsSubaru::send_log_window_message(QString message, bool timestamp, bool linefeed)
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

void BiuOperationsSubaru::delay(int timeout)
{
    QTime dieTime = QTime::currentTime().addMSecs(timeout);
    while (QTime::currentTime() < dieTime)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
}
