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

    struct ConfigValuesStructure {
        QString serial_port;
        QString baudrate;
        QString flash_method;
        QString car_model;

        QString base_directory = QDir::homePath() + "/FastECU_OEM";
        QString calibration_files_base_directory = base_directory + "/calibrations";
        QString config_base_directory = base_directory + "/config";
        QString definition_files_base_directory = base_directory + "/definitions";
        QString kernel_files_base_directory = base_directory + "/kernels";
        QString log_files_base_directory = base_directory + "/logs";

        QString fastecu_config_file = config_base_directory + "/fastecu.cfg";
        QString menu_config_file = config_base_directory + "/menu.cfg";
        QString logger_config_file = config_base_directory + "/logger.cfg";

        QStringList calibration_files;
        QString calibration_files_directory = "./calibrations";
        QStringList ecu_definition_files;
        QString logger_definition_file;
        QString definition_files_directory = "./definitions";
        QString kernel_files_directory = "./kernels";
        QString kernel_files;
        QString log_files_directory = "./logs";
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

        QStringList log_values_name_sorted;

        QStringList dashboard_log_value_id;
        QStringList lower_panel_log_value_id;

        // Switch values
        QStringList log_switch_protocol;
        QStringList log_switch_id;
        QStringList log_switch_name;
        QStringList log_switch_description;
        QStringList log_switch_address;
        QStringList log_switch_ecu_byte_index;
        QStringList log_switch_ecu_bit;
        QStringList log_switch_target;
        QStringList switch_state;

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
        QStringList MapData;

        QStringList XScaleTypeList;
        QStringList XScaleNameList;
        QStringList XScaleAddressList;
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
        QStringList XScaleData;

        QStringList YScaleTypeList;
        QStringList YScaleNameList;
        QStringList YScaleAddressList;
        QStringList YScaleUnitsList;
        QStringList YScaleFormatList;
        QStringList YScaleFineIncList;
        QStringList YScaleCoarseIncList;
        QStringList YScaleStorageTypeList;
        QStringList YScaleEndianList;
        QStringList YScaleLogParamList;
        QStringList YScaleFromByteList;
        QStringList YScaleToByteList;
        QStringList YScaleData;


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
    ConfigValuesStructure *checkConfigDir();

    /****************************
     * Open FastECU config file
     ***************************/
    ConfigValuesStructure *readConfigFile();

    /****************************
     * Open FastECU config file
     ***************************/
    ConfigValuesStructure *saveConfigFile();

    /************************
     * Read logger def file
     ***********************/
    LogValuesStructure *readLoggerDefinitionFile();

    /****************************
     * Read ECU base definition
     ***************************/
    EcuCalDefStructure *readEcuBaseDef(FileActions::EcuCalDefStructure *ecuCalDef);

    /***********************
     * Read ECU definition
     **********************/
    EcuCalDefStructure *readEcuDef(FileActions::EcuCalDefStructure *ecuCalDef, QString ecuId);

    /***********************************************
     * Open ECU ROM file, including possible
     * checksum calculations and value conversions
     **********************************************/
    EcuCalDefStructure *openRomFile(FileActions::EcuCalDefStructure *ecuCalDef, QString fileName);

    /***********************************************
     * Save ECU ROM file, including possible
     * checksum calculations and value conversions
     **********************************************/
    EcuCalDefStructure *saveRomFile(FileActions::EcuCalDefStructure *ecuCalDef, QString fileName);

    /***************************
     * Read software menu file
     * for menu creation
     **************************/
    QSignalMapper *readMenuFile(QMenuBar *menubar, QToolBar *toolBar);

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
