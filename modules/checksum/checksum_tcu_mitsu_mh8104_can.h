#ifndef CHECKSUM_TCU_MITSU_MH8104_CAN_H
#define CHECKSUM_TCU_MITSU_MH8104_CAN_H

#include <QDebug>
#include <QMessageBox>

class ChecksumTcuMitsuMH8104Can
{
public:
    ChecksumTcuMitsuMH8104Can();
    ~ChecksumTcuMitsuMH8104Can();

    static QByteArray calculate_checksum(QByteArray romData);

private:

};

#endif // CHECKSUM_TCU_MITSU_MH8104_CAN_H
