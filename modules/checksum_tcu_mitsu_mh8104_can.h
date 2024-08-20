#ifndef CHECKSUM_TCU_MITSU_MH8104_CAN_H
#define CHECKSUM_TCU_MITSU_MH8104_CAN_H

#include <QDebug>
#include <QMessageBox>
#include <QWidget>

class ChecksumTcuMitsuMH8104Can : public QWidget
{
    Q_OBJECT

public:
    ChecksumTcuMitsuMH8104Can();
    ~ChecksumTcuMitsuMH8104Can();

    QByteArray calculate_checksum(QByteArray romData);

private:

};

#endif // CHECKSUM_TCU_MITSU_MH8104_CAN_H
