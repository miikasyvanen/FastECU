#ifndef FILE_ACTIONS_H
#define FILE_ACTIONS_H

#include <QApplication>
#include <QWidget>
#include <QScreen>
//#include <QDesktopWidget>
#include <QWidget>
#include <QFileDialog>
#include <QDomDocument>
#include <QXmlStreamReader>
#include <QMessageBox>
#include <QDebug>
#include <QSignalMapper>
#include <QMenu>
#include <QMenuBar>
#include <QToolBar>
#include <QElapsedTimer>
#include <QDateTime>
#include <QDirIterator>
#include <QPushButton>
#include <QVBoxLayout>
#include <QLabel>
#include <QRadioButton>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QTextEdit>

#include <string.h>
#include <iostream>
#include <math.h>

#include "modules/checksum/checksum_ecu_subaru_denso_sh705x_diesel.h"
#include "modules/checksum/checksum_ecu_subaru_denso_sh7xxx.h"
#include "modules/checksum/checksum_ecu_subaru_hitachi_m32r.h"
#include "modules/checksum/checksum_ecu_subaru_hitachi_sh7058.h"
#include "modules/checksum/checksum_ecu_subaru_hitachi_sh72543r.h"

#include "modules/checksum/checksum_tcu_subaru_denso_sh7055.h"
#include "modules/checksum/checksum_tcu_subaru_hitachi_m32r_can.h"
#include "modules/checksum/checksum_tcu_mitsu_mh8104_can.h"

#include <kernelmemorymodels.h>


#ifdef WIN32
    #include <windows.h>
#else
    #include <unistd.h>
#endif // win32

using namespace std;

class FileActions : public QWidget
{
    Q_OBJECT

public:
    FileActions(QWidget *parent = nullptr);

    uint8_t float_precision = 15;
    int def_map_index = 0;
    //QString ecu_protocol;

    struct ConfigValuesStructure {
        QString software_name = "FastECU";
        QString software_title = "FastECU";
        QString software_version = "0.1.0-beta.2";

        QString serial_port = "ttyUSB0";
        QString baudrate = "4800";
        QString window_size = "default";
        QString window_width = "default";
        QString window_height = "default";
        QString toolbar_iconsize = "32";

#ifdef Q_OS_LINUX
        QString app_base_config_directory = QDir::homePath() + "/.config/FastECU/";
#elif defined(_WIN32) || defined(WIN32) || defined (_WIN64) || defined (WIN64)
        QString app_base_config_directory = QDir::homePath() + "/AppData/Local/FastECU/";
#endif
        QString base_config_directory = app_base_config_directory;
        QString calibration_files_base_directory = base_config_directory + software_version + "/calibrations/";
        QString config_files_base_directory = base_config_directory + software_version + "/config/";
        QString definition_files_base_directory = base_config_directory + software_version + "/definitions/";
        QString kernel_files_base_directory = base_config_directory + software_version + "/kernels/";
        QString datalog_files_base_directory = base_config_directory + software_version + "/logs/";

        QString version_config_directory;
        QString calibration_files_directory;
        QString config_files_directory;
        QString definition_files_directory;
        QString kernel_files_directory;
        QString datalog_files_directory;

        QString config_file = "fastecu.cfg";
        QString menu_file = "menu.cfg";
        QString logger_file = "logger.cfg";
        QString protocols_file = "protocols.cfg";

        QStringList calibration_files;
        QStringList romraider_definition_files;
        QString ecuflash_definition_files_directory;
        QString romraider_logger_definition_file;
        QString kernel_files;

        QString use_romraider_definitions = "disabled";
        QString use_ecuflash_definitions = "disbled";
        QString primary_definition_base;

        QStringList ecuflash_def_cal_id;
        QStringList ecuflash_def_cal_id_addr;
        QStringList ecuflash_def_ecu_id;
        QStringList ecuflash_def_filename;
        QStringList romraider_def_cal_id;
        QStringList romraider_def_cal_id_addr;
        QStringList romraider_def_ecu_id;
        QStringList romraider_def_filename;

        QStringList flash_protocol_id;
        QStringList flash_protocol_alias;
        QStringList flash_protocol_make;
        QStringList flash_protocol_model;
        QStringList flash_protocol_version;
        QStringList flash_protocol_type;
        QStringList flash_protocol_kw;
        QStringList flash_protocol_hp;
        QStringList flash_protocol_fuel;
        QStringList flash_protocol_year;
        QStringList flash_protocol_ecu;
        QStringList flash_protocol_mcu;
        QStringList flash_protocol_mode;
        QStringList flash_protocol_checksum;
        QStringList flash_protocol_read;
        QStringList flash_protocol_test_write;
        QStringList flash_protocol_write;
        QStringList flash_protocol_flash_transport;
        QStringList flash_protocol_log_transport;
        QStringList flash_protocol_log_protocol;
        QStringList flash_protocol_ecu_id_ascii;
        QStringList flash_protocol_ecu_id_addr;
        QStringList flash_protocol_ecu_id_length;
        QStringList flash_protocol_cal_id_ascii;
        QStringList flash_protocol_cal_id_addr;
        QStringList flash_protocol_cal_id_length;
        QStringList flash_protocol_kernel;
        QStringList flash_protocol_kernel_addr;
        QStringList flash_protocol_description;
        QStringList flash_protocol_protocol_name;

        QString flash_protocol_selected_id;
        QString flash_protocol_selected_make;
        QString flash_protocol_selected_model;
        QString flash_protocol_selected_version;
        QString flash_protocol_selected_mcu;
        QString flash_protocol_selected_checksum;
        QString flash_protocol_selected_flash_transport;
        QString flash_protocol_selected_log_transport;
        QString flash_protocol_selected_log_protocol;
        QString flash_protocol_selected_protocol_name;
        QString flash_protocol_selected_description;

    } ConfigValuesStruct;

    struct protocolsStructure {
        QStringList protocols;
        QStringList baudrate;
        QStringList databits;
        QStringList stopbits;
        QStringList parity;
        QStringList connect_timeout;
        QStringList send_timeout;
    } protocolsStruct;

    struct LogValuesStructure {
        QString ecu_id;
        QStringList log_value_protocol;
        QStringList log_value_id;
        QStringList log_value_name;
        QStringList log_value_description;
        QStringList log_value_ecu_byte_index;
        QStringList log_value_ecu_bit;
        QStringList log_value_target;
        QStringList log_value_address;
        QStringList log_value_units;
        QStringList log_value_from_byte;
        QStringList log_value_format;
        QStringList log_value_gauge_min;
        QStringList log_value_gauge_max;
        QStringList log_value_gauge_step;

        QStringList log_value_ecu_id;
        QStringList log_value_length;
        QStringList log_value_type;
        QStringList log_value;

        QStringList log_value_enabled;

        QStringList log_values_names_sorted;
        QStringList log_values_by_protocol;

        QStringList dashboard_log_value_id;
        QStringList lower_panel_log_value_id;
        QString logging_values_protocol;

        // Switch values
        QStringList log_switch_protocol;
        QStringList log_switch_id;
        QStringList log_switch_name;
        QStringList log_switch_description;
        QStringList log_switch_address;
        QStringList log_switch_ecu_byte_index;
        QStringList log_switch_ecu_bit;
        QStringList log_switch_target;
        QStringList log_switch_enabled;
        QStringList log_switch_state;

        QStringList log_switches_names_sorted;

        QStringList lower_panel_switch_id;
    } LogValuesStruct;

    struct dt_codes_structure {
        QStringList dt_code_id;
        QStringList dt_code_name;
        QStringList dt_code_description;
        QStringList dt_code_temp_address;
        QStringList dt_code_mem_address;
        QStringList dt_code_ecu_bit;
    } dt_codes_struct;

    struct EcuCalDefStructure {
        QString FileName;
        QString DefinitionFileName;
        QString FullFileName;
        QString FileSize;
        QStringList IdList;
        QStringList TypeList;
        QStringList NameList;
        QStringList AddressList;
        QStringList CategoryList;
        QStringList CategoryExpandedList;
        QStringList XSizeList;
        QStringList YSizeList;
        QStringList StartPosList;
        QStringList IntervalList;
        QStringList MinValueList;
        QStringList MaxValueList;
        QStringList UnitsList;
        QStringList FormatList;
        QStringList FineIncList;
        QStringList CoarseIncList;
        QStringList VisibleList;
        QStringList SelectionsNameList;
        QStringList SelectionsValueList;
        QStringList DescriptionList;
        QStringList StateList;
        QStringList MapScalingNameList;
        QStringList MapData;

        QStringList ScaleTypeList[2];
        QStringList ScaleNameList[2];
        QStringList ScaleAddressList[2];
        QStringList ScaleSizeList[2];
        QStringList ScaleStartPosList[2];
        QStringList ScaleIntervalList[2];
        QStringList ScaleMinValueList[2];
        QStringList ScaleMaxValueList[2];
        QStringList ScaleUnitsList[2];
        QStringList ScaleFormatList[2];
        QStringList ScaleFineIncList[2];
        QStringList ScaleCoarseIncList[2];
        QStringList ScaleStorageTypeList[2];
        QStringList ScaleEndianList[2];
        QStringList ScaleLogParamList[2];
        QStringList ScaleFromByteList[2];
        QStringList ScaleToByteList[2];
        QStringList ScaleStaticDataList[2];
        QStringList ScaleScalingNameList[2];
        QStringList ScaleData[2];

        QStringList XScaleTypeList;
        QStringList XScaleNameList;
        QStringList XScaleAddressList;
        QStringList XScaleStartPosList;
        QStringList XScaleIntervalList;
        QStringList XScaleMinValueList;
        QStringList XScaleMaxValueList;
        QStringList XScaleUnitsList;
        QStringList XScaleFormatList;
        QStringList XScaleFineIncList;
        QStringList XScaleCoarseIncList;
        QStringList XScaleStorageTypeList;
        QStringList XScaleEndianList;
        QStringList XScaleLogParamList;
        QStringList XScaleFromByteList;
        QStringList XScaleToByteList;
        QStringList XScaleStaticDataList;
        QStringList XScaleScalingNameList;
        QStringList XScaleData;

        QStringList YScaleTypeList;
        QStringList YScaleNameList;
        QStringList YScaleAddressList;
        QStringList YScaleStartPosList;
        QStringList YScaleIntervalList;
        QStringList YScaleMinValueList;
        QStringList YScaleMaxValueList;
        QStringList YScaleUnitsList;
        QStringList YScaleFormatList;
        QStringList YScaleFineIncList;
        QStringList YScaleCoarseIncList;
        QStringList YScaleStorageTypeList;
        QStringList YScaleEndianList;
        QStringList YScaleLogParamList;
        QStringList YScaleFromByteList;
        QStringList YScaleToByteList;
        QStringList YScaleScalingNameList;
        QStringList YScaleData;

        QStringList ScalingNameList;
        QStringList ScalingUnitsList;
        QStringList ScalingFromByteList;
        QStringList ScalingToByteList;
        QStringList ScalingFormatList;
        QStringList ScalingMinValueList;
        QStringList ScalingMaxValueList;
        QStringList ScalingCoarseIncList;
        QStringList ScalingFineIncList;
        QStringList ScalingStorageTypeList;
        QStringList ScalingEndianList;
        //QStringList ScalingBloblistList;
        QStringList ScalingSelectionsNameList;
        QStringList ScalingSelectionsValueList;

        QStringList RomInfo;
        QString RomInfoExpanded;
        QString RomBase;
        QString RomId;
        QString Kernel;
        QString KernelStartAddr;
        QString FlashMethod;
        QString McuType;

        QStringList StorageTypeList;
        QStringList EndianList;
        QStringList LogParamList;
        QStringList FromByteList;
        QStringList ToByteList;
        QStringList MapDefined;

        QByteArray FullRomData;
        bool OemEcuFile;
        bool SyncedWithEcu;
        bool use_romraider_definition;
        bool use_ecuflash_definition;

        QStringList RomInfoStrings = {
            "XML ID",
            "Internal ID Address",
            "Internal ID String",
            "ECU ID",
            "Make",
            "Market",
            "Model",
            "Submodel",
            "Transmission",
            "Year",
            "Flash Method",
            "Memory Model",
            "Checksum Module",
            "Rom Base",
            "File Size",
            "Def File",
        };

        QStringList RomInfoNames = {
            "xmlid",
            "internalidaddress",
            "internalidstring",
            "ecuid",
            "make",
            "market",
            "model",
            "submodel",
            "transmission",
            "year",
            "flashmethod",
            "memmodel",
            "checksummodule",
            "rombase",
            "filesize",
            "deffile",
        };

        QStringList DefHeaderStrings = {
            "XML ID",
            "Internal ID Address",
            "Internal ID String",
            "ECU ID",
            "Make",
            "Market",
            "Model",
            "Submodel",
            "Transmission",
            "Year",
            "Flash Method",
            "Memory Model",
            "Checksum Module",
            "Include",
            "Notes",
        };

        QStringList DefHeaderNames = {
            "xmlid",
            "internalidaddress",
            "internalidstring",
            "ecuid",
            "make",
            "market",
            "model",
            "submodel",
            "transmission",
            "year",
            "flashmethod",
            "memmodel",
            "checksummodule",
            "include",
            "notes",
        };

    } EcuCalDefStruct;

    //EcuCalDefStructure *ecuCalDefTemp;

    enum RomInfoEnum {
        XmlId,
        InternalIdAddress,
        InternalIdString,
        EcuId,
        Make,
        Market,
        Model,
        SubModel,
        Transmission,
        Year,
        FlashMethod,
        MemModel,
        ChecksumModule,
        RomBase,
        FileSize,
        DefFile,
    };

    /****************************************************
     * Check if FastECU dir exists in users home folder
     * If not, create one with appropriate files
     ***************************************************/
    ConfigValuesStructure *check_config_dir(ConfigValuesStructure *configValues);
    bool copy_directory_files(const QString &source_dir, const QString &target_dir, bool cover_file_if_exist);

    /****************************
     * Read FastECU config file
     ***************************/
    ConfigValuesStructure *read_config_file(ConfigValuesStructure *configValues);

    /****************************
     * Save FastECU config file
     ***************************/
    ConfigValuesStructure *save_config_file(FileActions::ConfigValuesStructure *configValues);

    /*************************************
     * Read FastECU flash protocols file
     ************************************/
    ConfigValuesStructure *read_protocols_file(FileActions::ConfigValuesStructure *configValues);

    /************************
     * Read logger def file
     ***********************/
    LogValuesStructure *read_logger_definition_file();

    /*************************
     * Read logger conf file
     ************************/
    LogValuesStructure *read_logger_conf(FileActions::LogValuesStructure *logValues, QString ecu_id, bool modify);

    /************************
     * Save logger conf file
     ************************/
    void *save_logger_conf(FileActions::LogValuesStructure *logValues, QString ecu_id);

    /*****************************************************
     * Search and read RomRaider ECU definition from file
     *****************************************************/
    ConfigValuesStructure *create_romraider_def_id_list(ConfigValuesStructure *configValues);
    EcuCalDefStructure *read_romraider_ecu_base_def(FileActions::EcuCalDefStructure *ecuCalDef);
    EcuCalDefStructure *read_romraider_ecu_def(FileActions::EcuCalDefStructure *ecuCalDef, QString ecuId);
    EcuCalDefStructure *add_romraider_def_list_item(EcuCalDefStructure *ecuCalDef);

    /*****************************************************
     * Search and read RomRaider ECU definition from file
     *****************************************************/
    QString convert_value_format(QString value_format);
    ConfigValuesStructure *create_ecuflash_def_id_list(ConfigValuesStructure *configValues);
    //EcuCalDefStructure *read_ecuflash_ecu_base_def(FileActions::EcuCalDefStructure *ecuCalDef);
    EcuCalDefStructure *read_ecuflash_ecu_def(FileActions::EcuCalDefStructure *ecuCalDef, QString cal_id);
    EcuCalDefStructure *parse_ecuflash_def_scalings(EcuCalDefStructure *ecuCalDef);
    EcuCalDefStructure *add_ecuflash_def_list_item(EcuCalDefStructure *ecuCalDef);

    //EcuCalDefStructure *read_ecuflash_ecu_def_test(FileActions::EcuCalDefStructure *ecuCalDef, QString cal_id);

    QString parse_hex_ecuid(uint8_t byte);
    EcuCalDefStructure *parse_ecuid_ecuflash_def_files(FileActions::EcuCalDefStructure *ecuCalDef, bool is_ascii);
    EcuCalDefStructure *parse_ecuid_romraider_def_files(FileActions::EcuCalDefStructure *ecuCalDef, bool is_ascii);

    EcuCalDefStructure *create_new_definition_for_rom(FileActions::EcuCalDefStructure *ecuCalDef);
    EcuCalDefStructure *use_existing_definition_for_rom(FileActions::EcuCalDefStructure *ecuCalDef);

    /***********************************************
     * Open ECU ROM file, including possible
     * checksum calculations and value conversions
     **********************************************/
    EcuCalDefStructure *open_subaru_rom_file(FileActions::EcuCalDefStructure *ecuCalDef, QString fileName);

    /***********************************************
     * Save ECU ROM file, including possible
     * checksum calculations and value conversions
     **********************************************/
    EcuCalDefStructure *save_subaru_rom_file(FileActions::EcuCalDefStructure *ecuCalDef, QString fileName);

    /***************************
     * Read software menu file
     * for menu creation
     **************************/
    QSignalMapper *read_menu_file(QMenuBar *menubar, QToolBar *toolBar);

    /***************************
     * Calculate Subaru 32-bit
     * checksums
     **************************/
    EcuCalDefStructure *checksum_correction(FileActions::EcuCalDefStructure *ecuCalDef);

    /*************************************
     * Parse expression strings for used
     * in ROM map data conversion
     ************************************/
    QStringList parse_stringlist_from_expression_string(QString expression, QString x);

    /**************************************************
     * Calculate ROM map data with parsed expressions
     *************************************************/
    double calculate_value_from_expression(QStringList expression);

};

#endif // FILE_ACTIONS_H
