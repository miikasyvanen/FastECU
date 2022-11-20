#include "serial_port_actions.h"

SerialPortActions::SerialPortActions()
{
    j2534 = new J2534();
}

SerialPortActions::~SerialPortActions()
{
    delete j2534;
}

bool SerialPortActions::is_serial_port_open()
{
    if (!serial->isOpen()){
        if (!J2534_init_ok)
            return false;
        else
            return j2534->is_serial_port_open();
    }

    return serial->isOpen();
}

int SerialPortActions::change_port_speed(QString portSpeed)
{
    serial_port_baudrate = portSpeed;
    baudrate = portSpeed.toUInt();

    if (is_serial_port_open())
    {
        if (!use_openport2_adapter)
        {
            //send_log_window_message("Change serial port '" + serialPort + "' speed to " + portSpeed + " baud", true, true);
            if (serial->setBaudRate(serial_port_baudrate.toDouble()))
            {
                delay(50);
                //qDebug() << "Baudrate set to" << serial_port_baudrate;
                return STATUS_SUCCESS;
            }
            else
            {
                qDebug() << "Set baudrate ERROR!";
                return STATUS_ERROR;
            }
        }
        else
        {
            //close_j2534_serial_port();

            SCONFIG_LIST scl;
            SCONFIG scp[1] = {{DATA_RATE,0}};
            scl.NumOfParams = 1;
            scp[0].Value = baudrate;
            scl.ConfigPtr = scp;
            if (j2534->PassThruIoctl(chanID,SET_CONFIG,&scl,NULL))
            {
                reportJ2534Error();
                return STATUS_ERROR;
            }
            else
            {
                //qDebug() << "Set baudrate to" << baudrate << "OK";
                return STATUS_SUCCESS;
            }
        }
    }

    return STATUS_ERROR;
}

int SerialPortActions::set_lec_lines(int lec1_state, int lec2_state)
{
    line_end_check_1_toggled(lec1_state);
    line_end_check_2_toggled(lec2_state);

    return STATUS_SUCCESS;
}

int SerialPortActions::pulse_lec_1_line(int timeout)
{
    line_end_check_1_toggled(requestToSendEnabled);
    delay(timeout);
    line_end_check_1_toggled(requestToSendDisabled);
    delay(timeout);

    read_serial_data(100, 50);

    return STATUS_SUCCESS;
}

int SerialPortActions::pulse_lec_2_line(int timeout)
{
    line_end_check_2_toggled(dataTerminalEnabled);
    delay(timeout);
    line_end_check_2_toggled(dataTerminalDisabled);
    delay(timeout);

    read_serial_data(100, 50);

    return STATUS_SUCCESS;
}

int SerialPortActions::line_end_check_1_toggled(int state)
{
    if (state == requestToSendEnabled)
    {
        if (use_openport2_adapter)
        {
            j2534->PassThruSetProgrammingVoltage(devID, J1962_PIN_11, 12000);
            #ifdef Q_OS_LINUX
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

int SerialPortActions::line_end_check_2_toggled(int state)
{
    QByteArray received;

    if (state == dataTerminalEnabled)
    {
        if (use_openport2_adapter)
        {
            j2534->PassThruSetProgrammingVoltage(devID, J1962_PIN_9, 5000);
            #ifdef Q_OS_LINUX
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
/*
QStringList SerialPortActions::check_win_serial_ports()
{
    QStringList list;

    return list;
}
*/
QStringList SerialPortActions::check_serial_ports()
{
    const auto serialPortsInfo = QSerialPortInfo::availablePorts();
    QStringList serial_ports;

    serialPortAvailable = false;

    //ui->serial_ports->clear();
    for (const QSerialPortInfo &serialPortInfo : serialPortsInfo){
        //ui->serial_ports->addItem(serialPortPrefix + serialPortInfo.portName());
        serial_ports.append(serialPortInfo.portName() + " - " + serialPortInfo.description());
        qDebug() << "Serial port name:" << serialPortInfo.portName() << serialPortInfo.description();
    }
    #if defined(_WIN32) || defined(WIN32) || defined (_WIN64) || defined (WIN64)
        if (j2534->init())
            if (!j2534->PassThruOpen(NULL, &devID))
                serial_ports.append("J2534 - OpenPort 2.0");
        j2534->PassThruClose(devID);
    #endif

    sort(serial_ports.begin(), serial_ports.end(), less<QString>());

    return serial_ports;
}

QString SerialPortActions::open_serial_port(QStringList serial_port_list)
{
    //qDebug() << "Serial port =" << serial_port_list;
    QString serial_port_text = serial_port_list.at(1);
    #ifdef Q_OS_LINUX
        serial_port = serial_port_prefix_linux + serial_port_list.at(0);
    #endif
    #if defined(_WIN32) || defined(WIN32) || defined (_WIN64) || defined (WIN64)
        serial_port = serial_port_prefix_win + serial_port_list.at(0);
    #endif

    if (serial_port_text == "OpenPort 2.0")
    {
        close_serial_port();
        use_openport2_adapter = true;
        if (!J2534_init_ok)
        {
            long result;

            result = init_j2534_connection();

            if (result == STATUS_SUCCESS)
            {
                J2534_init_ok = true;
                j2534->J2534_init_ok = true;
            }
            openedSerialPort = serial_port;
        }
        if (!j2534->is_serial_port_open())
        {
            close_j2534_serial_port();
        }
    }
    else if (J2534_init_ok)
    {
        use_openport2_adapter = false;
        close_j2534_serial_port();
    }
    if (!use_openport2_adapter && openedSerialPort != serial_port)
    {
        close_serial_port();

        #ifdef Q_OS_LINUX
            //serial_port = serial_port_prefix_linux + serial_port;
        #endif
        #if defined(_WIN32) || defined(WIN32) || defined (_WIN64) || defined (WIN64)
            //serial_port = serial_port_prefix_win + serial_port;
        #endif

        if (!(serial->isOpen() && serial->isWritable())){
            serial->setPortName(serial_port);
            serial->setBaudRate(serial_port_baudrate.toDouble());
            serial->setDataBits(QSerialPort::Data8);
            serial->setStopBits(QSerialPort::OneStop);
            serial->setParity(QSerialPort::NoParity);
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
                connect(serial, SIGNAL(error(QSerialPort::SerialPortError)), this, SLOT(handle_error(QSerialPort::SerialPortError)));

                //send_log_window_message("Serial port '" + serialPort + "' is open at baudrate " + serialPortBaudRate, true, true);
                qDebug() << "Serial port '" + serial_port + "' is open at baudrate " + serial_port_baudrate;
                return openedSerialPort;
            }
            else
            {
                //SendLogWindowMessage("Couldn't open serial port '" + serialPort + "'", true, true);
                qDebug() << "Couldn't open serial port '" + serial_port + "'";
                return NULL;
            }

        }
        else{
            //SendLogWindowMessage("Serial port '" + serialPort + "' is already opened", true, true);
            //qDebug() << "Serial port '" + serial_port + "' is already opened";
            return openedSerialPort;
        }
    }

    //if (J2534_init_ok)
    //    ssm_init();

    return openedSerialPort;
}

void SerialPortActions::reset_connection()
{
    close_serial_port();
    close_j2534_serial_port();
}

void SerialPortActions::close_serial_port()
{
    if (serial->isOpen())
    {
        serial->close();
        openedSerialPort.clear();
    }
}

void SerialPortActions::close_j2534_serial_port()
{
    //qDebug() << "J2534_init_ok:" << J2534_init_ok;
    if (j2534->is_serial_port_open())
    {
        j2534->PassThruDisconnect(chanID);
        j2534->PassThruClose(devID);
    }
    J2534_open_ok = false;
    J2534_get_version_ok = false;
    J2534_connect_ok = false;
    J2534_timing_ok = false;
    J2534_filters_ok = false;
    J2534_init_ok = false;
    j2534->J2534_init_ok = false;
    openedSerialPort.clear();
    //j2534->close();
    //J2534 *j2534;
    delete j2534;
    j2534 = new J2534;
}

QByteArray SerialPortActions::read_serial_data(uint32_t datalen, unsigned long timeout)
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

QByteArray SerialPortActions::write_serial_data(QByteArray output)
{
    QByteArray received;
    QByteArray msg;
    //uint8_t msgLen = 0;
    //uint8_t chk_sum = 0;

    if (is_serial_port_open())
    {
        if (serialport_protocol_14230)
        {
            output = write_serial_iso14230_data(output);
        }
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
        return received;
    }
    //send_log_window_message("Serial port not open", true, true);

    return received;
}

QByteArray SerialPortActions::write_serial_data_echo_check(QByteArray output)
{
    QByteArray received;
    QByteArray msg;

    if (is_serial_port_open())
    {
        if (serialport_protocol_14230)
        {
            output = write_serial_iso14230_data(output);
        }
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
        QTime dieTime = QTime::currentTime().addMSecs(200);
        while (received.length() < output.length() && (QTime::currentTime() < dieTime))
        {
            if (serial->bytesAvailable())
            {
                dieTime = QTime::currentTime().addMSecs(200);
                received.append(serial->read(1));
            }
            QCoreApplication::processEvents(QEventLoop::AllEvents, 1);
        }
        return received;
    }
    //send_log_window_message("Serial port not open", true, true);

    return received;
}

QByteArray SerialPortActions::write_serial_iso14230_data(QByteArray output)
{
    uint8_t chk_sum = 0;
    uint8_t msglength = output.length();

    //qDebug() << "Adding iso14230 header to message";

    output.insert(0, 0x80);
    output.insert(1, 0x10);
    output.insert(2, 0xFC);
    output.insert(3, msglength);

    for (int i = 0; i < output.length(); i++)
    {
        chk_sum = chk_sum + output.at(i);
    }
    output.append(chk_sum);

    return output;
}

int SerialPortActions::write_j2534_data(QByteArray output)
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
        txmsg.ProtocolID = protocol;
        txmsg.RxStatus = 0;
        txmsg.TxFlags = 0;
        if (is_can_connection)
            txmsg.TxFlags = CAN_29BIT_ID;
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

        numMsgs++;
        output.remove(0, txMsgLen);
        txMsgLen = output.length();
        if (txMsgLen > PASSTHRU_MSG_DATA_SIZE)
            txMsgLen -= txMsgLen - PASSTHRU_MSG_DATA_SIZE;
    }

    return STATUS_SUCCESS;
}

QByteArray SerialPortActions::read_j2534_data(unsigned long timeout)
{
    PASSTHRU_MSG rxmsg;
    unsigned long numRxMsg;
    unsigned int msgCnt = 0;
    unsigned int byteCnt = 0;
    time_t last_status_update = time(NULL);
    QByteArray received;

    rxmsg.DataSize = 0;
    numRxMsg = 1;
    j2534->PassThruReadMsgs(chanID, &rxmsg, &numRxMsg, timeout);
    if (numRxMsg)
    {
        //qDebug() << numRxMsg << "messages, rx status" << rxmsg.RxStatus;
        //dump_msg(&rxmsg);
        msgCnt++;
        byteCnt += rxmsg.DataSize;

        if (is_can_connection)
        {
            for (unsigned long i = 4; i < rxmsg.DataSize; i++)
                received.append((uint8_t)rxmsg.Data[i]);
        }
        else
        {
            if (rxmsg.RxStatus & START_OF_MESSAGE){
                j2534->PassThruReadMsgs(chanID, &rxmsg, &numRxMsg, timeout);
                //qDebug() << "START_OF_MESSAGE, read actual message";
            }
            for (unsigned long i = 0; i < rxmsg.DataSize; i++)
                received.append((uint8_t)rxmsg.Data[i]);
        }
    }
    return received;
}

void SerialPortActions::dump_msg(PASSTHRU_MSG* msg)
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
    qDebug() << "Timestamp:" << msg->Timestamp << "msg length:" << msg->DataSize << "msg:" << datamsg;
}

bool SerialPortActions::get_serial_num(char* serial)
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

    if (j2534->PassThruIoctl(devID,TX_IOCTL_APP_SERVICE,&inbuf,&outbuf))
    {
        serial[0] = 0;
        return false;
    }

    memcpy(serial,outbuf.data,outbuf.length);
    serial[outbuf.length] = 0;
    return true;
}

int SerialPortActions::init_j2534_connection()
{
    // If Linux, open serial port
    #ifdef Q_OS_LINUX
        if (j2534->open_serial_port(serial_port) != serial_port)
            return STATUS_ERROR;
    #endif

    // Init J2534 connection (in windows, load DLL etc.)
    if (!j2534->init())
    {
        qDebug() << "Can't connect to J2534 DLL.";
        return STATUS_ERROR;
    }
    else
    {
        qDebug() << "J2534 DLL connected.";
    }

    // Open J2534 connection
    if (j2534->PassThruOpen(NULL, &devID))
    {
        reportJ2534Error();
        return STATUS_ERROR;
    }
    else
    {
        qDebug() << "J2534 opened, devID" << devID;
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

    qDebug() << "J2534 API Version:" << strApiVersion;
    qDebug() << "J2534 DLL Version:" << strDllVersion;
    qDebug() << "Device Firmware Version:" << strFirmwareVersion;
    qDebug() << "Device Serial Number:" << strSerial;

    // Create J2534 to device connections
    if (is_iso15765_connection)
    {
        //set_j2534_can_filters();
        set_j2534_can_connection();
        set_j2534_can_timings();
        set_j2534_can_connection_filters();
    }
    else if (is_can_connection)
    {
        set_j2534_can_connection();
        set_j2534_can_timings();
        set_j2534_can_connection_filters();
    }
    else
    {
        set_j2534_iso9141_connection();
        set_j2534_iso9141_timings();
        set_j2534_iso9141_connection_filters();
    }

    return STATUS_SUCCESS;
}

int SerialPortActions::set_j2534_can_bl_connection()
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

int SerialPortActions::set_j2534_can_connection()
{
    if (is_can_connection)
    {
        protocol = CAN;
        flags = CAN_29BIT_ID;
    }
    else
    {
        protocol = ISO15765;
        flags = ISO15765_FRAME_PAD | CAN_ID_BOTH;
    }
    baudrate = 500000;

    // use ISO9141_NO_CHECKSUM to disable checksumming on both tx and rx messages
    if (j2534->PassThruConnect(devID, protocol, flags, baudrate, &chanID))
    {
        reportJ2534Error();
        return STATUS_ERROR;
    }
    else
    {
        #ifdef Q_OS_LINUX
            chanID = protocol;
        #endif
        qDebug() << "Connected:" << devID << protocol << baudrate << chanID;
    }

    return STATUS_SUCCESS;
}

int SerialPortActions::set_j2534_can_timings()
{
    // Set timeouts etc.
    SCONFIG_LIST scl;
    SCONFIG scp[1] = {{LOOPBACK,0}};
    scl.NumOfParams = 1;
    scp[0].Value = 0;
    scl.ConfigPtr = scp;
    if (j2534->PassThruIoctl(chanID,SET_CONFIG,&scl,NULL))
    {
        reportJ2534Error();
        return STATUS_ERROR;
    }
    else
    {
        qDebug() << "Set timings OK";
    }

    return STATUS_SUCCESS;
}

int SerialPortActions::set_j2534_can_connection_filters()
{
    // now setup the filter(s)
    PASSTHRU_MSG rxmsg, txmsg;
    PASSTHRU_MSG msgMask, msgPattern, msgFlow;
    unsigned long msgId;
    unsigned long numRxMsg;

    // simply create a "pass all" filter so that we can see
    // everything unfiltered in the raw stream

    if (protocol == CAN)
    {
        qDebug() << "Set CAN filters";
        txmsg.ProtocolID = protocol;
        txmsg.RxStatus = 0;
        txmsg.TxFlags = CAN_29BIT_ID;
        txmsg.Timestamp = 0;
        txmsg.DataSize = 4;
        txmsg.ExtraDataIndex = 0;

        msgMask = msgPattern = txmsg;
        memset(msgMask.Data, 0xFF, txmsg.DataSize); // mask the first 4 byte to 0
        memset(msgPattern.Data, 0x00, txmsg.DataSize);// match it with 0 (i.e. pass everything)

        if (j2534->PassThruStartMsgFilter(chanID, PASS_FILTER, &msgMask, &msgPattern, NULL, &msgId))
        {
            reportJ2534Error();
            return STATUS_ERROR;
        }

        //msgPattern.DataSize = 5;
        msgPattern.Data[3] = 0x21;
        //msgPattern.Data[4] = 0x00;

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
        txmsg.TxFlags = ISO15765_FRAME_PAD | CAN_ID_BOTH;
        txmsg.Timestamp = 0;
        txmsg.DataSize = 4;
        txmsg.ExtraDataIndex = 0;
        msgMask = msgPattern = msgFlow = txmsg;
        memset(msgMask.Data, 0x00, txmsg.DataSize); // mask the first 4 byte to 0
        memset(msgPattern.Data, 0x00, txmsg.DataSize); // mask the first 4 byte to 0
        memset(msgFlow.Data, 0x00, txmsg.DataSize); // mask the first 4 byte to 0
/*
        msgPattern.Data[0] = 0x00;
        msgPattern.Data[1] = 0x00;
        msgPattern.Data[2] = 0x07;
        msgPattern.Data[3] = 0xE0;
        msgFlow.Data[0] = 0x00;
        msgFlow.Data[1] = 0x00;
        msgFlow.Data[2] = 0x07;
        msgFlow.Data[3] = 0xE8;
*/
        if (j2534->PassThruStartMsgFilter(chanID, PASS_FILTER, &msgMask, &msgPattern, NULL, &msgId))
        //if (j2534->PassThruStartMsgFilter(chanID, FLOW_CONTROL_FILTER, &msgMask, &msgPattern, &msgFlow, &msgId))
        {
            reportJ2534Error();
            return STATUS_ERROR;
        }
/*
        msgPattern.Data[0] = 0x00;
        msgPattern.Data[1] = 0x00;
        msgPattern.Data[2] = 0x07;
        msgPattern.Data[3] = 0xE8;
        msgFlow.Data[0] = 0x00;
        msgFlow.Data[1] = 0x00;
        msgFlow.Data[2] = 0x07;
        msgFlow.Data[3] = 0xE0;

        if (j2534->PassThruStartMsgFilter(chanID, FLOW_CONTROL_FILTER, &msgMask, &msgPattern, &msgFlow, &msgId))
        {
            reportJ2534Error();
            return STATUS_ERROR;
        }
*/

    }
    else
        return STATUS_ERROR;

    qDebug() << "Set CAN filters OK";

    return STATUS_SUCCESS;
}

int SerialPortActions::set_j2534_can_filters()
{
    baudrate = 500000;

    if (!j2534->init()) {
        qDebug() << "failed to connect to J2534 DLL.";
        return STATUS_ERROR;
    }

    if (j2534->PassThruOpen(NULL, &devID)) {
        qDebug() << "failed to PassThruOpen()";
        return STATUS_ERROR;
    }

    if (j2534->PassThruConnect(devID, ISO15765, CAN_ID_BOTH, baudrate, &chanID)) {
        reportJ2534Error();
        return STATUS_ERROR;
    }
    j2534->PassThruIoctl(chanID, CLEAR_MSG_FILTERS, NULL, NULL);

    unsigned long filterID = 0;

    PASSTHRU_MSG maskMSG;// = {0};
    PASSTHRU_MSG maskPattern;// = {0};
    PASSTHRU_MSG flowControlMsg;// = {0};
    for (uint8_t i = 0; i < 7; i++) {
        maskMSG.ProtocolID = ISO15765;
        maskMSG.TxFlags = ISO15765_FRAME_PAD;
        maskMSG.RxStatus = 0;
        maskMSG.Timestamp = 0;
        maskMSG.ExtraDataIndex = 0;
        maskMSG.Data[0] = 0x00;
        maskMSG.Data[1] = 0x00;
        maskMSG.Data[2] = 0x07;
        maskMSG.Data[3] = 0xff;
        maskMSG.DataSize = 4;

        maskPattern.ProtocolID = ISO15765;
        maskPattern.TxFlags = ISO15765_FRAME_PAD;
        maskPattern.RxStatus = 0;
        maskPattern.Timestamp = 0;
        maskPattern.ExtraDataIndex = 0;
        maskPattern.Data[0] = 0x00;
        maskPattern.Data[1] = 0x00;
        maskPattern.Data[2] = 0x07;
        maskPattern.Data[3] = (0xE8 + i);
        maskPattern.DataSize = 4;

        flowControlMsg.ProtocolID = ISO15765;
        flowControlMsg.TxFlags = ISO15765_FRAME_PAD;
        flowControlMsg.RxStatus = 0;
        flowControlMsg.Timestamp = 0;
        flowControlMsg.ExtraDataIndex = 0;
        flowControlMsg.Data[0] = 0x00;
        flowControlMsg.Data[1] = 0x00;
        flowControlMsg.Data[2] = 0x07;
        flowControlMsg.Data[3] = (0xE0 + i);
        flowControlMsg.DataSize = 4;

        if (j2534->PassThruStartMsgFilter(chanID, FLOW_CONTROL_FILTER, &maskMSG, &maskPattern, &flowControlMsg, &filterID))
        {
            reportJ2534Error();
            return STATUS_ERROR;

        }
    }
    maskMSG.ProtocolID = ISO15765;
    maskMSG.TxFlags = ISO15765_FRAME_PAD;
    maskMSG.Data[0] = 0x00;
    maskMSG.Data[1] = 0x00;
    maskMSG.Data[2] = 0x07;
    maskMSG.Data[3] = 0xf8;
    maskMSG.DataSize = 4;

    maskPattern.ProtocolID = ISO15765;
    maskPattern.TxFlags = ISO15765_FRAME_PAD;
    maskPattern.Data[0] = 0x00;
    maskPattern.Data[1] = 0x00;
    maskPattern.Data[2] = 0x07;
    maskPattern.Data[3] = 0xE8;
    maskPattern.DataSize = 4;

    if (j2534->PassThruStartMsgFilter(chanID, PASS_FILTER, &maskMSG, &maskPattern, NULL, &filterID)) {
        qDebug() << "Failed to set message filter";
        reportJ2534Error();
        return STATUS_ERROR;
    }

    if (j2534->PassThruIoctl(chanID, CLEAR_TX_BUFFER, NULL, NULL)) {
        qDebug() << "Failed to clear j2534 TX buffer";
        reportJ2534Error();
        return STATUS_ERROR;
    }

    if (j2534->PassThruIoctl(chanID, CLEAR_RX_BUFFER, NULL, NULL)) {
        qDebug() << "Failed to clear j2534 RX buffer";
        reportJ2534Error();
        return STATUS_ERROR;
    }
    return STATUS_SUCCESS;
}

int SerialPortActions::set_j2534_iso9141_connection()
{
    protocol = ISO9141;
    flags = ISO9141_NO_CHECKSUM;
    baudrate = 4800;

    // use ISO9141_NO_CHECKSUM to disable checksumming on both tx and rx messages
    if (j2534->PassThruConnect(devID, protocol, flags, baudrate, &chanID))
    {
        reportJ2534Error();
        return STATUS_ERROR;
    }
    else
    {
        chanID = protocol;
        qDebug() << "Connected:" << devID << protocol << baudrate << chanID;
        //qDebug() << "J2534 connected";
    }

    return STATUS_SUCCESS;
}

int SerialPortActions::set_j2534_iso9141_timings()
{
    // Set timeouts etc.
    SCONFIG_LIST scl;
    SCONFIG scp[5] = {{LOOPBACK,0},{P1_MAX,0},{P3_MIN,0},{P4_MIN,0},{PARITY,0}};
    scl.NumOfParams = 5;
    scp[0].Value = 0;
    scp[1].Value = 1;
    scp[2].Value = 0;
    scp[3].Value = 0;
    scp[4].Value = parity;
    scl.ConfigPtr = scp;
    if (j2534->PassThruIoctl(chanID,SET_CONFIG,&scl,NULL))
    {
        reportJ2534Error();
        return STATUS_ERROR;
    }
    else
    {
        qDebug() << "Set timings OK";
    }

    return STATUS_SUCCESS;
}

int SerialPortActions::set_j2534_iso9141_connection_filters()
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
    memset(msgMask.Data, 0, 4); // mask the first 4 byte to 0
    memset(msgPattern.Data, 0, 4);// match it with 0 (i.e. pass everything)
    if (j2534->PassThruStartMsgFilter(chanID, PASS_FILTER, &msgMask, &msgPattern, NULL, &msgId))
    {
        reportJ2534Error();
        return STATUS_ERROR;
    }
    else
    {
        qDebug() << "Set filters OK";
    }
    //j2534->PassThruSetProgrammingVoltage(devID, J1962_PIN_9, 5000);

    return STATUS_SUCCESS;
}

int SerialPortActions::clear_rx_buffer()
{
    if (j2534->PassThruIoctl(chanID,CLEAR_RX_BUFFER,NULL,NULL))
    {
        reportJ2534Error();
        return STATUS_ERROR;
    }
    else
    {
        qDebug() << "RX flush OK";
    }
    if (j2534->PassThruIoctl(chanID,CLEAR_TX_BUFFER,NULL,NULL))
    {
        reportJ2534Error();
        return STATUS_ERROR;
    }
    else
    {
        qDebug() << "TX flush OK";
    }

    return STATUS_SUCCESS;
}

void SerialPortActions::reportJ2534Error()
{
    char err[512];
    j2534->PassThruGetLastError(err);
    qDebug() << "J2534 error:" << err;
}

void SerialPortActions::handle_error(QSerialPort::SerialPortError error)
{
    //qDebug() << "Error:" << error;

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
    else if (error == QSerialPort::ParityError)
    {
    }
    else if (error == QSerialPort::FramingError)
    {
    }
    else if (error == QSerialPort::BreakConditionError)
    {
    }
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

void SerialPortActions::delay(int n)
{
    QTime dieTime = QTime::currentTime().addMSecs(n);
    while (QTime::currentTime() < dieTime)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
}

