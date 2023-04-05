#ifndef BIUOPERATIONSSUBARU_H
#define BIUOPERATIONSSUBARU_H

#include <QApplication>
#include <QByteArray>
#include <QCoreApplication>
#include <QDebug>
#include <QMainWindow>
#include <QSerialPort>
#include <QTime>
#include <QTimer>
#include <QWidget>
#include <QDialog>

#include <serial_port_actions.h>
#include <biu_ops_subaru_switches.h>
#include <biu_ops_subaru_data.h>
#include <biu_ops_subaru_input1.h>
#include <biu_ops_subaru_input2.h>

QT_BEGIN_NAMESPACE
namespace Ui
{
    class BiuOperationsSubaruWindow;
}
QT_END_NAMESPACE

class BiuOperationsSubaru : public QDialog
{
    Q_OBJECT

public:
    explicit BiuOperationsSubaru(SerialPortActions *serial, QWidget *parent = nullptr);
    ~BiuOperationsSubaru();

private:

    enum BiuCommands {
        NO_COMMAND = 0x00,
        CONNECT = 0x81,
        DISCONNECT = 0x82,
        DTC_READ = 0x18,
        DTC_CLEAR = 0x14,
        INFO_REQUEST = 0x21,
        IN_OUT_SWITCHES = 0x50,
        LIGHTING_SWITCHES = 0x51,
        BIU_DATA = 0x40,
        CAN_DATA = 0x41,
        TIME_TEMP_READ = 0x52,
        OPTIONS_READ = 0x53,
        VDC_ABS_CONDITION = 0x60,
        DEST_TOUCH_STATUS = 0x61,
        FACTORY_STATUS = 0x54,
        TESTER_PRESENT = 0x3E,
        WRITE_DATA = 0x3B,
        TIME_TEMP_WRITE = 0x8A,
        OPTIONS_WRITE = 0x8C
    };

    enum ConnectionState {
        NOT_CONNECTED = 0,
        CONNECTED = 1,
    };

    QStringList biu_messages = {    "COMM: Connect", "81",
                                    "COMM: Disconnect", "82",
                                    "READ: DTCs", "18",
                                    "READ: Input/output switches", "21,50",
                                    "READ: Lighting switches", "21,51",
                                    "READ: BIU data", "21,40",
                                    "READ: CAN data", "21,41",
                                    "READ: Times & Temps", "21,52",
                                    "READ: Car options", "21,53",
                                    "READ: VDC/ABC Condition", "21,60",
                                    "READ: Destination / Touch SW", "21,61",
                                    "READ: BIU Status", "21,54",
                                    "SET:  Clear DTCs", "14,40,00",
                                    "SET:  Times & Temps", "3B,8A",
                                    "SET:  Car options", "3B,8C",
                                    "Custom", ""
                               };

    QStringList biu_dtc_list = {    "B0100", "BIU internal error",
                                    "B0101", "Battery Power supply error",
                                    "B0102", "Battery Power backup error",
                                    "B0103", "IGN power error",
                                    "B0104", "ACC power error",
                                    "B0105", "Key interlock circuit short",
                                    "B0106", "Shift Lock circuit short",
                                    "B0201", "HS CAN off",
                                    "B0202", "HS CAN off",
                                    "B0211", "HS CAN EGI error",
                                    "B0212", "HS CAN TCM error",
                                    "B0213", "HS CAN VDC/ABS error",
                                    "B0214", "",
                                    "B0216", "",
                                    "B0217", "HS CAN EPS data error",
                                    "B0218", "",
                                    "B0221", "HS CAN ECM no data",
                                    "B0222", "HS CAN TCM no data",
                                    "B0223", "HS CAN VDC/ABS no data",
                                    "B0224", "",
                                    "B0226", "",
                                    "B0227", "HS CAN EPS no data",
                                    "B0228", "",
                                    "B0300", "LS CAN error",
                                    "B0301", "LS CAN malfunction",
                                    "B0302", "LS CAN off",
                                    "B0303", "Wake up line abnormal",
                                    "B0311", "LS CAN meter error",
                                    "B0313", "",
                                    "B0314", "",
                                    "B0321", "LS CAN meter no data",
                                    "B0323", "",
                                    "B0327", "LS CAN gateway no data",
                                    "B0401", "M collation bad",
                                    "B0402", "Immobilizer Key Collation bad",
                                    "B0403", "E/G request bad",
                                    "B0404", "Smart registration bad",
                                    "B0500", "Keyless UART comm error",
                               };

    QStringList biu_switch_names = {"Key-lock warning SW                  ",
                                    "Stop Light Switch                    ",
                                    "Front fog lamp SW input              ",
                                    "Rear fog lamp SW input               ",
                                    "Lighting SW input                    ",
                                    "Door key-lock SW input               ",
                                    "Door unlock SW input                 ",
                                    "---- unused ----                     ",
                                    "Driver’s door SW input               ",
                                    "P-door SW input                      ",
                                    "Rear right door SW input             ",
                                    "Rear left door SW input              ",
                                    "R Gate SW input                      ",
                                    "Manual lock SW input                 ",
                                    "Manual unlock SW input               ",
                                    "Lock SW (guess)                      ",
                                    "Bright SW input                      ",
                                    "Shift Button SW input                ",
                                    "Economy Switch                       ",
                                    "Tiptronic Mode Switch                ",
                                    "TIP UPSW input                       ",
                                    "TIP DOWN SW input                    ",
                                    "P SW                                 ",
                                    "MT Reverse Switch                    ",
                                    "R wiper ON SW input                  ",
                                    "R wiper INT SW input                 ",
                                    "R washer SW input                    ",
                                    "Wiper deicer SW input                ",
                                    "Rear Defogger SW                     ",
                                    "Driver’s Seat SW input               ",
                                    "P seatbelt SW input                  ",
                                    "Fr wiper input                       ",
                                    "Parking brake SW                     ",
                                    "Registration SW                      ",
                                    "Identification SW input              ",
                                    "Driver’s seat lock status SW input   ",
                                    "Passenger’s seat lock status SW input",
                                    "R gate lock status SW input          ",
                                    "Smart wake-up input                  ",
                                    "---- unknown ----                    ",
                                    "Rr defogger output                   ",
                                    "---- unused ----                     ",
                                    "Lock actuat. LOCK output             ",
                                    "All seat UNLOCK output               ",
                                    "D-seat UNLOCK output                 ",
                                    "R gate/trunk UNLK output             ",
                                    "Double lock output                   ",
                                    "R wiper output                       ",
                                    "Shift Lock Solenoid                  ",
                                    "Key locking output                   ",
                                    "Wiper deicer output                  ",
                                    "Starter cutting output               ",
                                    "Hazard Output                        ",
                                    "Belt buzzer output                   ",
                                    "Horn Output                          ",
                                    "Siren Output                         ",
                                    "D-belt warning light O/P             ",
                                    "P-belt warning light O/P             ",
                                    "Illumination lamp O/P                ",
                                    "Room lamp output                     ",
                                    "Key illumi. lamp o/p                 ",
                                    "R fog lamp output                    ",
                                    "R fog lamp monitor                   ",
                                    "Immobilizer lamp output              ",
                                    "Keyless Operation 1                  ",
                                    "Keyless Operation 2                  ",
                                    "EK alarm output                      ",
                                    "TL alarm output                      ",
                                    "---- unused ----                     ",
                                    "---- unused ----                     ",
                                    "---- unused ----                     ",
                                    "---- unused ----                     ",
                                    "CC Main Lamp                         ",
                                    "CC Set Lamp                          ",
                                    "SPORT Lamp                           ",
                                    "SPORT Blink                          ",
                                    "ATF Temperature Lamp                 ",
                                    "ATF Blink                            ",
                                    "ECO Lamp (AT)                        ",
                                    "ECO Lamp (MT)                        ",
                                    "Tire diameter 1                      ",
                                    "Tire diameter 2                      ",
                                    "Shift Up indication                  ",
                                    "Shift Down indication                ",
                                    "Sport Shift (buzzer 1)               ",
                                    "Sport Shift (buzzer 2)               ",
                                    "ABS/VDC Judging (ON = VDC)           ",
                                    "ADA Existence Judging                ",
                                    "Small Light SW                       ",
                                    "Headlamp                             ",
                                    "Headlight HI ON/OFF                  ",
                                    "Turn signal LH ON/OFF                ",
                                    "Turn signal RH ON/OFF                ",
                                    "RR Defogger Switch                   ",
                                    "Australia Judging Flag               ",
                                    "Large Diameter Tire                  ",
                                    "Number of Cylinders (ON = 6CYL)      ",
                                    "Camshaft Type (ON = DOHC)            ",
                                    "Turbo (ON = NA)                      ",
                                    "E/G displacement (2.5L)              ",
                                    "E/G displacement (3.0L)              ",
                                    "AT Vehicle Signal ID (ON = MT)       ",
                                    "E/G Cooling Fan                      ",
                                    "Heater Cock V/V Output               ",
                                    "Power Window (Up)                    ",
                                    "Power Window (Down)                  ",
                                    "Keyless Buzzer                       ",
                                    "Bright Request                       ",
                                    "P/W ECM Failure                      ",
                                    "Keyless Hook SW                      ",
                                    "---- unknown ----                    ",
                                    "---- unused ----                     ",
                                    "Door lock SW (open)                  ",
                                    "Door lock SW (closed)                ",
                                    "Door Key SW (open)                   ",
                                    "Door Key SW (closed)                 ",
                                    "Under hook registration              ",
                                    "Hook registration end                ",
                                    "Unlock request                       ",
                                    "---- unknown ----                    ",
                                    "Centre Display Failure (ON = NG)     ",
                                    "Navi Failure (ON = NG)               ",
                                    "IE Bus Failure (ON = NG)             ",
                                    "Auto A/C Failure (ON = NG)           ",
                                    "EBD Warning Light                    ",
                                    "ABS Warning Light                    ",
                                    "VDC Off Flag                         ",
                                    "VDC/ABS OK                           ",
                                };

    QStringList biu_lightsw_names = {"Lighting I sw input    ",
                                     "Lighting II sw input   ",
                                     "---- unused ----       ",
                                     "Dimmer hi sw input     ",
                                     "---- unused ----       ",
                                     "Dimmer pass sw input   ",
                                     "---- unused ----       ",
                                     "---- unused ----       ",
                                     "Lighting I lamp output ",
                                     "Lighting II lamp output",
                                     "Lighting hi lamp output",
                                     "Front fog lamp output  ",
                                     "DRL cancel output      ",
                                     "Power supply transistor",
                                     "Foot lamp output       ",
                                     "---- unused ----       ",
                                     "---- unused ----       ",
                                     "---- unused ----       ",
                                     "---- unused ----       ",
                                     "---- unused ----       ",
                                     "---- unused ----       ",
                                     "---- unused ----       ",
                                     "---- unused ----       ",
                                     "Economy switch         ",
                                    };

    QStringList biu_data_names = {  "Battery voltage (control) ", "volts",
                                    "Battery voltage (backup)  ", "volts",
                                    "Ignition system voltage   ", "volts",
                                    "Accessory voltage         ", "volts",
                                    "Illumination VR           ", "volts",
                                    "Illumination d-ratio      ", "%    ",
                                    "Ambient temp sensor       ", "volts",
                                    "Ambient temp              ", "degC ",
                                    "Fuel level                ", "volts",
                                    "Fuel level resistance     ", "ohms ",
                                    "Key lock solenoid         ", "volts",
                                    "Number of keys registered ", "keys ",
                                };

    float biu_data_factors[24] ={   0.0843, 0,
                                    0.0843, 0,
                                    0.0843, 0,
                                    0.0843, 0,
                                    0.0196, 0,
                                    0.4,    0,
                                    0.0196, 0,
                                    0.5,    -40,
                                    0.0392, 0,
                                    0.4,    0,
                                    0.0843, 0,
                                    1,      0
                                };

    QStringList can_data_names = {  "Front wheel speed     ", "km/hr",
                                    "VDC/ABS latest f-code ", "     ",
                                    "Blower fan steps      ", "steps",
                                    "Fuel level resistance ", "ohms ",
                                    "Fuel consumption      ", "cc/s ",
                                    "Engine coolant temp   ", "degC ",
                                    "Longitudinal g-force  ", "m/s^2",
                                    "Sport shift stages    ", "step ",
                                    "Shift position        ", "     ",
                                };

    float can_data_factors[18] = {  0.0562, 0,
                                    1,      0,
                                    1,      0,
                                    0.0016, 0,
                                    0.001,  0,
                                    1,      -40,
                                    0.1235, 0,
                                    1,      0,
                                    1,      0
                                };

    QStringList biu_tt_names = { "Room lamp off delay time ", "     ",
                                 "Auto-lock time           ", "secs ",
                                 "Outside temp offset      ", "degC ",
                               };

    QStringList biu_option_names = { "Rear defogger op mode       ", "NORMAL", "CONTINUOUS",
                                     "Wiper deicer op mode        ", "NORMAL", "CONTINUOUS",
                                     "Security alarm setup        ", "ON", "OFF",
                                     "Impact sensor setup         ", "ON", "OFF",
                                     "Alarm monitor delay setting ", "30s DELAY", "0s DELAY",
                                     "Lockout prevention          ", "ON", "OFF",
                                     "Impact sensor               ", "YES", "NO",
                                     "Siren installed             ", "YES", "NO",
                                     "Answer back buzzer setup    ", "ON", "OFF",
                                     "Hazard answer back setup    ", "ON", "OFF",
                                     "Automatic locking setup     ", "ON", "OFF",
                                     "Ans-back buzzer             ", "YES", "NO",
                                     "Auto locking                ", "YES", "NO",
                                     "Initial keyless setting     ", "EXEC", "-",
                                     "Initial button setting      ", "EXEC", "-",
                                     "Initial security setting    ", "EXEC", "-",
                                     "Select unlock switch        ", "ON", "OFF",
                                     "Passive alarm               ", "ON", "OFF",
                                     "Door open warning           ", "ON", "OFF",
                                     "Dome light alarm setting    ", "ON", "OFF",
                                     "Map light setting           ", "ON", "OFF",
                                     "Belt warning switch         ", "ON", "OFF",
                                     "---- unused ----            ", "-", "-",
                                     "Keyless P/W switch          ", "ON", "OFF",
                                     "Illumination control        ", "ON", "OFF",
                                     "A/C ECM setting             ", "YES", "NO",
                                     "P/W ECM setting             ", "YES", "NO",
                                     "Center display failure      ", "YES", "NO",
                                     "Wiper deicer                ", "YES", "NO",
                                     "Rear fog light setting      ", "YES", "NO",
                                     "UK security setup           ", "ON", "OFF",
                                     "MT/AT                       ", "AT", "MT",
                                     "Sedan/wagon setting         ", "SEDAN", "WAGON",
                                     "Double lock                 ", "ON", "OFF",
                                     "6MT setting                 ", "6MT", "NOT 6MT",
                                     "Option code b0              ", "1", "0",
                                     "Option code b1              ", "1", "0",
                                     "Option code b2              ", "1", "0",
                                     "Option code b3              ", "1", "0",
                                     "EK model                    ", "ON", "OFF",
                                    };

    QTimer *keep_alive_timer;

    uint8_t calculate_checksum(QByteArray output, bool dec_0x100);
    void parse_biu_message(QByteArray message);
    QString parse_message_to_hex(QByteArray received);
    int send_log_window_message(QString message, bool timestamp, bool linefeed);
    void delay(int timeout);
    BiuOpsSubaruSwitches* update_biu_ops_subaru_switches_window(BiuOpsSubaruSwitches *biuOpsSubaruSwitches);
    BiuOpsSubaruData* update_biu_ops_subaru_data_window(BiuOpsSubaruData *biuOpsSubaruData);
    void close_results_windows();
    void closeEvent(QCloseEvent *event);

    SerialPortActions *serial;
    QByteArray *biu_tt_result;
    QByteArray *biu_option_result;
    QStringList *switch_result;
    QStringList *data_result;
    BiuOpsSubaruSwitches *biuOpsSubaruSwitchesIo;
    BiuOpsSubaruSwitches *biuOpsSubaruSwitchesLighting;
    BiuOpsSubaruSwitches *biuOpsSubaruSwitchesOptions;

    BiuOpsSubaruData *biuOpsSubaruDataDtcs;
    BiuOpsSubaruData *biuOpsSubaruDataBiu;
    BiuOpsSubaruData *biuOpsSubaruDataCan;
    BiuOpsSubaruData *biuOpsSubaruDataTt;
    BiuOpsSubaruData *biuOpsSubaruDataVdcabs;
    BiuOpsSubaruData *biuOpsSubaruDataDest;
    BiuOpsSubaruData *biuOpsSubaruDataFactory;

    BiuOpsSubaruInput1 *biuOpsSubaruInput1;
    BiuOpsSubaruInput2 *biuOpsSubaruInput2;

    QByteArray cmd, output;
    int counter;
    uint8_t current_command;
    ConnectionState connection_state;

    Ui::BiuOperationsSubaruWindow *ui;

private slots:
    void keep_alive();
    void parse_biu_cmd();
    void prepare_biu_set_cmd(QByteArray cmd_settings);
    void prepare_biu_msg();
    void send_biu_msg();


signals:

};

#endif // BIUOPERATIONSSUBARU_H
