#ifndef CHECKSUM_ECU_SUBARU_DENSO_SH705X_DIESEL_H
#define CHECKSUM_ECU_SUBARU_DENSO_SH705X_DIESEL_H

#include <QDebug>
#include <QMessageBox>
#include <QWidget>

class ChecksumEcuSubaruDensoSH705xDiesel : public QWidget
{
    Q_OBJECT

public:
    ChecksumEcuSubaruDensoSH705xDiesel();
    ~ChecksumEcuSubaruDensoSH705xDiesel();

    QByteArray calculate_checksum(QByteArray romData, uint32_t checksum_area_start, uint32_t checksum_area_length);

private:

};

#endif // CHECKSUM_ECU_SUBARU_DENSO_SH705X_DIESEL_H
