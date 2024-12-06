// This is an open source non-commercial project. Dear PVS-Studio, please check it.

// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

#include "serial_port_actions_direct.h"

SerialPortActionsDirect::SerialPortActionsDirect(QObject *parent)
    : QObject(parent)
    , serial(new QSerialPort(this))
{
    j2534 = new J2534();
}

SerialPortActionsDirect::~SerialPortActionsDirect()
{
    delete j2534;
    delete serial;
}

bool SerialPortActionsDirect::is_serial_port_open()
{
    if (!serial->isOpen()){
        if (!J2534_init_ok)
            return false;
        else
            return j2534->is_serial_port_open();
    }

    return serial->isOpen();
}

int SerialPortActionsDirect::change_port_speed(QString portSpeed)
{
    serial_port_baudrate = portSpeed;
    baudrate = portSpeed.toInt();

    qDebug() << "Changing baudrate, checking if port is open...";
    if (is_serial_port_open())
    {
        qDebug() << "Port is open, checking adapter type...";
        if (!use_openport2_adapter)
        {
            qDebug() << "Adapter type is generic OBD2...";

            if (serial->setBaudRate(serial_port_baudrate.toDouble()))
            {
                delay(50);
                qDebug() << "Baudrate set to" << serial_port_baudrate;
                return STATUS_SUCCESS;
            }
            else
            {
                qDebug() << "ERROR setting baudrate!";
                return STATUS_ERROR;
            }
        }
        else
        {
            qDebug() << "Adapter type is J2534...";

            SCONFIG_LIST scl;
            SCONFIG scp[1] = {{DATA_RATE,0}};
            scl.NumOfParams = 1;
            scp[0].Value = baudrate;
            scl.ConfigPtr = scp;
            if (!j2534->PassThruIoctl(chanID,SET_CONFIG,&scl,NULL))
            {
                qDebug() << "Baudrate set to" << baudrate << "OK";
                return STATUS_SUCCESS;
            }
            else
            {
                reportJ2534Error();
                return STATUS_ERROR;
            }
        }
    }

    return STATUS_ERROR;
}

int SerialPortActionsDirect::fast_init(QByteArray output)
{
    if (use_openport2_adapter)
    {
        unsigned long status;
        PASSTHRU_MSG InputMsg;
        PASSTHRU_MSG OutputMsg;

        memset(&InputMsg, 0, sizeof(InputMsg));
        memset(&OutputMsg, 0, sizeof(OutputMsg));

        InputMsg.ProtocolID = ISO14230;
        InputMsg.TxFlags = 0;
        for (int i = 0; i < output.length(); i++)
        {
            InputMsg.Data[i] = output.at(i);
        }
        InputMsg.DataSize = output.length();

        status = j2534->PassThruIoctl(chanID, FAST_INIT, &InputMsg, &OutputMsg);
        if (status)
        {
            reportJ2534Error();
            return STATUS_ERROR;
        }
        else
        {
            //qDebug() << "FAST_INIT OK";
        }
    }
    else
    {
        QByteArray init;
        QByteArray received;
        double seconds = (double)350 / (double)1000;
        auto spin_start = std::chrono::high_resolution_clock::now();

        //serial->setBaudRate(10400);

        /* Set timeout to 350ms before init */
        seconds = (double)350 / (double)1000;
        spin_start = std::chrono::high_resolution_clock::now();
        while ((std::chrono::high_resolution_clock::now() - spin_start).count() / 1e9 < seconds);
        /* Set break to set seril line low */
        serial->setBreakEnabled(true);
        /* Set timeout to 25ms to generate 25ms low pulse  */
        seconds = (double)25.1 / (double)1000;
        spin_start = std::chrono::high_resolution_clock::now();
        while ((std::chrono::high_resolution_clock::now() - spin_start).count() / 1e9 < seconds);
        /* Unset break to set seril line high */
        serial->setBreakEnabled(false);
        /* Set timeout to 25ms to generate 25ms high pulse before init data is sent */
        seconds = (double)24.7 / (double)1000;
        spin_start = std::chrono::high_resolution_clock::now();
        while ((std::chrono::high_resolution_clock::now() - spin_start).count() / 1e9 < seconds);
        /* Send init data */
        received = write_serial_data_echo_check(output);
        received.append(read_serial_data(1, 10));
        qDebug() << parse_message_to_hex(received);
        delay(100);
    }

    return STATUS_SUCCESS;
}

int SerialPortActionsDirect::clear_rx_buffer()
{
    if (use_openport2_adapter)
    {
        unsigned long status;

        status = j2534->PassThruIoctl(chanID, CLEAR_RX_BUFFER, NULL, NULL);
        if (status)
        {
            reportJ2534Error();
            return STATUS_ERROR;
        }
        else
        {
            qDebug() << "RX BUFFER EMPTY";
        }
    }

    return STATUS_SUCCESS;
}

int SerialPortActionsDirect::clear_tx_buffer()
{
    if (use_openport2_adapter)
    {
        unsigned long status;

        status = j2534->PassThruIoctl(chanID, CLEAR_TX_BUFFER, NULL, NULL);
        if (status)
        {
            reportJ2534Error();
            return STATUS_ERROR;
        }
        else
        {
            qDebug() << "TX BUFFER EMPTY";
        }
    }

    return STATUS_SUCCESS;
}

int SerialPortActionsDirect::set_lec_lines(int lec1_state, int lec2_state)
{
    line_end_check_1_toggled(lec1_state);
    line_end_check_2_toggled(lec2_state);

    return STATUS_SUCCESS;
}

int SerialPortActionsDirect::pulse_lec_1_line(int timeout)
{
    line_end_check_1_toggled(requestToSendEnabled);
    delay(timeout);
    line_end_check_1_toggled(requestToSendDisabled);
    delay(timeout);

    read_serial_data(100, 50);

    return STATUS_SUCCESS;
}

int SerialPortActionsDirect::pulse_lec_2_line(int timeout)
{
    line_end_check_2_toggled(dataTerminalEnabled);
    delay(timeout);
    line_end_check_2_toggled(dataTerminalDisabled);
    delay(timeout);

    read_serial_data(100, 50);

    return STATUS_SUCCESS;
}

int SerialPortActionsDirect::line_end_check_1_toggled(int state)
{
    if (state == requestToSendEnabled)
    {
        if (use_openport2_adapter)
        {
            j2534->PassThruSetProgrammingVoltage(devID, J1962_PIN_11, 12000);
#if defined Q_OS_UNIX
            delay(17);
#endif
        }
        else
        {
            serial->setRequestToSend(requestToSendEnabled);
            setRequestToSend = false;
        }
    }
    else
    {
        if (use_openport2_adapter)
        {
            j2534->PassThruSetProgrammingVoltage(devID, J1962_PIN_11, -2);
        }
        else
        {
            serial->setRequestToSend(requestToSendDisabled);
            setRequestToSend = true;
        }
    }

    return STATUS_SUCCESS;
}

int SerialPortActionsDirect::line_end_check_2_toggled(int state)
{
    QByteArray received;

    if (state == dataTerminalEnabled)
    {
        if (use_openport2_adapter)
        {
            j2534->PassThruSetProgrammingVoltage(devID, J1962_PIN_9, 5000);
#if defined Q_OS_UNIX
            delay(17);
#endif
        }
        else
        {
            serial->setDataTerminalReady(dataTerminalEnabled);
            setDataTerminalReady = false;
        }
    }
    else
    {
        if (use_openport2_adapter)
        {
            j2534->PassThruSetProgrammingVoltage(devID, J1962_PIN_9, -2);
        }
        else
        {
            serial->setDataTerminalReady(dataTerminalDisabled);
            setDataTerminalReady = true;
        }
    }

    return STATUS_SUCCESS;
}

QStringList SerialPortActionsDirect::check_serial_ports()
{
    const auto serialPortsInfo = QSerialPortInfo::availablePorts();
    QStringList serial_ports;
    QString j2534DllName = "j2534.dll";

    serialPortAvailable = false;

    for (const QSerialPortInfo &serialPortInfo : serialPortsInfo){
        serial_ports.append(serialPortInfo.portName() + " - " + serialPortInfo.description());
        qDebug() << "Serial port name:" << serialPortInfo.portName() << serialPortInfo.description();
    }
    std::sort(serial_ports.begin(), serial_ports.end(), std::less<QString>());

#if defined(_WIN32) || defined(WIN32) || defined (_WIN64) || defined (WIN64)
    QStringList j2534_interfaces;
    installed_drivers = getAllJ2534DriversNames();
    for (const QString installed_vendor : installed_drivers.keys())
    {
        j2534_interfaces.append(installed_vendor);
    }
    std::sort(j2534_interfaces.begin(), j2534_interfaces.end(), std::less<QString>());
    serial_ports.append(j2534_interfaces);
#endif

    return serial_ports;
}

#if defined(_WIN32) || defined(WIN32) || defined (_WIN64) || defined (WIN64)
    #if defined(_WIN32) || defined(WIN32)
        #define REGISTRY_FORMAT QSettings::Registry32Format
    #else
        #define REGISTRY_FORMAT QSettings::Registry64Format
    #endif

//Find first connected device
//TODO find all devices
QStringList SerialPortActionsDirect::check_j2534_devices(QMap<QString, QString> installed_drivers)
{
    bool j2534DeviceFound = false;
    QStringList j2534_devices;
    int driver_count = 0;
    for (const QString vendor : installed_drivers.keys())
    {
        driver_count++;
        j2534->disable();
        //close_j2534_serial_port();
        QString j2534DllName = installed_drivers[vendor];
        qDebug() << "Testing for " + j2534DllName;
        j2534->setDllName(j2534DllName.toLocal8Bit().data());
        if (j2534->init())
        {
            qDebug() << j2534DllName << "init successfull";
            //0 means no error
            if (!j2534->PassThruOpen(NULL, &devID))
            {
                qDebug() << "Successfully opened" << devID << vendor << j2534DllName;
                j2534_devices.append(vendor);
                j2534DeviceFound = true;
                j2534->PassThruClose(devID);
            }
            else
                qDebug() << devID << vendor << "device not connected";
        }
        else
            qDebug() << j2534DllName << "not found";
        if (j2534DeviceFound)
            break;
    }
    qDebug() << "Tested installed drivers:" << driver_count;

    return j2534_devices;
}

QMap<QString, QString> SerialPortActionsDirect::getAllJ2534DriversNames()
{
    QSettings registry("HKEY_LOCAL_MACHINE\\SOFTWARE\\PassThruSupport.04.04", REGISTRY_FORMAT);
    QMap<QString, QString> drivers_map;
    for (const QString &i : registry.childGroups())
    {
        QString vendor = i;
        //qDebug() << "J2534 Drivers:" << vendor;
        vendor.replace("\\", "/");
        QString dllName = registry.value(i+"/FunctionLibrary").toString();
        drivers_map[vendor] = dllName;
    }
    qDebug() << "Found installed drivers:" << drivers_map;
    return drivers_map;
}
#endif

QString SerialPortActionsDirect::open_serial_port()
{
    qDebug() << "Serial port =" << serial_port_list;
    //QString serial_port_text = serial_port_list.at(1);
#if defined Q_OS_UNIX
    serial_port = serial_port_prefix_linux + serial_port_list.at(0);
    serial_port = serial_port.split(" - ").at(0);
#endif
#if defined(_WIN32) || defined(WIN32) || defined (_WIN64) || defined (WIN64)
    serial_port = serial_port_prefix_win + serial_port_list.at(0);
#endif
    qDebug() << "Interface:" << serial_port;

    if (!serial_port.isEmpty())
    {
        reset_connection();
        //close_serial_port();
        //use_openport2_adapter = true;

        J2534_is_denso_dsti = serial_port.contains("DST-i");

        //QMap<QString, QString> user_j2534_drivers; // Local drivers in software folder
#if defined(_WIN32) || defined(WIN32) || defined (_WIN64) || defined (WIN64)
        QString localDllName;
        QString installedDllName;

        QStringList dllName = installed_drivers.value(serial_port).split("\\");
        localDllName = dllName.at(dllName.count() - 1);
        installedDllName = installed_drivers.value(serial_port);
        qDebug() << "Local DLL Name:" << localDllName;
        qDebug() << "Installed DLL Name:" << installedDllName;

        QMap<QString, QString> user_j2534_drivers;

        qDebug() << "Opening device:" << serial_port;
        QStringList j2534_driver;
        user_j2534_drivers[serial_port] = localDllName;
        j2534_driver = check_j2534_devices(user_j2534_drivers);
        user_j2534_drivers[serial_port] = installedDllName;
        if (j2534_driver.isEmpty())
            j2534_driver = check_j2534_devices(user_j2534_drivers);
        if (!j2534_driver.isEmpty())
            j2534->setDllName(j2534_driver.at(0).toLocal8Bit().data());
        else
            qDebug() << "Initializing interface failed!";
#endif
        long result;

        qDebug() << "Testing j2534 interface, please wait...";
        result = init_j2534_connection();

        if (result == STATUS_SUCCESS)
        {
            qDebug() << "Interface opened succesfully!";
            use_openport2_adapter = true;
            J2534_init_ok = true;
            j2534->J2534_init_ok = true;
            openedSerialPort = serial_port;
        }
        else
        {
            qDebug() << "Failed to open interface!";
            use_openport2_adapter = false;
            reset_connection();
        }
    }
    if (!use_openport2_adapter && openedSerialPort != serial_port)
    {
        qDebug() << "Testing serial interface, please wait...";
        //close_serial_port();

        serial_port = serial_port.split(" - ").at(0);
#if defined Q_OS_UNIX
            //serial_port = serial_port_prefix_linux + serial_port;
#elif defined Q_OS_WIN32
            //serial_port = serial_port_prefix_win + serial_port;
#endif

        if (!(serial->isOpen() && serial->isWritable())){
            serial->setPortName(serial_port);
            serial->setBaudRate(serial_port_baudrate.toDouble());
            serial->setDataBits(QSerialPort::Data8);
            serial->setStopBits(QSerialPort::OneStop);
            //serial->setParity(QSerialPort::EvenParity);
            serial->setParity((QSerialPort::Parity)serial_port_parity);
            serial->setFlowControl(QSerialPort::NoFlowControl);

            if (serial->open(QIODevice::ReadWrite)){
                //serial->setDataTerminalReady(setDataTerminalReady);
                //serial->setRequestToSend(setRequestToSend);
                serial->clearError();
                serial->clear();
                serial->flush();
                openedSerialPort = serial_port;
                //connect(serial, SIGNAL(readyRead()), this, SLOT(ReadSerialDataSlot()), Qt::DirectConnection);
                qRegisterMetaType<QSerialPort::SerialPortError>();
                connect(serial, SIGNAL(errorOccurred(QSerialPort::SerialPortError)), this, SLOT(handle_error(QSerialPort::SerialPortError)));

                //send_log_window_message("Serial port '" + serialPort + "' is open at baudrate " + serialPortBaudRate, true, true);
                qDebug() << "Serial port '" + serial_port + "' is open at baudrate " + serial_port_baudrate;
                return openedSerialPort;
            }
            else
            {
                //sendLogWindowMessage("Couldn't open serial port '" + serialPort + "'", true, true);
                qDebug() << "Couldn't open serial port '" + serial_port + "'";
                return NULL;
            }

        }
        else{
            //sendLogWindowMessage("Serial port '" + serialPort + "' is already opened", true, true);
            qDebug() << "Serial port '" + serial_port + "' is already opened";
            return openedSerialPort;
        }
    }

    return openedSerialPort;
}

void SerialPortActionsDirect::reset_connection()
{
    close_serial_port();
    close_j2534_serial_port();
}

void SerialPortActionsDirect::close_serial_port()
{
    if (serial->isOpen())
    {
        serial->close();
        openedSerialPort.clear();
    }
}

void SerialPortActionsDirect::close_j2534_serial_port()
{
    //qDebug() << "J2534_init_ok:" << J2534_init_ok;
    if (j2534->is_serial_port_open())
    {
        j2534->PassThruDisconnect(chanID);
        j2534->PassThruClose(devID);
    }
    use_openport2_adapter = false;
    J2534_open_ok = false;
    J2534_get_version_ok = false;
    J2534_connect_ok = false;
    J2534_timing_ok = false;
    J2534_filters_ok = false;
    J2534_init_ok = false;
    j2534->J2534_init_ok = false;
    openedSerialPort.clear();
    char dllName[256];
    j2534->getDllName(dllName);
    delete j2534;
    j2534 = new J2534();
    j2534->setDllName(dllName);
}

QByteArray SerialPortActionsDirect::read_serial_data(uint32_t datalen, uint16_t timeout)
{
    QByteArray ReceivedData;
    QByteArray PayloadData;
    QByteArray received;

    if (is_serial_port_open())
    {
        if (use_openport2_adapter)
        {
            received = read_j2534_data(timeout);
            return received;
        }
        else
        {
            QTime dieTime = QTime::currentTime().addMSecs(timeout);
            while ((uint32_t)ReceivedData.length() < datalen && (QTime::currentTime() < dieTime))
            {
                if (serial->bytesAvailable())
                {
                    dieTime = QTime::currentTime().addMSecs(timeout);
                    ReceivedData.append(serial->read(1));
                }
                QCoreApplication::processEvents(QEventLoop::AllEvents, 1);

            }
        }
        return ReceivedData;
    }
    return ReceivedData;
}

QByteArray SerialPortActionsDirect::write_serial_data(QByteArray output)
{
    QByteArray received;
    QByteArray msg;

    msg.append((uint8_t)0x00);

    if (is_serial_port_open())
    {
        if (add_iso14230_header)
            output = add_packet_header(output);

        if (use_openport2_adapter)
        {
            write_j2534_data(output);
            return 0;
        }
        for (int i = 0; i < output.length(); i++)
        {
            msg[0] = output.at(i);
            serial->write(msg, 1);
        }
        //qDebug() << "Data sent:" << parse_message_to_hex(output);

        return received;
    }
    //send_log_window_message("Serial port not open", true, true);

    return received;
}

QByteArray SerialPortActionsDirect::write_serial_data_echo_check(QByteArray output)
{
    QByteArray received;
    QByteArray msg;

    msg.append((uint8_t)0x00);

    if (is_serial_port_open())
    {
        if (add_iso14230_header)
            output = add_packet_header(output);

        if (use_openport2_adapter)
        {
            write_j2534_data(output);
            return STATUS_SUCCESS;
        }
        for (int i = 0; i < output.length(); i++)
        {
            msg[0] = output.at(i);
            serial->write(msg, 1);
            // Add serial echo read during transmit to speed up a little
            if (serial->bytesAvailable())
                received.append(serial->read(1));
        }
        QTime dieTime = QTime::currentTime().addMSecs(echo_check_timout);
        //qDebug() << "Data sent:" << parse_message_to_hex(output);

        while (received.length() < output.length() && (QTime::currentTime() < dieTime))
        {
            if (serial->bytesAvailable())
            {
                dieTime = QTime::currentTime().addMSecs(echo_check_timout);
                received.append(serial->read(1));
            }
            QCoreApplication::processEvents(QEventLoop::AllEvents, 1);
        }

        return received;
    }
    //send_log_window_message("Serial port not open", true, true);

    return received;
}

QByteArray SerialPortActionsDirect::add_packet_header(QByteArray output)
{
    uint8_t chk_sum = 0;
    uint8_t msglength = output.length();

    //qDebug() << "Adding iso14230 header to message";

    output.insert(0, iso14230_startbyte);
    output.insert(1, iso14230_target_id);
    output.insert(2, iso14230_tester_id);
    if (msglength < 0x40)
        output[0] = output[0] | msglength;
    else
        output.insert(3, msglength);

    for (int i = 0; i < output.length(); i++)
        chk_sum = chk_sum + output.at(i);

    output.append(chk_sum);

    return output;
}

int SerialPortActionsDirect::write_j2534_data(QByteArray output)
{
    PASSTHRU_MSG txmsg;
    unsigned long NumMsgs;
    unsigned long numMsgs;
    PASSTHRU_MSG rxmsg;
    unsigned long numRxMsg;
    unsigned long msgID = 10;
    long txMsgLen;
    int PASSTHRU_MSG_MAX_DATA_SIZE = 256;

    txMsgLen = output.length();
    if (txMsgLen > PASSTHRU_MSG_DATA_SIZE)
        txMsgLen -= txMsgLen - PASSTHRU_MSG_DATA_SIZE;

    numMsgs = 0;

    while (txMsgLen > 0)
    {
        txmsg.ProtocolID = protocol;
        txmsg.RxStatus = 0;
        txmsg.TxFlags = 0;
        if (protocol == CAN)
        {
            if (is_29_bit_id)
                txmsg.TxFlags = CAN_29BIT_ID;
        }
        txmsg.TxFlags |= ISO15765_FRAME_PAD;
        txmsg.Timestamp = 0;
        txmsg.DataSize = txMsgLen;
        txmsg.ExtraDataIndex = 0;

        for (long i = 0; i < txMsgLen; i++)
        {
            txmsg.Data[i] = (uint8_t)output.at(i);
        }
        // Indicate that the PASSTHRU_MSG array contains just a single message.
        NumMsgs = 1;

        j2534->PassThruWriteMsgs(chanID, &txmsg, &NumMsgs, 100);
        //qDebug() << "Data sent:" << parse_message_to_hex(output);

        numMsgs++;
        output.remove(0, txMsgLen);
        txMsgLen = output.length();
        if (txMsgLen > PASSTHRU_MSG_DATA_SIZE)
            txMsgLen -= txMsgLen - PASSTHRU_MSG_DATA_SIZE;
    }

    return STATUS_SUCCESS;
}

int SerialPortActionsDirect::send_periodic_j2534_data(QByteArray output, int timeout)
{
    PASSTHRU_MSG txmsg;
    unsigned long NumMsgs;
    unsigned long numMsgs;
    PASSTHRU_MSG rxmsg;
    unsigned long numRxMsg;
    long txMsgLen;
    int PASSTHRU_MSG_MAX_DATA_SIZE = 256;

    txMsgLen = output.length();
    if (txMsgLen > PASSTHRU_MSG_DATA_SIZE)
        txMsgLen -= txMsgLen - PASSTHRU_MSG_DATA_SIZE;

    numMsgs = 0;

    while (txMsgLen > 0)
    {
        qDebug() << "Start periodic messages with protocol:" << protocol;
        txmsg.ProtocolID = protocol;
        txmsg.RxStatus = 0;
        txmsg.TxFlags = 0;
        txmsg.Timestamp = 0;
        txmsg.DataSize = txMsgLen;
        txmsg.ExtraDataIndex = 0;

        for (long i = 0; i < txMsgLen; i++)
        {
            txmsg.Data[i] = (uint8_t)output.at(i);
        }
        // Indicate that the PASSTHRU_MSG array contains just a single message.
        NumMsgs = 1;

        j2534->PassThruStartPeriodicMsg(chanID, &txmsg, &msgID, timeout);
        numMsgs++;
        output.remove(0, txMsgLen);
        txMsgLen = output.length();
        if (txMsgLen > PASSTHRU_MSG_DATA_SIZE)
            txMsgLen -= txMsgLen - PASSTHRU_MSG_DATA_SIZE;
    }

    delay(10);

    qDebug() << "Periodic msgID:" << msgID;

    return STATUS_SUCCESS;
}

int SerialPortActionsDirect::stop_periodic_j2534_data()
{
    PASSTHRU_MSG rxmsg;
    unsigned long numRxMsg;
    unsigned long timeout = 100;

    j2534->PassThruStopPeriodicMsg(chanID, msgID);
    delay(10);
    j2534->PassThruReadMsgs(chanID, &rxmsg, &numRxMsg, timeout);

    return STATUS_SUCCESS;
}

QByteArray SerialPortActionsDirect::read_j2534_data(unsigned long timeout)
{
    PASSTHRU_MSG rxmsg;
    unsigned long numRxMsg;
    unsigned int msgCnt = 0;
    unsigned int byteCnt = 0;
    time_t last_status_update = time(NULL);
    QByteArray received;

    received.clear();

    rxmsg.DataSize = 0;
    numRxMsg = 1;
    //j2534->PassThruReadMsgs(chanID, &rxmsg, &numRxMsg, timeout);
    //0 means no error, all other values mean error
    if (j2534->PassThruReadMsgs(chanID, &rxmsg, &numRxMsg, timeout))
        goto exit;
    //qDebug() << numRxMsg << "messages, rx status" << rxmsg.RxStatus;
    if (numRxMsg)
    {
        //qDebug() << numRxMsg << "messages, rx status" << rxmsg.RxStatus;
        dump_msg(&rxmsg);
        msgCnt++;
        byteCnt += rxmsg.DataSize;

        if (is_can_connection)
        {
            for (unsigned long i = 4; i < rxmsg.DataSize; i++)
                received.append((uint8_t)rxmsg.Data[i]);
        }
        else
        {
            //qDebug() << "RX MSG status:" << rxmsg.RxStatus;
            if (rxmsg.RxStatus & TX_DONE){
                //qDebug() << "TX_DONE_MSG, read actual message";
                rxmsg.DataSize = 0;
                rxmsg.Data[0] = 0x00;
                j2534->PassThruReadMsgs(chanID, &rxmsg, &numRxMsg, timeout);
                //qDebug() << "New RX MSG status:" << rxmsg.RxStatus;
            }
            if (rxmsg.RxStatus & START_OF_MESSAGE){
                //qDebug() << "START_OF_MESSAGE, read actual message";
                j2534->PassThruReadMsgs(chanID, &rxmsg, &numRxMsg, timeout);
            }
            if (rxmsg.RxStatus & RX_MSG_END_IND){
                //qDebug() << "END_OF_MESSAGE" << rxmsg.Data;
            }
            for (unsigned long i = 0; i < rxmsg.DataSize; i++)
                received.append((uint8_t)rxmsg.Data[i]);
        }
    }
    //qDebug() << "RECEIVED:" << parse_message_to_hex(received);
    exit:
    return received;
}

int SerialPortActionsDirect::set_j2534_ioctl(unsigned long parameter, int value)
{
    // Set timeouts etc.
    SCONFIG_LIST scl;
    SCONFIG scp[1] = {{parameter,0}};
    scl.NumOfParams = 1;
    scp[0].Value = value;
    scl.ConfigPtr = scp;
    if (j2534->PassThruIoctl(chanID,SET_CONFIG,&scl,NULL))
    {
        reportJ2534Error();
        return STATUS_ERROR;
    }
    else
    {
        //qDebug() << "Set timings OK";
    }

    return STATUS_SUCCESS;
}

void SerialPortActionsDirect::dump_msg(PASSTHRU_MSG* msg)
{
    QByteArray datamsg;

    //qDebug() << "Dump msg";
    if (msg->RxStatus & START_OF_MESSAGE)
        return; // skip

    datamsg.clear();
    for (unsigned int i = 0; i < msg->DataSize; i++)
    {
        datamsg.append(QString("%1 ").arg(msg->Data[i],2,16,QLatin1Char('0')).toUtf8());
    }
    //qDebug() << "Timestamp:" << msg->Timestamp << "msg length:" << msg->DataSize << "msg:" << datamsg;
}

bool SerialPortActionsDirect::get_serial_num(char* serial)
{
    struct
    {
        unsigned int length;
        unsigned int svcid;
        unsigned short infosvcid;

    } inbuf;

    struct
    {
        unsigned int length;
        unsigned char data[256];
    } outbuf;

    inbuf.length = 2;
    inbuf.svcid = 5; // info
    inbuf.infosvcid = 1; // serial

    outbuf.length = sizeof(outbuf.data);
/*
    if (j2534->PassThruIoctl(devID,TX_IOCTL_APP_SERVICE,&inbuf,&outbuf))
    {
        serial[0] = 0;
        return false;
    }
*/
    memcpy(serial,outbuf.data,outbuf.length);
    serial[outbuf.length] = 0;
    return true;
}

int SerialPortActionsDirect::init_j2534_connection()
{
// If Linux, open serial port
#if defined Q_OS_UNIX
    if (j2534->open_serial_port(serial_port) != serial_port)
        return STATUS_ERROR;
#endif

    // Init J2534 connection (in windows, load DLL etc.)
    if (!j2534->init())
    {
        //qDebug() << "INIT: Can't load J2534 DLL.";
        return STATUS_ERROR;
    }
    else
    {
        //qDebug() << "INIT: J2534 DLL loaded.";
    }

    // Open J2534 connection
    if (j2534->PassThruOpen(NULL, &devID))
    {
#if defined Q_OS_UNIX
        j2534->close_serial_port();
#endif
        reportJ2534Error();
        return STATUS_ERROR;
    }
    else
    {
        //qDebug() << "INIT: J2534 opened, devID" << devID;
    }

    // Get J2534 adapter and driver version numbers
    char strApiVersion[256];
    char strDllVersion[256];
    char strFirmwareVersion[256];
    char strSerial[256];

    if (j2534->PassThruReadVersion(strApiVersion, strDllVersion, strFirmwareVersion, devID))
    {
        reportJ2534Error();
        return STATUS_ERROR;
    }

    if (!get_serial_num(strSerial))
    {
        reportJ2534Error();
        return STATUS_ERROR;
    }

    strApiVersion[strlen(strApiVersion)-1] = '\0';
    strDllVersion[strlen(strDllVersion)-1] = '\0';
    strFirmwareVersion[strlen(strFirmwareVersion)-1] = '\0';
    strSerial[strlen(strSerial)-1] = '\0';
    qDebug() << "J2534 API Version:" << strApiVersion;
    qDebug() << "J2534 DLL Version:" << strDllVersion;
    qDebug() << "Device Firmware Version:" << strFirmwareVersion;
    qDebug() << "Device Serial Number:" << strSerial;

    // Create J2534 to device connections
    if (is_iso15765_connection)
    {
        //set_j2534_can_filters();
        set_j2534_can();
        set_j2534_can_timings();
        set_j2534_can_filters();
    }
    else if (is_can_connection)
    {
        set_j2534_can();
        set_j2534_can_timings();
        set_j2534_can_filters();
    }
    else
    {
        set_j2534_iso9141();
        set_j2534_iso9141_timings();
        set_j2534_iso9141_filters();
    }

    if(is_iso15765_connection)
        qDebug() << "ISO CAN init ready";
    else if (is_can_connection)
        qDebug() << "CAN init ready";
    else
        qDebug() << "K-Line init ready";

    return STATUS_SUCCESS;
}
/*
int SerialPortActionsDirect::set_j2534_can()
{
    QByteArray output;

    output = "\n\nati\n";

    output = "ata\n";                   // open
    output = "ato5 256 500000 0\n";     // connect
    output = "atf5 1 256 4 4\n";        // filters FF FF FF FF 00 00 00 00
    output = "atf5 1 256 4 5\n";        // filters FF FF FF FF 00 00 00 21
    output = "atr 16\n";                // 'read battery voltage'
    output = "att5 12 256 2000000\n";   // send 00 0F FF FE FF 86 00 00 00 00 00 00

    output = "att5 12 256 2000000\n";   // send 00 0F FF FE 7A 9C FF FF 60 00 00 00
                                        // rsp  00 00 00 21 7A 9C FF FF 60 00 41 42

    output = "att5 12 256 2000000\n";   // send 00 0F FF FE 7A AE ?? ?? ?? ?? ?? ??
}
*/
int SerialPortActionsDirect::set_j2534_can()
{
    if (is_can_connection)
    {
        protocol = CAN;
        if (is_29_bit_id)
            flags = CAN_29BIT_ID;
        else
            flags = 0;
    }
    else
    {
        protocol = ISO15765;
        if (is_29_bit_id)
            flags = CAN_29BIT_ID;
        else
            flags = 0;
    }
    //Denso DST-i hack
    if (J2534_is_denso_dsti && protocol == ISO15765)
    {
        flags = 0;
    }
    baudrate = can_speed.toUInt();
    // use ISO9141_NO_CHECKSUM to disable checksumming on both tx and rx messages
    if (j2534->PassThruConnect(devID, protocol, flags, baudrate, &chanID))
    {
        reportJ2534Error();
        return STATUS_ERROR;
    }
    else
    {
#if defined Q_OS_UNIX
        chanID = protocol;
#endif
        //qDebug() << "Connected:" << devID << protocol << baudrate << chanID;
    }

    return STATUS_SUCCESS;
}

int SerialPortActionsDirect::unset_j2534_can()
{
    if (j2534->PassThruDisconnect(devID))
    {
        reportJ2534Error();
        return STATUS_ERROR;
    }

    return STATUS_SUCCESS;
}

/*int SerialPortActionsDirect::set_j2534_stmin_tx()
{
    // Set timeouts etc.
    SCONFIG_LIST scl;
    SCONFIG scp[1] = {{STMIN_TX,0}};
    scl.NumOfParams = 1;
    scp[0].Value = 65535;
    scl.ConfigPtr = scp;
    if (j2534->PassThruIoctl(chanID,SET_CONFIG,&scl,NULL))
    {
        reportJ2534Error();
        return STATUS_ERROR;
    }
    else
    {
        //qDebug() << "Set timings OK";
    }

    return STATUS_SUCCESS;
}*/

int SerialPortActionsDirect::set_j2534_can_timings()
{
    // Set timeouts etc.
    SCONFIG_LIST scl;
    SCONFIG scp[] = {{PARITY,0}, {LOOPBACK, 0}};
    scl.NumOfParams = ARRAYSIZE(scp);
    scp[0].Value = NO_PARITY;
    scl.ConfigPtr = scp;
    if (j2534->PassThruIoctl(chanID,SET_CONFIG,&scl,NULL))
    {
        reportJ2534Error();
        return STATUS_ERROR;
    }
    else
    {
        //qDebug() << "Set timings OK";
    }

    return STATUS_SUCCESS;
}

int SerialPortActionsDirect::set_j2534_can_filters()
{
    // now setup the filter(s)
    PASSTHRU_MSG rxmsg, txmsg;
    PASSTHRU_MSG msgMask, msgPattern, msgFlow;
    unsigned long msgId;
    unsigned long numRxMsg;

    j2534->PassThruIoctl(chanID, CLEAR_MSG_FILTERS, NULL, NULL);

    // simply create a "pass all" filter so that we can see
    // everything unfiltered in the raw stream

    if (protocol == CAN)
    {
        //qDebug() << "Set CAN filters";
        txmsg.ProtocolID = protocol;
        txmsg.RxStatus = 0;
        txmsg.TxFlags = ISO15765_FRAME_PAD;
        txmsg.Timestamp = 0;
        txmsg.DataSize = 4;
        txmsg.ExtraDataIndex = 0;

        msgMask = msgPattern = txmsg;
        memset(msgMask.Data, 0xFF, txmsg.DataSize);
        memset(msgPattern.Data, 0xFF, txmsg.DataSize);
        /*
        msgMask.Data[0] = (can_destination_address >> 24) & 0xFF;
        msgMask.Data[1] = (can_destination_address >> 16) & 0xFF;
        msgMask.Data[2] = (can_destination_address >> 8) & 0xFF;
        msgMask.Data[3] = (can_destination_address & 0xFF);
*/
        msgPattern.Data[0] = (can_destination_address >> 24) & 0xFF;
        msgPattern.Data[1] = (can_destination_address >> 16) & 0xFF;
        msgPattern.Data[2] = (can_destination_address >> 8) & 0xFF;
        msgPattern.Data[3] = (can_destination_address & 0xFF);

        if (j2534->PassThruStartMsgFilter(chanID, PASS_FILTER, &msgMask, &msgPattern, NULL, &msgId))
        {
            reportJ2534Error();
            return STATUS_ERROR;
        }
    }
    else if (protocol == ISO15765)
    {
        txmsg.ProtocolID = protocol;
        txmsg.RxStatus = 0;
        txmsg.TxFlags = ISO15765_FRAME_PAD;
        txmsg.Timestamp = 0;
        txmsg.DataSize = 4;
        txmsg.ExtraDataIndex = 0;
        msgMask = msgPattern = msgFlow = txmsg;
        memset(msgMask.Data, 0xFF, txmsg.DataSize);
        memset(msgPattern.Data, 0xFF, txmsg.DataSize);
        memset(msgFlow.Data, 0xFF, txmsg.DataSize);
        /*
        msgMask.Data[0] = (iso15765_destination_address >> 24) & 0xFF;
        msgMask.Data[1] = (iso15765_destination_address >> 16) & 0xFF;
        msgMask.Data[2] = (iso15765_destination_address >> 8) & 0xFF;
        msgMask.Data[3] = iso15765_destination_address & 0xFF;
*/
        msgPattern.Data[0] = (iso15765_destination_address >> 24) & 0xFF;
        msgPattern.Data[1] = (iso15765_destination_address >> 16) & 0xFF;
        msgPattern.Data[2] = (iso15765_destination_address >> 8) & 0xFF;
        msgPattern.Data[3] = iso15765_destination_address & 0xFF;
        msgFlow.Data[0] = (iso15765_source_address >> 24) & 0xFF;
        msgFlow.Data[1] = (iso15765_source_address >> 16) & 0xFF;
        msgFlow.Data[2] = (iso15765_source_address >> 8) & 0xFF;
        msgFlow.Data[3] = (iso15765_source_address & 0xFF);

        if (j2534->PassThruStartMsgFilter(chanID, FLOW_CONTROL_FILTER, &msgMask, &msgPattern, &msgFlow, &msgId))
        {
            reportJ2534Error();
            return STATUS_ERROR;
        }
        qDebug() << "msgId" << msgId;
        /*
        msgPattern.Data[0] = (iso15765_source_address >> 24) & 0xFF;
        msgPattern.Data[1] = (iso15765_source_address >> 16) & 0xFF;
        msgPattern.Data[2] = (iso15765_source_address >> 8) & 0xFF;
        msgPattern.Data[3] = iso15765_source_address & 0xFF;
        msgFlow.Data[0] = (iso15765_destination_address >> 24) & 0xFF;
        msgFlow.Data[1] = (iso15765_destination_address >> 16) & 0xFF;
        msgFlow.Data[2] = (iso15765_destination_address >> 8) & 0xFF;
        msgFlow.Data[3] = (iso15765_destination_address & 0xFF);

        if (j2534->PassThruStartMsgFilter(chanID, FLOW_CONTROL_FILTER, &msgMask, &msgPattern, &msgFlow, &msgId))
        {
            reportJ2534Error();
            return STATUS_ERROR;
        }
        qDebug() << "msgId" << msgId;
*/
    }
    else
        return STATUS_ERROR;

    qDebug() << "Set CAN / ISO15765 filters OK";

    return STATUS_SUCCESS;
}

int SerialPortActionsDirect::set_j2534_iso9141()
{
    if (is_iso14230_connection)
    {
        protocol = ISO14230;
        flags = ISO9141_NO_CHECKSUM | CAN_ID_BOTH;
        //baudrate = 10400;
    }
    else
    {
        protocol = ISO9141;
        flags = ISO9141_NO_CHECKSUM;
    }

    qDebug() << "Protocol:" << protocol;
    //baudrate = 4800;

    if (J2534_is_denso_dsti)
    {
        switch (protocol)
        {
        case ISO9141:
            //DST-i does not work well with ISO9141
            //sometimes it reads by 1 byte
            //Denso DST-i specific protocol, in fact ISO9141
            protocol = DSTI_ISO9141;
            flags = ISO9141_NO_CHECKSUM;
            baudrate = 10400;
            break;
        case ISO14230:
            flags = ISO9141_K_LINE_ONLY;
            //baudrate = 10400;
            break;
        }
    }

    // use ISO9141_NO_CHECKSUM to disable checksumming on both tx and rx messages
    if (j2534->PassThruConnect(devID, protocol, flags, baudrate, &chanID))
    {
        reportJ2534Error();
        return STATUS_ERROR;
    }
    else
    {
        qDebug() << "Connected:" << devID << protocol << baudrate << chanID;
//qDebug() << "J2534 connected";
#if defined Q_OS_UNIX
        chanID = protocol;
#endif
        qDebug() << "Connected:" << devID << protocol << baudrate << chanID;
    }

    return STATUS_SUCCESS;
}

int SerialPortActionsDirect::set_j2534_iso9141_timings()
{
    if (J2534_is_denso_dsti)
    {
        SCONFIG_LIST scl;
        SCONFIG scp_dsti_ISO14230[] = {{LOOPBACK,0},{P1_MAX,0xa},{P3_MIN,0x14},{P4_MIN,0},{DATA_RATE,4800}};
        SCONFIG scp_dsti_DSTI_ISO9141[] = {{DATA_RATE, 4800},  {LOOPBACK, 0},  {P1_MIN, 0},
                                           {P1_MAX, 4},        {P2_MIN, 4},    {P2_MAX, 0x14},
                                           {P3_MIN, 0x14},     {P3_MAX, 10000},{P4_MIN, 0},
                                           {P4_MAX, 0x14}};

        clear_tx_buffer();
        clear_rx_buffer();

        switch (protocol)
        {
        case ISO14230:
            scl.ConfigPtr = scp_dsti_ISO14230;
            scl.NumOfParams = ARRAYSIZE(scp_dsti_ISO14230);
            break;
        case DSTI_ISO9141:
            scl.ConfigPtr = scp_dsti_DSTI_ISO9141;
            scl.NumOfParams = ARRAYSIZE(scp_dsti_DSTI_ISO9141);
            break;
        }

        if (j2534->PassThruIoctl(chanID, SET_CONFIG, &scl, NULL))
        {
            reportJ2534Error();
            return STATUS_ERROR;
        }

    }
    else
    {
        // Set timeouts etc.
        SCONFIG_LIST scl;
        SCONFIG scp[6] = {{LOOPBACK,0},{P1_MAX,0},{P3_MIN,0},{P4_MIN,0},{PARITY,0},{TINIL,0}};
        scl.NumOfParams = 6;
        scp[0].Value = 0;
        scp[1].Value = 1;
        scp[2].Value = 0;
        scp[3].Value = 0;
        scp[4].Value = NO_PARITY;
        scp[5].Value = 25;
        scl.ConfigPtr = scp;
        if (j2534->PassThruIoctl(chanID,SET_CONFIG,&scl,NULL))
        {
            reportJ2534Error();
            return STATUS_ERROR;
        }
        else
        {
            //qDebug() << "Set timings OK";
        }
    }

    return STATUS_SUCCESS;
}

int SerialPortActionsDirect::set_j2534_iso9141_filters()
{
    // now setup the filter(s)
    PASSTHRU_MSG rxmsg,txmsg;
    PASSTHRU_MSG msgMask,msgPattern;
    unsigned long msgId;
    unsigned long numRxMsg;

    // simply create a "pass all" filter so that we can see
    // everything unfiltered in the raw stream

    txmsg.ProtocolID = protocol;
    txmsg.RxStatus = 0;
    txmsg.TxFlags = 0;
    txmsg.Timestamp = 0;
    txmsg.DataSize = 4;
    txmsg.ExtraDataIndex = 0;
    msgMask = msgPattern = txmsg;
    memset(msgMask.Data, 0, txmsg.DataSize); // mask the first 4 byte to 0
    memset(msgPattern.Data, 0, txmsg.DataSize);// match it with 0 (i.e. pass everything)
    if (j2534->PassThruStartMsgFilter(chanID, PASS_FILTER, &msgMask, &msgPattern, NULL, &msgId))
    {
        reportJ2534Error();
        return STATUS_ERROR;
    }
    else
    {
        //qDebug() << "Set filters OK";
    }
    //j2534->PassThruSetProgrammingVoltage(devID, J1962_PIN_9, 5000);

    return STATUS_SUCCESS;
}

void SerialPortActionsDirect::reportJ2534Error()
{
    char err[512];
    j2534->PassThruGetLastError(err);
    //qDebug() << "J2534 error:" << err;
}

void SerialPortActionsDirect::handle_error(QSerialPort::SerialPortError error)
{
    if (error != QSerialPort::NoError)
        qDebug() << "Error:" << error;

    if (error == QSerialPort::NoError)
    {
    }
    else if (error == QSerialPort::DeviceNotFoundError)
    {
        reset_connection();
    }
    else if (error == QSerialPort::PermissionError)
    {
    }
    else if (error == QSerialPort::OpenError)
    {
        reset_connection();
    }
    else if (error == QSerialPort::NotOpenError)
    {
        reset_connection();
    }
/*
    else if (error == QSerialPort::ParityError)
    {
    }
    else if (error == QSerialPort::FramingError)
    {
    }
    else if (error == QSerialPort::BreakConditionError)
    {
    }
*/
    else if (error == QSerialPort::WriteError)
    {
        reset_connection();
    }
    else if (error == QSerialPort::ReadError)
    {
        reset_connection();
    }
    else if (error == QSerialPort::ResourceError)
    {
        reset_connection();
    }
    else if (error == QSerialPort::UnsupportedOperationError)
    {
    }
    else if (error == QSerialPort::TimeoutError)
    {
        reset_connection();
        //qDebug() << "Timeout error";
        /*
        if (serial->isOpen())
            serial->flush();
        else
            serial->close();
        */
    }
    else if (error == QSerialPort::UnknownError)
    {
        reset_connection();
    }
}

void SerialPortActionsDirect::accurate_delay(int timeout)
{
    double seconds = (double)timeout / 1000.0;
    auto spinStart = std::chrono::high_resolution_clock::now();
    while ((std::chrono::high_resolution_clock::now() - spinStart).count() / 1e9 < seconds);
}

void SerialPortActionsDirect::fast_delay(int timeout)
{
    QTime dieTime = QTime::currentTime().addMSecs(timeout);
    while (QTime::currentTime() < dieTime)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 1);
}

void SerialPortActionsDirect::delay(int timeout)
{
    QTime dieTime = QTime::currentTime().addMSecs(timeout);
    while (QTime::currentTime() < dieTime)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
}

QString SerialPortActionsDirect::parse_message_to_hex(QByteArray received)
{
    QByteArray msg;

    for (unsigned long i = 0; i < received.length(); i++)
    {
        msg.append(QString("%1 ").arg((uint8_t)received.at(i),2,16,QLatin1Char('0')).toUtf8());
    }

    return msg;
}

