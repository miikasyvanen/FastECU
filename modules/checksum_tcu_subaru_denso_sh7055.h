#ifndef CHECKSUM_TCU_SUBARU_DENSO_SH7055_H
#define CHECKSUM_TCU_SUBARU_DENSO_SH7055_H

#include <QDebug>
#include <QMessageBox>
#include <QWidget>

class ChecksumTcuSubaruDensoSH7055 : public QWidget
{
    Q_OBJECT

public:
    ChecksumTcuSubaruDensoSH7055();
    ~ChecksumTcuSubaruDensoSH7055();

    QByteArray calculate_checksum(QByteArray romData);

private:

};

#endif // CHECKSUM_TCU_SUBARU_DENSO_SH7055_H
