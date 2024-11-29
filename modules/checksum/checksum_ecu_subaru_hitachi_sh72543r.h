#ifndef CHECKSUM_ECU_SUBARU_HITACHI_SH72543R_H
#define CHECKSUM_ECU_SUBARU_HITACHI_SH72543R_H

#include <QDebug>
#include <QMessageBox>

class ChecksumEcuSubaruHitachiSh72543r
{
public:
    ChecksumEcuSubaruHitachiSh72543r();
    ~ChecksumEcuSubaruHitachiSh72543r();

    static QByteArray calculate_checksum(QByteArray romData);

private:

};

#endif // CHECKSUM_ECU_SUBARU_HITACHI_SH72543R_H
