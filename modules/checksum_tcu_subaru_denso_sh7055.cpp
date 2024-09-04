#include "checksum_tcu_subaru_denso_sh7055.h"

ChecksumTcuSubaruDensoSH7055::ChecksumTcuSubaruDensoSH7055()
{

}

ChecksumTcuSubaruDensoSH7055::~ChecksumTcuSubaruDensoSH7055()
{

}

QByteArray ChecksumTcuSubaruDensoSH7055::calculate_checksum(QByteArray romData)
{
    // Checksum is calulated by adding 16bit values from each area. As added bytes are 16bit,
    // every area size (16bit byte count) needs to multiply by 2

    QStringList checksum_areas = {  "0x1000", "0x0800",
                                    "0x3000", "0x003a",
                                    "0x3080", "0x4c00",
                                    "0xc880", "0x83f6",
                                    "0x1d06c","0x83f6",
                                    "0x2d858","0x83f8",
                                    "0x3e048","0x0fdc",
                                    "0x40000","0x741c",
                                    "0x4e838","0x83f6",
                                    "0x5f024","0x83f8",
                                    "0x6f814","0x8174",
                                    "0x7fb80","0x0240" };
    QString msg;
    uint16_t checksum = 0;
    bool ok = false;
    bool checksum_ok = false;

    for (int i = 0; i < checksum_areas.length(); i += 2)
    {
        unsigned int area_start = checksum_areas.at(i).toUInt(&ok, 16);
        unsigned int area_end = area_start + (2 * checksum_areas.at(i+1).toUInt(&ok, 16));

        msg.clear();
        msg.append(QString("%1: ").arg(i / 2,2,10,QLatin1Char('0')).toUtf8());
        msg.append(QString("Read from 0x%1 to 0x%2").arg(area_start,8,16,QLatin1Char('0')).arg(area_end,8,16,QLatin1Char('0')).toUtf8());

        qDebug() << msg;

        for (unsigned int j = area_start; j < area_end; j += 2)
        {
            checksum += ((uint8_t)romData.at(j) << 8) + ((uint8_t)romData.at(j + 1));
        }
    }

    msg.clear();
    msg.append(QString("0x%1").arg(checksum,4,16,QLatin1Char('0')).toUtf8());

    qDebug() << "Checksum =" << msg;

    if (checksum != 0x5aa5)
    {
        qDebug() << "Checksum mismatch!";

        QByteArray balance_value_array;
        uint32_t balance_value_array_start = 0x7fff4;
        uint16_t balance_value = ((uint8_t)romData.at(0x7fff4) << 8) + ((uint8_t)romData.at(0x7fff5));

        msg.clear();
        msg.append(QString("Balance value before: 0x%1").arg(balance_value,4,16,QLatin1Char('0')).toUtf8());
        qDebug() << msg;

        balance_value += 0x5aa5 - checksum;

        msg.clear();
        msg.append(QString("Balance value after: 0x%1").arg(balance_value,4,16,QLatin1Char('0')).toUtf8());
        qDebug() << msg;

        balance_value_array.append((uint8_t)((balance_value >> 8) & 0xff));
        balance_value_array.append((uint8_t)(balance_value & 0xff));
        romData.replace(balance_value_array_start, balance_value_array.length(), balance_value_array);

        qDebug() << "Checksums corrected";
        QMessageBox::information(this, tr("Subaru Denso SH7055 TCU Checksum"), "Checksums corrected");
    }
    //else
    //    QMessageBox::information(this, tr("Subaru Denso SH7055 TCU Checksum"), "Checksums OK");

    return romData;
}
