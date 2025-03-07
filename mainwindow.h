#ifndef MAINWINDOW_H
#define MAINWINDOW_H

//Exit application with this code to restart it instead of quitting:
//qApp->exit(RESTART_CODE)
#define RESTART_CODE 1000

#include <QMainWindow>
#include <QDebug>
#include <QTreeWidget>
#include <QLabel>
#include <QGroupBox>
#include <QHeaderView>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QRect>
#include <QTimer>
#include <QTableWidget>
#include <QSignalMapper>
#include <QInputDialog>
#include <QLineEdit>
#include <QClipboard>
#include <QTime>
#include <QProgressDialog>
#include <QListWidget>
#include <QCheckBox>
#include <QStandardItemModel>
#include <QStackedWidget>
#include <QSplashScreen>
#include <QAtomicInteger>

#include <QFuture>
#include <QDebug>
#include <QThread>
#include <QMutex>

#include <cipher.h>
#include <calibration_maps.h>
#include <calibration_treewidget.h>
#include <protocol_select.h>
#include <vehicle_select.h>
#include <definition_file_convert.h>
#include <modules/biu/biu_operations_subaru.h>
#include <dataterminal.h>
#include <get_key_operations_subaru.h>
#include <file_actions.h>
#include <logbox.h>
#include <settings.h>
#include <dtc_operations.h>

// Flash modules
// BDM
#include <modules/bdm/flash_ecu_subaru_denso_mc68hc16y5_02_bdm.h>

// Bootmode
#include <modules/bootmode/flash_ecu_subaru_unisia_jecs_m32r_bootmode.h>

// OBD
#include <modules/eeprom/eeprom_ecu_subaru_denso_sh705x_kline.h>
#include <modules/eeprom/eeprom_ecu_subaru_denso_sh705x_can.h>

#include <modules/ecu/flash_ecu_subaru_denso_mc68hc16y5_02.h>
#include <modules/ecu/flash_ecu_subaru_denso_sh7055_02.h>
#include <modules/ecu/flash_ecu_subaru_denso_sh705x_densocan.h>
#include <modules/ecu/flash_ecu_subaru_denso_sh705x_kline.h>
#include <modules/ecu/flash_ecu_subaru_denso_sh7058_can.h>
#include <modules/ecu/flash_ecu_subaru_denso_sh7058_can_diesel.h>
#include <modules/ecu/flash_ecu_subaru_denso_sh72543_can_diesel.h>
#include <modules/ecu/flash_ecu_subaru_unisia_jecs.h>
#include <modules/ecu/flash_ecu_subaru_unisia_jecs_m32r.h>
#include <modules/ecu/flash_ecu_subaru_hitachi_m32r_kline.h>
#include <modules/ecu/flash_ecu_subaru_hitachi_m32r_can.h>
#include <modules/ecu/flash_ecu_subaru_mitsu_m32r_kline.h>
#include <modules/ecu/flash_ecu_subaru_hitachi_sh7058_can.h>
#include <modules/ecu/flash_ecu_subaru_hitachi_sh72543r_can.h>
#include <modules/ecu/flash_ecu_subaru_denso_sh72531_can.h>
#include <modules/ecu/flash_ecu_subaru_denso_1n83m_4m_can.h>
#include <modules/ecu/flash_ecu_subaru_denso_1n83m_1_5m_can.h>

#include <modules/tcu/flash_tcu_subaru_hitachi_m32r_kline.h>
#include <modules/tcu/flash_tcu_subaru_hitachi_m32r_can.h>
#include <modules/tcu/flash_tcu_cvt_subaru_hitachi_m32r_can.h>
#include <modules/tcu/flash_tcu_subaru_denso_sh705x_can.h>
#include <modules/tcu/flash_tcu_cvt_subaru_mitsu_mh8104_can.h>
#include <modules/tcu/flash_tcu_cvt_subaru_mitsu_mh8111_can.h>

// JTAG
#include <modules/jtag/flash_ecu_subaru_hitachi_m32r_jtag.h>

#include <systemlogger.h>

#include <remote_utility/remote_utility.h>

//Forward declaration
class SerialPortActions;

extern void log_error(const QString &message, bool timestamp, bool linefeed);
extern void log_warning(const QString &message, bool timestamp, bool linefeed);
extern void log_info(const QString &message, bool timestamp, bool linefeed);
extern void log_debug(const QString &message, bool timestamp, bool linefeed);

QT_BEGIN_NAMESPACE
namespace Ui
{
    class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QString peerAddress = "", QString peerPassword = "", QWidget *parent = nullptr);
    ~MainWindow();

    void delay(int n);

private:
    enum {
        _LOG_E = 0,   // error
        _LOG_W,   // warning
        _LOG_I,   // info
        _LOG_D,   // debug
    };

#ifndef ARRAYSIZE
#define ARRAYSIZE(A) (sizeof(A)/sizeof((A)[0]))
#endif

    QString software_name;
    QString software_title;
    QString software_version;

    QSplashScreen *startUpSplash;
    QLabel *startUpSplashLabel;
    QProgressBar *startUpSplashProgressBar;
    QMutex restartQuestionActive;

    QString peerAddress;
    QString peerPassword;
    QSplashScreen *netSplash;
    RemoteUtility *remote_utility;
    static const QColor RED_LIGHT_OFF;
    static const QColor RED_LIGHT_ON;
    static const QColor YELLOW_LIGHT_OFF;
    static const QColor YELLOW_LIGHT_ON;
    static const QColor GREEN_LIGHT_OFF;
    static const QColor GREEN_LIGHT_ON;

    bool logging_state = false;
    bool log_params_request_started = false;
    bool ecu_init_started = false;
    bool ecu_init_complete = false;
    bool haltech_ic7_display_on = false;
    bool simulate_obd_on = false;
    bool can_listener_on = false;

    int ecuCalDefIndex = 0;

    uint16_t receive_timeout = 500;
    uint16_t serial_read_timeout = 2000;
    uint16_t serial_read_extra_short_timeout = 50;
    uint16_t serial_read_short_timeout = 200;
    uint16_t serial_read_medium_timeout = 500;
    uint16_t serial_read_long_timeout = 800;
    uint16_t serial_read_extra_long_timeout = 3000;

    int mapCellWidthSelectable = 240;
    int mapCellWidth1D = 96;
    int mapCellWidth = 54;
    int mapCellHeight = 26;
    int cellFontSize = mapCellHeight / 2.25;

    int xSize = 0;
    int ySize = 0;

    int connectionTimeOutDelay = 5;
    int connectionTimeOutDelayCount = 50;

    FileActions *fileActions;
    FileActions::LogValuesStructure *logValues;
    FileActions::ConfigValuesStructure *configValues;
    FileActions::EcuCalDefStructure *ecuCalDef[100];
    //FileActions::EcuCalDefStructure *ecuCalDefTemp;

    SerialPortActions *serial;
    //QTimer *serial_poll_timer;
    uint16_t serial_poll_timer_timeout = 500;
    QString serial_port_baudrate = "4800";
    QString default_serial_port_baudrate = "4800";
    QString serial_port_linux = "/dev/ttyUSB0";
    QString serial_port_windows = "COM67";
    QString serial_port;
    QString previous_serial_port;
    QString serial_port_prefix;
    QStringList serial_ports;

    int ecu_protocols_list_length = 6;
    QString current_car_model ="";

    //QStringList flash_methods;
    QStringList flash_transports;
    QStringList log_transports;

//        "Mercedes",     "CR3 EDC16C31",     "K-Line",           "K-Line",           "iso14230", "Mercedes Benz 320CDI",

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

    QString ecuid = "";
    QString protocol = "";
    QString log_protocol = "";

    QTimer *vbatt_timer;
    uint16_t vbatt_timer_timeout = 1000;
    uint16_t vbatt_timer_comms_timeout = 5000;

    //QTimer *ssm_init_poll_timer;
    uint16_t ssm_init_poll_timer_timeout = 250;

    QTimer *logging_poll_timer;
    uint16_t logging_poll_timer_timeout = 150;
    uint16_t logging_counter = 0;
    bool logging_request_active = false;

    QTimer *logparams_poll_timer;
    uint16_t logparams_poll_timer_timeout = 10;
    bool logparams_read_active = false;

    QElapsedTimer *log_speed_timer;

    LogBox *logBoxes;

    QRadioButton *ecu_radio_button;
    QRadioButton *tcu_radio_button;

    QTreeWidget treeWidget;
    CalibrationTreeWidget *calibrationTreeWidget = new CalibrationTreeWidget();

    QLabel *status_bar_connection_label = new QLabel("");
    QLabel *status_bar_ecu_label = new QLabel("");

    QMenu *mainWindowMenu;

    QPushButton *refresh_serial_port_list;
    QComboBox *serial_port_list;
    QComboBox *flash_transport_list;
    QComboBox *log_transport_list;

    QFile datalog_file;
    QFile syslog_file;
    QTextStream datalog_file_outstream;
    QTextStream syslog_file_outstream;
    bool write_datalog_to_file = false;
    bool write_syslog_to_file = false;
    bool datalog_file_open = false;
    bool syslog_file_open = false;
    QElapsedTimer *log_file_timer;

    QDialog *settings_dialog;
    QListWidget *contents_widget;
    QStackedWidget *pages_widget;

    QSize toolbar_item_size = QSize(24, 24);

    SystemLogger *syslogger;
    Ui::MainWindow *ui;

    bool eventFilter(QObject *target, QEvent *event);

    // fileactions.c
    bool open_calibration_file(QString filename);
    void save_calibration_file();
    void save_calibration_file_as();
    QStringList parse_stringlist_from_expression_string(QString expression, QString x);
    float calculate_value_from_expression(QStringList expression);

    // log_operations
    void kline_listener();
    void canbus_listener();
    void ssm_init();
    void ssm_kline_init();
    void ssm_can_init();
    QString parse_log_params(QByteArray received, QString protocol);
    void parse_log_value_list(QByteArray received, QString protocol);
    QByteArray add_ssm_header(QByteArray output, bool dec_0x100);
    uint8_t calculate_checksum(QByteArray output, bool dec_0x100);
    void log_to_file();

    // logvalues.c
    void change_log_values(int tabIndex, QString protocol);

    // mainwindow.c
    //Connect signals for any flash class and execute ::run() method
    template <typename FLASH_CLASS>
    FLASH_CLASS* connect_signals_and_run_module(FLASH_CLASS *object);
    QByteArray aes_ecb_test(QByteArray challenge, QByteArray key);
    void aes_ecb_example();
    void SetComboBoxItemEnabled(QComboBox * comboBox, int index, bool enabled);
    void set_flash_arrow_state();
    void update_protocol_info(int rom_number);
    QStringList create_flash_transports_list();
    QStringList create_log_transports_list();
    //QString check_kernel(QString flash_method);
    void setSplashScreenProgress(QString text, int incValue);
    QTextEdit* iterateWidgetChild(QObjectList children);
    bool write_syslog(QString msg);

    // menuactions.c
    void inc_dec_value(QString action);
    void set_value();
    void interpolate_value(QString action);
    void copy_value();
    void paste_value();
    int connect_to_ecu();
    void disconnect_from_ecu();
    void ecu_definition_manager();
    void logger_definition_manager();
    void winols_csv_to_romraider_xml();
    void set_realtime_state(bool state);
    void toggle_realtime();
    void toggle_log_to_file();
    void set_maptablewidget_items();
    QString get_rom_data_value(uint8_t map_rom_number, uint32_t map_data_address, uint16_t map_value_index, QString map_value_storagetype, QString map_value_endian);
    void set_rom_data_value(uint8_t map_rom_number, uint32_t map_data_address, uint16_t map_value_index, QString map_value_storagetype, QString map_value_endian, float map_value);
    int get_mapvalue_decimal_count(QString valueFormat);
    int get_map_cell_colors(FileActions::EcuCalDefStructure *ecuCalDef, float mapDataValue, int mapIndex);
    void show_preferences_window();

    void toggle_haltech_ic7_display();
    int test_haltech_ic7_display();
    void toggle_simulate_obd();
    void toggle_can_listener();
    int simulate_obd();
    void show_dtc_window();
    void show_subaru_biu_window();
    void show_terminal_window();
    void show_subaru_get_key_window();

    //#include <modules/flash_sti04.h>

protected:

    void closeEvent(QCloseEvent *event);
    bool event(QEvent *event);
    void resizeEvent( QResizeEvent * event);

private slots:
    // External logger slot for string messages
    void external_logger(QString message);
    // External progress bar slot
    void external_logger_set_progressbar_value(int value);

    // calibrationtreewidget.c
    void calibration_files_treewidget_item_selected(QTreeWidgetItem* item);
    void calibration_data_treewidget_item_selected(QTreeWidgetItem* item);
    void calibration_data_treewidget_item_expanded(QTreeWidgetItem* item);
    void calibration_data_treewidget_item_collapsed(QTreeWidgetItem* item);

    // log_operations.c
    bool ecu_init();
    void log_ssm_values();
    void read_log_serial_data();

    // menu_actions.c
    void menu_action_triggered(QString action);

    // mainwindow.c
    void select_protocol();
    void select_protocol_finished(int result);
    void select_vehicle();
    void select_vehicle_finished(int result);
    void log_transport_changed();
    void flash_transport_changed();
    void check_serial_ports();
    void open_serial_port();
    int can_listener();
    int start_ecu_operations(QString cmd_type);
    void close_calibration();
    void close_calibration_map(QObject* obj);
    void change_gauge_values();
    void change_digital_values();
    void change_switch_values();
    void update_logboxes(QString protocol);
    void update_logbox_values(QString protocol);
    void add_new_ecu_definition_file();
    void remove_ecu_definition_file();
    void add_new_logger_definition_file();
    void remove_logger_definition_file();
    QString parse_message_to_hex(QByteArray received);
    QString parse_ecuid(QByteArray received);
    void set_status_bar_label(bool serialConnectionState, bool ecuConnectionState, QString romId);
    void custom_menu_requested(QPoint pos);
    void selectable_combobox_item_changed(QString item);
    void checkbox_state_changed(int state);
    void close_app();
    // Logger
    //void logger(int log_level, QString message, bool timestamp, bool linefeed);
    //void logger(QString message, bool timestamp, bool linefeed);
    //void sendMsgToLogWindow(QWidget* parent, QString msg);
    void send_message_to_log_window(QString msg);
    void network_state_changed(QRemoteObjectReplica::State state, QRemoteObjectReplica::State oldState);

    // logvalues.c
    void change_log_gauge_value(int index);
    void change_log_digital_value(int index);
    void change_log_switch_value(int index);

    void update_vbatt();

signals:
    void check_serial_port();
    void send_serial_data(QByteArray output);
    void LOG_E(QString message, bool timestamp, bool linefeed);
    void LOG_W(QString message, bool timestamp, bool linefeed);
    void LOG_I(QString message, bool timestamp, bool linefeed);
    void LOG_D(QString message, bool timestamp, bool linefeed);
    //void syslog(int logType, bool write_syslog_to_file, QString message, bool timestamp, bool linefeed);
    void syslog(QString message, bool timestamp, bool linefeed);
    void enable_log_write_to_file(bool enable);
};
#endif // MAINWINDOW_H
