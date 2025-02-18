// This is an open source non-commercial project. Dear PVS-Studio, please check it.

// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

#include "serial_port_actions_direct.h"

SerialPortActionsDirect::SerialPortActionsDirect(QObject *parent)
    : QObject(parent)
    , serial(new QSerialPort(this))
{
    j2534 = new J2534();
#if defined Q_OS_UNIX
    QObject::connect(j2534, &J2534::LOG_E, this, &SerialPortActionsDirect::LOG_E);
    QObject::connect(j2534, &J2534::LOG_W, this, &SerialPortActionsDirect::LOG_W);
    QObject::connect(j2534, &J2534::LOG_I, this, &SerialPortActionsDirect::LOG_I);
    QObject::connect(j2534, &J2534::LOG_D, this, &SerialPortActionsDirect::LOG_D);
#endif
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

    emit LOG_D("Changing baudrate, checking if port is open...", true, true);
    if (is_serial_port_open())
    {
        emit LOG_D("Port is open, checking adapter type...", true, true);
        if (!use_openport2_adapter)
        {
            emit LOG_D("Adapter type is generic OBD2...", true, true);

            if (serial->setBaudRate(serial_port_baudrate.toDouble()))
            {
                delay(50);
                emit LOG_D("Baudrate set to " + portSpeed + " OK", true, true);
                return STATUS_SUCCESS;
            }
            else
            {
                emit LOG_E("ERROR setting baudrate!", true, true);
                return STATUS_ERROR;
            }
        }
        else
        {
            emit LOG_D("Adapter type is J2534...", true, true);

            SCONFIG_LIST scl;
            SCONFIG scp[1] = {{DATA_RATE,0}};
            scl.NumOfParams = 1;
            scp[0].Value = baudrate;
            scl.ConfigPtr = scp;
            if (!j2534->PassThruIoctl(chanID,SET_CONFIG,&scl,NULL))
            {
                emit LOG_D("Baudrate set to " + portSpeed + " OK", true, true);
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
    QByteArray received;

    if (use_openport2_adapter)
    {
        unsigned long result;
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

        /* Set timeout to 350ms before init */
        accurate_delay(350);

        result = j2534->PassThruIoctl(chanID, FAST_INIT, &InputMsg, &OutputMsg);
        if (result)
        {
            reportJ2534Error();
            return STATUS_ERROR;
        }
        else
        {

        }
    }
    else
    {
        // Set timeout to 350ms before init
        accurate_delay(350);
        // Set break to set seril line low
        serial->setBreakEnabled(true);
        // Set timeout to 25ms to generate 25ms low pulse
        accurate_delay(23.7);
        // Unset break to set seril line high
        serial->setBreakEnabled(false);
        // Set timeout to 25ms to generate 25ms high pulse before init data is sent
        accurate_delay(23.8);
        // Send init data
        received = write_serial_data_echo_check(output);
        received = read_serial_data(1, 10);
        //emit LOG_D("Fast init response: " + parse_message_to_hex(received), true, true);
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
            emit LOG_D("RX BUFFER EMPTY", true, true);
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
            emit LOG_D("TX BUFFER EMPTY", true, true);
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
    accurate_delay(timeout);
    line_end_check_1_toggled(requestToSendDisabled);
    //delay(timeout);

    //read_serial_data(100, 50);

    return STATUS_SUCCESS;
}

int SerialPortActionsDirect::pulse_lec_2_line(int timeout)
{
    line_end_check_2_toggled(dataTerminalEnabled);
    accurate_delay(timeout);
    line_end_check_2_toggled(dataTerminalDisabled);
    //delay(timeout);

    //read_serial_data(100, 50);

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
            delay(1);
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
    if (state == dataTerminalEnabled)
    {
        if (use_openport2_adapter)
        {
            j2534->PassThruSetProgrammingVoltage(devID, J1962_PIN_9, 12000);
#if defined Q_OS_UNIX
            delay(1);
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
    //QString j2534DllName = "j2534.dll";

    serialPortAvailable = false;

    for (const QSerialPortInfo &serialPortInfo : serialPortsInfo){
        serial_ports.append(serialPortInfo.portName() + " - " + serialPortInfo.description());
        emit LOG_D("Serial port name: " + serialPortInfo.portName() + " " + serialPortInfo.description(), true, true);
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
    for (const QString &vendor : installed_drivers.keys())
    {
        driver_count++;
        j2534->disable();
        //close_j2534_serial_port();
        QString j2534DllName = installed_drivers[vendor];
        emit LOG_D("Testing for " + j2534DllName, true, true);
        j2534->setDllName(j2534DllName.toLocal8Bit().data());
        if (j2534->init())
        {
            emit LOG_D(j2534DllName + " init successfull", true, true);
            //0 means no error
            if (!j2534->PassThruOpen(NULL, &devID))
            {
                emit LOG_D("Successfully opened " + QString::number(devID) + " / " + vendor + " / " + j2534DllName, true, true);
                j2534_devices.append(vendor);
                j2534DeviceFound = true;
                j2534->PassThruClose(devID);
            }
            else
                emit LOG_E(QString::number(devID) + " / " + vendor + " device not connected", true, true);
        }
        else
            emit LOG_D(j2534DllName + " not found", true, true);
        if (j2534DeviceFound)
            break;
    }
    emit LOG_D("Tested installed drivers: " + QString::number(driver_count), true, true);

    return j2534_devices;
}

QMap<QString, QString> SerialPortActionsDirect::getAllJ2534DriversNames()
{
    QSettings registry("HKEY_LOCAL_MACHINE\\SOFTWARE\\PassThruSupport.04.04", REGISTRY_FORMAT);
    QMap<QString, QString> drivers_map;
    emit LOG_D("Found installed drivers: ", true, false);
    for (const QString &i : registry.childGroups())
    {
        QString vendor = i;
        //emit LOG_D("J2534 Drivers: " + vendor, true, true);
        vendor.replace("\\", "/");
        QString dllName = registry.value(i+"/FunctionLibrary").toString();
        drivers_map[vendor] = dllName;
        emit LOG_D(dllName + ", ", false, false);
    }
    emit LOG_D(" ", false, true);
    return drivers_map;
}
#endif

QString SerialPortActionsDirect::open_serial_port()
{
    emit LOG_D("Serial port = " + serial_port_list.join(", "), true, true);
    //QString serial_port_text = serial_port_list.at(1);
#if defined Q_OS_UNIX
    serial_port = serial_port_prefix_linux + serial_port_list.at(0);
    QString is_j2534 = serial_port.split(" - ").at(1);
    serial_port = serial_port.split(" - ").at(0);
#endif
#if defined(_WIN32) || defined(WIN32) || defined (_WIN64) || defined (WIN64)
    serial_port = serial_port_prefix_win + serial_port_list.at(0);
#endif
    emit LOG_D("Interface: " + serial_port, true, true);

#if defined Q_OS_UNIX
    if (!serial_port.isEmpty() && is_j2534 == "OpenPort 2.0")
#else
    if (!serial_port.isEmpty())
#endif
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
        emit LOG_D("Local DLL Name: " + localDllName, true, true);
        emit LOG_D("Installed DLL Name: " + installedDllName, true, true);

        QMap<QString, QString> user_j2534_drivers;

        emit LOG_D("Opening device: " + serial_port, true, true);
        QStringList j2534_driver;
        user_j2534_drivers[serial_port] = localDllName;
        j2534_driver = check_j2534_devices(user_j2534_drivers);
        user_j2534_drivers[serial_port] = installedDllName;
        if (j2534_driver.isEmpty())
            j2534_driver = check_j2534_devices(user_j2534_drivers);
        if (!j2534_driver.isEmpty())
            j2534->setDllName(j2534_driver.at(0).toLocal8Bit().data());
        else
            emit LOG_D("Initializing interface failed!", true, true);
#endif
        long result;

        emit LOG_D("Testing j2534 interface, please wait...", true, true);
        result = init_j2534_connection();

        if (result == STATUS_SUCCESS)
        {
            emit LOG_D("J2534: Interface opened succesfully!", true, true);
            use_openport2_adapter = true;
            J2534_init_ok = true;
            j2534->J2534_init_ok = true;
            openedSerialPort = serial_port;
        }
        else
        {
            emit LOG_D("J2534: Failed to open interface!", true, true);
            use_openport2_adapter = false;
            reset_connection();
        }
    }
    if (!use_openport2_adapter && openedSerialPort != serial_port)
    {
        emit LOG_D("Testing serial interface, please wait...", true, true);
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

                emit LOG_D("Serial port '" + serial_port + "' is open at baudrate " + serial_port_baudrate, true, true);
                return openedSerialPort;
            }
            else
            {
                emit LOG_E("Couldn't open serial port '" + serial_port + "'", true, true);
                return NULL;
            }

        }
        else{
            emit LOG_D("Serial port '" + serial_port + "' is already opened", true, true);
            return openedSerialPort;
        }
    }

    return openedSerialPort;
}

void SerialPortActionsDirect::reset_connection()
{
    close_j2534_serial_port();
    close_serial_port();
    delay(250);
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
    if (j2534->is_serial_port_open())
    {
        bool j2534_disconnect_ok = false;
        bool j2534_close_ok = false;
        for (int i = 0; i < 5; i++)
        {
            if (!j2534->PassThruDisconnect(chanID))
            {
                j2534_disconnect_ok = true;
                break;
            }
            delay(100);
        }
        if (!j2534_disconnect_ok)
            emit LOG_D("J2534 interface disconnect failed!", true, true);
        else
            emit LOG_D("J2534 interface disconnected succesfully!", true, true);
        for (int i = 0; i < 5; i++)
        {
            if (!j2534->PassThruClose(devID))
            {
                j2534_close_ok = true;
                break;
            }
            delay(100);
        }
        if (!j2534_close_ok)
            emit LOG_D("J2534 interface close failed!", true, true);
        else
            emit LOG_D("J2534 interface closed succesfully!", true, true);
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
    delay(100);
    j2534 = new J2534();
    j2534->setDllName(dllName);
#if defined Q_OS_UNIX
    QObject::connect(j2534, &J2534::LOG_E, this, &SerialPortActionsDirect::LOG_E);
    QObject::connect(j2534, &J2534::LOG_W, this, &SerialPortActionsDirect::LOG_W);
    QObject::connect(j2534, &J2534::LOG_I, this, &SerialPortActionsDirect::LOG_I);
    QObject::connect(j2534, &J2534::LOG_D, this, &SerialPortActionsDirect::LOG_D);
#endif
}

QByteArray SerialPortActionsDirect::read_serial_data(uint32_t datalen, uint16_t timeout)
{
    QByteArray received;
    QByteArray req_bytes;
    uint32_t msglen = 0;

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
            while (!serial->bytesAvailable() && QTime::currentTime() < dieTime)
            {
                QCoreApplication::processEvents(QEventLoop::AllEvents, 1);
            }
            if (serial->bytesAvailable())
            {
                while (received.length() < 4 && QTime::currentTime() < dieTime)
                {
                    if (serial->bytesAvailable())
                        received.append(serial->read(1));
                    QCoreApplication::processEvents(QEventLoop::AllEvents, 1);
                }
                if (received.length() < 4)
                    return received;

                if (is_iso14230_connection)
                {
                    qDebug() << "Read with ISO14230";

                    if (received.at(0) & 0x3f)
                    {
                        msglen = (received.at(0) & 0x3f); // Byte in index 3 is payload, no +1 for checksum
                    }
                    else
                        msglen = received.at(3) + 1; // +1 for checksum
                }
                else if (!is_iso14230_connection)
                {
                    if (received.startsWith("\xbe\xef"))
                        msglen = ((uint8_t)received.at(2) << 8) + (uint8_t)received.at(3) + 1; // +1 for checksum
                    if (received.startsWith("\x80\xf0"))
                        msglen = (uint8_t)received.at(3) + 1; // +1 for checksum
                }
                while ((uint32_t)req_bytes.length() < msglen && QTime::currentTime() < dieTime)
                {
                    if (serial->bytesAvailable())
                        req_bytes.append(serial->readAll());
                    QCoreApplication::processEvents(QEventLoop::AllEvents, 1);
                }
                if ((uint32_t)req_bytes.length() < msglen)
                    return received.append(req_bytes);
                //received.append(req_bytes);
            }
        }
        return received.append(req_bytes);
    }
    return received.append(req_bytes);
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
        //emit LOG_D("Data sent: " + parse_message_to_hex(output);

        return received;
    }

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

    return received;
}

QByteArray SerialPortActionsDirect::add_packet_header(QByteArray output)
{
    uint8_t chk_sum = 0;
    uint8_t msglength = output.length();

    //emit LOG_D("Adding iso14230 header to message";

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
    long txMsgLen;

    txMsgLen = output.length();
    if (txMsgLen > PASSTHRU_MSG_DATA_SIZE)
        txMsgLen = PASSTHRU_MSG_DATA_SIZE;

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
        //emit LOG_D("Data sent: " + parse_message_to_hex(output);

        output.remove(0, txMsgLen);
        txMsgLen = output.length();
        if (txMsgLen > PASSTHRU_MSG_DATA_SIZE)
            txMsgLen = PASSTHRU_MSG_DATA_SIZE;
    }

    return STATUS_SUCCESS;
}

int SerialPortActionsDirect::send_periodic_j2534_data(QByteArray output, int timeout)
{
    PASSTHRU_MSG txmsg;
    long txMsgLen;

    txMsgLen = output.length();
    if (txMsgLen > PASSTHRU_MSG_DATA_SIZE)
        txMsgLen = PASSTHRU_MSG_DATA_SIZE;

    while (txMsgLen > 0)
    {
        emit LOG_D("Start periodic messages with protocol: " + QString::number(protocol), true, true);
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
        j2534->PassThruStartPeriodicMsg(chanID, &txmsg, &msgID, timeout);
        output.remove(0, txMsgLen);
        txMsgLen = output.length();
        if (txMsgLen > PASSTHRU_MSG_DATA_SIZE)
            txMsgLen = PASSTHRU_MSG_DATA_SIZE;
    }

    delay(10);

    emit LOG_D("Start periodic message chanID: " + QString::number(chanID) + " and msgID: " + QString::number(chanID), true, true);

    return STATUS_SUCCESS;
}

int SerialPortActionsDirect::stop_periodic_j2534_data()
{
    emit LOG_D("Stop periodic message chanID: " + QString::number(chanID) + " and msgID: " + QString::number(chanID), true, true);
    j2534->PassThruStopPeriodicMsg(chanID, msgID);
    delay(10);
    //j2534->PassThruReadMsgs(chanID, &rxmsg, &numRxMsg, timeout);

    return STATUS_SUCCESS;
}

QByteArray SerialPortActionsDirect::read_j2534_data(unsigned long timeout)
{
    PASSTHRU_MSG rxmsg;
    unsigned long numRxMsg;
    QByteArray received;

    received.clear();

    rxmsg.DataSize = 0;
    numRxMsg = 1;
    // j2534->PassThruReadMsgs(chanID, &rxmsg, &numRxMsg, timeout);
    // 0 means no error, all other values mean error
    if(j2534->PassThruReadMsgs(chanID, &rxmsg, &numRxMsg, timeout))
        goto exit;
    //emit LOG_D(numRxMsg << "messages, rx status " + rxmsg.RxStatus;
    if (numRxMsg)
    {
        //emit LOG_D(numRxMsg << "messages, rx status " + rxmsg.RxStatus;
        dump_msg(&rxmsg);

        if (is_can_connection)
        {
            for (unsigned long i = 0; i < rxmsg.DataSize; i++)
                received.append((uint8_t)rxmsg.Data[i]);
        }
        else
        {
            //emit LOG_D("RX MSG status: " + rxmsg.RxStatus;
            if (rxmsg.RxStatus & TX_DONE){
                //emit LOG_D("TX_DONE_MSG, read actual message";
                rxmsg.DataSize = 0;
                rxmsg.Data[0] = 0x00;
                j2534->PassThruReadMsgs(chanID, &rxmsg, &numRxMsg, timeout);
                //emit LOG_D("New RX MSG status: " + rxmsg.RxStatus;
            }
            if (rxmsg.RxStatus & START_OF_MESSAGE){
                //emit LOG_D("START_OF_MESSAGE, read actual message";
                j2534->PassThruReadMsgs(chanID, &rxmsg, &numRxMsg, timeout);
            }
            if (rxmsg.RxStatus & RX_MSG_END_IND){
                //emit LOG_D("END_OF_MESSAGE " + rxmsg.Data;
            }
            for (unsigned long i = 0; i < rxmsg.DataSize; i++)
                received.append((uint8_t)rxmsg.Data[i]);
        }
    }
    //emit LOG_D("RECEIVED: " + parse_message_to_hex(received);
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
        //emit LOG_D("Set timings OK";
    }

    return STATUS_SUCCESS;
}

void SerialPortActionsDirect::dump_msg(PASSTHRU_MSG* msg)
{
    QByteArray datamsg;

    //emit LOG_D("Dump msg";
    if (msg->RxStatus & START_OF_MESSAGE)
        return; // skip

    datamsg.clear();
    for (unsigned int i = 0; i < msg->DataSize; i++)
    {
        datamsg.append(QString("%1 ").arg(msg->Data[i],2,16,QLatin1Char('0')).toUtf8());
    }
    //emit LOG_D("Timestamp: " + msg->Timestamp << "msg length: " + msg->DataSize << "msg: " + datamsg;
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
        emit LOG_E("INIT: Can't load J2534 DLL.", true, true);
        return STATUS_ERROR;
    }
    else
    {
        emit LOG_D("INIT: J2534 DLL loaded.", true, true);
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
        //emit LOG_D("INIT: J2534 opened, devID " + devID;
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
    emit LOG_D("J2534 API Version: " + QString(strApiVersion), true, true);
    emit LOG_D("J2534 DLL Version: " + QString(strDllVersion), true, true);
    emit LOG_D("Device Firmware Version: " + QString(strFirmwareVersion), true, true);
    emit LOG_D("Device Serial Number: " + parse_message_to_hex(strSerial), true, true);

    // Create J2534 to device connections
    if (is_iso15765_connection)
    {
        //set_j2534_can_filters();
        set_j2534_can();
        set_j2534_can_timings();
        set_j2534_can_filters();
        emit LOG_D("ISO15765 init ready", true, true);
    }
    else if (is_can_connection)
    {
        set_j2534_can();
        set_j2534_can_timings();
        set_j2534_can_filters();
        emit LOG_D("CAN init ready", true, true);
    }
    else
    {
        set_j2534_iso9141();
        set_j2534_iso9141_timings();
        set_j2534_iso9141_filters();
        emit LOG_D("K-Line init ready", true, true);
    }

    return STATUS_SUCCESS;
}

int SerialPortActionsDirect::set_j2534_can()
{
    if (is_can_connection)
    {
        emit LOG_D("Set CAN flags", true, true);
        protocol = CAN;
        if (is_29_bit_id)
            flags = CAN_29BIT_ID;
        else
            flags = 0;
    }
    else if (is_iso15765_connection)
    {
        emit LOG_D("Set iso15765 flags", true, true);
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
        //emit LOG_D("Connected: " + devID << protocol << baudrate << chanID;
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

int SerialPortActionsDirect::set_j2534_can_timings()
{
    // Set timeouts etc.
    emit LOG_D("Set CAN/iso15765 timings", true, true);
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
        //emit LOG_D("Set timings OK";
    }

    return STATUS_SUCCESS;
}

int SerialPortActionsDirect::set_j2534_can_filters()
{
    // now setup the filter(s)
    PASSTHRU_MSG txmsg;
    PASSTHRU_MSG msgMask, msgPattern, msgFlow;
    unsigned long msgId;

    j2534->PassThruIoctl(chanID, CLEAR_MSG_FILTERS, NULL, NULL);

    if (is_can_connection)
    {
        emit LOG_D("Set CAN filters", true, true);
        txmsg.ProtocolID = protocol;
        txmsg.RxStatus = 0;
        txmsg.TxFlags = ISO15765_FRAME_PAD;
        txmsg.Timestamp = 0;
        txmsg.DataSize = 4;
        txmsg.ExtraDataIndex = 0;

        msgMask = msgPattern = txmsg;
        memset(msgMask.Data, 0xFF, txmsg.DataSize);
        memset(msgPattern.Data, 0xFF, txmsg.DataSize);

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
    else if (is_iso15765_connection)
    {
        emit LOG_D("Set iso15765 filters", true, true);
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
        emit LOG_D("msgId " + QString::number(msgId), true, true);
    }
    else
        return STATUS_ERROR;

    if (is_can_connection)
        emit LOG_D("CAN filters OK", true, true);
    else if (is_iso15765_connection)
        emit LOG_D("ISO15765 filters OK", true, true);

    return STATUS_SUCCESS;
}

int SerialPortActionsDirect::set_j2534_iso9141()
{
    baudrate = serial_port_baudrate.toUInt();

    if (is_iso14230_connection)
    {
        protocol = ISO14230;
        flags = ISO9141_NO_CHECKSUM | CAN_ID_BOTH;
    }
    else
    {
        protocol = ISO9141;
        flags = ISO9141_NO_CHECKSUM;
    }

    emit LOG_D("Protocol: " + QString::number(protocol), true, true);

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
#if defined Q_OS_WIN32
        emit LOG_D("Connected: DevID " + QString::number(devID) + ", protocol " + QString::number(protocol) + ", baudrate " + QString::number(baudrate) + ", chanID " + QString::number(chanID), true, true);
#elif defined Q_OS_UNIX
        chanID = protocol;
        emit LOG_D("Connected: DevID " + QString::number(devID) + ", protocol " + QString::number(protocol) + ", baudrate " + QString::number(baudrate) + ", chanID " + QString::number(chanID), true, true);
#endif
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
        if (serial_port_parity == QSerialPort::OddParity)
            scp[4].Value = ODD_PARITY;
        else if (serial_port_parity == QSerialPort::EvenParity)
            scp[4].Value = EVEN_PARITY;
        scp[5].Value = 25;
        scl.ConfigPtr = scp;
        if (j2534->PassThruIoctl(chanID,SET_CONFIG,&scl,NULL))
        {
            reportJ2534Error();
            return STATUS_ERROR;
        }
        else
        {
            //emit LOG_D("Set timings OK";
        }
    }

    return STATUS_SUCCESS;
}

int SerialPortActionsDirect::set_j2534_iso9141_filters()
{
    // now setup the filter(s)
    PASSTHRU_MSG txmsg;
    PASSTHRU_MSG msgMask,msgPattern;
    unsigned long msgId;

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
        //emit LOG_D("Set filters OK";
    }
    //j2534->PassThruSetProgrammingVoltage(devID, J1962_PIN_9, 5000);

    return STATUS_SUCCESS;
}

void SerialPortActionsDirect::reportJ2534Error()
{
    char err[512];
    j2534->PassThruGetLastError(err);
    emit LOG_D("J2534 error: " + (QString)err, true, true);
}

void SerialPortActionsDirect::handle_error(QSerialPort::SerialPortError error)
{
    if (error != QSerialPort::NoError)
        emit LOG_D("Error: " + QString::number(error), true, true);

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
        //emit LOG_E("Timeout error";
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

void SerialPortActionsDirect::accurate_delay(double timeout)
{
    double seconds = timeout / 1000.0;
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
    double seconds = (double)timeout / 1000.0;
    auto spinStart = std::chrono::high_resolution_clock::now();
    while ((std::chrono::high_resolution_clock::now() - spinStart).count() / 1e9 < seconds)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 1);
    //QTime dieTime = QTime::currentTime().addMSecs(timeout);
    //while (QTime::currentTime() < dieTime)
    //    QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
}

QString SerialPortActionsDirect::parse_message_to_hex(QByteArray received)
{
    QByteArray msg;

    for (int i = 0; i < received.length(); i++)
    {
        msg.append(QString("%1 ").arg((uint8_t)received.at(i),2,16,QLatin1Char('0')).toUtf8());
    }

    return msg;
}

