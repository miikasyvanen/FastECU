#ifndef CHECKSUM_ECU_SUBARU_HITACHI_SH7058_H
#define CHECKSUM_ECU_SUBARU_HITACHI_SH7058_H

#include <QDebug>
#include <QMessageBox>

class ChecksumEcuSubaruHitachiSH7058
{
public:
    ChecksumEcuSubaruHitachiSH7058();
    ~ChecksumEcuSubaruHitachiSH7058();

    static QByteArray calculate_checksum(QByteArray romData);

private:

};

#endif // CHECKSUM_ECU_SUBARU_HITACHI_SH7058_H
