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

    if (serial->is_serial_port_open())
    {
        if (!ecu_init_complete)
        {
            if (configValues->flash_protocol_selected_make == "Subaru")
            {
                if (configValues->flash_protocol_selected_log_protocol == "CAN" || configValues->flash_protocol_selected_log_protocol == "iso15765")
                    ssm_can_init();
                else
                    //serial->fast_init(output);
                    ssm_kline_init();
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

void MainWindow::ssm_kline_init()
{
    QByteArray output;
    QByteArray received;

    //qDebug() << "Check ECU INIT";
    if (!ecu_init_started)
    {
        ecu_init_started = true;

        //qDebug() << "SSM init with BF";
        output.clear();
        output.append((uint8_t)0xBF);
        serial->write_serial_data_echo_check(add_ssm_header(output, false));
        delay(200);
        received = serial->read_serial_data(100, 500);
        //qDebug() << "ECU ID:" << parse_ecuid(received);
        //qDebug() << "ECU INIT length:" << QString::number((uint8_t)received.at(3));
        //qDebug() << "ECU INIT:" << parse_message_to_hex(received);
        if (received.length() > 0)
        {
            if (received.length() == (uint8_t)received.at(3) + 5)
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
        }
    }
    ecu_init_started = false;
}

void MainWindow::ssm_can_init()
{
    QByteArray output;
    QByteArray received;

    if (configValues->flash_protocol_selected_log_protocol == "CAN")
        received = serial->read_serial_data(100, 500);
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
        if (serial->use_openport2_adapter)
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

    //qDebug() << parse_message_to_hex(received);

    logValues->log_values_by_protocol.clear();

    for (int i = 0; i < log_value_count; i++)
    {
        if (logValues->log_value_protocol.at(i) == protocol)
        {
            logValues->log_values_by_protocol.append(logValues->log_value_name.at(i));

            uint16_t ecu_byte_index = logValues->log_value_ecu_byte_index.at(i).toUInt() + 5;
            if (ecu_byte_index <= received.length() && logValues->log_value_ecu_byte_index.at(i) != "No byte index")
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

    for (int i = 0; i < log_switch_count; i++)
    {
        if (logValues->log_switch_protocol.at(i) == protocol)
        {
            uint16_t switch_byte_index = logValues->log_switch_ecu_byte_index.at(i).toUInt() + 5;
            if (switch_byte_index <= received.length())
            {
                uint8_t switch_bit = logValues->log_switch_ecu_bit.at(i).toUInt();
                uint8_t value = (uint8_t)received.at(switch_byte_index);
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

    update_logboxes(protocol);
}

QByteArray MainWindow::add_ssm_header(QByteArray output, bool dec_0x100)
{
    uint8_t length = output.length();

    output.insert(0, (uint8_t)0x80);
    output.insert(1, (uint8_t)0x10);
    output.insert(2, (uint8_t)0xF0);
    output.insert(3, length);
    output.append(calculate_checksum(output, dec_0x100));

    //qDebug() << "Generated SSM message:" << parseMessageToHex(output);

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
    if (write_log_to_file){
        if (!log_file_open){
            QDateTime dateTime = dateTime.currentDateTime();
            QString dateTimeString = dateTime.toString("yyyy-MM-dd hh'h'mm'm'ss's'");

            QString log_file_name = configValues->log_files_base_directory;
            if (configValues->log_files_base_directory.at(configValues->log_files_base_directory.length() - 1) != "/")
                log_file_name.append("/");
            log_file_name.append("fastecu_" + dateTimeString + ".csv");

            log_file.setFileName(log_file_name);
            if (!log_file.open(QIODevice::WriteOnly)) {
                QMessageBox::information(this, tr("Unable to open file"),
                log_file.errorString());
                return;
            }
            else
            {
                log_file_open = true;
                log_file_timer->start();
            }

            log_file_outstream.setDevice(&log_file);
            log_file_outstream << "Time,";
            for (int j = 0; j < logValues->dashboard_log_value_id.count() ; j++){
                log_file_outstream << logValues->log_value_name.at(logValues->log_value_id.indexOf(logValues->dashboard_log_value_id.at(j), 0)) << ",";
            }
            for (int j = 0; j < logValues->lower_panel_log_value_id.count() ; j++){
                log_file_outstream << logValues->log_value_name.at(logValues->log_value_id.indexOf(logValues->lower_panel_log_value_id.at(j), 0)) << ",";
            }
            for (int j = 0; j < logValues->lower_panel_switch_id.count() ; j++){
                log_file_outstream << logValues->log_switch_name.at(logValues->log_switch_id.indexOf(logValues->lower_panel_switch_id.at(j),0)) << ",";
            }
            log_file_outstream << "\n";
        }
        else{

            log_file_outstream << QString::number(log_file_timer->elapsed() / 1000.0f) << ",";
            for (int j = 0; j < logValues->dashboard_log_value_id.count() ; j++){
                log_file_outstream << logValues->log_value.at(logValues->log_value_id.indexOf(logValues->dashboard_log_value_id.at(j), 0)) << ",";
            }
            for (int j = 0; j < logValues->lower_panel_log_value_id.count() ; j++){
                log_file_outstream << logValues->log_value.at(logValues->log_value_id.indexOf(logValues->lower_panel_log_value_id.at(j), 0)) << ",";
            }
            for (int j = 0; j < logValues->lower_panel_switch_id.count() ; j++){
                log_file_outstream << logValues->log_switch_state.at(logValues->log_switch_id.indexOf(logValues->lower_panel_switch_id.at(j), 0)) << ",";
            }
            log_file_outstream << "\n";
        }
    }
}

