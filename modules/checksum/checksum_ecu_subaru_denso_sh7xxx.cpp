#include "checksum_ecu_subaru_denso_sh7xxx.h"

ChecksumEcuSubaruDensoSH7xxx::ChecksumEcuSubaruDensoSH7xxx()
{

}

ChecksumEcuSubaruDensoSH7xxx::~ChecksumEcuSubaruDensoSH7xxx()
{

}

QByteArray ChecksumEcuSubaruDensoSH7xxx::calculate_checksum(QByteArray romData, uint32_t checksum_area_start, uint32_t checksum_area_length, int32_t offset)
{
    QByteArray checksum_array;

    uint32_t checksum_area_end = checksum_area_start + checksum_area_length;
    uint32_t checksum_dword_addr_lo = 0;
    uint32_t checksum_dword_addr_hi = 0;
    uint32_t checksum = 0;
    uint32_t checksum_temp = 0;
    uint32_t checksum_diff = 0;
    uint32_t checksum_check = 0;
    uint8_t checksum_block = 0;

    bool checksum_ok = true;


    for (uint32_t i = checksum_area_start; i < checksum_area_end; i+=12)
    {
        checksum = 0;
        checksum_temp = 0;
        checksum_dword_addr_lo = 0;
        checksum_dword_addr_hi = 0;
        checksum_diff = 0;

        for (int j = 0; j < 4; j++)
        {
            checksum_dword_addr_lo = (checksum_dword_addr_lo << 8) + (uint8_t)romData.at(i + j);
            checksum_dword_addr_hi = (checksum_dword_addr_hi << 8) + (uint8_t)romData.at(i + 4 + j);
            checksum_diff = (checksum_diff << 8) + (uint8_t)romData.at(i + 8 + j);
        }
        if (checksum_dword_addr_lo == 0 && checksum_dword_addr_hi == 0)
        {
            offset = 0;
        }
        uint32_t checksum_dword_addr_lo_with_offset = checksum_dword_addr_lo + offset;
        uint32_t checksum_dword_addr_hi_with_offset = checksum_dword_addr_hi + offset;
        if (i == checksum_area_start && checksum_dword_addr_lo_with_offset == 0 && checksum_dword_addr_hi_with_offset == 0 && checksum_diff == 0x5aa5a55a)
        {
            qDebug() << "ROM has all checksums disabled";
            QMessageBox::information(nullptr, QObject::tr("32-bit checksum"), "ROM has all checksums disabled");
            return 0;
        }

        if (checksum_dword_addr_lo_with_offset == 0 && checksum_dword_addr_hi_with_offset == 0 && checksum_diff == 0x5aa5a55a)
        {
            //QMessageBox::information(nullptr, QObject::tr("32-bit checksum"), "Checksums disabled");
        }
        if (checksum_dword_addr_lo_with_offset != 0 && checksum_dword_addr_hi_with_offset != 0 && checksum_diff != 0x5aa5a55a)
        {
            for (uint32_t j = checksum_dword_addr_lo_with_offset; j < checksum_dword_addr_hi_with_offset; j+=4)
            {
                for (int k = 0; k < 4; k++)
                {
                    checksum_temp = (checksum_temp << 8) + (uint8_t)(romData.at(j + k));
                }
                checksum += checksum_temp;
            }
        }
        checksum_check = 0x5aa5a55a - checksum;

        if (checksum_diff == checksum_check)
        {
            qDebug() << "Checksum block " + QString::number(checksum_block) + " OK";
        }
        else
        {
            qDebug() << "Checksum block " + QString::number(checksum_block) + " NOK";
            checksum_ok = false;
        }

        checksum_array.append(checksum_dword_addr_lo >> 24);
        checksum_array.append(checksum_dword_addr_lo >> 16);
        checksum_array.append(checksum_dword_addr_lo >> 8);
        checksum_array.append(checksum_dword_addr_lo);
        checksum_array.append(checksum_dword_addr_hi >> 24);
        checksum_array.append(checksum_dword_addr_hi >> 16);
        checksum_array.append(checksum_dword_addr_hi >> 8);
        checksum_array.append(checksum_dword_addr_hi);
        checksum_array.append(checksum_check >> 24);
        checksum_array.append(checksum_check >> 16);
        checksum_array.append(checksum_check >> 8);
        checksum_array.append(checksum_check);

        checksum_block++;
    }

    if (!checksum_ok)
    {
        romData.replace(checksum_area_start, checksum_area_length, checksum_array);
        qDebug() << "Checksums corrected";
        QMessageBox::information(nullptr, QObject::tr("Subaru Denso SH705x Checksum"), "Checksums corrected");
    }
    //else
    //    QMessageBox::information(nullptr, QObject::tr("Subaru Denso SH705x Checksum"), "Checksums OK");

    return romData;
}

