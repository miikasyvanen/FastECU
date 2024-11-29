#ifndef CHECKSUM_TCU_SUBARU_DENSO_SH7055_H
#define CHECKSUM_TCU_SUBARU_DENSO_SH7055_H

#include <QDebug>
#include <QMessageBox>

class ChecksumTcuSubaruDensoSH7055
{
public:
    ChecksumTcuSubaruDensoSH7055();
    ~ChecksumTcuSubaruDensoSH7055();

    static QByteArray calculate_checksum(QByteArray romData);

private:

};

#endif // CHECKSUM_TCU_SUBARU_DENSO_SH7055_H
