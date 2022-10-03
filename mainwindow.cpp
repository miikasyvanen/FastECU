#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    qApp->installEventFilter(this);
    qRegisterMetaType<QVector<int> >("QVector<int>");

#ifdef Q_OS_LINUX
    //send_log_window_message("Running on Linux Desktop ", true, false);
    //serialPort = serialPortLinux;
    serial_port_prefix = "/dev/";
#endif

#ifdef Q_OS_WIN32
    serialPort = serialPortWindows;
    serialPortPrefix = "";
#endif

#if Q_PROCESSOR_WORDSIZE == 4
    qDebug() << "32-bit executable";
    //send_log_window_message("32-bit executable", false, true);
#elif Q_PROCESSOR_WORDSIZE == 8
    qDebug() << "64-bit executable";
    //send_log_window_message("64-bit executable", false, true);
#endif

    ui->calibrationFilesTreeWidget->setHeaderLabel("Calibration Files");
    ui->calibrationDataTreeWidget->setHeaderLabel("Calibration Data");

    fileActions = new FileActions();
    //FileActions::ConfigValuesStructure *configValues = &fileActions->ConfigValuesStruct;
    configValues = &fileActions->ConfigValuesStruct;

    fileActions->checkConfigDir();
    configValues = fileActions->readConfigFile();

    if (QDir(configValues->kernel_files_directory).exists()){
        QDir dir(configValues->kernel_files_directory);
        QStringList nameFilter("*.bin");
        QStringList txtFilesAndDirectories = dir.entryList(nameFilter);
        qDebug() << txtFilesAndDirectories;
    }

    QSignalMapper *mapper = fileActions->readMenuFile(ui->menubar, ui->toolBar);
    connect(mapper, SIGNAL(mapped   (QString)), this, SLOT(menu_action_triggered(QString)));

    for (int i = 0; i < configValues->calibration_files.count(); i++)
    {
        QString fileName = configValues->calibration_files.at(i);
        //qDebug() << "Open file" << fileName;
        ecuCalDef[ecuCalDefIndex] = new FileActions::EcuCalDefStructure;
        //ecuCalDef[ecuCalDefIndex]->FullRomData.clear();
        fileActions->openRomFile(ecuCalDef[ecuCalDefIndex], fileName);
        if(ecuCalDef[ecuCalDefIndex] != NULL)
        {
            calibrationTreeWidget->buildCalibrationFilesTree(ecuCalDefIndex, ui->calibrationFilesTreeWidget, ecuCalDef[ecuCalDefIndex]);
            calibrationTreeWidget->buildCalibrationDataTree(ui->calibrationDataTreeWidget, ecuCalDef[ecuCalDefIndex]);

            ecuCalDefIndex++;
        }
    }
    if(ecuCalDefIndex > 0)
    {
        //QTreeWidgetItem *currentItem = ui->calibrationFilesTreeWidget->topLevelItem(0);
        const QModelIndex index = ui->calibrationFilesTreeWidget->selectionModel()->currentIndex();
        //currentItem->setSelected(true);
        emit ui->calibrationFilesTreeWidget->clicked(index);
    }
    logValues = &fileActions->LogValuesStruct;
    logValues = fileActions->readLoggerDefinitionFile();
    //fileActions->saveLoggerDefinitionFile();

    logBoxes = new LogBox();

    int switchBoxCount = 20;
    int logBoxCount = 12;

    for (int i = 0; i < logValues->lower_panel_switch_id.count(); i++)
    {
        if (logValues->lower_panel_switch_id.at(i).toInt() > -1)
        {
            QGroupBox *switchBox = logBoxes->drawLogBoxes("switch", i, switchBoxCount, logValues->log_switch_name.at(i), logValues->log_switch_name.at(i), logValues->switch_state.at(i));
            switchBox->setAttribute(Qt::WA_TransparentForMouseEvents);
            ui->switchBoxLayout->addWidget(switchBox);
        }
    }
    for (int i = 0; i < logValues->lower_panel_log_value_id.count(); i++)
    {
        if (logValues->lower_panel_log_value_id.at(i).toInt() > -1)
        {
            QStringList value_unit = logValues->log_value_units.at(i).split(",");
            QGroupBox *logBox = logBoxes->drawLogBoxes("log", i, logBoxCount, logValues->log_value_name.at(i), value_unit.at(0), logValues->log_value.at(i));
            //QGroupBox *logBox = logBoxes->drawLogBoxes("log", i, logBoxCount, logValues->log_value_name.at(i), logValues->log_value_units.at(i), logValues->log_value.at(i));
            logBox->setAttribute(Qt::WA_TransparentForMouseEvents);
            ui->logBoxLayout->addWidget(logBox);
        }
    }

    connect(ui->switchBoxWidget, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(change_switch_values()));
    connect(ui->logBoxWidget, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(change_digital_values()));
    connect(ui->mdiArea, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(change_gauge_values()));
    //connect(ui->action_Preferences, SIGNAL(triggered()), this, SLOT(openPreferences()));
    connect(ui->calibrationFilesTreeWidget, SIGNAL(itemClicked(QTreeWidgetItem *,int)), this, SLOT(calibration_files_treewidget_item_selected(QTreeWidgetItem*)));
    connect(ui->calibrationDataTreeWidget, SIGNAL(itemClicked(QTreeWidgetItem *,int)), this, SLOT(calibration_data_treewidget_item_selected(QTreeWidgetItem*)));
    connect(ui->calibrationDataTreeWidget, SIGNAL(itemExpanded(QTreeWidgetItem*)), this, SLOT(calibration_data_treewidget_item_expanded(QTreeWidgetItem*)));
    connect(ui->calibrationDataTreeWidget, SIGNAL(itemCollapsed(QTreeWidgetItem*)), this, SLOT(calibration_data_treewidget_item_collapsed(QTreeWidgetItem*)));
    connect(calibrationTreeWidget, SIGNAL(closeRom()), this, SLOT(close_calibration()));

    //QLabel *statusBarLabel = new QLabel(statusBarString);
    statusBarLabel->setMargin(5);
    statusBarLabel->setStyleSheet("QLabel { background-color : red; color : white; }");
    set_status_bar_label(false, false, "");

    statusBar()->addWidget(statusBarLabel);
    statusBar()->setSizeGripEnabled(true);

    ui->calibrationDataTreeWidget->resizeColumnToContents(0);
    ui->calibrationDataTreeWidget->resizeColumnToContents(1);
    ui->calibrationFilesTreeWidget->setMinimumHeight(125);
    ui->calibrationFilesTreeWidget->minimumHeight();
    ui->calibrationFilesTreeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->calibrationFilesTreeWidget, SIGNAL(customContextMenuRequested(QPoint)), SLOT(custom_menu_requested(QPoint)));

    //ui->splitter->setStretchFactor(2, 1);
    ui->splitter->setSizes(QList<int>({125, INT_MAX}));

    serial = new SerialPortActions();

    QWidget* spacer = new QWidget();
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    ui->toolBar->addWidget(spacer);

    ui->toolBar->addSeparator();

    car_model_list = new QComboBox();
    car_model_list->setFixedWidth(120);
    car_model_list->setObjectName("car_model_list");
    QStringList car_models = create_car_models_list();
    int car_model_index = 0;
    foreach (QString car_model, car_models){
        car_model_list->addItem(car_model);
        if (configValues->car_model == car_model)
            car_model_list->setCurrentIndex(car_model_index);
        car_model_index++;
    }
    ui->toolBar->addWidget(car_model_list);

    flash_method_list = new QComboBox();
    flash_method_list->setFixedWidth(80);
    flash_method_list->setObjectName("flash_method_list");
    QStringList flash_methods = create_flash_methods_list();
    int flash_method_index = 0;
    foreach (QString flash_method, flash_methods){
        flash_method_list->addItem(flash_method);
        if (configValues->flash_method == flash_method)
            flash_method_list->setCurrentIndex(flash_method_index);
        flash_method_index++;
    }
    connect(flash_method_list, SIGNAL(currentIndexChanged(int)), this, SLOT(flash_method_changed()));
    ui->toolBar->addWidget(flash_method_list);

    ui->toolBar->addSeparator();

    QLabel *log_select = new QLabel("Log:  ");
    ui->toolBar->addWidget(log_select);

    QCheckBox *ecu_check_box = new QCheckBox("ECU");
    ecu_check_box->setCheckState(Qt::Checked);
    ui->toolBar->addWidget(ecu_check_box);
    QCheckBox *tcu_check_box = new QCheckBox("TCU");
    ui->toolBar->addWidget(tcu_check_box);

    ui->toolBar->addSeparator();

    QLabel *serial_port_select = new QLabel("Port:  ");
    ui->toolBar->addWidget(serial_port_select);

    serial_port_list = new QComboBox();
    serial_port_list->setFixedWidth(160);
    serial_port_list->setObjectName("serial_port_list");
    serial_ports = serial->check_serial_ports();
    for (int i = 0; i < serial_ports.length(); i += 2)
    {
        serial_port_list->addItem(serial_ports.at(i));
        if (configValues->serial_port == serial_ports.at(i))
            serial_port_list->setCurrentIndex(i);
    }
    ui->toolBar->addWidget(serial_port_list);

    QPushButton *refresh_serial_list = new QPushButton();
    //refresh_serial_list->setText("Refresh");
    refresh_serial_list->setIcon(QIcon(":/icons/help-browser.png"));
    //connect(refresh_serial_list, SIGNAL(clicked(bool)), this, SLOT(start_manual_ecu_operations()));
    connect(refresh_serial_list, SIGNAL(clicked(bool)), this, SLOT(check_serial_ports()));
    ui->toolBar->addWidget(refresh_serial_list);

    serial_port = serial_port_prefix + configValues->serial_port;

    serial_port_baudrate = default_serial_port_baudrate;
    serial->serial_port_baudrate = serial_port_baudrate;
    serial->serial_port = serial_port;

    serial_poll_timer = new QTimer(this);
    serial_poll_timer->setInterval(serial_poll_timer_timeout);
    connect(serial_poll_timer, SIGNAL(timeout()), this, SLOT(open_serial_port()));
    serial_poll_timer->start();

    ssm_init_poll_timer = new QTimer(this);
    ssm_init_poll_timer->setInterval(ssm_init_poll_timer_timeout);
    connect(ssm_init_poll_timer, SIGNAL(timeout()), this, SLOT(ecu_init()));
    ssm_init_poll_timer->start();

    logging_poll_timer = new QTimer(this);
    logging_poll_timer->setInterval(logging_poll_timer_timeout);
    connect(logging_poll_timer, SIGNAL(timeout()), this, SLOT(log_ssm_values()));
    //logging_poll_timer->start();

    //qDebug() << "Started";
}

MainWindow::~MainWindow()
{
    delete ui;
}

bool MainWindow::ecu_init()
{
    QString car_model;
    QComboBox *car_model_list = ui->toolBar->findChild<QComboBox*>("car_model_list");
    QComboBox *flash_method_list = ui->toolBar->findChild<QComboBox*>("flash_method_list");

    //qDebug() << "ECU init";

    if (serial->is_serial_port_open())
    {
        if (!ecu_init_complete)
        {
            if (car_model_list->currentText() == "Subaru")
            {
                if (flash_method_list->currentText() == "subarucan")
                    ssm_can_init();
                else
                    ssm_kline_init();
            }
        }
    }
    else
    {
        //qDebug() << "Connection is not ready!";
        ecu_init_complete = false;
        ecuid.clear();
    }
    //qDebug() << "ECU ID check complete";

    return ecu_init_complete;
}

void MainWindow::ssm_can_init()
{
    QByteArray output;
    QByteArray received;
    uint32_t start_address = 0xFFFF6000;

    output.clear();
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x07);
    output.append((uint8_t)0xE0);
    output.append((uint8_t)0xAA);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    serial->write_serial_data_echo_check(output);
    delay(200);
    received = serial->read_serial_data(100, 500);
    qDebug() << "ECU CAN init:" << parse_message_to_hex(received);
    qDebug() << "ECU ID:" << parse_ecuid(received);

    output.clear();
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x07);
    output.append((uint8_t)0xE0);
    output.append((uint8_t)0xFF);
    output.append((uint8_t)0x86);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    serial->write_serial_data_echo_check(output);
    delay(200);
    received = serial->read_serial_data(100, 500);
    qDebug() << "SSM CAN init:" << parse_message_to_hex(received);
    qDebug() << "ECU ID:" << parse_ecuid(received);

    for (int i = 0; i < 5; i++)
    {
        output.clear();
        output.append((uint8_t)0x00);
        output.append((uint8_t)0x00);
        output.append((uint8_t)0x07);
        output.append((uint8_t)0xE0);
        output.append((uint8_t)0x7A);
        output.append((uint8_t)(0x98 + 0x04));
        output.append((uint8_t)((start_address >> 24) & 0xFF));
        output.append((uint8_t)((start_address >> 16) & 0xFF));
        output.append((uint8_t)((start_address >> 8) & 0xFF));
        output.append((uint8_t)(start_address & 0xFF));
        output.append((uint8_t)0x00);
        output.append((uint8_t)0x00);
        serial->write_serial_data_echo_check(output);
        delay(200);
        received = serial->read_serial_data(100, 500);

        qDebug() << "SSM CAN init" + QString::number(i) + ":" << parse_message_to_hex(received);
        qDebug() << "ECU ID:" << parse_ecuid(received);

        start_address += 0x100;
    }
    /*
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x07);
    output.append((uint8_t)0xE0);
    output.append((uint8_t)0x10);
    output.append((uint8_t)0x03);

    serial->write_serial_data_echo_check(output);
    received = serial->read_serial_data(100, 500);

    qDebug() << "SSM CAN init:" << parse_message_to_hex(received);
    qDebug() << "ECU ID:" << parse_ecuid(received);
    */

}

void MainWindow::ssm_kline_init()
{
    QByteArray output;
    QByteArray received;

    qDebug() << "Check ECU INIT";
    if (!ecu_init_started)
    {
        ecu_init_started = true;
/*
        output.clear();
        output.append((uint8_t)0xB8);
        output.append((uint8_t)0xFF);
        output.append((uint8_t)0xDF);
        output.append((uint8_t)0xFC);
        output.append((uint8_t)0x5A);
        output.append((uint8_t)0xA5);
        output.append((uint8_t)0xA5);
        output.append((uint8_t)0x5A);
        serial->write_serial_data_echo_check(add_ssm_header(output, false));
        received = serial->read_serial_data(100, 500);
        qDebug() << "MEM:" << parse_message_to_hex(received);
    */
        qDebug() << "Read MEM with A0";

        output.clear();
        output.append((uint8_t)0xA0);
        output.append((uint8_t)0x00);
        output.append((uint8_t)0xFF);
        output.append((uint8_t)0xDF);
        output.append((uint8_t)0xFC);
        output.append((uint8_t)0x03);
        serial->write_serial_data_echo_check(add_ssm_header(output, false));
        delay(200);
        received = serial->read_serial_data(100, 500);
        qDebug() << "MEM:" << parse_message_to_hex(received);

        qDebug() << "Read ECU ID with A8";
        output.clear();
        output.append((uint8_t)0xA8);
        output.append((uint8_t)0x00);
        output.append((uint8_t)0x00);
        output.append((uint8_t)0x00);
        output.append((uint8_t)0x01);
        output.append((uint8_t)0x00);
        output.append((uint8_t)0x00);
        output.append((uint8_t)0x02);
        output.append((uint8_t)0x00);
        output.append((uint8_t)0x00);
        output.append((uint8_t)0x03);
        output.append((uint8_t)0x00);
        output.append((uint8_t)0x00);
        output.append((uint8_t)0x04);
        output.append((uint8_t)0x00);
        output.append((uint8_t)0x00);
        output.append((uint8_t)0x05);
        serial->write_serial_data_echo_check(add_ssm_header(output, false));
        delay(200);
        received = serial->read_serial_data(100, 500);
        qDebug() << "ECU ID:" << parse_message_to_hex(received);

        qDebug() << "SSM init with BF";
        output.clear();
        output.append((uint8_t)0xBF);
        serial->write_serial_data_echo_check(add_ssm_header(output, false));
        delay(200);
        received = serial->read_serial_data(100, 500);
        //qDebug() << "ECU ID:" << parse_ecuid(received);
        if (received.length() == 62)
        {
            ecu_init_complete = true;
            //set_status_bar_label(true, true, ecuid);
            ecuid = parse_ecuid(received);

            received = serial->read_serial_data(1, 100);
            while(received.length() > 0)
            {
                received = serial->read_serial_data(1, 100);
            }
        }
    }
    ecu_init_started = false;
}

void MainWindow::log_ssm_values()
{
    QByteArray output;
    QByteArray received;
    bool ok = false;

    if (ecu_init_complete)
    {
        output.append((uint8_t)0xA8);
        output.append((uint8_t)0x00);
        for (int i = 0; i < logValues->lower_panel_log_value_id.length(); i++)
        {
            if (logValues->lower_panel_log_value_id.at(i) != "-1")
            {
                //qDebug() << logValues->log_value_units.at(i);
                output.append((uint8_t)(logValues->log_value_address.at(i).toUInt(&ok,16) >> 16));
                output.append((uint8_t)(logValues->log_value_address.at(i).toUInt(&ok,16) >> 8));
                output.append((uint8_t)logValues->log_value_address.at(i).toUInt(&ok,16));
            }
        }
        //qDebug() << parse_message_to_hex(add_ssm_header(output, false));
        serial->write_serial_data_echo_check(add_ssm_header(output, false));
        received = serial->read_serial_data(100, 500);
        received.remove(0, 5);
        received.remove(received.length() - 1, 1);
        parse_log_params(received);
        //qDebug() << parse_log_params(received);
    }
}

QString MainWindow::parse_log_params(QByteArray received)
{
    QString params;

    if (!logging_request_active)
    {
        logging_request_active = true;
        qDebug() << "Log read count:" << logging_counter++;
        for (int i = 0; i < logValues->lower_panel_log_value_id.length(); i++)
        {
            if (logValues->lower_panel_log_value_id.at(i) != "-1")
            {
                QStringList conversion = logValues->log_value_units.at(i).split(",");
                QString value_name = logValues->log_value_name.at(i);
                QString unit = conversion.at(1);
                QString from_byte = conversion.at(2);
                QString format = conversion.at(3);
                //QString gauge_min = conversion.at(4);
                //QString gauge_max = conversion.at(5);
                //QString gauge_step = conversion.at(6);
                QString value = QString::number((uint8_t)received.at(i));
                QString calc_value = QString::number(fileActions->calculate_value_from_expression(fileActions->parse_stringlist_from_expression_string(from_byte, value)));
                qDebug() << value_name + ": " + calc_value + " " + unit + " from_byte: " + value + " via expr: " + from_byte;
                params.append(calc_value);
                params.append(", ");
            }
        }
        qDebug() << parse_message_to_hex(received);
        qDebug() << " ";
        logging_request_active = false;
    }

    return params;
}

QByteArray MainWindow::add_ssm_header(QByteArray output, bool dec_0x100)
{
    uint8_t length = output.length();

    output.insert(0, (uint8_t)0x80);
    output.insert(1, (uint8_t)0x10);
    output.insert(2, (uint8_t)0xF0);
    output.insert(3, length);
    output.append(calculate_checksum(output, dec_0x100));

    //qDebug() << "Generated SSM message:" << parseMessageToHex(output);

    return output;
}

uint8_t MainWindow::calculate_checksum(QByteArray output, bool dec_0x100)
{
    uint8_t checksum = 0;

    for (uint16_t i = 0; i < output.length(); i++)
    {
        checksum += (uint8_t)output.at(i);
    }
    if (dec_0x100)
        checksum = (uint8_t) (0x100 - checksum);

    return checksum;
}

QStringList MainWindow::create_car_models_list()
{
    QStringList car_models;
    car_models.append("Subaru");

    return car_models;
}

QStringList MainWindow::create_flash_methods_list()
{
/*
    QStringList flash_methods;
    flash_methods.append("wrx02");
    flash_methods.append("fxt02");
    flash_methods.append("sti04");
    flash_methods.append("sti05");
    flash_methods.append("subarucan");
*/
    return flash_methods;
}

QString MainWindow::check_kernel(QString flash_method)
{
    QString kernel;
    QString prefix = configValues->kernel_files_directory;

    if (flash_method == "wrx02")
        kernel = prefix + "ssmk_HC16.bin";
    if (flash_method == "fxt02")
        kernel = prefix + "ssmk_SH7055_02fxt_35.bin";
    if (flash_method == "fxt02can")
        kernel = prefix + "ssmk_SH7055_02fxt_35.bin";
    if (flash_method == "sti04")
        kernel = prefix + "ssmk_SH7055_02fxt_35.bin";
    if (flash_method == "sti04can")
        kernel = prefix + "ssmk_SH7055_02fxt_35.bin";
    if (flash_method == "sti05")
        kernel = prefix + "ssmk_SH7058.bin";
    if (flash_method == "sti05can")
        kernel = prefix + "ssmk_SH7058.bin";
    if (flash_method == "subarucan")
        kernel = prefix + "ssmk_SH7058.bin";

    return kernel;
}

void MainWindow::flash_method_changed()
{
    ssm_init_poll_timer->stop();
    QComboBox *flash_method_list = ui->toolBar->findChild<QComboBox*>("flash_method_list");

    if (flash_method_list->currentText() == "subarucan")
        serial->is_can_connection = true;
    else
        serial->is_can_connection = false;

    qDebug() << "CAN:" << serial->is_can_connection;

    serial->reset_connection();
    ecuid.clear();
    ecu_init_complete = false;
    ssm_init_poll_timer->start();

}

void MainWindow::check_serial_ports()
{
    QStringList serial_ports = serial->check_serial_ports();
    QComboBox *serial_port_list = ui->toolBar->findChild<QComboBox*>("serial_port_list");
    serial_port_list->clear();

    for (int i = 0; i < serial_ports.length(); i += 2)
    {
        serial_port_list->addItem(serial_ports.at(i));
        if (configValues->serial_port == serial_ports.at(i))
            serial_port_list->setCurrentIndex(i);
    }
}

void MainWindow::open_serial_port()
{
    QStringList serial_port;
    serial_port.append(serial_ports.at(serial_port_list->currentIndex() * 2));
    serial_port.append(serial_ports.at(serial_port_list->currentIndex() * 2 + 1));
    //qDebug() << serial_port;

    QString opened_serial_port = serial->open_serial_port(serial_port);
    if (opened_serial_port != NULL)
    {
        if (opened_serial_port != previous_serial_port)
        {
            ecuid.clear();
            ecu_init_complete = false;
        }
        //qDebug() << "Serial port" << opened_serial_port << "opened" << previous_serial_port;
        previous_serial_port = opened_serial_port;
        if (ecuid == "")
            set_status_bar_label(true, false, "");
        else
            set_status_bar_label(true, true, ecuid);
    }
    else
    {
        set_status_bar_label(false, false, "");
        ecu_init_complete = false;
    }
}

void MainWindow::start_ecu_operations(QString cmd_type)
{
    QTreeWidgetItem *selectedItem = ui->calibrationFilesTreeWidget->selectedItems().at(0);
    int romNumber = ui->calibrationFilesTreeWidget->indexOfTopLevelItem(selectedItem);

    if (cmd_type == "test_write" || cmd_type == "write")
    {
        ecuCalDef[romNumber]->Kernel = check_kernel(flash_method_list->currentText());
        qDebug() << ecuCalDef[romNumber]->Kernel;
        EcuOperationsSubaru *ecuOperationsSubaru = new EcuOperationsSubaru(serial, ecuCalDef[romNumber], cmd_type);

        delete ecuOperationsSubaru;
    }
    else
    {
        ecuCalDef[ecuCalDefIndex] = new FileActions::EcuCalDefStructure;
        while (ecuCalDef[ecuCalDefIndex]->RomInfo.length() < fileActions->RomInfoStrings.length())
            ecuCalDef[ecuCalDefIndex]->RomInfo.append(" ");
        ecuCalDef[ecuCalDefIndex]->RomInfo.replace(fileActions->FlashMethod, flash_method_list->currentText());
        ecuCalDef[ecuCalDefIndex]->Kernel = check_kernel(flash_method_list->currentText());
        qDebug() << ecuCalDef[ecuCalDefIndex]->Kernel;
        EcuOperationsSubaru *ecuOperationsSubaru = new EcuOperationsSubaru(serial, ecuCalDef[ecuCalDefIndex], cmd_type);
        if (ecuCalDef[ecuCalDefIndex]->FullRomData.length())
        {
            fileActions->openRomFile(ecuCalDef[ecuCalDefIndex], ecuCalDef[ecuCalDefIndex]->FullFileName);
            if(ecuCalDef[ecuCalDefIndex] != NULL)
            {
                calibrationTreeWidget->buildCalibrationFilesTree(ecuCalDefIndex, ui->calibrationFilesTreeWidget, ecuCalDef[ecuCalDefIndex]);
                calibrationTreeWidget->buildCalibrationDataTree(ui->calibrationDataTreeWidget, ecuCalDef[ecuCalDefIndex]);

                ecuCalDefIndex++;
            }
        }
        //else
            //delete ecuCalDef[ecuCalDefIndex];

        delete ecuOperationsSubaru;
    }
    //ecuFunctionsSubaru->show();

    serial->serialport_protocol_14230 = false;
    serial->change_port_speed("4800");
}

void MainWindow::start_manual_ecu_operations()
{
    QTreeWidgetItem *selectedItem = ui->calibrationFilesTreeWidget->selectedItems().at(0);
    int romNumber = ui->calibrationFilesTreeWidget->indexOfTopLevelItem(selectedItem);

    EcuManualOperations *ecuManualOperations = new EcuManualOperations(serial, ecuCalDef[romNumber]);

    //ecuManualFunctions->show();
    //delete ecuManualFunctions;

    serial->change_port_speed("4800");
}

void MainWindow::custom_menu_requested(QPoint pos)
{
    QModelIndex index = ui->calibrationFilesTreeWidget->indexAt(pos);
    emit ui->calibrationFilesTreeWidget->clicked(index);

    //qDebug() << "Files tree custom menu request at pos" << pos << "and index" << index;
    QMenu *menu = new QMenu(this);
    menu->addAction("Sync with ECU", this, SLOT(syncCalWithEcu()));
    menu->addAction("Close", this, SLOT(close_calibration()));
    menu->popup(ui->calibrationFilesTreeWidget->viewport()->mapToGlobal(pos));

    //connect(removeAction, SIGNAL(triggered()), this, SLOT(removeLogPanelView()));
}

void MainWindow::open_calibration_file()
{
    ecuCalDef[ecuCalDefIndex] = new FileActions::EcuCalDefStructure;
    qDebug() << ecuCalDefIndex;
    fileActions->openRomFile(ecuCalDef[ecuCalDefIndex], NULL);
    if(ecuCalDef[ecuCalDefIndex] != NULL)
    {
        configValues->calibration_files.append(ecuCalDef[ecuCalDefIndex]->FullFileName);
        //CalibrationTreeWidget *calibrationTreeWidget = new CalibrationTreeWidget();
        //qDebug() << "Create cal file tree";
        calibrationTreeWidget->buildCalibrationFilesTree(ecuCalDefIndex, ui->calibrationFilesTreeWidget, ecuCalDef[ecuCalDefIndex]);
        //qDebug() << "Create cal data tree";
        calibrationTreeWidget->buildCalibrationDataTree(ui->calibrationDataTreeWidget, ecuCalDef[ecuCalDefIndex]);
        //qDebug() << "Created!";

        ecuCalDefIndex++;
        fileActions->saveConfigFile();
    }
    else
    {
        //qDebug() << "Failed!!";
        ecuCalDef[ecuCalDefIndex] = {};
    }
}

void MainWindow::save_calibration_file()
{
    if (ecuCalDefIndex == 0){
        QMessageBox::information(this, tr("Calibration file"), "No calibration to save!");
        return;
    }

    QTreeWidgetItem *selectedItem = ui->calibrationFilesTreeWidget->selectedItems().at(0);

    int romNumber = ui->calibrationFilesTreeWidget->indexOfTopLevelItem(selectedItem);
    int romIndex = ui->calibrationFilesTreeWidget->selectedItems().at(0)->text(2).toInt();

    qDebug() << "ROM number" << romNumber << "close button clicked";
    qDebug() << "ROM index" << romIndex << "close button clicked";

    fileActions->saveRomFile(ecuCalDef[romNumber], ecuCalDef[romNumber]->FullFileName);

}

void MainWindow::save_calibration_file_as()
{
    if (ecuCalDefIndex == 0){
        QMessageBox::information(this, tr("Calibration file"), "No calibration to save!");
        return;
    }

    QTreeWidgetItem *selectedItem = ui->calibrationFilesTreeWidget->selectedItems().at(0);

    int romNumber = ui->calibrationFilesTreeWidget->indexOfTopLevelItem(selectedItem);
    int romIndex = ui->calibrationFilesTreeWidget->selectedItems().at(0)->text(2).toInt();

    qDebug() << "ROM number" << romNumber << "close button clicked";
    qDebug() << "ROM index" << romIndex << "close button clicked";

    QString filename = "";

    QFileDialog saveDialog;
    saveDialog.setDefaultSuffix("bin");
    if (ecuCalDef[romNumber]->OemEcuFile)
    {
        filename = QFileDialog::getSaveFileName(this, tr("Save calibration file"), configValues->calibration_files_base_directory, tr("Calibration file (*.bin)"));
        if(!filename.endsWith(QString(".bin")))
             filename.append(QString(".bin"));
    }

    if (filename.isEmpty()){
        QMessageBox::information(this, tr("Calibration file"), "No file selected");
        return;
    }

    fileActions->saveRomFile(ecuCalDef[romNumber], filename);

}

void MainWindow::selectable_combobox_item_changed(QString item)
{
    //qDebug() << "Combobox" << item << "clicked";
    int mapRomNumber = 0;
    int mapNumber = 0;

    QMdiSubWindow* w = ui->mdiArea->activeSubWindow();
    if (w)
    {
        QStringList mapWindowString = w->objectName().split(",");
        mapRomNumber = mapWindowString.at(0).toInt();
        mapNumber = mapWindowString.at(1).toInt();

        //qDebug() << w->objectName();
        QTableWidget* mapTableWidget = w->findChild<QTableWidget*>(w->objectName());
        if (mapTableWidget)
        {
            QStringList selectionsList = ecuCalDef[mapRomNumber]->SelectionsList.at(mapNumber).split(",");
            QStringList selectionsListSorted = ecuCalDef[mapRomNumber]->SelectionsListSorted.at(mapNumber).split(",");

            for (int j = 0; j < selectionsList.length(); j++){
                if (selectionsList[j] == item){
                    ecuCalDef[mapRomNumber]->MapData.replace(mapNumber, QString::number(j));
                }
            }
        }
    }
}

void MainWindow::calibration_files_treewidget_item_selected(QTreeWidgetItem* item)
{
    QList<QTreeWidgetItem *> itemList;
    QTreeWidgetItem *selectedItem = ui->calibrationFilesTreeWidget->selectedItems().at(0);

    for( int i = 0; i < ui->calibrationFilesTreeWidget->topLevelItemCount(); i++ )
    {
        QTreeWidgetItem *item = ui->calibrationFilesTreeWidget->topLevelItem(i);
        item->setCheckState(0, Qt::Unchecked);
    }

    int romNumber = ui->calibrationFilesTreeWidget->indexOfTopLevelItem(selectedItem);

    selectedItem->setCheckState(0, Qt::Checked);

    QString romId = selectedItem->text(1);

    calibrationTreeWidget->buildCalibrationDataTree(ui->calibrationDataTreeWidget, ecuCalDef[romNumber]);

    QComboBox *flash_method_list = ui->toolBar->findChild<QComboBox*>("flash_method_list");
    for (int i = 0; i < flash_method_list->count(); i++)
    {
        if(ecuCalDef[romNumber]->RomInfo.at(FlashMethod) == flash_method_list->itemText(i))
            flash_method_list->setCurrentIndex(i);
    }
}

void MainWindow::calibration_data_treewidget_item_selected(QTreeWidgetItem* item)
{
    const QModelIndex index = ui->calibrationDataTreeWidget->selectionModel()->currentIndex();
    QString selectedText = index.data(Qt::DisplayRole).toString();
    //find out the hierarchy level of the selected item
    int hierarchyLevel=1;
    QModelIndex seekRoot = index;
    QString selectedRom;

    selectedText = item->text(0);

    while(seekRoot.parent() != QModelIndex())
    {
        seekRoot = seekRoot.parent();
        hierarchyLevel++;
    }

    if (ui->calibrationDataTreeWidget->indexOfTopLevelItem(item) > -1){
        //qDebug() << "hierarchyLevel = 1";
        hierarchyLevel = 1;
    }
    else if (ui->calibrationDataTreeWidget->indexOfTopLevelItem(item->parent()) > -1){
        //qDebug() << "hierarchyLevel = 2, parent is" << ui->calibrationDataTreeWidget->indexOfTopLevelItem(item->parent());
        hierarchyLevel = 2;

        QTreeWidgetItem *selectedFilesTreeItem = ui->calibrationFilesTreeWidget->selectedItems().at(0);
        int romNumber = ui->calibrationFilesTreeWidget->indexOfTopLevelItem(selectedFilesTreeItem);
        int romIndex = selectedFilesTreeItem->text(2).toInt();
        //qDebug() << "Check tree item name";

        for (int i = 0; i < ecuCalDef[romNumber]->NameList.count(); i++)
        {
            if (ecuCalDef[romNumber]->NameList.at(i) == selectedText)//itemText)
            {
                if (ecuCalDef[romNumber]->VisibleList.at(i) == "1")
                {
                    //qDebug() << "Calibration map found visible";
                    QWidget* w = ui->mdiArea->findChild<QWidget*>(QString::number(romIndex) + "," + QString::number(i) + "," + ecuCalDef[romNumber]->NameList.at(i) + "," + ecuCalDef[romNumber]->TypeList.at(i));
                    if (w)
                    {
                        //qDebug() << "w =" << w->objectName() << "- active =" << ui->mdiArea->activeSubWindow()->objectName();
                        if (w->objectName() == ui->mdiArea->activeSubWindow()->objectName())
                        {
                            ui->mdiArea->removeSubWindow(w);
                            //w->close();
                            ecuCalDef[romNumber]->VisibleList.replace(i, "0");
                            item->setCheckState(0, Qt::Unchecked);
                            //qDebug() << "MdiSubWindow closed";
                        }
                        else
                        {
                            w->setFocus();
                            item->setCheckState(0, Qt::Checked);
                            //qDebug() << "MdiSubWindow focus set";

                        }
                    }
                }
                else
                {
                    ecuCalDef[romNumber]->VisibleList.replace(i, "1");
                    //ecuCalDef[romNumber]->SelectedList.replace(i, "1");
                    item->setCheckState(0, Qt::Checked);
                    //qDebug() << "MdiSubWindow created";

                    //qDebug() << "Open map number" << i;
                    CalibrationMaps *calibrationMaps = new CalibrationMaps(ecuCalDef[romNumber], romIndex, i, ui->mdiArea->contentsRect());
                    QMdiSubWindow *subWindow = ui->mdiArea->addSubWindow(calibrationMaps);
                    subWindow->setAttribute(Qt::WA_DeleteOnClose, true);
                    //subWindow->setObjectName(ecuCalDef[romNumber]->NameList.at(i));
                    subWindow->setObjectName(calibrationMaps->objectName());
                    subWindow->show();
                    subWindow->adjustSize();
                    subWindow->move(0,0);
                    subWindow->setFixedWidth(subWindow->width());
                    subWindow->setFixedHeight(subWindow->height());

                    connect(calibrationMaps, SIGNAL(selectable_combobox_item_changed(QString)), this, SLOT(selectable_combobox_item_changed(QString)));
                    connect(subWindow, SIGNAL(destroyed(QObject*)), this, SLOT(close_calibration_map(QObject*)));
                }
            }
        }
    }
}

void MainWindow::calibration_data_treewidget_item_expanded(QTreeWidgetItem* item)
{
    QTreeWidgetItem *selectedItem = ui->calibrationFilesTreeWidget->selectedItems().at(0);

    int itemIndex = ui->calibrationDataTreeWidget->indexOfTopLevelItem(item);
    int romNumber = ui->calibrationFilesTreeWidget->indexOfTopLevelItem(selectedItem);
    QString categoryName = ui->calibrationDataTreeWidget->topLevelItem(itemIndex)->text(0);

    //qDebug() << "Item" << categoryName << "expanded in rom" << romNumber;
    calibrationTreeWidget->calibrationDataTreeWidgetItemExpanded(ecuCalDef[romNumber], categoryName);
}

void MainWindow::calibration_data_treewidget_item_collapsed(QTreeWidgetItem* item)
{
    QTreeWidgetItem *selectedItem = ui->calibrationFilesTreeWidget->selectedItems().at(0);

    int itemIndex = ui->calibrationDataTreeWidget->indexOfTopLevelItem(item);
    int romNumber = ui->calibrationFilesTreeWidget->indexOfTopLevelItem(selectedItem);
    QString categoryName = ui->calibrationDataTreeWidget->topLevelItem(itemIndex)->text(0);

    //qDebug() << "Item" << categoryName << "collapsed in rom" << romNumber;

    calibrationTreeWidget->calibrationDataTreeWidgetItemCollapsed(ecuCalDef[romNumber], categoryName);
}

void MainWindow::close_calibration()
{
    QTreeWidgetItem *selectedItem = ui->calibrationFilesTreeWidget->selectedItems().at(0);

    int romNumber = ui->calibrationFilesTreeWidget->indexOfTopLevelItem(selectedItem);
    int romIndex = ui->calibrationFilesTreeWidget->selectedItems().at(0)->text(2).toInt();

    //qDebug() << "ROM number" << romNumber << "close button clicked";
    //qDebug() << "ROM index" << romIndex << "close button clicked";

    //qDebug() << "ToplevelItemCount" << ui->calibrationDataTreeWidget->topLevelItemCount();
    for (int i = 0; i < ui->calibrationDataTreeWidget->topLevelItemCount(); i++)
    {
        //qDebug() << "ChildCount" << ui->calibrationDataTreeWidget->topLevelItem(i)->childCount();
        for (int j = 0; j < ui->calibrationDataTreeWidget->topLevelItem(i)->childCount(); j++)
        {
            //qDebug() << "Subwindow list count" << ui->mdiArea->subWindowList().count();
            for (int k = 0; k < ui->mdiArea->subWindowList().count(); k++)
            {
                int mapNumber = ui->calibrationDataTreeWidget->topLevelItem(i)->child(j)->text(1).toInt();
                QString mapName = ui->calibrationDataTreeWidget->topLevelItem(i)->child(j)->text(0);
                QString mapType = ecuCalDef[romNumber]->TypeList.at(mapNumber);
                //qDebug() << romNumber;
                //qDebug() << mapNumber;
                //qDebug() << mapName;
                //qDebug() << mapType;

                QString calMapWindowName = QString::number(romIndex) + "," + QString::number(mapNumber) + "," + mapName;
                //QMdiSubWindow *w = ui->mdiArea->findChild<QMdiSubWindow*>(calMapWindowName);
                QMdiSubWindow *w = ui->mdiArea->subWindowList().at(k);
                //qDebug() << "Checking if map" << calMapWindowName << "found";
                //qDebug() << "Mapwindow is " << w->objectName();
                if (w->objectName().startsWith(calMapWindowName))
                {
                    qDebug() << calMapWindowName << "found, closing...";
                    //w->close();
                    ui->mdiArea->removeSubWindow(w);
                }

            }

        }
    }
    //qDebug() << "Delete rom item";
    delete ui->calibrationFilesTreeWidget->takeTopLevelItem(romNumber);

    //qDebug() << "Copy rom data structures";
    ecuCalDefIndex--;
    for (int i = romNumber; i < ecuCalDefIndex; i++)
    {
        if (ecuCalDefIndex > romNumber)
        {
            //qDebug() << i;
            ecuCalDef[i] = ecuCalDef[i + 1];
            int rom_index = ui->calibrationFilesTreeWidget->topLevelItem(i)->text(2).toInt();
            //qDebug() << i << rom_index;
            ui->calibrationFilesTreeWidget->topLevelItem(i)->setText(2, QString::number(rom_index - 1));
        }
    }
    ecuCalDef[ecuCalDefIndex] = {};

    //qDebug() << "Set new active rom";
    if (ui->calibrationFilesTreeWidget->topLevelItemCount() > 0)
    {
        for (int i = 0; i < ui->calibrationFilesTreeWidget->topLevelItemCount(); i++)
        {
            ui->calibrationFilesTreeWidget->topLevelItem(i)->setSelected(false);
        }
        int activeRom = 0;
        if (romNumber > 0)
            activeRom = romNumber - 1;
        QTreeWidgetItem *currentItem = ui->calibrationFilesTreeWidget->topLevelItem(activeRom);
        currentItem->setSelected(true);
        const QModelIndex index = ui->calibrationFilesTreeWidget->selectionModel()->currentIndex();
        emit ui->calibrationFilesTreeWidget->clicked(index);
    }
    else
    {
        for (int i = ui->calibrationDataTreeWidget->topLevelItemCount(); i > 0; i--)
        {
            delete ui->calibrationDataTreeWidget->takeTopLevelItem(0);
        }
    }
    //qDebug() << "Ecu Cal def index" << ecuCalDefIndex;
    configValues->calibration_files.removeAt(romNumber);
    fileActions->saveConfigFile();
}

void MainWindow::close_calibration_map(QObject* obj)
{
    QStringList mapWindowString = obj->objectName().split(",");
    int mapRomNumber = mapWindowString.at(0).toInt();
    //int mapNumber = mapWindowString.at(1).toInt();
    QString mapName = mapWindowString.at(2);

    //qDebug() << "Map closed";
    for (int i = 0; i < ui->calibrationFilesTreeWidget->topLevelItemCount(); i++)
    {
        ui->calibrationFilesTreeWidget->topLevelItem(i)->setSelected(false);
    }
    //qDebug() << "Map window close event";
    //qDebug() << "QObject" << obj->objectName();
    QTreeWidgetItem *currentItem = ui->calibrationFilesTreeWidget->topLevelItem(mapRomNumber);
    //qDebug() << "Get item";
    currentItem->setSelected(true);
    //qDebug() << "Set item selected";
    const QModelIndex index = ui->calibrationFilesTreeWidget->selectionModel()->currentIndex();
    //qDebug() << "Emit item selected signal";
    emit ui->calibrationFilesTreeWidget->clicked(index);

    QTreeWidgetItem *selectedItem = ui->calibrationFilesTreeWidget->selectedItems().at(0);
    //int treeRomNumber = ui->calibrationFilesTreeWidget->indexOfTopLevelItem(selectedItem);
    int romIndex = selectedItem->text(2).toInt();

    //qDebug() << mapRomNumber;
    //qDebug() << mapNumber;
    //qDebug() << mapName;

    QTreeWidgetItem *item;
    //qDebug() << "Top level item count" << ui->calibrationDataTreeWidget->topLevelItemCount();
    for(int i = 0 ; i < ui->calibrationDataTreeWidget->topLevelItemCount(); i++)
    {
        //qDebug() << "Top level item child count" << ui->calibrationDataTreeWidget->topLevelItem(i)->childCount();
        for(int j = 0 ; j < ui->calibrationDataTreeWidget->topLevelItem(i)->childCount(); j++)
        {
            item = ui->calibrationDataTreeWidget->topLevelItem(i)->child(j);
            if (item->text(0) == mapName)
            {
                //qDebug() << item->text(0);
                if (mapRomNumber == romIndex)
                    item->setCheckState(0, Qt::Unchecked);

                for (int i = 0; i < ecuCalDef[mapRomNumber]->NameList.count(); i++)
                {
                    if (ecuCalDef[mapRomNumber]->NameList.at(i) == mapName)
                    {
                        ecuCalDef[mapRomNumber]->VisibleList.replace(i, "0");
                    }
                }

            }
        }
    }

}

void MainWindow::closeEvent(QCloseEvent *event)
{
    //qDebug() << "Closing application with event:" << event;
    close_app();
}

void MainWindow::close_app()
{
    FileActions::ConfigValuesStructure *configValues = &fileActions->ConfigValuesStruct;

    //qDebug() << "Closing serial port" << configValues->serialPort;

    //qDebug() << "Closing mainwindow";
    qApp->exit();
}

void MainWindow::change_gauge_values()
{
    //qDebug() << "Change log values start";
    LogValues *logValuesDialog = new LogValues(logValues, 0);
    logValuesDialog->show();
    //qDebug() << "Change log values end";
}

void MainWindow::change_digital_values()
{
    //qDebug() << "Change log values start";
    LogValues *logValuesDialog = new LogValues(logValues, 1);
    logValuesDialog->show();
    //qDebug() << "Change log values end";
}

void MainWindow::change_switch_values()
{
    //qDebug() << "Change switch values start";
    LogValues *logValuesDialog = new LogValues(logValues, 2);
    logValuesDialog->show();
    //qDebug() << "Change switch values end";
}

void MainWindow::resizeEvent( QResizeEvent * event)
{
    QWidget::resizeEvent(event);
    QWidget* w = ui->mdiArea->findChild<QWidget*>("gaugeWindow");
    if (w){
        delete w;
    }
}

void MainWindow::set_status_bar_label(bool serialConnectionState, bool ecuConnectionState, QString romId)
{
    QString connectionStatusString;

    QString softwareVersionString = "FastECU";
    if (ecuConnectionState){
        connectionStatusString = "ECU connected";
        statusBarLabel->setStyleSheet("QLabel { background-color : green; color : white; color: white;}");
    }
    else if(serialConnectionState){
        connectionStatusString = "ECU not connected";
        statusBarLabel->setStyleSheet("QLabel { background-color : yellow; color : white; color: black;}");
    }
    else{
        connectionStatusString = "Serial port unavailable";
        statusBarLabel->setStyleSheet("QLabel { background-color : red; color : white; color: white;}");
    }
    QString romIdString = "ECU ID: " + romId;
    QString statusBarString = softwareVersionString + " | " + connectionStatusString + " | " + romIdString;
    statusBarLabel->setText(statusBarString);
}

void MainWindow::delay(int n)
{
    QTime dieTime = QTime::currentTime().addMSecs(n);
    while (QTime::currentTime() < dieTime)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
}

void MainWindow::add_new_ecu_definition_file()
{
    QString filename;
    QObject* obj = sender();
    // Search for listwidget
    QListWidget* definition_files = obj->parent()->parent()->findChild<QListWidget*>("ecu_definition_files_list");

    // Get new file name
    QFileDialog openDialog;
    openDialog.setDefaultSuffix("xml");
    filename = QFileDialog::getOpenFileName(this, tr("Select definition file"), configValues->definition_files_directory, tr("ECU definition file (*.xml)"));

    // No file selected
    if (filename.isEmpty())
    {
        QMessageBox::information(this, tr("ECU definition file"), "No file selected");
    }
    else
    {
        // Add file to list
        definition_files->addItem(filename);
        configValues->ecu_definition_files.append(filename);
        // save config file with new settings
        fileActions->saveConfigFile();
    }

}

void MainWindow::remove_ecu_definition_file()
{
    QObject* obj = sender();
    // Search for listwidget
    QListWidget* definition_files = obj->parent()->parent()->findChild<QListWidget*>("ecu_definition_files_list");
    // Get selected indexes
    QList<QModelIndex> index = definition_files->selectionModel()->selectedIndexes();

    int row = 0;
    for (int i = index.length() - 1; i >= 0; i--)
    {
        row = index.at(i).row();
        // Remove item from list
        definition_files->model()->removeRow(row);
        configValues->ecu_definition_files.removeAt(row);
    }
    // If items removed, save config file with new settings
    if (index.length() > 0)
        fileActions->saveConfigFile();
}

void MainWindow::add_new_logger_definition_file()
{
    QObject* obj = sender();

}

void MainWindow::remove_logger_definition_file()
{
    QObject* obj = sender();

}

QString MainWindow::parse_message_to_hex(QByteArray received)
{
    QString msg;

    for (unsigned long i = 0; i < received.length(); i++)
    {
        msg.append(QString("%1 ").arg((uint8_t)received.at(i),2,16,QLatin1Char('0')));
    }

    return msg;
}

QString MainWindow::parse_ecuid(QByteArray received)
{
    QString msg;
    received.remove(0,8);
    received.remove(5, received.length() - 5);

    for (unsigned long i = 0; i < received.length(); i++)
    {
        msg.append(QString("%1").arg((uint8_t)received.at(i),2,16,QLatin1Char('0')).toUpper());
    }

    return msg;
}
