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
        CONNECT = 0x81,
        DISCONNECT = 0x82,
        DTC_READ = 0x18,
        DTC_CLEAR = 0x14,
        INFO_REQUEST = 0x21,
        IN_OUT_SWITCHES = 0x50,
        LIGHTING_SWITCHES = 0x51,
        BIU_DATA = 0x40,
        CAN_DATA = 0x41,
        BIU_CUSTOM_TIME_TEMP = 0x52,
        DEALER_COUNTRY_OPTIONS = 0x53,
    };

    QStringList biu_messages = {    "Connect", "81",
                                    "Disconnect", "82",
                                    "DTC read", "18",
                                    "DTC clear", "14,40,00",
                                    "Input/output switches", "21,50",
                                    "Lighting related switches", "21,51",
                                    "BIU data", "21,40",
                                    "CAN data", "21,41",
                                    "BIU Customisable Times & Temps", "21,52",
                                    "Dealer / country options", "21,53",
                                    "Custom", "",
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

    QStringList biu_switch_names = {"key-lock warning SW                  ",
                                    "Stop Light Switch                    ",
                                    "Front fog lamp SW input              ",
                                    "Rear fog lamp SW input               ",
                                    "lighting SW input                    ",
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
                                    "wiper deicer SW input                ",
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
                                    "lock actuat. LOCK output             ",
                                    "All seat UNLOCK output               ",
                                    "D-seat UNLOCK output                 ",
                                    "R gate/trunk UNLK output             ",
                                    "Double lock output                   ",
                                    "R wiper output                       ",
                                    "Shift Lock Solenoid                  ",
                                    "Key locking output                   ",
                                    "wiper deicer output                  ",
                                    "Starter cutting output               ",
                                    "Hazard Output                        ",
                                    "Belt buzzer output                   ",
                                    "Horn Output                          ",
                                    "Siren Output                         ",
                                    "D-belt warning light O/P             ",
                                    "P-belt warning light O/P             ",
                                    "Illumination lamp O/P                ",
                                    "Room lamp output                     ",
                                    "key illumi. lamp o/p                 ",
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
                                    "ABS/VDC Judging                      ",
                                    "ADA Existence Judging                ",
                                    "Small Light SW                       ",
                                    "Headlamp                             ",
                                    "Headlight HI ON/OFF                  ",
                                    "Turn signal LH ON/OFF                ",
                                    "Turn signal RH ON/OFF                ",
                                    "RR Defogger Switch                   ",
                                    "Australia Judging Flag               ",
                                    "Large Diameter Tire                  ",
                                    "Number of Cylinders                  ",
                                    "Camshaft Type                        ",
                                    "Turbo                                ",
                                    "E/G displacement (2.5L)              ",
                                    "E/G displacement (3.0L)              ",
                                    "AT Vehicle Signal ID                 ",
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
                                    "under hook registration              ",
                                    "hook registration end                ",
                                    "unlock request                       ",
                                    "---- unknown ----                    ",
                                    "Centre Display Failure               ",
                                    "Navi Failure                         ",
                                    "IE Bus Failure                       ",
                                    "Auto A/C Failure                     ",
                                    "EBD Warning Light                    ",
                                    "ABS Warning Light                    ",
                                    "VDC Off Flag                         ",
                                    "VDC/ABS OK                           ",
                                };

    QStringList biu_lightsw_names = {"lighting I sw input    ",
                                     "lighting II sw input   ",
                                     "---- unused ----       ",
                                     "dimmer hi sw input     ",
                                     "---- unused ----       ",
                                     "dimmer pass sw input   ",
                                     "---- unused ----       ",
                                     "---- unused ----       ",
                                     "lighting I lamp output ",
                                     "lighting II lamp output",
                                     "lighting hi lamp output",
                                     "front fog lamp output  ",
                                     "DRL cancel output      ",
                                     "power supply transistor",
                                     "foot lamp output       ",
                                     "---- unused ----       ",
                                     "---- unused ----       ",
                                     "---- unused ----       ",
                                     "---- unused ----       ",
                                     "---- unused ----       ",
                                     "---- unused ----       ",
                                     "---- unused ----       ",
                                     "---- unused ----       ",
                                     "economy switch         ",
                                    };

    QStringList biu_data_names = {  "battery voltage (control) ", "volts",
                                    "battery voltage (backup)  ", "volts",
                                    "ignition system voltage   ", "volts",
                                    "accessory voltage         ", "volts",
                                    "illumination VR           ", "volts",
                                    "illumination d-ratio      ", "%    ",
                                    "ambient temp sensor       ", "volts",
                                    "ambient temp              ", "degC ",
                                    "fuel level                ", "volts",
                                    "fuel level resistance     ", "ohms ",
                                    "key lock solenoid         ", "volts",
                                    "number of keys registered ", "keys ",
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

    QStringList can_data_names = {  "front wheel speed     ", "km/hr",
                                    "VDC/ABS latest f-code ", "     ",
                                    "blower fan steps      ", "steps",
                                    "fuel level resistance ", "ohms ",
                                    "fuel consumption      ", "cc/s ",
                                    "engine coolant temp   ", "degC ",
                                    "longitudinal g-force  ", "m/s^2",
                                    "sport shift stages    ", "step ",
                                    "shift position        ", "     ",
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

    QStringList biu_tt_names = { "room lamp off delay time ", "     ",
                                 "auto-lock time           ", "secs ",
                                 "outside temp offset      ", "degC ",
                               };

    uint8_t calculate_checksum(QByteArray output, bool dec_0x100);
    void parse_biu_message(QByteArray message);
    QString parse_message_to_hex(QByteArray received);
    int send_log_window_message(QString message, bool timestamp, bool linefeed);
    void delay(int timeout);

    SerialPortActions *serial;
    Ui::BiuOperationsSubaruWindow *ui;

private slots:
    void send_biu_msg();

signals:

};

#endif // BIUOPERATIONSSUBARU_H
