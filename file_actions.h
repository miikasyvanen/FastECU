#ifndef FILE_ACTIONS_H
#define FILE_ACTIONS_H

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

#include <string.h>
#include <iostream>

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
    FileActions();

    uint8_t float_precision = 15;
    int def_map_index = 0;


    struct ConfigValuesStructure {
        QString serial_port = "ttyUSB0";
        QString baudrate = "4800";
        QString flash_method = "wrx02";
        QString flash_protocol = "K-Line";
        QString car_model = "Subaru";
        QString log_protocol = "K-Line";

        QString base_directory = QDir::homePath() + "/FastECU_OEM";
        QString calibration_files_base_directory = base_directory + "/calibrations";
        QString config_base_directory = base_directory + "/config";
        QString definition_files_base_directory = base_directory + "/definitions";
        QString kernel_files_base_directory = base_directory + "/kernels";
        QString log_files_base_directory = base_directory + "/logs";

        QString config_file = config_base_directory + "/fastecu.cfg";
        QString menu_config_file = config_base_directory + "/menu.cfg";
        QString logger_config_file = config_base_directory + "/logger.cfg";

        QStringList calibration_files;
        QString calibration_files_directory = calibration_files_base_directory;
        QStringList romraider_definition_files;
        QString ecuflash_definition_files_directory = definition_files_base_directory + "/ecuflash";
        QString logger_definition_file = definition_files_base_directory + "/logger_METRIC_EN_v370.xml";
        QString definition_files_directory = definition_files_base_directory;
        QString kernel_files_directory = kernel_files_base_directory;
        QString kernel_files;
        QString log_files_directory = log_files_base_directory;

        QStringList definition_types;
        QStringList ecuflash_def_ecu_id;
        QStringList ecuflash_def_filename;
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
        QStringList MinValueList;
        QStringList MaxValueList;
        QStringList UnitsList;
        QStringList FormatList;
        QStringList FineIncList;
        QStringList CoarseIncList;
        QStringList VisibleList;
        QStringList SelectionsList;
        QStringList SelectionsListSorted;
        QStringList DescriptionList;
        QStringList StateList;
        QStringList MapScalingNameList;
        QStringList MapData;

        QStringList XScaleTypeList;
        QStringList XScaleNameList;
        QStringList XScaleAddressList;
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
        QStringList ScalingIncList;
        QStringList ScalingStorageTypeList;
        QStringList ScalingEndianList;

        QStringList RomInfo;
        QString RomInfoExpanded;
        QString RomBase;
        QString RomId;
        QString Kernel;

        QStringList StorageTypeList;
        QStringList EndianList;
        QStringList LogParamList;
        QStringList FromByteList;
        QStringList ToByteList;

        QByteArray FullRomData;
        bool OemEcuFile;
        bool SyncedWithEcu;
        bool use_romraider_definition;
        bool use_ecuflash_definition;

    } EcuCalDefStruct;

    QStringList RomInfoStrings = {
        "XmlId",
        "InternalIdAddress",
        "Make",
        "Model",
        "SubModel",
        "Market",
        "Transmission",
        "Year",
        "EcuId",
        "InternalIdString",
        "MemModel",
        "ChecksumModule",
        "RomBase",
        "FlashMethod",
        "FileSize",
    };

    enum RomInfoEnum {
        XmlId,
        InternalIdAddress,
        Make,
        Model,
        SubModel,
        Market,
        Transmission,
        Year,
        EcuId,
        InternalIdString,
        MemModel,
        ChecksumModule,
        RomBase,
        FlashMethod,
        FileSize,
    };

    /****************************************************
     * Check if FastECU dir exists in users home folder
     * If not, create one with appropriate files
     ***************************************************/
    ConfigValuesStructure *check_config_dir();

    /****************************
     * Open FastECU config file
     ***************************/
    ConfigValuesStructure *read_config_file();

    /****************************
     * Open FastECU config file
     ***************************/
    ConfigValuesStructure *save_config_file();

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
    //EcuCalDefStructure *read_ecu_definition_file(EcuCalDefStructure *ecuCalDef, QString ecuId);
    EcuCalDefStructure *read_romraider_ecu_base_def(FileActions::EcuCalDefStructure *ecuCalDef);
    EcuCalDefStructure *read_romraider_ecu_def(FileActions::EcuCalDefStructure *ecuCalDef, QString ecuId);
    EcuCalDefStructure *add_romraider_def_list_item(EcuCalDefStructure *ecuCalDef);

    /*****************************************************
     * Search and read RomRaider ECU definition from file
     *****************************************************/
    QString convert_value_format(QString value_format);
    ConfigValuesStructure *create_ecuflash_def_id_list(ConfigValuesStructure *configValues);
    EcuCalDefStructure *read_ecuflash_ecu_base_def(FileActions::EcuCalDefStructure *ecuCalDef);
    EcuCalDefStructure *read_ecuflash_ecu_def(FileActions::EcuCalDefStructure *ecuCalDef, QString ecuId);
    EcuCalDefStructure *add_ecuflash_def_list_item(EcuCalDefStructure *ecuCalDef);

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

    /***********************************************
     * Apply changes made to calibration
     * to rom data array
     **********************************************/
    EcuCalDefStructure *apply_subaru_cal_changes_to_rom_data(FileActions::EcuCalDefStructure *ecuCalDef);

    /***************************
     * Read software menu file
     * for menu creation
     **************************/
    QSignalMapper *read_menu_file(QMenuBar *menubar, QToolBar *toolBar);

    /***************************
     * Calculate Subaru 32-bit
     * checksums
     **************************/
    QByteArray checksum_module_subarudbw(FileActions::EcuCalDefStructure *ecuCalDef, uint32_t checksum_area_start, uint32_t checksum_area_end);

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
