#include "J2534_linux.h"

J2534::J2534()
{

}

J2534::~J2534()
{

}

QString J2534::open_serial_port(QString serial_port)
{
    if (opened_serial_port != serial_port){
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
                opened_serial_port = serial_port;
                //connect(serial, SIGNAL(readyRead()), this, SLOT(ReadSerialDataSlot()), Qt::DirectConnection);
                qRegisterMetaType<QSerialPort::SerialPortError>();
                connect(serial, SIGNAL(errorOccurred(QSerialPort::SerialPortError)), this, SLOT(handle_error(QSerialPort::SerialPortError)));

                emit LOG_D("Linux j2534 serial port '" + serial_port + "' is open at baudrate " + serial_port_baudrate, true, true);
                return opened_serial_port;
            }
            else
            {
                emit LOG_D("Couldn't open Linux j2534 serial port '" + serial_port + "'", true, true);
                return NULL;
            }

        }
        else{
            emit LOG_D("Linux j2534 serial port '" + serial_port + "' is already opened", true, true);
            return opened_serial_port;
        }
    }

    return opened_serial_port;
}

void J2534::close_serial_port()
{
    if (serial->isOpen())
    {
        serial->close();
        delay(500);
    }
    opened_serial_port = "";
}

bool J2534::is_serial_port_open()
{
    return serial->isOpen();
}

QByteArray J2534::read_serial_data(uint32_t datalen, uint16_t timeout)
{
    QByteArray ReceivedData;
    QByteArray PayloadData;

    ReceivedData.clear();

    if (serial->isOpen())
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
        //emit LOG_D("Read J2534 msg:" << parseMessageToHex(ReceivedData);
    }

    return ReceivedData;
}

int J2534::write_serial_data(QByteArray output)
{
    QByteArray received;
    QByteArray msg;
    long result = STATUS_NOERROR;

    if (serial->isOpen())
    {
        //emit LOG_D("Send J2534 msg:" << parseMessageToHex(output);
        for (int i = 0; i < output.length(); i++)
        {
            msg.clear();
            msg.append(output.at(i));
            serial->write(msg, 1);
        }

        return result;
    }

    return 1;
}

QByteArray J2534::write_serial_iso14230_data(QByteArray output)
{
    uint8_t chk_sum = 0;
    uint8_t msglength = output.length();

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

QString J2534::parseMessageToHex(QByteArray received)
{
    QByteArray msg;

    for (unsigned long i = 0; i < received.length(); i++)
    {
        msg.append(QString("%1 ").arg((uint8_t)received.at(i),2,16,QLatin1Char('0')).toUtf8());
    }

    return msg;
}

uint32_t J2534::parse_ts(const char *data)
{
    uint32_t timestamp = 0;
    memcpy(&timestamp, data, 4);
    //if (littleEndian)
    //    timestamp = bswap(timestamp);
    return timestamp;
}

long J2534::PassThruOpen(const void *pName, unsigned long *pDeviceID)
{
    QByteArray output;
    QByteArray received;
    QByteArray check_result = "ar";
    QString name = (char*)pName;
    unsigned long devID = (unsigned long)pDeviceID;
    long result = ERR_NOT_SUPPORTED;

    pDeviceID = 0;
    emit LOG_D("Open J2534 device " + name + " with ID: " + QString::number(devID), true, true);

    output = "ata\r\n";
    emit LOG_D("Send data: " + parseMessageToHex(output), true, true);
    write_serial_data(output);
    received = read_serial_data(7, 50);
    emit LOG_D("Result check against " + check_result + ": " + parseMessageToHex(received), true, true);
    if (received.startsWith(check_result))
    {
        emit LOG_D("Result check OK", true, true);
        result = STATUS_NOERROR;
    }
    else
        emit LOG_D("Result check failed, not maybe an j2534 interface!", true, true);

    return result;
}

long J2534::PassThruClose(unsigned long DeviceID)
{
    QByteArray output;
    QByteArray received;
    long result = STATUS_NOERROR;

    //emit LOG_D("Close J2534 device ID:" << DeviceID;

    output = "atz\r\n";
    //emit LOG_D("Send data:" << output;
    write_serial_data(output);
    received = read_serial_data(8, 50);
    //emit LOG_D("Received:" << received;
    close_serial_port();

    return result;
}

long J2534::PassThruConnect(unsigned long DeviceID, unsigned long ProtocolID, unsigned long Flags, unsigned long Baudrate, unsigned long *pChannelID)
{
    QByteArray output;
    QByteArray received;
    unsigned long chanID = (unsigned long)pChannelID;
    long result = STATUS_NOERROR;

    switch ((int)ProtocolID) {
    case ISO9141:
        pChannelID = (unsigned long*)ISO9141;
        break;
    case ISO14230:
        pChannelID = (unsigned long*)ISO14230;
        break;
    case CAN:
        pChannelID = (unsigned long*)CAN;
        break;
    case ISO15765:
        pChannelID = (unsigned long*)ISO15765;
        break;
    case CAN_CH1:
        pChannelID = (unsigned long*)CAN_CH1;
        break;
    default:
        return 0;//J2534_ERR_INVALID_PROTOCOL_ID;
    }

    output.clear();
    QString str = "ato" + QString::number(ProtocolID) + " " + QString::number(Flags) + " " + QString::number(Baudrate) + " " + QString::number(ProtocolID) + "\r\n";
    output.append(str.toUtf8());
    //emit LOG_D("Send data:" << output;
    write_serial_data(output);
    received = read_serial_data(100, 50);
    emit LOG_D("Connect received: " + parseMessageToHex(received) + " " + received + " " + QString::number(chanID), true, true);

    return result;
}

long J2534::PassThruDisconnect(unsigned long ChannelID)
{
    QByteArray output;
    QByteArray received;
    long result = STATUS_NOERROR;

    //emit LOG_D("Disconnect J2534 device in channel:" << ChannelID;

    output.clear();
    QString str = "atc" + QString::number(ChannelID) + "\r\n";
    output.append(str.toUtf8());
    //emit LOG_D("Send data:" << output;
    write_serial_data(output);
    received = read_serial_data(100, 50);
    //emit LOG_D("Received:" << received;

    return result;
}

long J2534::PassThruReadMsgs(unsigned long ChannelID, PASSTHRU_MSG *pMsg, unsigned long *pNumMsgs, unsigned long Timeout)
{
    QByteArray received;
    QByteArray msg;
    long result = STATUS_NOERROR;
    unsigned long msg_cnt = 0;
    unsigned long chunk_cnt = 0;
    uint8_t msg_type = 0;
    uint32_t msg_index = 0;
    QString msg_type_string = 0;
    unsigned long msg_byte_cnt = 0;
    bool stop_reading = false;

    //emit LOG_D("Read received message from J2534 device in channel:" << ChannelID;

    received = read_serial_data(3, Timeout);
    //emit LOG_D("Message block length:" << received.length() << "data:" << parseMessageToHex(received);
    while (received.length() > 0 && is_serial_port_open())
    {
        //emit LOG_D("RECEIVED:" << received << parseMessageToHex(received);

        //emit LOG_D("Message header" << received.at(0) << received.at(1) << received.at(2);
        if (received.at(0) == 0x61 && received.at(1) == 0x72)
        {
            if (received.at(2) == 0x6f)
            {
                read_serial_data(2, Timeout);
                //msg_type_string = "ACK";
                received.clear();
            }
            else if (received.at(2) == 'e')
            {
                //received.append(read_serial_data(4, Timeout));
                while ((uint8_t)received.at(received.length() - 1) == 0x0d)
                    received.append(read_serial_data(1, Timeout));
                //emit LOG_D("Error sending message: " + received + " | " + parseMessageToHex(received);
                received.clear();
            }
            else if (received.at(2) == 'm')
            {
                received.append(read_serial_data(2, Timeout));
                msg.clear();
                while ((uint8_t)msg[msg.length()-1] != 0x20)
                    msg.append(read_serial_data(1, Timeout));
                received.append(msg);
                periodic_msg_id = msg.remove(msg.length()-1, 1).toULong();
                //emit LOG_D("Msg ID:" << periodic_msg_id;

                while ((uint8_t)msg[msg.length()-1] != 0x0a)
                    msg.append(read_serial_data(1, Timeout));
                received.append(msg);
                msg = read_serial_data(msg_byte_cnt, Timeout);
                received.append(msg);

                msg_index = 0;
            }
            else if (received.at(2) == 'y')
            {
                received.append(read_serial_data(2, Timeout));
                msg.clear();
                while ((uint8_t)msg[msg.length()-1] != 0x20)
                    msg.append(read_serial_data(1, Timeout));
                received.append(msg);
                msg_byte_cnt = msg.remove(msg.length()-1, 1).toInt();
                //emit LOG_D(msg_byte_cnt << "bytes long payload";
                while ((uint8_t)msg[msg.length()-1] != 0x0a)
                    msg.append(read_serial_data(1, Timeout));
                received.append(msg);
                msg = read_serial_data(msg_byte_cnt, Timeout);
                received.append(msg);

                msg_index = 0;
                for (unsigned long i = 0; i < msg_byte_cnt; i++)
                    pMsg->Data[msg_index++] = (uint8_t)msg.at(i);

                //emit LOG_D("Response:" << parseMessageToHex(received);
                //emit LOG_D("Msg:" << parseMessageToHex(msg);

                pMsg->RxStatus = NORM_MSG;
                pMsg->DataSize = msg_index;
                msg_cnt++;
            }
            else if (received.at(2) == '3' || received.at(2) == '4' || received.at(2) == '5' || received.at(2) == '6')
            {
                received.append(read_serial_data(2, Timeout));
                //emit LOG_D("Read message";
                msg_byte_cnt = received.at(3) - 1;
                msg_type = received.at(4);
                switch (msg_type) {
                    case NORM_MSG:
                        msg_type_string = "NORM_MSG";
                        break;
                    case START_OF_MESSAGE:
                        msg_type_string = "START_OF_MESSAGE";
                        break;
                    case TX_DONE_MSG:
                        msg_type_string = "TX_DONE_MSG";
                        break;
                    case TX_LB_MSG:
                        msg_type_string = "TX_LB_MSG";
                        break;
                    case RX_MSG_END_IND:
                        msg_type_string = "RX_MSG_END_IND";
                        break;
                    case EXT_ADDR_MSG_END_IND:
                        msg_type_string = "EXT_ADDR_MSG_END_IND";
                        break;
                    case LB_MSG_END_IND:
                        msg_type_string = "LB_MSG_END_IND";
                        break;
                    case NORM_MSG_START_IND:
                        msg_type_string = "NORM_MSG_START_IND";
                        break;
                    case TX_LB_START_IND:
                        msg_type_string = "TX_LB_START_IND";
                        break;
                    default:
                        //emit LOG_D("HEADER" << parseMessageToHex(received);
                        break;
                }
                //emit LOG_D("Message received at channel" << ChannelID << ", size is" << msg_byte_cnt << "and message type is" << msg_type_string << "(" << msg_type << ")";

                if (msg_type == START_OF_MESSAGE)
                {
                    //pMsg->RxStatus = START_OF_MESSAGE;
                    //received.append(read_serial_data(msg_byte_cnt, Timeout));
                    msg_index = 0;
                    msg_cnt++;
                    //emit LOG_D("START_OF_MESSAGE" << parseMessageToHex(received);
                    //stop_reading = true;
                    //received.clear();
                }

                if (msg_type == TX_DONE_MSG)
                {
                    pMsg->RxStatus = TX_DONE_MSG;
                    received.append(read_serial_data(msg_byte_cnt, Timeout));
                    msg_index = 0;
                    msg_cnt = 0;
                    //emit LOG_D("TX_DONE_MSG" << parseMessageToHex(received);
                    received.clear();
                }
                if (msg_type == TX_LB_START_IND)
                {
                    pMsg->RxStatus = TX_LB_START_IND;
                    received.append(read_serial_data(msg_byte_cnt, Timeout));
                    msg_index = 0;
                    msg_cnt = 0;
                    //emit LOG_D("TX_LB_START_IND" << parseMessageToHex(received);
                    received.clear();
                }
                if (msg_type == TX_LB_MSG)
                {
                    pMsg->RxStatus = TX_LB_MSG;
                    received.append(read_serial_data(msg_byte_cnt, Timeout));
                    msg_index = 0;
                    msg_cnt = 0;
                    //emit LOG_D("TX_LB_MSG" << parseMessageToHex(received);
                    received.clear();
                }
                if (msg_type == LB_MSG_END_IND)
                {
                    pMsg->RxStatus = LB_MSG_END_IND;
                    received.append(read_serial_data(msg_byte_cnt, Timeout));
                    msg_index = 0;
                    msg_cnt = 0;
                    //emit LOG_D("LB_MSG_END_IND" << parseMessageToHex(received);
                    received.clear();
                }
                if (msg_type == NORM_MSG_START_IND)
                {
                    pMsg->RxStatus = NORM_MSG_START_IND;
                    received.append(read_serial_data(msg_byte_cnt, Timeout));

                    msg_index = 0;
                    msg_cnt++;
                    chunk_cnt = 0;

                    //emit LOG_D("NORM_MSG_START_IND:" << parseMessageToHex(received);
                    received.clear();
                }
                if (msg_type == NORM_MSG || msg_type == START_OF_MESSAGE)
                {
                    pMsg->RxStatus = NORM_MSG;

                    received.append(read_serial_data(msg_byte_cnt, Timeout));
                    //emit LOG_D("NORM_MSG:" << parseMessageToHex(received);

                    //emit LOG_D("msg_byte_cnt:" << msg_byte_cnt;
                    if (received.at(2) == '5' || received.at(2) == '6')
                    {
                        msg_byte_cnt -= 4;
                        if (chunk_cnt)
                            msg_byte_cnt -= 4;
                    }
                    //emit LOG_D("msg_byte_cnt:" << msg_byte_cnt;
                    for (unsigned long i = 0; i < msg_byte_cnt; i++)
                    {
                        if (received.at(2) == '3' || received.at(2) == '4')
                            pMsg->Data[msg_index++] = (uint8_t)received.at(i + 5);
                        if (received.at(2) == '5' || received.at(2) == '6')
                        {
                            if (chunk_cnt)
                                pMsg->Data[msg_index++] = (uint8_t)received.at(i + 13);
                            else
                                pMsg->Data[msg_index++] = (uint8_t)received.at(i + 9);
                        }
                    }
                    chunk_cnt++;

                    if (received.at(2) == '5')
                    {
                        char data[4];
                        data[0] = (uint8_t)received.at(8);
                        data[1] = (uint8_t)received.at(7);
                        data[2] = (uint8_t)received.at(6);
                        data[3] = (uint8_t)received.at(5);
                        pMsg->Timestamp = parse_ts(data);
                        pMsg->DataSize = msg_index;
                        msg_cnt++;
                        stop_reading = true;
                    }
                    received.clear();
                }
                if (msg_type == RX_MSG_END_IND)
                {
                    pMsg->RxStatus = RX_MSG_END_IND;

                    received.append(read_serial_data(msg_byte_cnt, Timeout));
                    //emit LOG_D("RX_MSG_END_IND:" << parseMessageToHex(received);

                    if (received.at(2) == '6')
                    {
                        msg_byte_cnt -= 4;
                        if (chunk_cnt)
                            msg_byte_cnt -= 4;
                        for (unsigned long i = 0; i < msg_byte_cnt; i++)
                        {
                            if (chunk_cnt)
                                pMsg->Data[msg_index++] = (uint8_t)received.at(i + 13);
                            else
                                pMsg->Data[msg_index++] = (uint8_t)received.at(i + 9);
                        }
                    }
                    char data[4];
                    data[0] = (uint8_t)received.at(8);
                    data[1] = (uint8_t)received.at(7);
                    data[2] = (uint8_t)received.at(6);
                    data[3] = (uint8_t)received.at(5);
                    pMsg->Timestamp = parse_ts(data);
                    pMsg->DataSize = msg_index;
                    msg_cnt++;

                    received.clear();
                    stop_reading = true;
                }
            }
        }
        if (!stop_reading)
        {
            QByteArray response = read_serial_data(3, Timeout);

            if (response.length() > 0)
                received.append(response);
            else
                received.clear();
        }
        //emit LOG_D("Parsing read messages:" << received.length() << received << parseMessageToHex(received);
    }

    *pNumMsgs = msg_cnt;
    received.clear();
    return result;
}

long J2534::PassThruWriteMsgs(unsigned long ChannelID, const PASSTHRU_MSG *pMsg, unsigned long *pNumMsgs, unsigned long Timeout)
{
    QByteArray output;
    QByteArray received;
    long result = STATUS_NOERROR;

    //emit LOG_D("Send" << *pNumMsgs << "messages";
    for (unsigned long msg_index = 0; msg_index < *pNumMsgs; msg_index++)
    {
        output.clear();
        QString str = "att" + QString::number(ChannelID) + " " + QString::number(pMsg->DataSize) + " " + QString::number(pMsg->TxFlags) + "\r\n";
        output.append(str.toUtf8());
        for (unsigned long i = 0; i < pMsg->DataSize; i++)
        {
            output.append(pMsg->Data[i]);
        }
        //output.append("\r\n");
        //emit LOG_D("TX:" << parseMessageToHex(output);
        write_serial_data(output);
        //PassThruReadMsgs(ChannelID, &rxmsg, &numRxMsg, Timeout);
        pMsg++;
    }

    return result;
}

long J2534::PassThruStartPeriodicMsg(unsigned long ChannelID, const PASSTHRU_MSG *pMsg, unsigned long *pMsgID, unsigned long TimeInterval)
{
    PASSTHRU_MSG rxmsg;
    unsigned long numRxMsg;
    unsigned long timeout = 10;

    QByteArray output;
    long result = STATUS_NOERROR;

    output.clear();
    QString str = "atm" + QString::number(ChannelID) + " " + QString::number(TimeInterval * 1000) + " 0 " + QString::number(pMsg->TxFlags) + " " + QString::number(pMsg->DataSize) + "\r\n";
    output.append(str.toUtf8());
    for (unsigned long i = 0; i < pMsg->DataSize; i++)
    {
        output.append(pMsg->Data[i]);
    }

    write_serial_data(output);
    PassThruReadMsgs(ChannelID, &rxmsg, &numRxMsg, timeout);

    *pMsgID = periodic_msg_id;

    return result;
}

long J2534::PassThruStopPeriodicMsg(unsigned long ChannelID, unsigned long MsgID)
{
    QByteArray output;
    long result = STATUS_NOERROR;

    QString str = "atn" + QString::number(ChannelID) + " " + QString::number(MsgID) + "\r\n";
    output.append(str.toUtf8());

    write_serial_data(output);

    return result;
}

long J2534::PassThruStartMsgFilter(unsigned long ChannelID, unsigned long FilterType, const PASSTHRU_MSG *pMaskMsg, const PASSTHRU_MSG *pPatternMsg, const PASSTHRU_MSG *pFlowControlMsg, unsigned long *pMsgID)
{
    QByteArray output;
    QByteArray received;
    long result = STATUS_NOERROR;

    output.clear();
    QString str = "atf" + QString::number(ChannelID) + " " + QString::number(FilterType) + " " + QString::number(pMaskMsg->TxFlags) + " " + QString::number(pMaskMsg->DataSize);
    output.append(str.toUtf8());
    //if (pPatternMsg->DataSize > 0)
        //output.append(" " + QString::number(pPatternMsg->DataSize));
    //if (pFlowControlMsg != NULL)
        //if (pFlowControlMsg->DataSize > 0)
            //output.append(" " + QString::number(pFlowControlMsg->DataSize));
    output.append("\r\n");

    for (unsigned long  i = 0; i < pMaskMsg->DataSize; i++)
    {
        output.append(pMaskMsg->Data[i]);
    }
    for (unsigned long  i = 0; i < pPatternMsg->DataSize; i++)
    {
        output.append(pPatternMsg->Data[i]);
    }
    if (pFlowControlMsg)
    {
        for (unsigned long  i = 0; i < pFlowControlMsg->DataSize; i++)
        {
            output.append(pFlowControlMsg->Data[i]);
        }
    }
    //emit LOG_D("Send data:" << parseMessageToHex(output);
    write_serial_data(output);
    received = read_serial_data(100, 50);
    //emit LOG_D("Received:" << received;

    return result;
}

long J2534::PassThruStopMsgFilter(unsigned long ChannelID, unsigned long MsgID)
{
    long result = STATUS_NOERROR;

    return result;
}

long J2534::PassThruSetProgrammingVoltage(unsigned long DeviceID, unsigned long Pin, unsigned long Voltage)
{
    QByteArray output;
    QByteArray received;
    long result = STATUS_NOERROR;

    output.clear();
    QString str = "atv" + QString::number(Pin) + " " + QString::number(Voltage) + "\r\n";
    output.append(str.toUtf8());
    write_serial_data(output);
    //received = read_serial_data(5, 50);

    return result;
}

long J2534::PassThruReadVersion(char *pApiVersion,char *pDllVersion,char *pFirmwareVersion,unsigned long DeviceID)
{
    QByteArray output;
    QByteArray received;
    const char *fw_version = "Main code version: 1.17.4877";
    long result = STATUS_NOERROR;

    strncpy(pApiVersion, API_VERSION, strlen(API_VERSION));
    strncpy(pDllVersion, DLL_VERSION, strlen(DLL_VERSION));
    //strncpy(pFirmwareVersion, fw_version, strlen(fw_version));

    output = "\r\n\r\nati\r\n";
    //emit LOG_D("Send data:" << output;
    write_serial_data(output);
    delay(50);
    received = read_serial_data(50, 100);
    QString response = QString::fromUtf8(received);
    QStringList fw_ver = response.split("ari ");
    fw_ver = fw_ver.at(fw_ver.length()-1).split("\r\n");
    std::string fw_ver_str = fw_ver.at(0).toUtf8().data();
    char *str = fw_ver.at(0).toUtf8().data();
    strncpy(pFirmwareVersion, str, strlen(str));
    //emit LOG_D("Read version received:" << fw_ver.at(0);

    return result;
}

long J2534::PassThruGetLastError(char *pErrorDescription)
{
    long result = STATUS_NOERROR;

    return result;
}

int J2534::is_valid_sconfig_param(SCONFIG s)
{
    switch (s.Parameter)
    {
    case P1_MIN:
    case P2_MIN:
    case P3_MAX:
    case P4_MAX:
        return 0;
        break;
    default:
        return 1;
    }
}

void J2534::dump_sbyte_array(const SBYTE_ARRAY* s)
{
    //emit LOG_D("SBYTE_ARRAY size =" << s->NumOfBytes;
    //DBGPRINT(("SBYTE_ARRAY size=%u\n",s->NumOfBytes));
    //DBGDUMP((s->BytePtr,s->NumOfBytes,0));
}

void J2534::dump_sconfig_param(SCONFIG s)
{
    char paramName[128];

    switch (s.Parameter)
    {
    case DATA_RATE:
        strcpy(paramName,"DATA_RATE");
        break;
    case LOOPBACK:
        strcpy(paramName,"LOOPBACK");
        break;
    case NODE_ADDRESS:
        strcpy(paramName,"NODE_ADDRESS");
        break;
    case NETWORK_LINE:
        strcpy(paramName,"NETWORK_LINE");
        break;
    case P1_MIN:
        strcpy(paramName,"P1_MIN");
        break;
    case P1_MAX:
        strcpy(paramName,"P1_MAX");
        break;
    case P2_MIN:
        strcpy(paramName,"P2_MIN");
        break;
    case P2_MAX:
        strcpy(paramName,"P2_MAX");
        break;
    case P3_MIN:
        strcpy(paramName,"P3_MIN");
        break;
    case P3_MAX:
        strcpy(paramName,"P3_MAX");
        break;
    case P4_MIN:
        strcpy(paramName,"P4_MIN");
        break;
    case P4_MAX:
        strcpy(paramName,"P4_MAX");
        break;
    case W1:
        strcpy(paramName,"W1");
        break;
    case W2:
        strcpy(paramName,"W2");
        break;
    case W3:
        strcpy(paramName,"W3");
        break;
    case W4:
        strcpy(paramName,"W4");
        break;
    case W5:
        strcpy(paramName,"W5");
        break;
    case TIDLE:
        strcpy(paramName,"TIDLE");
        break;
    case TINIL:
        strcpy(paramName,"TINIL");
        break;
    case TWUP:
        strcpy(paramName,"TWUP");
        break;
    case PARITY:
        strcpy(paramName,"PARITY");
        break;
    case BIT_SAMPLE_POINT:
        strcpy(paramName,"BIT_SAMPLE_POINT");
        break;
    case SYNC_JUMP_WIDTH:
        strcpy(paramName,"SYNC_JUMP_WIDTH");
        break;
    case W0:
        strcpy(paramName,"W0");
        break;
    case T1_MAX:
        strcpy(paramName,"T1_MAX");
        break;
    case T2_MAX:
        strcpy(paramName,"T2_MAX");
        break;
    case T4_MAX:
        strcpy(paramName,"T4_MAX");
        break;
    case T5_MAX:
        strcpy(paramName,"T5_MAX");
        break;
    case ISO15765_BS:
        strcpy(paramName,"ISO15765_BS");
        break;
    case ISO15765_STMIN:
        strcpy(paramName,"ISO15765_STMIN");
        break;
    case DATA_BITS:
        strcpy(paramName,"DATA_BITS");
        break;
    case FIVE_BAUD_MOD:
        strcpy(paramName,"FIVE_BAUD_MOD");
        break;
    case BS_TX:
        strcpy(paramName,"BS_TX");
        break;
    case STMIN_TX:
        strcpy(paramName,"STMIN_TX");
        break;
    case T3_MAX:
        strcpy(paramName,"T3_MAX");
        break;
    case ISO15765_WFT_MAX:
        strcpy(paramName,"ISO15765_WFT_MAX");
        break;
    case CAN_MIXED_FORMAT:
        strcpy(paramName,"CAN_MIXED_FORMAT");
        break;
    case J1962_PINS:
        strcpy(paramName,"J1962_PINS");
        break;
    case SW_CAN_HS_DATA_RATE:
        strcpy(paramName,"W_CAN_HS_DATA_RATE");
        break;
    case SW_CAN_SPEEDCHANGE_ENABLE:
        strcpy(paramName,"SW_CAN_SPEEDCHANGE_ENABLE");
        break;
    case SW_CAN_RES_SWITCH:
        strcpy(paramName,"SW_CAN_RES_SWITCH");
        break;
    case ACTIVE_CHANNELS:
        strcpy(paramName,"ACTIVE_CHANNELS");
        break;
    case SAMPLE_RATE:
        strcpy(paramName,"SAMPLE_RATE");
        break;
    case SAMPLES_PER_READING:
        strcpy(paramName,"SAMPLES_PER_READING");
        break;
    case READINGS_PER_MSG:
        strcpy(paramName,"READINGS_PER_MSG");
        break;
    case AVERAGING_METHOD:
        strcpy(paramName,"AVERAGING_METHOD");
        break;
    case SAMPLE_RESOLUTION:
        strcpy(paramName,"SAMPLE_RESOLUTION");
        break;
    case INPUT_RANGE_LOW:
        strcpy(paramName,"INPUT_RANGE_LOW");
        break;
    case INPUT_RANGE_HIGH:
        strcpy(paramName,"INPUT_RANGE_HIGH");
        break;
    default:
        sprintf(paramName,"%lu(unknown)",s.Parameter);
        break;
    }

    //DBGPRINT(("    %s : %u",paramName,s.Value));
    //emit LOG_D("    " << paramName << s.Value;
}


long J2534::PassThruIoctl(unsigned long ChannelID, unsigned long IoctlID, const void *pInput, void *pOutput)
{
    QByteArray output;
    QByteArray received;
    uint32_t par_cnt = 0;

    int input_as_sa = 0;
    int output_as_sa = 0;
    unsigned int i;
    SCONFIG_LIST* scl;
    long result = STATUS_NOERROR;
    char IoctlName[128];

    SCONFIG *cfgitem;
    //const SCONFIG_LIST *inputlist = pInput;

    switch (IoctlID)
    {
    case GET_CONFIG:
        strcpy(IoctlName,"GET_CONFIG");
        break;
    case SET_CONFIG:
        strcpy(IoctlName,"SET_CONFIG");
        break;
    case READ_VBATT:
        strcpy(IoctlName,"READ_VBATT");
        break;
    case FIVE_BAUD_INIT:
        strcpy(IoctlName,"FIVE_BAUD_INIT");
        input_as_sa = 1;
        output_as_sa = 1;
        break;
    case FAST_INIT:
        strcpy(IoctlName,"FAST_INIT");
        break;
    case CLEAR_TX_BUFFER:
        strcpy(IoctlName,"CLEAR_TX_BUFFER");
        break;
    case CLEAR_RX_BUFFER:
        strcpy(IoctlName,"CLEAR_RX_BUFFER");
        break;
    case CLEAR_PERIODIC_MSGS:
        strcpy(IoctlName,"CLEAR_PERIODIC_MSGS");
        break;
    case CLEAR_MSG_FILTERS:
        strcpy(IoctlName,"CLEAR_MSG_FILTERS");
        break;
    case CLEAR_FUNCT_MSG_LOOKUP_TABLE:
        strcpy(IoctlName,"CLEAR_FUNCT_MSG_LOOKUP_TABLE");
        break;
    case ADD_TO_FUNCT_MSG_LOOKUP_TABLE:
        strcpy(IoctlName,"ADD_TO_FUNCT_MSG_LOOKUP_TABLE");
        break;
    case DELETE_FROM_FUNCT_MSG_LOOKUP_TABLE:
        strcpy(IoctlName,"DELETE_FROM_FUNCT_MSG_LOOKUP_TABLE");
        break;
    case READ_PROG_VOLTAGE:
        strcpy(IoctlName,"READ_PROG_VOLTAGE");
        break;
//    case TX_IOCTL_APP_SERVICE:
//        strcpy(IoctlName,"APP_SERVICE");
//        break;
    default:
        sprintf(IoctlName,"%lu(unknown)",IoctlID);
        break;
    }

    if (IoctlID == GET_CONFIG)
    {

    }
    if (IoctlID == SET_CONFIG)
    {
        pOutput = NULL; // make some DLLs happy

        // dump params
        scl = (SCONFIG_LIST*)pInput;
        for (i = 0; i < scl->NumOfParams; i++)
            dump_sconfig_param((scl->ConfigPtr)[i]);

        //Enabling this could break some J2534 devices such as Denso DST-i
        /*for (i = 0; i < scl->NumOfParams; i++)
            if (!is_valid_sconfig_param((scl->ConfigPtr)[i]))
            {
                //emit LOG_D("param not allowed - not passing through and instead faking success" << result;
                return STATUS_NOERROR;
            }*/

        SCONFIG *cfgitem;
        par_cnt = scl->NumOfParams;
        for (i = 0; i < par_cnt; ++i)
        {
            cfgitem = &scl->ConfigPtr[i];
            output.clear();
            QString str = "ats" + QString::number(ChannelID) + " " + QString::number(cfgitem->Parameter) + " " + QString::number(cfgitem->Value) + "\r\n";
            output.append(str.toUtf8());
            //emit LOG_D("Send data:" << output;
            write_serial_data(output);
            received = read_serial_data(100, 50);
            //emit LOG_D("Received:" << received;
            //r = usb_send_expect(data, strlen(data), MAX_LEN, 2000, NULL);
        }

    }
    if (IoctlID == READ_VBATT)
    {
        long* vBatt = (long*)pOutput;
        long pin = 16;
        output.clear();
        QString str = "atr " + QString::number((int)pin) + "\r\n";
        write_serial_data(output);
        delay(50);
        received = read_serial_data(100, 50);
        emit LOG_D("Pin 16 voltage = " + received + " " + parseMessageToHex(received), true, true);
    }

    if (IoctlID == FAST_INIT)
    {
        PASSTHRU_MSG *msg = (PASSTHRU_MSG*)pInput;

        output.clear();
        QString str = "aty" + QString::number(ChannelID) + " " + QString::number(msg->DataSize) + " 0\r\n";
        output.append(str.toUtf8());
        for (i = 0; i < msg->DataSize; i++)
        {
            //emit LOG_D("Value:" << hex << msg->Data[i];
            output.append(msg->Data[i]);
        }
        write_serial_data(output);
        received = read_serial_data(100, 50);
    }

    if (input_as_sa)
    {
        //emit LOG_D("Input";
        dump_sbyte_array((SBYTE_ARRAY*)pInput);
    }

    //result = (*pfPassThruIoctl)(ChannelID,IoctlID,pInput,pOutput);

    if (output_as_sa)
    {
        //emit LOG_D("Output";
        dump_sbyte_array((SBYTE_ARRAY*)pOutput);
    }

    return result;
}

void J2534::delay(int n)
{
    QTime dieTime = QTime::currentTime().addMSecs(n);
    while (QTime::currentTime() < dieTime)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
}

void J2534::handle_error(QSerialPort::SerialPortError error)
{
    //emit LOG_D("Error:" << error;

    if (error == QSerialPort::NoError)
    {
    }
    else if (error == QSerialPort::DeviceNotFoundError)
    {
        close_serial_port();
    }
    else if (error == QSerialPort::PermissionError)
    {
    }
    else if (error == QSerialPort::OpenError)
    {
        close_serial_port();
    }
    else if (error == QSerialPort::NotOpenError)
    {
        close_serial_port();
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
        close_serial_port();
    }
    else if (error == QSerialPort::ReadError)
    {
        close_serial_port();
    }
    else if (error == QSerialPort::ResourceError)
    {
        close_serial_port();
    }
    else if (error == QSerialPort::UnsupportedOperationError)
    {
    }
    else if (error == QSerialPort::TimeoutError)
    {
        close_serial_port();
        //emit LOG_D("Timeout error";
        /*
        if (serial->isOpen())
            serial->flush();
        else
            serial->close();
        */
    }
    else if (error == QSerialPort::UnknownError)
    {
        close_serial_port();
    }
}
