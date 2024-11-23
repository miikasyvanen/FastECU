#include "mainwindow.h"
#include "ui_mainwindow.h"

void MainWindow::kline_listener()
{
    QByteArray received;

    //serial->change_port_speed("10400");
    serial->change_port_speed("125000");

    while (haltech_ic7_display_on)
    {
        received = serial->read_serial_data(100, 500);
        //qDebug() << parse_message_to_hex(received);
        //delay(5);
    }
}

void MainWindow::canbus_listener()
{
    QByteArray received;

    while (haltech_ic7_display_on)
    {
        received = serial->read_serial_data(100, 500);
        //qDebug() << parse_message_to_hex(received);
        //delay(5);
    }
}

bool MainWindow::ecu_init()
{
/*
    QString car_model;

    //qDebug() << "ECU init";
    QByteArray output;
    uint8_t chksum = 0;
    output.clear();
    output.append((uint8_t)0x81);
    output.append((uint8_t)0x40);
    output.append((uint8_t)0xf0);
    output.append((uint8_t)0x81);
    for (int i = 0; i < output.length(); i++)
        chksum += (uint8_t)output.at(i);
    output.append(chksum & 0xFF);
*/
    if (serial->is_serial_port_open())
    {
        if (!ecu_init_complete)
        {
            if (configValues->flash_protocol_selected_make == "Subaru")
            {
                if (configValues->flash_protocol_selected_log_transport == "CAN" || configValues->flash_protocol_selected_log_transport == "iso15765")
                    ssm_can_init();
                else if (configValues->flash_protocol_selected_log_transport == "K-Line")
                    //serial->fast_init(output);
                    ssm_kline_init();
                else if (configValues->flash_protocol_selected_log_transport == "SSM")
                    ssm_init();
            }
        }
    }
    else
    {
        //qDebug() << "Connection is not ready!";
        ecu_init_complete = false;
        ecuid.clear();
    }
    //qDebug() << "ECU ID check complete";

    return ecu_init_complete;
}

void MainWindow::ssm_init()
{
    QByteArray output;
    QByteArray received;

    serial->reset_connection();
    serial->set_serial_port_baudrate("1953");
    serial->set_serial_port_parity(QSerialPort::EvenParity);
    serial->open_serial_port();

    qDebug() << "Using SSM1 protocol";

    qDebug() << "Issue read cmd...";
    received.clear();
    output.clear();
    output.append((uint8_t)0x78);
    output.append((uint8_t)0x12);
    output.append((uint8_t)0x34);
    output.append((uint8_t)0x00);
    serial->write_serial_data_echo_check(output);
    //delay(1000);
    for (int i = 0; i < 10; i++)
    {
        received.append(serial->read_serial_data(100, 500));
    }
    qDebug() << "Received:" << parse_message_to_hex(received);
    //if (received.length() > 0)
    //    qDebug() << "Something received" << parse_message_to_hex(received);
    //else
    //    qDebug() << "No response...";


    qDebug() << "Issue init cmd...";
    received.clear();
    output.clear();
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x46);
    output.append((uint8_t)0x48);
    output.append((uint8_t)0x49);
    serial->write_serial_data(output);

    qDebug() << "Init sent, delaying 2s...";
    //delay(2000);
    for (int i = 0; i < 2; i++)
    {
        received.append(serial->read_serial_data(100, 500));
    }
    qDebug() << "Received:" << parse_message_to_hex(received);
    //qDebug() << "Checking response...";
    //received = serial->read_serial_data(100, 500);
    received.clear();
    output.clear();
    output.append((uint8_t)0x12);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    serial->write_serial_data(output);
    received.append(serial->read_serial_data(100, 500));

    if (received.length() > 0)
    {
        qDebug() << "Something received" << parse_message_to_hex(received);;

        if (received.length() == (uint8_t)received.at(3) + 5)
        {
            ecu_init_complete = true;
            ecuid = parse_ecuid(received);
            qDebug() << "ECU ID:" << ecuid;
            parse_log_value_list(received, "SSM");
            if (ecuid == "")
                set_status_bar_label(true, false, "");
            else
                set_status_bar_label(true, true, ecuid);

            received = serial->read_serial_data(1, 100);
            while(received.length() > 0)
            {
                ecu_init_complete = true;
                ecuid = parse_ecuid(received);
                parse_log_value_list(received, "SSM");

                received = serial->read_serial_data(1, 100);
                while(received.length() > 0)
                {
                    received = serial->read_serial_data(1, 100);
                }
            }
        }
    }
    else
        qDebug() << "No response...";

}

void MainWindow::ssm_kline_init()
{
    QByteArray output;
    QByteArray received;
    int loopcount = 0;

    //qDebug() << "ECU K-Line INIT";
    if (!ecu_init_started)
    {
        ecu_init_started = true;

        qDebug() << "K-Line SSM init with BF";
        output.clear();
        output.append((uint8_t)0xBF);
        serial->write_serial_data_echo_check(add_ssm_header(output, false));
        qDebug() << "K-Line SSM init sent";
        delay(200);
        received = serial->read_serial_data(100, 200);
        qDebug() << "K-Line SSM init response:" << parse_message_to_hex(received);
        while (received.length() < 4 && loopcount < 10)
        {
            received.append(serial->read_serial_data(100, 50));
            loopcount++;
        }
        if (received.length() < 4)
            return;

        qDebug() << "SSM Init response header received";
        loopcount = 0;
        while (received.length() < (uint8_t)received.at(3) + 5 && loopcount < 10)
        {
            received.append(serial->read_serial_data(100, 50));
            loopcount++;
        }
        if (received.length() < (uint8_t)received.at(3) + 5)
            return;

        qDebug() << "ECU INIT length:" << QString::number((uint8_t)received.at(3)) << received.length();
        if (received.length() >= (uint8_t)received.at(3) + 5)
        {
            ecu_init_complete = true;
            //set_status_bar_label(true, true, ecuid);
            ecuid = parse_ecuid(received);
            qDebug() << "ECU ID:" << ecuid;
            parse_log_value_list(received, "SSM");
            //qDebug() << "ECU ID:" << ecuid;
            if (ecuid == "")
                set_status_bar_label(true, false, "");
            else
                set_status_bar_label(true, true, ecuid);
            return;
        }
        /*
        received.append(serial->read_serial_data(10, 100));
        while(received.length() > 0)
        {
            //qDebug() << "ECU ID:" << parse_ecuid(received);
            ecu_init_complete = true;
            //set_status_bar_label(true, true, ecuid);
            ecuid = parse_ecuid(received);
            parse_log_value_list(received, "SSM");

            received = serial->read_serial_data(1, 100);
            while(received.length() > 0)
            {
                received = serial->read_serial_data(1, 100);
            }
        }
*/
    }
    ecu_init_started = false;
}

void MainWindow::ssm_can_init()
{
    QByteArray output;
    QByteArray received;
    uint32_t control_unit_addr = 0;

    if (ecu_radio_button->isChecked())
        control_unit_addr = 0x7E0;
    else if (tcu_radio_button->isChecked())
        control_unit_addr = 0x7E1;

    const uint8_t cu_addr_b3 = (uint8_t)((control_unit_addr >> 24) & 0xFF);
    const uint8_t cu_addr_b2 = (uint8_t)((control_unit_addr >> 16) & 0xFF);
    const uint8_t cu_addr_b1 = (uint8_t)((control_unit_addr >> 8) & 0xFF);
    const uint8_t cu_addr_b0 = (uint8_t)(control_unit_addr & 0xFF);

    qDebug() << Q_FUNC_INFO << configValues->flash_protocol_selected_log_protocol;

    if (configValues->flash_protocol_selected_log_protocol == "CAN")
        received = serial->read_serial_data(100, 500);
    else if (configValues->flash_protocol_selected_log_transport == "iso15765")
    {
        // Set serial port
        serial->reset_connection();
        serial->set_is_iso14230_connection(false);
        serial->set_add_iso14230_header(false);
        serial->set_is_can_connection(false);
        serial->set_is_iso15765_connection(true);
        serial->set_is_29_bit_id(false);
        serial->set_can_speed("500000");
        serial->set_iso15765_source_address(control_unit_addr);
        serial->set_iso15765_destination_address(0x7E8);
        // Open serial port
        serial->open_serial_port();

        output.clear();
        output.append(cu_addr_b3);
        output.append(cu_addr_b2);
        output.append(cu_addr_b1);
        output.append(cu_addr_b0);
        output.append((uint8_t)0x22);
        output.append((uint8_t)0xF1);
        output.append((uint8_t)0x82);
        serial->write_serial_data_echo_check(output);
        received = serial->read_serial_data(100, 100);
        if (received.length() > 7 &&
            (uint8_t)received.at(4) == 0x62 &&
            (uint8_t)received.at(5) == 0xF1 &&
            (uint8_t)received.at(6) == 0x82)
        {
            ecu_init_complete = true;
            received = received.remove(0, 7);
            QString msg;
            for (int i = 0; i < received.length(); i++)
            {
                msg.append(QString("%1").arg((uint8_t)received.at(i),2,16,QLatin1Char('0')).toUpper());
            }
            qDebug() << "ECU ID" << msg;
            set_status_bar_label(true, true, msg);
        }
    }
}

void MainWindow::log_ssm_values()
{
    QByteArray output;
    QByteArray received;
    uint16_t value_count = 0;
    bool ok = false;

    if (ecu_init_complete)// && !log_params_request_started)
    {
        if (!logging_state)
        {
            output.append((uint8_t)0xA8);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x07);

        }
        else
        {
            //qDebug() << "Send log request";
            output.append((uint8_t)0xA8);
            output.append((uint8_t)0x01);
            for (int i = 0; i < logValues->lower_panel_log_value_id.length(); i++)
            {
                for (int j = 0; j < logValues->log_value_id.length(); j++)
                {
                    if (logValues->lower_panel_log_value_id.at(i) == logValues->log_value_id.at(j) && logValues->log_value_protocol.at(j) == protocol)
                    {
                        value_count++;
                        //qDebug() << logValues->log_value_id.at(i) << "at" << logValues->log_value_address.at(i) << "enabled, length" << logValues->log_value_length.at(i);
                        output.append((uint8_t)(logValues->log_value_address.at(j).toUInt(&ok,16) >> 16));
                        output.append((uint8_t)(logValues->log_value_address.at(j).toUInt(&ok,16) >> 8));
                        output.append((uint8_t)logValues->log_value_address.at(j).toUInt(&ok,16));
                    }
                }
            }

            for (int i = 0; i < logValues->dashboard_log_value_id.length(); i++)
            {
                for (int j = 0; j < logValues->log_value_id.length(); j++)
                {
                    if (logValues->dashboard_log_value_id.at(i) == logValues->log_value_id.at(j) && logValues->log_value_protocol.at(j) == protocol)
                    {
                        value_count++;
                        //qDebug() << logValues->log_value_id.at(i) << "at" << logValues->log_value_address.at(i) << "enabled, length" << logValues->log_value_length.at(i);
                        output.append((uint8_t)(logValues->log_value_address.at(j).toUInt(&ok,16) >> 16));
                        output.append((uint8_t)(logValues->log_value_address.at(j).toUInt(&ok,16) >> 8));
                        output.append((uint8_t)logValues->log_value_address.at(j).toUInt(&ok,16));
                    }
                }
            }
        }
        serial->write_serial_data_echo_check(add_ssm_header(output, false));
        delay(10);
    }
}

void MainWindow::read_log_serial_data()
{
    QByteArray received;

    if (!logparams_read_active)
    {
        logparams_read_active = true;
        //qDebug() << "Read logger data from ECU";
        if (serial->get_use_openport2_adapter())
            received = serial->read_serial_data(1, 100);
        else
        {
            int loop_count = 0;
            while (received.length() < 3 && loop_count < 100)
            {
                received.append(serial->read_serial_data(1, 10));
                loop_count++;
            }
            //qDebug() << "Header data" << parse_message_to_hex(received);
            loop_count = 0;
            while (((uint8_t)received.at(0) != 0x80 || (uint8_t)received.at(1) != 0xf0 || (uint8_t)received.at(2) != 0x10) && loop_count < 200)
            {
                received.remove(0, 1);
                received.append(serial->read_serial_data(1, 50));
                loop_count++;
            }
            received.append(serial->read_serial_data(1, 50));
            received.append(serial->read_serial_data((uint8_t)received.at(3) + 1, 100));
        }

        if (received.length() > 6 && (uint8_t)received.at(4) == 0xe8)
        {
            received.remove(0, 5);
            received.remove(received.length() - 1, 1);
            parse_log_params(received, protocol);
            //qDebug() << "Log params data:" << parse_message_to_hex(received);
        }
        else
        {
            //qDebug() << "Log params get failed:" << parse_message_to_hex(received);
        }

        logparams_read_active = false;
    }
}

QString MainWindow::parse_log_params(QByteArray received, QString protocol)
{
    QString params;

    uint16_t log_value_index = 0;
    uint16_t value_index = 0;

    double timer_elapsed = log_speed_timer->elapsed();
    if (timer_elapsed < 1)
        timer_elapsed = 1;
    double data_received = received.length();
    double log_speed = 1000.0f / timer_elapsed * data_received;

    //qDebug() << (uint16_t)log_speed << "values read per second:" << data_received << "bytes at" << timer_elapsed << "milliseconds";

    if (!logging_request_active)
    {
        logging_request_active = true;
        //qDebug() << "Log read count:" << logging_counter++;
        for (int i = 0; i < logValues->lower_panel_log_value_id.length(); i++)
        {
            for (int j = 0; j < logValues->log_value_id.length(); j++)
            {
                if (logValues->lower_panel_log_value_id.at(i) == logValues->log_value_id.at(j) && logValues->log_value_protocol.at(j) == protocol)
                {
                    if (logValues->log_value_enabled.at(j) == "1" && log_value_index < received.length())
                    {
                        QStringList conversion = logValues->log_value_units.at(j).split(",");
                        QString value_name = logValues->log_value_name.at(j);
                        QString unit = conversion.at(1);
                        QString from_byte = conversion.at(2);
                        QStringList format_str_lst = conversion.at(3).split(".");
                        uint8_t format = 0;
                        if (format_str_lst.length() > 1)
                            format = format_str_lst.at(1).count(QRegularExpression("0"));

                        //QString gauge_min = conversion.at(4);
                        //QString gauge_max = conversion.at(5);
                        //QString gauge_step = conversion.at(6);
                        QString value;
                        uint8_t log_value_length = logValues->log_value_length.at(j).toUInt();
                        for (uint8_t k = 0; k < log_value_length; k++)
                        {
                            value.append(QString::number((uint8_t)received.at(i + k)));
                        }
                        float calc_float_value = fileActions->calculate_value_from_expression(fileActions->parse_stringlist_from_expression_string(from_byte, value));
                        QString calc_value = QString::number(calc_float_value, 'f', format);
                        logValues->log_value.replace(j, calc_value);

                        //qDebug() << QString::number(log_value_index) + ". " + value_name + ": " + calc_value + " " + unit + " from_byte: " + value + " via expr: " + from_byte;
                        //params.append(calc_value);
                        //params.append(", ");

                        log_value_index++;
                    }
                }
            }
        }

        //qDebug() << "indexOf" << logValues->log_value_id.indexOf(QString(logValues->dashboard_log_value_id.at(2)))+1;
        for (int i = 0; i < logValues->dashboard_log_value_id.length(); i++)
        {
            for (int j = 0; j < logValues->log_value_id.length(); j++)
            {
                //value_index = logValues->log_value_id.indexOf(QString(logValues->log_values_by_protocol.at(i)))+1;
                if (logValues->dashboard_log_value_id.at(i) == logValues->log_value_id.at(j) && logValues->log_value_protocol.at(j) == protocol)
                {
                    //qDebug() << logValues->log_value_name.at(j) << logValues->log_value_enabled.at(j);
                    if (logValues->log_value_enabled.at(j) == "1" && log_value_index < received.length())
                    {
                        QStringList conversion = logValues->log_value_units.at(j).split(",");
                        QString value_name = logValues->log_value_name.at(j);
                        QString unit = conversion.at(1);
                        QString from_byte = conversion.at(2);
                        QStringList format_str_lst = conversion.at(3).split(".");
                        uint8_t format = 0;
                        if (format_str_lst.length() > 1)
                            format = format_str_lst.at(1).count(QRegularExpression("0"));

                        //QString gauge_min = conversion.at(4);
                        //QString gauge_max = conversion.at(5);
                        //QString gauge_step = conversion.at(6);
                        QString value;
                        uint8_t log_value_length = logValues->log_value_length.at(j).toUInt();
                        for (uint8_t k = 0; k < log_value_length; k++)
                        {
                            value.append(QString::number((uint8_t)received.at(log_value_index + k)));
                        }
                        float calc_float_value = fileActions->calculate_value_from_expression(fileActions->parse_stringlist_from_expression_string(from_byte, value));
                        QString calc_value = QString::number(calc_float_value, 'f', format);
                        logValues->log_value.replace(j, calc_value);

                        //qDebug() << QString::number(log_value_index) + ". " + value_name + ": " + calc_value + " " + unit + " from_byte: " + value + " via expr: " + from_byte;
                        //params.append(calc_value);
                        //params.append(", ");

                        log_value_index += log_value_length;
                    }
                }
            }
        }

        //qDebug() << parse_message_to_hex(received);
        //qDebug() << " ";
        logging_request_active = false;
        log_to_file();
    }

    log_speed_timer->start();
    update_logbox_values(protocol);

    return params;
}

void MainWindow::parse_log_value_list(QByteArray received, QString protocol)
{
    uint16_t enabled_log_value_count = 0;
    uint16_t enabled_log_switch_count = 0;
    uint16_t log_value_count = logValues->log_value_id.length();
    uint16_t log_switch_count = logValues->log_switch_id.length();

    received.remove(0, 5);

    //qDebug() << parse_message_to_hex(received);

    logValues->log_values_by_protocol.clear();

    //qDebug() << "Parsing log values list";
    for (int i = 0; i < log_value_count; i++)
    {
        if (logValues->log_value_protocol.at(i) == protocol)
        {
            logValues->log_values_by_protocol.append(logValues->log_value_name.at(i));

            uint16_t ecu_byte_index = logValues->log_value_ecu_byte_index.at(i).toUInt();
            if (ecu_byte_index < received.length() && logValues->log_value_ecu_byte_index.at(i) != "No byte index")
            {
                //qDebug() << "Value: " + logValues->log_value_id.at(i) + " " + logValues->log_value_name.at(i);
                uint8_t ecu_bit = logValues->log_value_ecu_bit.at(i).toUInt();
                uint16_t value = (uint8_t)received.at(ecu_byte_index);
                if (((value) & (1 << (ecu_bit))))
                {
                    logValues->log_value_enabled.replace(i, "1");
                    //qDebug() << "Enabled: " + logValues->log_value_id.at(i) + " " + logValues->log_value_name.at(i) + " " + logValues->log_value_enabled.at(i);
                    enabled_log_value_count++;
                }
                else
                {
                    logValues->log_value_enabled.replace(i, "0");
                    //qDebug() << "Disabled: " + logValues->log_value_id.at(i) + " " + logValues->log_value_name.at(i) + " " + logValues->log_value_enabled.at(i);
                }
            }
            else
                logValues->log_value_enabled[i] = "0";

        }
    }
    //qDebug() << "Log values list ready";

    //qDebug() << "Parsing log switches list";
    for (int i = 0; i < log_switch_count; i++)
    {
        if (logValues->log_switch_protocol.at(i) == protocol)
        {
            // 'switch_byte_index' is byte index in SSM init response
            uint16_t switch_byte_index = logValues->log_switch_ecu_byte_index.at(i).toUInt();
            if (switch_byte_index < received.length())
            {
                uint8_t switch_bit = logValues->log_switch_ecu_bit.at(i).toUInt();
                //qDebug() << "1" << switch_byte_index;
                uint8_t value = (uint8_t)received.at(switch_byte_index);
                //qDebug() << "2";
                if (((value) & (1 << (switch_bit))))
                {
                    logValues->log_switch_enabled.replace(i, "1");
                    //qDebug() << "Switch: " + logValues->log_switch_id.at(i) + " " + logValues->log_switch_name.at(i) + " " + logValues->log_switch_enabled.at(i);
                    enabled_log_switch_count++;
                }
                else
                    logValues->log_switch_enabled.replace(i, "0");
            }
        }
    }
    fileActions->read_logger_conf(logValues, ecuid, false);
    //qDebug() << "Log switches list ready";

    update_logboxes(protocol);
}

QByteArray MainWindow::add_ssm_header(QByteArray output, bool dec_0x100)
{
    uint8_t length = output.length();

    output.insert(0, (uint8_t)0x80);
    if (ecu_radio_button->isChecked())
    {
        qDebug() << "ECU selected";
        output.insert(1, (uint8_t)0x10);
    }
    else
    {
        qDebug() << "TCU selected";
        output.insert(1, (uint8_t)0x18);
    }
    output.insert(2, (uint8_t)0xF0);
    output.insert(3, length);
    output.append(calculate_checksum(output, dec_0x100));

    qDebug() << "Generated SSM message:" << parse_message_to_hex(output);

    return output;
}

uint8_t MainWindow::calculate_checksum(QByteArray output, bool dec_0x100)
{
    uint8_t checksum = 0;

    for (uint16_t i = 0; i < output.length(); i++)
    {
        checksum += (uint8_t)output.at(i);
    }
    if (dec_0x100)
        checksum = (uint8_t) (0x100 - checksum);

    return checksum;
}

void MainWindow::log_to_file(){
    if (write_datalog_to_file){
        if (!datalog_file_open){
            QDateTime dateTime = dateTime.currentDateTime();
            QString dateTimeString = dateTime.toString("yyyy-MM-dd_hh'h'mm'm'ss's'");

            QString log_file_name = configValues->datalog_files_directory;
            if (configValues->datalog_files_directory.at(configValues->datalog_files_directory.length() - 1) != '/')
                log_file_name.append("/");
            log_file_name.append("fastecu_" + dateTimeString + ".csv");

            datalog_file.setFileName(log_file_name);
            if (!datalog_file.open(QIODevice::WriteOnly)) {
                QMessageBox::information(this, tr("Unable to open file"),
                datalog_file.errorString());
                return;
            }
            else
            {
                datalog_file_open = true;
                log_file_timer->start();
            }

            datalog_file_outstream.setDevice(&datalog_file);
            datalog_file_outstream << "Time,";
            for (int j = 0; j < logValues->dashboard_log_value_id.count() ; j++){
                datalog_file_outstream << logValues->log_value_name.at(logValues->log_value_id.indexOf(logValues->dashboard_log_value_id.at(j), 0)) << ",";
            }
            for (int j = 0; j < logValues->lower_panel_log_value_id.count() ; j++){
                datalog_file_outstream << logValues->log_value_name.at(logValues->log_value_id.indexOf(logValues->lower_panel_log_value_id.at(j), 0)) << ",";
            }
            for (int j = 0; j < logValues->lower_panel_switch_id.count() ; j++){
                datalog_file_outstream << logValues->log_switch_name.at(logValues->log_switch_id.indexOf(logValues->lower_panel_switch_id.at(j),0)) << ",";
            }
            datalog_file_outstream << "\n";
        }
        else{

            datalog_file_outstream << QString::number(log_file_timer->elapsed() / 1000.0f) << ",";
            for (int j = 0; j < logValues->dashboard_log_value_id.count() ; j++){
                datalog_file_outstream << logValues->log_value.at(logValues->log_value_id.indexOf(logValues->dashboard_log_value_id.at(j), 0)) << ",";
            }
            for (int j = 0; j < logValues->lower_panel_log_value_id.count() ; j++){
                datalog_file_outstream << logValues->log_value.at(logValues->log_value_id.indexOf(logValues->lower_panel_log_value_id.at(j), 0)) << ",";
            }
            for (int j = 0; j < logValues->lower_panel_switch_id.count() ; j++){
                datalog_file_outstream << logValues->log_switch_state.at(logValues->log_switch_id.indexOf(logValues->lower_panel_switch_id.at(j), 0)) << ",";
            }
            datalog_file_outstream << "\n";
        }
    }
}

