#include "ecu_operations_iso14230.h"

EcuOperationsIso14230::EcuOperationsIso14230(QWidget *ui, SerialPortActions *serial)
{
    this->setParent(ui);
    this->setAttribute(Qt::WA_QuitOnClose, true);
    this->serial = serial;
    this->flash_window = ui;

    set_progressbar_value(0);
}

EcuOperationsIso14230::~EcuOperationsIso14230()
{

}

int EcuOperationsIso14230::fast_init(QByteArray output)
{
    serial->fast_init(output);

    return STATUS_SUCCESS;
}

QByteArray EcuOperationsIso14230::request_timings()
{
    QByteArray output;
    QByteArray received;

    output.clear();
    output.append((uint8_t)0xC1);
    output.append((uint8_t)0x33);
    output.append((uint8_t)0xF1);
    output.append((uint8_t)0x83);
    serial->write_serial_data(output);
    delay(100);
    received = serial->read_serial_data(50, 50);
    if (received.at(3) == 0x7f)
    {
        //qDebug() << parse_message_to_hex(received);
        received.clear();
        received.append("N/A");
        return received;
    }

    //qDebug() << "Timings:" << parse_message_to_hex(timings);

    return received;
}

QByteArray EcuOperationsIso14230::request_service1_pids()
{
    QByteArray output;
    QByteArray received;
    QByteArray supported_pids;

    //qDebug() << "Request 'Service 1 PIDs'";
    supported_pids.clear();
    output.clear();
    output.append((uint8_t)0xC2);
    output.append((uint8_t)0x33);
    output.append((uint8_t)0xF1);
    output.append((uint8_t)0x01);
    output.append((uint8_t)0x00);
    for (int i = 0; i < 0xE0; i += 0x20)
    {
        //output = service1pid0;
        output[4] = (uint8_t)i;
        //qDebug() << parse_message_to_hex(output);
        serial->write_serial_data(output);
        delay(100);
        received = serial->read_serial_data(7, 50);
        if (received.at(3) != 0x7f)
        {
            //qDebug() << "Supported PID's:" << parse_message_to_hex(received);
            for (int j = 0; j < 4; j++)
            {
                for (int k = 8; k > 0; k--)
                {
                    if ((received.at(5 + j) >> k) & 0xFF)
                        supported_pids.append(i + j * 8 + 9 - k);
                }
            }
        }
        delay(50);
    }
    //qDebug() << "Supported PIDs:" << parse_message_to_hex(supported_pids);
    /*
    for (i = 0; i < supported_pids.length(); i++)
    {
        output[4] = (uint8_t)supported_pids.at(i);
        serial->write_serial_data(output);
        delay(50);
        received = serial->read_serial_data(7, 500);
        if (received.at(2) == 0x7a && received.at(3) == 0x41)
        {
            QString msg = QString("PID 0x%1 RX: ").arg(supported_pids.at(i),2,16,QLatin1Char('0'));
            qDebug() << msg + parse_message_to_hex(received);
        }
        else
        {
            QString msg = QString("PID 0x%1 not supported").arg(supported_pids.at(i),2,16,QLatin1Char('0'));
            qDebug() << msg;
        }
        delay(50);
    }
    */
    return supported_pids;
}

QByteArray EcuOperationsIso14230::request_service9_pids()
{
    QByteArray output;
    QByteArray received;
    QByteArray supported_pids;

    //qDebug() << "Request 'Service 9 PIDs'";
    supported_pids.clear();
    output.clear();
    output.append((uint8_t)0xC2);
    output.append((uint8_t)0x33);
    output.append((uint8_t)0xF1);
    output.append((uint8_t)0x09);
    output.append((uint8_t)0x00);

    serial->write_serial_data(output);
    delay(100);
    received = serial->read_serial_data(7, 50);
    if (received.at(3) != 0x7f)
    {
        //qDebug() << "Supported PID's:" << parse_message_to_hex(received);
        for (int j = 0; j < 4; j++)
        {
            for (int k = 8; k > 0; k--)
            {
                if ((received.at(6 + j) >> k) & 0xFF)
                    supported_pids.append(j * 8 + 9 - k);
            }
        }
    }

    //qDebug() << "Supported PID's:" << parse_message_to_hex(received);

    return supported_pids;
}

QString EcuOperationsIso14230::request_vin()
{
    QByteArray output;
    QByteArray received;
    QByteArray vin;

    int vin_msg_count = 0;

    output.clear();
    output.append((uint8_t)0xC2);
    output.append((uint8_t)0x33);
    output.append((uint8_t)0xF1);
    output.append((uint8_t)0x09);
    output.append((uint8_t)0x01);
    serial->write_serial_data(output);
    delay(100);
    received = serial->read_serial_data(50, 50);
    if (received.at(3) == 0x7f)
    {
        //qDebug() << parse_message_to_hex(received);
        received.clear();
        received.append("N/A");
        return received;
    }
    vin_msg_count = received.at(5);
    //qDebug() << "VIN msg count:" << vin_msg_count << parse_message_to_hex(received);

    output[4] = (uint8_t)0x02;
    serial->write_serial_data(output);
    for (int i = 0; i < vin_msg_count; i++)
    {
        delay(100);
        received = serial->read_serial_data(50, 50);
        received.remove(0, 6);
        received.replace('\x00', "");
        vin.append(received);
    }
    return vin;
}

QStringList EcuOperationsIso14230::request_cal_id()
{
    QByteArray output;
    QByteArray received;
    QStringList cal_id_list;

    int cal_id_msg_count = 0;

    output.clear();
    output.append((uint8_t)0xC2);
    output.append((uint8_t)0x33);
    output.append((uint8_t)0xF1);
    output.append((uint8_t)0x09);
    output.append((uint8_t)0x03);
    serial->write_serial_data(output);
    delay(100);
    received = serial->read_serial_data(50, 50);
    if (received.at(3) == 0x7f)
    {
        //qDebug() << parse_message_to_hex(received);
        cal_id_list.clear();
        cal_id_list.append("N/A");
        return cal_id_list;
    }
    cal_id_msg_count = received.at(5) / 4;
    //qDebug() << "cal id msg count:" << cal_id_msg_count << parse_message_to_hex(received);

    output[4] = (uint8_t)0x04;
    serial->write_serial_data(output);
    for (int i = 0; i < cal_id_msg_count; i++)
    {
        QByteArray cal_id;
        for (int j = 0; j < 4; j++)
        {
            received = serial->read_serial_data(50, 50);
            //qDebug() << "cal id msg:" << parse_message_to_hex(received);
            received.remove(0, 6);
            received.replace('\x00', "");
            cal_id.append(received);
        }
        cal_id_list.append(cal_id);
    }
    return cal_id_list;
}

QString EcuOperationsIso14230::request_cvn()
{
    QByteArray output;
    QByteArray received;
    QString cvn;

    int cvn_msg_count = 0;

    output.clear();
    output.append((uint8_t)0xC2);
    output.append((uint8_t)0x33);
    output.append((uint8_t)0xF1);
    output.append((uint8_t)0x09);
    output.append((uint8_t)0x05);
    serial->write_serial_data(output);
    delay(100);
    received = serial->read_serial_data(50, 50);
    if (received.at(3) == 0x7f)
    {
        //qDebug() << parse_message_to_hex(received);
        received.clear();
        received.append("N/A");
        return received;
    }
    cvn_msg_count = received.at(5);
    //qDebug() << "CVN msg count:" << cvn_msg_count << parse_message_to_hex(received);

    output[4] = (uint8_t)0x06;
    serial->write_serial_data(output);
    for (int i = 0; i < cvn_msg_count; i++)
    {
        received = serial->read_serial_data(50, 50);
        //qDebug() << "CVN msg:" << parse_message_to_hex(received);
        received.remove(0, 6);
        received.replace('\x00', "");
        for (int j = 0; j < received.length(); j++)
        {
            QString msg = QString("%1").arg(received.at(j),2,16,QLatin1Char('0'));
            cvn.append(msg);
        }
    }
    return cvn;
}

QString EcuOperationsIso14230::request_ecu_name()
{
    QByteArray output;
    QByteArray received;
    QByteArray ecu_name;

    int ecu_name_msg_count = 0;

    output.clear();
    output.append((uint8_t)0xC2);
    output.append((uint8_t)0x33);
    output.append((uint8_t)0xF1);
    output.append((uint8_t)0x09);
    output.append((uint8_t)0x09);
    serial->write_serial_data(output);
    delay(100);
    received = serial->read_serial_data(50, 50);
    if (received.at(3) == 0x7f)
    {
        //qDebug() << parse_message_to_hex(received);
        received.clear();
        received.append("N/A");
        return received;
    }
    ecu_name_msg_count = received.at(5);
    qDebug() << "ECU name msg count:" << ecu_name_msg_count << parse_message_to_hex(received);

    output[4] = (uint8_t)0x0A;
    serial->write_serial_data(output);
    for (int i = 0; i < ecu_name_msg_count; i++)
    {
        delay(100);
        received = serial->read_serial_data(50, 50);
        qDebug() << "ECU name msg:" << parse_message_to_hex(received);
        received.remove(0, 6);
        received.replace('\x00', "");
        ecu_name.append(received);
    }
    return ecu_name;
}
/*
QByteArray EcuOperationsIso14230::read_data_by_local_id()
{

}
*/
QByteArray EcuOperationsIso14230::read_data(QByteArray header, QByteArray payload)
{
    QByteArray output;
    QByteArray received;

    output.clear();
    output.append(header);
    output.append(payload);

    serial->write_serial_data(output);
    delay(50);
    received = serial->read_serial_data(50, 50);

    return received;
}








QString EcuOperationsIso14230::parse_message_to_hex(QByteArray received)
{
    QByteArray msg;

    for (int i = 0; i < received.length(); i++)
    {
        msg.append(QString("%1 ").arg((uint8_t)received.at(i),2,16,QLatin1Char('0')).toUtf8());
    }

    return msg;
}

void EcuOperationsIso14230::set_progressbar_value(int value)
{
    QProgressBar* progressbar = flash_window->findChild<QProgressBar*>("progressbar");
    if (progressbar)
    {
        progressbar->setValue(value);
    }
    QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
}

void EcuOperationsIso14230::delay(int timeout)
{
    QTime dieTime = QTime::currentTime().addMSecs(timeout);
    while (QTime::currentTime() < dieTime)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
}

