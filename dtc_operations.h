#ifndef DTC_OPERATIONS_H
#define DTC_OPERATIONS_H

#include <QApplication>
#include <QByteArray>
#include <QStandardItemModel>
#include <QCoreApplication>
#include <QDebug>
#include <QMainWindow>
#include <QSerialPort>
#include <QTime>
#include <QTimer>
#include <QWidget>

#include <kernelcomms.h>
#include <kernelmemorymodels.h>
#include <file_actions.h>
#include <serial_port_actions.h>
#include <ui_dtc_operations.h>

QT_BEGIN_NAMESPACE
namespace Ui
{
class DtcOperationsWindow;
}
QT_END_NAMESPACE

class DtcOperations : public QDialog
{
    Q_OBJECT

public:
    explicit DtcOperations(SerialPortActions *serial, QWidget *parent = nullptr);
    ~DtcOperations();

    void run();

signals:
    void external_logger(QString message);
    void external_logger(int value);
    void LOG_E(QString message, bool timestamp, bool linefeed);
    void LOG_W(QString message, bool timestamp, bool linefeed);
    void LOG_I(QString message, bool timestamp, bool linefeed);
    void LOG_D(QString message, bool timestamp, bool linefeed);

private:
    FileActions::EcuCalDefStructure *ecuCalDef;
    QString cmd_type;

#define STATUS_SUCCESS							0x00
#define STATUS_ERROR							0x01


    bool kill_process = false;
    bool five_baud_init_iso9141_ok = false;
    bool five_baud_init_iso14230_ok = false;
    bool fast_init_ok = false;
    bool can_init_ok = false;

    int result;

    uint8_t startbyte = 0;
    uint8_t tester_id = 0;
    uint8_t target_id = 0;
    uint16_t source_id = 0;
    uint16_t destination_id = 0;

    uint16_t receive_timeout = 500;
    uint16_t serial_read_timeout = 2000;
    uint16_t serial_read_extra_short_timeout = 50;
    uint16_t serial_read_short_timeout = 200;
    uint16_t serial_read_medium_timeout = 500;
    uint16_t serial_read_long_timeout = 800;
    uint16_t serial_read_extra_long_timeout = 3000;

    struct kline_timings
    {
        int _P1_MIN;
        int _P1_MAX;
    };

    const uint8_t fast_init_OBD = 0x81;                                      // Init ISO14230 fast init
    const uint8_t five_baud_init_OBD = 0x33;                                 // Init ISO9141 five baud init

    const uint8_t start_bytes[3] = { 0xC1, 0x33, 0xF1 };
    const uint8_t start_bytes_9141[3] = { 0x68, 0x6A, 0xF1 };

    const uint8_t live_data_start_bytes[3] = { 0xC2, 0x33, 0xF1 };
    const uint8_t live_data_start_bytes_9141[3] = { 0x68, 0x6A, 0xF1 };

    //-------------------------------------------------------------------------------------//
    // Service mode IDs (https://en.wikipedia.org/wiki/OBD-II_PIDs)
    //-------------------------------------------------------------------------------------//
    const uint8_t live_data = 0x01;                                     // Show current data
    const uint8_t freeze_frame = 0x02;                                  // Show freeze frame data
    const uint8_t read_stored_DTCs = 0x03;                                     // Show stored Diagnostic Trouble Codes
    const uint8_t clear_DTCs = 0x04;                                    // Clear Diagnostic Trouble Codes and stored values
    const uint8_t test_result_kline = 0x05;                             // Test results, oxygen sensor monitoring (non CAN only)
    const uint8_t test_result_can = 0x06;                               // Test results, other component/system monitoring (Test results, oxygen sensor monitoring for CAN only)
    const uint8_t read_pending_DTCs = 0x07;                             // Show pending Diagnostic Trouble Codes (detected during current or last driving cycle)
    const uint8_t control_operation = 0x08;                             // Control operation of on-board component/system
    const uint8_t vehicle_info = 0x09;                                  // Request vehicle information
    const uint8_t read_permanent_DTCs = 0x0A;                           // Permanent Diagnostic Trouble Codes (DTCs) (Cleared DTCs)

    //-------------------------------------------------------------------------------------//
    // PIDs (https://en.wikipedia.org/wiki/OBD-II_PIDs)
    //-------------------------------------------------------------------------------------//
    // SID 0x09 - Vehicle info
    const uint8_t request_support_info[7] = { 0x00, 0x20, 0x40, 0x60, 0x80, 0xA0, 0xC0 }; // Read support info
    const uint8_t request_VIN_length = 0x01;                            // Request VIN msg length
    const uint8_t request_VIN = 0x02;                                   // Read VIN
    const uint8_t request_CAL_ID_length = 0x03;                         // Read Calibration ID msg length
    const uint8_t request_CAL_ID = 0x04;                                // Read Calibration ID
    const uint8_t request_CVN_length = 0x05;                     // Read Calibration ID number msg length
    const uint8_t request_CVN = 0x06;                            // Read Calibration ID number

    // SID 0x01 - Live data
    const uint8_t SUPPORTED_PIDS_1_20              = 0x00;  // bit encoded
    const uint8_t MONITOR_STATUS_SINCE_DTC_CLEARED = 0x01;  // bit encoded          suported
    const uint8_t FREEZE_DTC                       = 0x02;  //                      suported
    const uint8_t FUEL_SYSTEM_STATUS               = 0x03;  // bit encoded          suported
    const uint8_t ENGINE_LOAD                      = 0x04;  // %
    const uint8_t ENGINE_COOLANT_TEMP              = 0x05;  // °C
    const uint8_t SHORT_TERM_FUEL_TRIM_BANK_1      = 0x06;  // %   suported
    const uint8_t LONG_TERM_FUEL_TRIM_BANK_1       = 0x07;  // %   suported
    const uint8_t SHORT_TERM_FUEL_TRIM_BANK_2      = 0x08;  // %
    const uint8_t LONG_TERM_FUEL_TRIM_BANK_2       = 0x09;  // %
    const uint8_t FUEL_PRESSURE                    = 0x0A;  // kPa
    const uint8_t INTAKE_MANIFOLD_ABS_PRESSURE     = 0x0B;  // kPa
    const uint8_t ENGINE_RPM                       = 0x0C;  // rpm
    const uint8_t VEHICLE_SPEED                    = 0x0D;  // km/h
    const uint8_t TIMING_ADVANCE                   = 0x0E;  // ° before TDC
    const uint8_t INTAKE_AIR_TEMP                  = 0x0F;  // °C
    const uint8_t MAF_FLOW_RATE                    = 0x10;  // g/s
    const uint8_t THROTTLE_POSITION                = 0x11;  // %
    const uint8_t COMMANDED_SECONDARY_AIR_STATUS   = 0x12;  // bit encoded
    const uint8_t OXYGEN_SENSORS_PRESENT_2_BANKS   = 0x13;  // bit encoded    suported
    const uint8_t OXYGEN_SENSOR_1_A                = 0x14;  // V %            suported
    const uint8_t OXYGEN_SENSOR_2_A                = 0x15;  // V %            suported
    const uint8_t OXYGEN_SENSOR_3_A                = 0x16;  // V %
    const uint8_t OXYGEN_SENSOR_4_A                = 0x17;  // V %
    const uint8_t OXYGEN_SENSOR_5_A                = 0x18;  // V %
    const uint8_t OXYGEN_SENSOR_6_A                = 0x19;  // V %
    const uint8_t OXYGEN_SENSOR_7_A                = 0x1A;  // V %
    const uint8_t OXYGEN_SENSOR_8_A                = 0x1B;  // V %
    const uint8_t OBD_STANDARDS                    = 0x1C;  // bit encoded  suported
    const uint8_t OXYGEN_SENSORS_PRESENT_4_BANKS   = 0x1D;  // bit encoded
    const uint8_t AUX_INPUT_STATUS                 = 0x1E;  // bit encoded
    const uint8_t RUN_TIME_SINCE_ENGINE_START      = 0x1F;  // sec

    const uint8_t SUPPORTED_PIDS_21_40             = 0x20;  // bit encoded
    const uint8_t DISTANCE_TRAVELED_WITH_MIL_ON    = 0x21;  // km           suported
    const uint8_t FUEL_RAIL_PRESSURE               = 0x22;  // kPa
    const uint8_t FUEL_RAIL_GUAGE_PRESSURE         = 0x23;  // kPa
    const uint8_t OXYGEN_SENSOR_1_B                = 0x24;  // ratio V
    const uint8_t OXYGEN_SENSOR_2_B                = 0x25;  // ratio V
    const uint8_t OXYGEN_SENSOR_3_B                = 0x26;  // ratio V
    const uint8_t OXYGEN_SENSOR_4_B                = 0x27;  // ratio V
    const uint8_t OXYGEN_SENSOR_5_B                = 0x28;  // ratio V
    const uint8_t OXYGEN_SENSOR_6_B                = 0x29;  // ratio V
    const uint8_t OXYGEN_SENSOR_7_B                = 0x2A;  // ratio V
    const uint8_t OXYGEN_SENSOR_8_B                = 0x2B;  // ratio V
    const uint8_t COMMANDED_EGR                    = 0x2C;  // %
    const uint8_t EGR_ERROR                        = 0x2D;  // %
    const uint8_t COMMANDED_EVAPORATIVE_PURGE      = 0x2E;  // %
    const uint8_t FUEL_TANK_LEVEL_INPUT            = 0x2F;  // %
    const uint8_t WARM_UPS_SINCE_CODES_CLEARED     = 0x30;  // count
    const uint8_t DIST_TRAV_SINCE_CODES_CLEARED    = 0x31;  // km
    const uint8_t EVAP_SYSTEM_VAPOR_PRESSURE       = 0x32;  // Pa
    const uint8_t ABS_BAROMETRIC_PRESSURE          = 0x33;  // kPa
    const uint8_t OXYGEN_SENSOR_1_C                = 0x34;  // ratio mA
    const uint8_t OXYGEN_SENSOR_2_C                = 0x35;  // ratio mA
    const uint8_t OXYGEN_SENSOR_3_C                = 0x36;  // ratio mA
    const uint8_t OXYGEN_SENSOR_4_C                = 0x37;  // ratio mA
    const uint8_t OXYGEN_SENSOR_5_C                = 0x38;  // ratio mA
    const uint8_t OXYGEN_SENSOR_6_C                = 0x39;  // ratio mA
    const uint8_t OXYGEN_SENSOR_7_C                = 0x3A;  // ratio mA
    const uint8_t OXYGEN_SENSOR_8_C                = 0x3B;  // ratio mA
    const uint8_t CATALYST_TEMP_BANK_1_SENSOR_1    = 0x3C;  // °C
    const uint8_t CATALYST_TEMP_BANK_2_SENSOR_1    = 0x3D;  // °C
    const uint8_t CATALYST_TEMP_BANK_1_SENSOR_2    = 0x3E;  // °C
    const uint8_t CATALYST_TEMP_BANK_2_SENSOR_2    = 0x3F;  // °C

    const uint8_t SUPPORTED_PIDS_41_60             = 0x40;  // bit encoded
    const uint8_t MONITOR_STATUS_THIS_DRIVE_CYCLE  = 0x41;  // bit encoded
    const uint8_t CONTROL_MODULE_VOLTAGE           = 0x42;  // V
    const uint8_t ABS_LOAD_VALUE                   = 0x43;  // %
    const uint8_t FUEL_AIR_COMMANDED_EQUIV_RATIO   = 0x44;  // ratio
    const uint8_t RELATIVE_THROTTLE_POSITION       = 0x45;  // %
    const uint8_t AMBIENT_AIR_TEMP                 = 0x46;  // °C
    const uint8_t ABS_THROTTLE_POSITION_B          = 0x47;  // %
    const uint8_t ABS_THROTTLE_POSITION_C          = 0x48;  // %
    const uint8_t ABS_THROTTLE_POSITION_D          = 0x49;  // %
    const uint8_t ABS_THROTTLE_POSITION_E          = 0x4A;  // %
    const uint8_t ABS_THROTTLE_POSITION_F          = 0x4B;  // %
    const uint8_t COMMANDED_THROTTLE_ACTUATOR      = 0x4C;  // %
    const uint8_t TIME_RUN_WITH_MIL_ON             = 0x4D;  // min
    const uint8_t TIME_SINCE_CODES_CLEARED         = 0x4E;  // min
    const uint8_t MAX_VALUES_EQUIV_V_I_PRESSURE    = 0x4F;  // ratio V mA kPa
    const uint8_t MAX_MAF_RATE                     = 0x50;  // g/s
    const uint8_t FUEL_TYPE                        = 0x51;  // ref table
    const uint8_t ETHANOL_FUEL_PERCENT             = 0x52;  // %
    const uint8_t ABS_EVAP_SYS_VAPOR_PRESSURE      = 0x53;  // kPa
    const uint8_t EVAP_SYS_VAPOR_PRESSURE          = 0x54;  // Pa
    const uint8_t SHORT_TERM_SEC_OXY_SENS_TRIM_1_3 = 0x55;  // %
    const uint8_t LONG_TERM_SEC_OXY_SENS_TRIM_1_3  = 0x56;  // %
    const uint8_t SHORT_TERM_SEC_OXY_SENS_TRIM_2_4 = 0x57;  // %
    const uint8_t LONG_TERM_SEC_OXY_SENS_TRIM_2_4  = 0x58;  // %
    const uint8_t FUEL_RAIL_ABS_PRESSURE           = 0x59;  // kPa
    const uint8_t RELATIVE_ACCELERATOR_PEDAL_POS   = 0x5A;  // %
    const uint8_t HYBRID_BATTERY_REMAINING_LIFE    = 0x5B;  // %
    const uint8_t ENGINE_OIL_TEMP                  = 0x5C;  // °C
    const uint8_t FUEL_INJECTION_TIMING            = 0x5D;  // °
    const uint8_t ENGINE_FUEL_RATE                 = 0x5E;  // L/h
    const uint8_t EMISSION_REQUIREMENTS            = 0x5F;  // bit encoded

    const uint8_t SUPPORTED_PIDS_61_80             = 0x60;  // bit encoded
    const uint8_t DEMANDED_ENGINE_PERCENT_TORQUE   = 0x61;  // %
    const uint8_t ACTUAL_ENGINE_TORQUE             = 0x62;  // %
    const uint8_t ENGINE_REFERENCE_TORQUE          = 0x63;  // Nm
    const uint8_t ENGINE_PERCENT_TORQUE_DATA       = 0x64;  // %
    const uint8_t AUX_INPUT_OUTPUT_SUPPORTED       = 0x65;  // bit encoded





    void closeEvent(QCloseEvent *event);
    int select_operation();
    int five_baud_init(QString protocol);
    int fast_init();
    int iso15765_init();

    QByteArray request_data(const uint8_t cmd, const uint8_t sub_cmd);
    void request_vehicle_info();
    QByteArray request_dtc_list(uint8_t cmd);

    uint8_t calculate_checksum(QByteArray output, bool dec_0x100);
    QString parse_message_to_hex(QByteArray received);
    void delay(int timeout);

    SerialPortActions *serial;
    Ui::DtcOperationsWindow *ui;

private slots:
    int read_dtc();
    int clear_dtc();
};

#endif // DTC_OPERATIONS_H
