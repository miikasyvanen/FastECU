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

#include <calibration_maps.h>
#include <calibration_treewidget.h>
#include <ecu_operations_nissan.h>
#include <ecu_operations_subaru.h>
#include <ecu_operations_manual.h>
#include <file_actions.h>
#include <logbox.h>
#include <logvalues.h>
#include <preferences.h>
#include <serial_port_actions.h>


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
    bool loggingState = false;
    bool ecu_init_started = false;
    bool ecu_init_complete = false;

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

    QStringList flash_methods = {
        "wrx02",
        "wrx04",
        "ftx02",
        "sti04",
        "sti05",
        "subarucan",
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

    QString ecuid;

    QTimer *ssm_init_poll_timer;
    uint16_t ssm_init_poll_timer_timeout = 250;

    QTimer *logging_poll_timer;
    uint16_t logging_poll_timer_timeout = 150;
    uint16_t logging_counter = 0;
    bool logging_request_active = false;

    LogBox *logBoxes;

    QTreeWidget treeWidget;
    CalibrationTreeWidget *calibrationTreeWidget = new CalibrationTreeWidget();

    QLabel *statusBarLabel = new QLabel("");

    QMenu *mainWindowMenu;

    QComboBox *serial_port_list;
    QComboBox *flash_method_list;
    QComboBox *car_model_list;

    Ui::MainWindow *ui;

    // fileactions.c
    void open_calibration_file();
    void save_calibration_file();
    void save_calibration_file_as();
    QStringList parse_stringlist_from_expression_string(QString expression, QString x);
    float calculate_value_from_expression(QStringList expression);

    // mainwindow.c
    void ssm_kline_init();
    void ssm_can_init();
    QString parse_log_params(QByteArray received);
    QByteArray add_ssm_header(QByteArray output, bool dec_0x100);
    uint8_t calculate_checksum(QByteArray output, bool dec_0x100);
    QStringList create_car_models_list();
    QStringList create_flash_methods_list();
    QString check_kernel(QString flash_method);

    // menuactions.c
    void inc_dec_value(QString action);
    void set_value();
    void interpolate_value(QString action);
    void copy_value();
    void paste_value();
    void ecu_definition_manager();
    void logger_definition_manager();
    void toggle_realtime();
    void toggle_log_to_file();
    void set_maptablewidget_items();
    int get_mapvalue_decimal_count(QString valueFormat);
    int get_map_cell_colors(FileActions::EcuCalDefStructure *ecuCalDef, float mapDataValue, int mapIndex);

protected:

    void closeEvent(QCloseEvent *event);
    void resizeEvent( QResizeEvent * event);

private slots:
    // calibrationtreewidget.c
    void calibration_files_treewidget_item_selected(QTreeWidgetItem* item);
    void calibration_data_treewidget_item_selected(QTreeWidgetItem* item);
    void calibration_data_treewidget_item_expanded(QTreeWidgetItem* item);
    void calibration_data_treewidget_item_collapsed(QTreeWidgetItem* item);
    // menuactions.c
    void menu_action_triggered(QString action);
    // mainwindow.c
    bool ecu_init();
    void log_ssm_values();
    void open_serial_port();
    void start_ecu_operations(QString cmd_type);
    void start_manual_ecu_operations();
    void close_calibration();
    void close_calibration_map(QObject* obj);
    void change_gauge_values();
    void change_digital_values();
    void change_switch_values();
    void add_new_ecu_definition_file();
    void remove_ecu_definition_file();
    void add_new_logger_definition_file();
    void remove_logger_definition_file();
    QString parse_message_to_hex(QByteArray received);
    QString parse_ecuid(QByteArray received);
    void set_status_bar_label(bool serialConnectionState, bool ecuConnectionState, QString romId);
    // serialactions.c
    // void parse_received_data(QByteArray ReceivedDataToParse);
    void custom_menu_requested(QPoint pos);

    void selectable_combobox_item_changed(QString item);

    void close_app();

signals:
    void check_serial_port();
    void send_serial_data(QByteArray output);

private slots:
    void flash_method_changed();
    void check_serial_ports();
};
#endif // MAINWINDOW_H
