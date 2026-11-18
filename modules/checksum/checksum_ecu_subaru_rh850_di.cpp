#include "checksum_ecu_subaru_rh850_di.h"

ChecksumEcuSubaruRh850Di::ChecksumEcuSubaruRh850Di()
{

}

ChecksumEcuSubaruRh850Di::~ChecksumEcuSubaruRh850Di()
{

}

QByteArray ChecksumEcuSubaruRh850Di::calculate_checksum(QByteArray romData)
{
    /*******************
     *
     * Subaru DI (RH850, 2022+ "LHB*" calibrations, e.g. 2022/2023 WRX VB MT).
     *
     * The image is the flash region only: file offset 0 == CPU address 0x10000,
     * length 0x3F0000 (CPU 0x10000 - 0x400000). Little-endian 16-bit summation
     * over the whole image must equal 0x5AA5; the magic word at the start is
     * 0x5555. The balance (correction) word is the 16-bit LE value at file
     * offset 0x3DFF00 (CPU 0x3EFF00), which is included in the sum.
     *
     * Derived from the stock images LHBHB10B (2022) and LHBKC40M (2023), both of
     * which validate to 0x5AA5 with this rule. Balance-word location confirmed by
     * flip-and-correct round trip on those images.
     *
     ******************/

    QByteArray msg;
    uint16_t chksum = 0;

    for (int i = 0; i < romData.length(); i += 2)
    {
        chksum += (uint16_t)((uint8_t)romData.at(i) | ((uint8_t)romData.at(i + 1) << 8));
    }
    msg.clear();
    msg.append(QString("CHKSUM: 0x%1").arg(chksum, 4, 16, QLatin1Char('0')).toUtf8());
    qDebug() << msg;
    if (chksum != 0x5aa5)
    {
        qDebug() << "Checksum mismatch!";

        QByteArray balance_value_array;
        uint32_t balance_value_array_start = 0x3DFF00;
        uint16_t balance_value = (uint16_t)((uint8_t)romData.at(balance_value_array_start)
                                 | ((uint8_t)romData.at(balance_value_array_start + 1) << 8));

        msg.clear();
        msg.append(QString("Balance value before: 0x%1").arg(balance_value, 4, 16, QLatin1Char('0')).toUtf8());
        qDebug() << msg;

        balance_value += 0x5aa5 - chksum;

        msg.clear();
        msg.append(QString("Balance value after: 0x%1").arg(balance_value, 4, 16, QLatin1Char('0')).toUtf8());
        qDebug() << msg;

        // Little-endian: low byte first
        balance_value_array.append((uint8_t)(balance_value & 0xff));
        balance_value_array.append((uint8_t)((balance_value >> 8) & 0xff));
        romData.replace(balance_value_array_start, balance_value_array.length(), balance_value_array);

        qDebug() << "Checksums corrected";
        QMessageBox::information(nullptr, QObject::tr("Subaru RH850 DI ECU Checksum"), "Checksums corrected");
    }

    return romData;
}
