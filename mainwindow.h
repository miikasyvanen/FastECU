#ifndef MAINWINDOW_H
#define MAINWINDOW_H

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

#include <calibration_maps.h>
#include <calibration_treewidget.h>
#include <protocol_select.h>
#include <definition_file_convert.h>
#include <biu_operations_subaru.h>
#include <ecu_operations_nissan.h>
#include <ecu_operations_mercedes.h>
#include <ecu_operations_subaru.h>
#include <ecu_operations_manual.h>
#include <file_actions.h>
#include <logbox.h>
#include <settings.h>
#include <serial_port_actions.h>

// Flash modules
#include <modules/flash_denso_wrx02.h>
#include <modules/flash_denso_fxt02.h>
#include <modules/flash_denso_can02.h>
#include <modules/flash_denso_sti04.h>
#include <modules/flash_denso_subarucan.h>
#include <modules/flash_denso_subarucan_diesel.h>
//

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
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void delay(int n);

private:
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

    int ecuCalDefIndex = 0;
    struct FileActions::EcuCalDefStructure *ecuCalDef[100];

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
    EcuOperationsSubaru *ecuOperationsSubaru;
    EcuOperationsMercedes *ecuOperationsRenault;

    FlashDensoWrx02 *flashDensoWrx02;
    FlashDensoFxt02 *flashDensoFxt02;
    FlashDensoCan02 *flashDensoCan02;
    FlashDensoSti04 *flashDensoSti04;
    FlashDensoSubaruCan *flashDensoSubaruCan;
    FlashDensoSubaruCanDiesel *flashDensoSubaruCanDiesel;

    SerialPortActions *serial;
    QTimer *serial_poll_timer;
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

    QString ecuid = "";
    QString protocol = "";
    QString log_protocol = "";

    QTimer *ssm_init_poll_timer;
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

    QTreeWidget treeWidget;
    CalibrationTreeWidget *calibrationTreeWidget = new CalibrationTreeWidget();

    QLabel *status_bar_connection_label = new QLabel("");
    QLabel *status_bar_ecu_label = new QLabel("");

    QMenu *mainWindowMenu;

    QComboBox *serial_port_list;
    QComboBox *flash_transport_list;
    QComboBox *log_transport_list;

    QFile log_file;
    QTextStream log_file_outstream;
    bool write_log_to_file = false;
    bool log_file_open = false;
    QElapsedTimer *log_file_timer;

    QDialog *settings_dialog;
    QListWidget *contents_widget;
    QStackedWidget *pages_widget;

    QSize toolbar_item_size = QSize(24, 24);

    Ui::MainWindow *ui;

    // fileactions.c
    bool open_calibration_file(QString filename);
    void save_calibration_file();
    void save_calibration_file_as();
    QStringList parse_stringlist_from_expression_string(QString expression, QString x);
    float calculate_value_from_expression(QStringList expression);

    // log_operations
    void kline_listener();
    void canbus_listener();
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
    void SetComboBoxItemEnabled(QComboBox * comboBox, int index, bool enabled);
    QStringList create_flash_transports_list();
    QStringList create_log_transports_list();
    QString check_kernel(QString flash_method);

    // menuactions.c
    void inc_dec_value(QString action);
    void set_value();
    void interpolate_value(QString action);
    void copy_value();
    void paste_value();
    void ecu_definition_manager();
    void logger_definition_manager();
    void winols_csv_to_romraider_xml();
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
    int simulate_obd();
    void show_subaru_biu_window();

    //#include <modules/flash_sti04.h>

protected:

    void closeEvent(QCloseEvent *event);
    bool event(QEvent *event);
    void resizeEvent( QResizeEvent * event);

private slots:
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
    void log_transport_changed();
    void flash_transport_changed();
    void check_serial_ports();
    void open_serial_port();
    int start_ecu_operations(QString cmd_type);
    int start_manual_ecu_operations();
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

    // logvalues.c
    void change_log_gauge_value(int index);
    void change_log_digital_value(int index);
    void change_log_switch_value(int index);

signals:
    void check_serial_port();
    void send_serial_data(QByteArray output);

};
#endif // MAINWINDOW_H
