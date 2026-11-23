#ifndef CHECKSUM_ECU_SUBARU_RH850_DI_H
#define CHECKSUM_ECU_SUBARU_RH850_DI_H

#include <QDebug>
#include <QMessageBox>

class ChecksumEcuSubaruRh850Di
{
public:
    ChecksumEcuSubaruRh850Di();
    ~ChecksumEcuSubaruRh850Di();

    static QByteArray calculate_checksum(QByteArray romData);

private:

};

#endif // CHECKSUM_ECU_SUBARU_RH850_DI_H
