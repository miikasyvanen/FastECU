#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QSplashScreen>

const QColor MainWindow::RED_LIGHT_OFF = QColor(96, 32, 32);
const QColor MainWindow::YELLOW_LIGHT_OFF = QColor(96, 96, 32);
const QColor MainWindow::GREEN_LIGHT_OFF = QColor(32, 96, 32);
const QColor MainWindow::RED_LIGHT_ON = QColor(255, 64, 64);
const QColor MainWindow::YELLOW_LIGHT_ON = QColor(223, 223, 64);
const QColor MainWindow::GREEN_LIGHT_ON = QColor(64, 255, 64);

MainWindow::MainWindow(QString peerAddress, QString peerPassword, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , peerAddress(peerAddress)
    , peerPassword(peerPassword)
{
    ui->setupUi(this);
    qApp->installEventFilter(this);
    qRegisterMetaType<QVector<int> >("QVector<int>");

    QPixmap startUpSplashImage(":/images/startup_splash.jpg");
    int startUpSplashProgressBarValue = 0;

    startUpSplash = new QSplashScreen(startUpSplashImage);
    QVBoxLayout *startUpSplashLayout = new QVBoxLayout(startUpSplash);
    //startUpSplashLayout->setMargin(0);
    startUpSplashLayout->setSpacing(0);
    startUpSplashLayout->setAlignment(Qt::AlignBottom);
    startUpSplashLabel = new QLabel(QString("Starting FastECU..."));
    startUpSplashLabel->setStyleSheet("QLabel { background-color : black; color : white; }");
    startUpSplashLayout->addWidget(startUpSplashLabel);

    startUpSplashProgressBar = new QProgressBar();
    startUpSplashProgressBar->setMinimum(0);
    startUpSplashProgressBar->setMaximum(100);
    startUpSplashProgressBar->setValue(startUpSplashProgressBarValue);
    startUpSplashProgressBar->setFixedHeight(16);
    startUpSplashLayout->addWidget(startUpSplashProgressBar);
    //startUpSplash->setEnabled(false);
    startUpSplash->show();
    // Move splashscreen to the center of the screen
    QScreen *screen = QGuiApplication::primaryScreen();
    QRect  screenGeometry = screen->geometry();
    startUpSplash->move(screenGeometry.center() - startUpSplash->rect().center());
    QCoreApplication::processEvents(QEventLoop::AllEvents, 10);

    setSplashScreenProgress("Reading config files...", 10);
    fileActions = new FileActions();
    configValues = &fileActions->ConfigValuesStruct;
    fileActions->set_base_dirs(configValues);

    software_name = configValues->software_name;
    software_title = configValues->software_title;
    software_version = configValues->software_version;
    this->setWindowTitle(software_title + " " + software_version);

    QThread *syslog_thread = new QThread();
    syslogger = new SystemLogger(configValues->syslog_files_directory, software_name, software_version);
    syslogger->moveToThread(syslog_thread);
    QObject::connect(this, &MainWindow::LOG_E, syslogger, &SystemLogger::log_messages);
    QObject::connect(this, &MainWindow::LOG_W, syslogger, &SystemLogger::log_messages);
    QObject::connect(this, &MainWindow::LOG_I, syslogger, &SystemLogger::log_messages);
    QObject::connect(this, &MainWindow::LOG_D, syslogger, &SystemLogger::log_messages);
    QObject::connect(this, &MainWindow::enable_log_write_to_file, syslogger, &SystemLogger::enable_log_write_to_file);
    QObject::connect(syslogger, &SystemLogger::send_message_to_log_window, this, &MainWindow::send_message_to_log_window);
    QObject::connect(syslogger, &SystemLogger::finished, syslog_thread, &QThread::quit);
    QObject::connect(syslogger, &SystemLogger::finished, syslogger, &SystemLogger::deleteLater);
    QObject::connect(syslog_thread, &QThread::finished, syslog_thread, &QThread::deleteLater);
    QObject::connect(syslog_thread, &QThread::started, syslogger, &SystemLogger::run);
    syslog_thread->start();

#if defined Q_OS_UNIX
    emit LOG_D("Running on Linux Desktop ", true, false);
    //serialPort = serialPortLinux;
    serial_port_prefix = "/dev/";
#elif defined Q_OS_WIN32
    emit LOG_D("Running on Windows Desktop ", true, false);
    //serialPort = serialPortWindows;
    serial_port_prefix = "";
#endif

#if Q_PROCESSOR_WORDSIZE == 4
    emit LOG_D("32-bit executable", true, true);
#elif Q_PROCESSOR_WORDSIZE == 8
    emit LOG_D("64-bit executable", true, true);
#endif

    QObject::connect(fileActions, &FileActions::LOG_E, syslogger, &SystemLogger::log_messages);
    QObject::connect(fileActions, &FileActions::LOG_W, syslogger, &SystemLogger::log_messages);
    QObject::connect(fileActions, &FileActions::LOG_I, syslogger, &SystemLogger::log_messages);
    QObject::connect(fileActions, &FileActions::LOG_D, syslogger, &SystemLogger::log_messages);

    emit enable_log_write_to_file(true);

    QObject::connect(calibrationTreeWidget, &CalibrationTreeWidget::LOG_E, syslogger, &SystemLogger::log_messages);
    QObject::connect(calibrationTreeWidget, &CalibrationTreeWidget::LOG_W, syslogger, &SystemLogger::log_messages);
    QObject::connect(calibrationTreeWidget, &CalibrationTreeWidget::LOG_I, syslogger, &SystemLogger::log_messages);
    QObject::connect(calibrationTreeWidget, &CalibrationTreeWidget::LOG_D, syslogger, &SystemLogger::log_messages);

    fileActions->check_config_dirs(configValues);

    configValues = fileActions->read_config_file(configValues);

    fileActions->read_protocols_file(configValues);
    emit LOG_D("ECU protocols read", true, true);
    emit LOG_D("Protocols ID: " + configValues->flash_protocol_selected_id + "/" + QString::number(configValues->flash_protocol_id.length()), true, true);

    if (configValues->flash_protocol_selected_id.toInt() > configValues->flash_protocol_id.length())
        configValues->flash_protocol_selected_id = "0";
    configValues->flash_protocol_selected_make = configValues->flash_protocol_make.at(configValues->flash_protocol_selected_id.toInt());
    configValues->flash_protocol_selected_mcu = configValues->flash_protocol_mcu.at(configValues->flash_protocol_selected_id.toInt());
    configValues->flash_protocol_selected_checksum = configValues->flash_protocol_checksum.at(configValues->flash_protocol_selected_id.toInt());
    configValues->flash_protocol_selected_model = configValues->flash_protocol_model.at(configValues->flash_protocol_selected_id.toInt());
    configValues->flash_protocol_selected_version = configValues->flash_protocol_version.at(configValues->flash_protocol_selected_id.toInt());
    configValues->flash_protocol_selected_protocol_name = configValues->flash_protocol_protocol_name.at(configValues->flash_protocol_selected_id.toInt());
    configValues->flash_protocol_selected_description = configValues->flash_protocol_description.at(configValues->flash_protocol_selected_id.toInt());
    //configValues->flash_protocol_selected_flash_transport = configValues->flash_protocol_flash_transport.at(configValues->flash_protocol_selected_id.toInt());
    //configValues->flash_protocol_selected_log_transport = configValues->flash_protocol_log_transport.at(configValues->flash_protocol_selected_id.toInt());
    //configValues->flash_protocol_selected_log_protocol = configValues->flash_protocol_log_protocol.at(configValues->flash_protocol_selected_id.toInt());

    emit LOG_D(configValues->flash_protocol_selected_make, true, true);
    emit LOG_D(configValues->flash_protocol_selected_mcu, true, true);
    emit LOG_D(configValues->flash_protocol_selected_checksum, true, true);
    emit LOG_D(configValues->flash_protocol_selected_model, true, true);
    emit LOG_D(configValues->flash_protocol_selected_version, true, true);
    emit LOG_D(configValues->flash_protocol_selected_protocol_name, true, true);
    emit LOG_D(configValues->flash_protocol_selected_description, true, true);
    emit LOG_D(configValues->flash_protocol_selected_flash_transport, true, true);
    emit LOG_D(configValues->flash_protocol_selected_log_transport, true, true);
    emit LOG_D(configValues->flash_protocol_selected_log_protocol, true, true);
    emit LOG_D("ECU protocols set", true, true);

    QRect qrect = MainWindow::geometry();

    if (configValues->window_width != "maximized" && configValues->window_height != "maximized")
        this->setGeometry(qrect.x(), qrect.y(), configValues->window_width.toInt(), configValues->window_height.toInt());
    else
        this->setWindowState(Qt::WindowMaximized);

    setSplashScreenProgress("Preparing ROM definitions...", 10);
    if (!configValues->romraider_definition_files.length() && !configValues->ecuflash_definition_files_directory.length())
        QMessageBox::warning(this, tr("Ecu definition file"), "No definition file(s), use 'Settings' in 'Edit' menu to choose file(s)");

    setSplashScreenProgress("Preparing EcuFlash ROM definitions...", 10);
    fileActions->create_ecuflash_def_id_list(configValues);

    setSplashScreenProgress("Preparing RomRaider ROM definitions...", 10);
    fileActions->create_romraider_def_id_list(configValues);

    if (QDir(configValues->kernel_files_directory).exists()){
        QDir dir(configValues->kernel_files_directory);
        QStringList nameFilter("*.bin");
        QStringList txtFilesAndDirectories = dir.entryList(nameFilter);
        //emit LOG_D(txtFilesAndDirectories;
    }

    setSplashScreenProgress("Setting up menus...", 10);
    QSignalMapper *mapper = fileActions->read_menu_file(ui->menubar, ui->toolBar);
#if QT_VERSION >=0x060000
    connect(mapper, SIGNAL(mappedString(QString)), this, SLOT(menu_action_triggered(QString)));
#elif QT_VERSION >=0x050000
    connect(mapper, SIGNAL(mapped(QString)), this, SLOT(menu_action_triggered(QString)));
#endif

/*
    for (int i = 0; i < configValues->calibration_files.count(); i++)
    {
        QString filename = configValues->calibration_files.at(i);
        bool result = false;
        //emit LOG_D("Open file" << filename;
        //ecuCalDef[ecuCalDefIndex] = new FileActions::EcuCalDefStructure;
        result = open_calibration_file(filename);
        if (result)
        {
            configValues->calibration_files.removeAt(i);
            fileActions->saveConfigFile();
            i--;
        }
    }
    if(ecuCalDefIndex > 0)
    {
        const QModelIndex index = ui->calibrationFilesTreeWidget->selectionModel()->currentIndex();
        emit ui->calibrationFilesTreeWidget->clicked(index);
    }
*/
    connect(ui->switchBoxWidget, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(change_switch_values()));
    connect(ui->logBoxWidget, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(change_digital_values()));
    connect(ui->mdiArea, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(change_gauge_values()));
    //connect(ui->action_Preferences, SIGNAL(triggered()), this, SLOT(openPreferences()));
    connect(ui->calibrationFilesTreeWidget, SIGNAL(itemClicked(QTreeWidgetItem *,int)), this, SLOT(calibration_files_treewidget_item_selected(QTreeWidgetItem*)));
    connect(ui->calibrationDataTreeWidget, SIGNAL(itemClicked(QTreeWidgetItem *,int)), this, SLOT(calibration_data_treewidget_item_selected(QTreeWidgetItem*)));
    connect(ui->calibrationDataTreeWidget, SIGNAL(itemExpanded(QTreeWidgetItem*)), this, SLOT(calibration_data_treewidget_item_expanded(QTreeWidgetItem*)));
    connect(ui->calibrationDataTreeWidget, SIGNAL(itemCollapsed(QTreeWidgetItem*)), this, SLOT(calibration_data_treewidget_item_collapsed(QTreeWidgetItem*)));
    connect(calibrationTreeWidget, SIGNAL(closeRom()), this, SLOT(close_calibration()));

    setSplashScreenProgress("Setting up statusbar...", 10);
    status_bar_connection_label->setMargin(5);
    //status_bar_connection_label->setStyleSheet("QLabel { background-color : red; color : white; }");
    set_status_bar_label(false, false, "");

    status_bar_ecu_label->setText("");
    status_bar_ecu_label->setMargin(5);
    status_bar_ecu_label->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    //status_bar_ecu_label->setStyleSheet("QLabel { background-color : red; color : white; }");

    QWidget* status_bar_spacer = new QWidget();
    status_bar_spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    statusBar()->addWidget(status_bar_connection_label);
    statusBar()->addWidget(status_bar_spacer);
    statusBar()->addPermanentWidget(status_bar_ecu_label);
    statusBar()->setSizeGripEnabled(true);

    setSplashScreenProgress("Preparing up treewidget...", 10);
    ui->calibrationFilesTreeWidget->setHeaderLabel("Calibration Files");
    ui->calibrationDataTreeWidget->setHeaderLabel("Calibration Data");
    ui->calibrationDataTreeWidget->resizeColumnToContents(0);
    ui->calibrationDataTreeWidget->resizeColumnToContents(1);
    ui->calibrationFilesTreeWidget->setMinimumHeight(125);
    ui->calibrationFilesTreeWidget->minimumHeight();
    ui->calibrationFilesTreeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->calibrationFilesTreeWidget, SIGNAL(customContextMenuRequested(QPoint)), SLOT(custom_menu_requested(QPoint)));

    //ui->splitter->setStretchFactor(2, 1);
    ui->splitter->setSizes(QList<int>({125, INT_MAX}));

    setSplashScreenProgress("Preparing remote connection...", 10);
    //Splash screen
    netSplash = new QSplashScreen();
    QVBoxLayout *netSplashLayout = new QVBoxLayout(netSplash);
    netSplashLayout->setAlignment(Qt::AlignCenter);
    QLabel *netSplashLabel = new QLabel(QString("Waiting for peer "+peerAddress+"..."), netSplash);
    netSplashLabel->setAlignment(Qt::AlignCenter);
    QProgressBar *netSplashProgressBar = new QProgressBar(netSplash);
    netSplashProgressBar->setAlignment(Qt::AlignCenter);
    netSplashProgressBar->setMinimum(0);
    //Number of network connection stages
    netSplashProgressBar->setMaximum(2);
    netSplashProgressBar->setValue(0);
    QPushButton *btnCloseApp = new QPushButton("Close app", netSplash);
    netSplashLayout->addWidget(netSplashLabel);
    netSplashLayout->addWidget(netSplashProgressBar);
    netSplashLayout->addWidget(btnCloseApp);
    //splash->setLayout(layout);
    netSplash->resize(350,50);
    //Show it in remote mode only
    //Prepare for remote mode
    if (!peerAddress.isEmpty())
    {
        netSplash->show();
        // Move splashscreen to the center of the screen
        QScreen *screen = QGuiApplication::primaryScreen();
        QRect  screenGeometry = screen->geometry();
        netSplash->move(screenGeometry.center() - netSplash->rect().center());
        QCoreApplication::processEvents(QEventLoop::AllEvents, 10);

        QString wt = this->windowTitle();
        if (peerAddress.length() > 0)
            wt += " - Remote Connection to " + peerAddress;
        this->setWindowTitle(wt);
    }
    //Add option to close app while waiting for network connection
    QObject::connect(btnCloseApp, &QPushButton::released, this, [&]()
                     {
                         netSplashLabel->setText("Closing app, please wait...");
                         exit(1);
                     });

    //Init may take a long time due to network
    //Do it in a non-blocking way
    //This timer will timeout as fast as possible
    //processing events
    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [&]()
    {
        QApplication::processEvents();
    });
    timer->start();

    serial = new SerialPortActions(peerAddress, peerPassword, nullptr, this);
    QObject::connect(serial, &SerialPortActions::LOG_E, syslogger, &SystemLogger::log_messages);
    QObject::connect(serial, &SerialPortActions::LOG_W, syslogger, &SystemLogger::log_messages);
    QObject::connect(serial, &SerialPortActions::LOG_I, syslogger, &SystemLogger::log_messages);
    QObject::connect(serial, &SerialPortActions::LOG_D, syslogger, &SystemLogger::log_messages);

    remote_utility = new RemoteUtility(peerAddress, peerPassword, nullptr, this);
    if (!serial->isDirectConnection())
    {
        netSplashProgressBar->setValue(0);
        netSplashProgressBar->setFormat("Connecting to J2534 and serial devices...");
        serial->waitForSource();
        netSplashProgressBar->setValue(1);
        netSplashProgressBar->setFormat("Connecting to utility functions...");
        remote_utility->waitForSource();
        netSplashProgressBar->setValue(2);
    }
    external_logger("Connection successfull.");

    timer->stop();
    netSplash->close();
    timer->deleteLater();
    connect(serial, &SerialPortActions::stateChanged,
            this, &MainWindow::network_state_changed, Qt::DirectConnection);
    connect(remote_utility, &RemoteUtility::stateChanged,
            this, &MainWindow::network_state_changed, Qt::DirectConnection);

    // Set timer to read vbatt value
    vbatt_timer = new QTimer(this);
    vbatt_timer->setInterval(vbatt_timer_timeout);
    connect(vbatt_timer, SIGNAL(timeout()), this, SLOT(update_vbatt()));

    setSplashScreenProgress("Setting up toolbar...", 10);
    toolbar_item_size.setWidth(configValues->toolbar_iconsize.toInt());
    toolbar_item_size.setHeight(configValues->toolbar_iconsize.toInt());
    ui->toolBar->setIconSize(toolbar_item_size);

    QWidget* spacer = new QWidget();
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    ui->toolBar->addWidget(spacer);

    ui->toolBar->addSeparator();

    QPushButton *select_protocol_button = new QPushButton();
    select_protocol_button->setText("Select protocol");
    //car_make_button->setMargin(10);
    select_protocol_button->setFixedHeight(toolbar_item_size.height());
    ui->toolBar->addWidget(select_protocol_button);
    connect(select_protocol_button, SIGNAL(clicked(bool)), this, SLOT(select_protocol()));

    QPushButton *select_vehicle_button = new QPushButton();
    select_vehicle_button->setText("Select vehicle");
    //car_make_button->setMargin(10);
    select_vehicle_button->setFixedHeight(toolbar_item_size.height());
    ui->toolBar->addWidget(select_vehicle_button);
    connect(select_vehicle_button, SIGNAL(clicked(bool)), this, SLOT(select_vehicle()));

    flash_transport_list = new QComboBox();
    flash_transport_list->setFixedHeight(toolbar_item_size.height());
    flash_transport_list->setFixedWidth(90);
    flash_transport_list->setObjectName("flash_transport_list");
    ui->toolBar->addWidget(flash_transport_list);

    ui->toolBar->addSeparator();

    QLabel *log_transport = new QLabel("Log:");
    log_transport->setMargin(10);
    ui->toolBar->addWidget(log_transport);

    log_transport_list = new QComboBox();
    log_transport_list->setFixedHeight(toolbar_item_size.height());
    log_transport_list->setFixedWidth(90);
    log_transport_list->setObjectName("log_transport_list");
    ui->toolBar->addWidget(log_transport_list);

    flash_transports = create_flash_transports_list();
    log_transports = create_log_transports_list();
    connect(flash_transport_list, SIGNAL(currentIndexChanged(int)), this, SLOT(flash_transport_changed()));
    connect(log_transport_list, SIGNAL(currentIndexChanged(int)), this, SLOT(log_transport_changed()));

    //ui->toolBar->addSeparator();

    QLabel *log_select = new QLabel();
    log_select->setMargin(5);
    ui->toolBar->addWidget(log_select);

    ecu_radio_button = new QRadioButton("ECU");
    ecu_radio_button->setChecked(true);
    ui->toolBar->addWidget(ecu_radio_button);
    tcu_radio_button = new QRadioButton("TCU");
    ui->toolBar->addWidget(tcu_radio_button);

    ui->toolBar->addSeparator();

    QLabel *serial_port_select = new QLabel("Port:");
    serial_port_select->setMargin(10);
    ui->toolBar->addWidget(serial_port_select);

    serial_port_list = new QComboBox();
    serial_port_list->setFixedHeight(toolbar_item_size.height());
    serial_port_list->setFixedWidth(180);
    serial_port_list->setObjectName("serial_port_list");
    serial_ports = serial->check_serial_ports();
    for (int i = 0; i < serial_ports.length(); i++)
    {
        serial_port_list->addItem(serial_ports.at(i));
        if (configValues->serial_port == serial_ports.at(i).split(" - ").at(0))
            serial_port_list->setCurrentIndex(i);
    }
    ui->toolBar->addWidget(serial_port_list);

    refresh_serial_port_list = new QPushButton();
    refresh_serial_port_list->setIcon(QIcon(":/icons/view-refresh.png"));
    refresh_serial_port_list->setFixedHeight(toolbar_item_size.height());
    refresh_serial_port_list->setFixedWidth(toolbar_item_size.height());
    //refresh_serial_port_list->setIconSize(toolbar_item_size);
    connect(refresh_serial_port_list, SIGNAL(clicked(bool)), this, SLOT(check_serial_ports()));
    ui->toolBar->addWidget(refresh_serial_port_list);

    logValues = &fileActions->LogValuesStruct;
    logValues = fileActions->read_logger_definition_file();
    logBoxes = new LogBox();

    if (logValues != NULL)
    {
        update_logboxes(configValues->flash_protocol_selected_log_protocol);
    }

    serial_port = serial_port_prefix + configValues->serial_port;
    serial_port_baudrate = default_serial_port_baudrate;
    serial->set_serial_port_baudrate(serial_port_baudrate);
    serial->set_serial_port(serial_port);
/*
    serial_poll_timer = new QTimer(this);
    serial_poll_timer->setInterval(serial_poll_timer_timeout);
    connect(serial_poll_timer, SIGNAL(timeout()), this, SLOT(open_serial_port()));
    serial_poll_timer->start();

    ssm_init_poll_timer = new QTimer(this);
    ssm_init_poll_timer->setInterval(ssm_init_poll_timer_timeout);
    connect(ssm_init_poll_timer, SIGNAL(timeout()), this, SLOT(ecu_init()));
    ssm_init_poll_timer->start();
*/
    logging_poll_timer = new QTimer(this);
    logging_poll_timer->setInterval(logging_poll_timer_timeout);
    connect(logging_poll_timer, SIGNAL(timeout()), this, SLOT(log_ssm_values()));

    logparams_poll_timer = new QTimer(this);
    logparams_poll_timer->setInterval(logparams_poll_timer_timeout);
    connect(logparams_poll_timer, SIGNAL(timeout()), this, SLOT(read_log_serial_data()));

    log_speed_timer = new QElapsedTimer();
    log_file_timer = new QElapsedTimer();

    if(ecuCalDefIndex > 0)
    {
        const QModelIndex index = ui->calibrationFilesTreeWidget->selectionModel()->currentIndex();
        emit ui->calibrationFilesTreeWidget->clicked(index);
    }

    emit log_transport_list->currentIndexChanged(log_transport_list->currentIndex());

    status_bar_ecu_label->setText(configValues->flash_protocol_selected_description + " ");

    set_flash_arrow_state();

    startUpSplashLabel->setText("Starting FastECU GUI...");
    startUpSplashProgressBarValue = startUpSplashProgressBar->value();
    while (startUpSplashProgressBarValue < 100)
    {
        startUpSplashProgressBar->setValue(startUpSplashProgressBarValue += 1);
        delay(10);
        QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
    }

    startUpSplash->close();
    netSplash->deleteLater();
/*
    // AES-128 ECB examples start
    emit LOG_D("Solving challenge...";
    //QByteArray key = { "\x46\x9a\x20\xab\x30\x8d\x5c\xa6\x4b\xcd\x5b\xbe\x53\x5b\xd8\x5f" };
    QByteArray key = { "\x7D\x89\xDD\xE1\xC9\x5A\x22\x4E\xD7\x23\xE0\x44\x96\xF4\xC0\xAE" };
    //QByteArray challenge = { "\x5f\x75\x8c\x11\x92\xdc\x56\xfb\x69\xe3\x40\x2d\x83\xfb\x75\xe4" };
    QByteArray challenge = { "\x3D\xA9\x19\x57\x6E\x88\xD3\xBF\x25\x2C\x02\xC4\x4F\x70\x0B\x63" };
    QByteArray response;
    emit LOG_D("*********";
    response.append(aes_ecb_test(challenge, key));
    key = "\x5C\x9F\x97\xAD\xE5\x1D\xA6\xD0\x60\x9D\x49\xBB\x05\xA4\x17\xE9";
    response.append(aes_ecb_test(challenge, key));
    key = "\xC9\x22\x70\xE6\x64\x63\x39\x54\x07\xF6\xC3\x01\x4D\xC8\x90\xB0";
    response.append(aes_ecb_test(challenge, key));
    key = "\x66\xD5\x36\x4A\x0D\x2D\x45\xC6\x7E\x8D\xB1\x20\xED\x47\x14\x38";
    response.append(aes_ecb_test(challenge, key));
    key = "\x37\x49\x0E\x2C\x46\xC5\x7F\x8C\xB2\x2F\xEC\x48\x13\x39\x65\xD6";    emit LOG_D("*********";
    response.append(aes_ecb_test(challenge, key));
    //aes_ecb_example();
    // AES-128 ECB examples end
*/
    emit LOG_I("FastECU initialized", true, true);
}

MainWindow::~MainWindow()
{
    if (logging_state)
    {
        logging_state = false;
        log_params_request_started = false;
        log_ssm_values();
        delay(200);
    }
    //ssm_init_poll_timer->stop();
    //serial_poll_timer->stop();
    delete ui;
}

QByteArray MainWindow::aes_ecb_test(QByteArray challenge, QByteArray key)
{
    // Clean: "5f758c1192dc56fb69e3402d83fb75e4"
    // Key  : "469a20ab308d5ca64bcd5bbe535bd85f"
    // Enc  : "b8f73be3b1dcc30e93d88f0af5a860ca"

    Cipher cipher;
    unsigned char encrypted[64];
    unsigned char decrypted[64];

    char *data = challenge.data();
    char *cKey = key.data();

    int decrypted_len, encrypted_len;

    // Encrypt the original data
    encrypted_len = cipher.encrypt_aes128_ecb((unsigned char*)data, strlen(data), (unsigned char*)cKey, encrypted);
    // Decrypt the encrypted data
    decrypted_len = cipher.decrypt_aes128_ecb(encrypted, encrypted_len, (unsigned char*)cKey, decrypted);
    QByteArray challengeReply(QByteArray::fromRawData((const char*)encrypted, 16));
    emit LOG_D("Key: " + parse_message_to_hex(key), true, true);
    emit LOG_D("Challenge: " + parse_message_to_hex(challenge), true, true);
    emit LOG_D("Decrypted: " + parse_message_to_hex(challengeReply), true, true);
    emit LOG_D("Length: 0x" + QString::number(encrypted_len, 16), true, true);

    QByteArray challengeDecrypt(QByteArray::fromRawData((const char*)decrypted, 16));
    return challengeReply;
}

void MainWindow::network_state_changed(QRemoteObjectReplica::State state, QRemoteObjectReplica::State oldState)
{
    if (state == QRemoteObjectReplica::Valid)
    {
        emit LOG_D("Network connection established", true, true);
    }
    else if (oldState == QRemoteObjectReplica::Valid)
    {
        if (!restartQuestionActive.tryLock())
            return;
        emit LOG_D("Network connection lost, reconnecting...", true, true);
        QMessageBox msgBox;
        msgBox.setText("Network connection lost.");
        msgBox.setInformativeText("Do you want to restart application and try to connect again?");
        QPushButton *restartButton = msgBox.addButton(tr("Restart"), QMessageBox::YesRole);
        QPushButton *quitButton = msgBox.addButton(tr("Quit"), QMessageBox::NoRole);
        msgBox.addButton(tr("Continue work"), QMessageBox::NoRole);
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setDetailedText("Press Restart to restart application. All your unsaved progress will be lost.\n\n"
                               "Press Quit to quit application. All your unsaved progress will be lost.\n\n"
                               "Press Continue work to return to application and save your progress. "
                               "In this case all ECU operations will be unavailable until you restart application.");

        msgBox.exec();

        if (msgBox.clickedButton() == restartButton)
            qApp->exit(RESTART_CODE);
        else if (msgBox.clickedButton() == quitButton)
            qApp->exit(1);

        restartQuestionActive.unlock();
    }
}

void MainWindow::aes_ecb_example()
{
    // Clean: "5f758c1192dc56fb69e3402d83fb75e4"
    // Key  : "469a20ab308d5ca64bcd5bbe535bd85f"
    // Enc  : "b8f73be3b1dcc30e93d88f0af5a860ca"

    emit LOG_D("aes_ecb_example:", true, true);

    Cipher cipher;
    // A 128 bit key
    unsigned char key[] = { 0x46, 0x9a, 0x20, 0xab, 0x30, 0x8d, 0x5c, 0xa6, 0x4b, 0xcd, 0x5b, 0xbe, 0x53, 0x5b, 0xd8, 0x5f };
    QByteArray engine_key_1((const char*)key, ARRAYSIZE(key));

    // Message to be encrypted
    unsigned char data[] = { 0x5f, 0x75, 0x8c, 0x11, 0x92, 0xdc, 0x56, 0xfb, 0x69, 0xe3, 0x40, 0x2d, 0x83, 0xfb, 0x75, 0xe4 };
    QByteArray ch_data((const char*)data, ARRAYSIZE(data));

    emit LOG_D("Received challenge:", true, true);
    emit LOG_D(parse_message_to_hex(ch_data), true, true);

    QByteArray output;
    emit LOG_D("Reply to challenge:", true, true);
    output = cipher.encrypt_aes128_ecb(ch_data, engine_key_1);
    emit LOG_D(parse_message_to_hex(output), true, true);

    emit LOG_D("Decrypted data is:", true, true);
    output = cipher.decrypt_aes128_ecb(output, engine_key_1);
    emit LOG_D(parse_message_to_hex(output), true, true);

}


void MainWindow::SetComboBoxItemEnabled(QComboBox * comboBox, int index, bool enabled)
{
    auto * model = qobject_cast<QStandardItemModel*>(comboBox->model());
    assert(model);
    if(!model) return;

    auto * item = model->item(index);
    assert(item);
    if(!item) return;
    item->setEnabled(enabled);
}

QStringList MainWindow::create_flash_transports_list()
{
    QStringList flash_protocols;

    flash_protocols.append(configValues->flash_protocol_flash_transport.at(configValues->flash_protocol_selected_id.toInt()).split(","));

    flash_transport_list->clear();
    for (int i = 0; i < flash_protocols.length(); i++){
        flash_transport_list->addItem(flash_protocols.at(i));
        if (configValues->flash_protocol_selected_flash_transport == flash_protocols.at(i))
            flash_transport_list->setCurrentIndex(i);
    }
    return flash_protocols;
}

QStringList MainWindow::create_log_transports_list()
{
    QStringList log_transports;

    log_transports.append(configValues->flash_protocol_log_transport.at(configValues->flash_protocol_selected_id.toInt()).split(","));

    log_transport_list->clear();
    for (int i = 0; i < log_transports.length(); i++){
        log_transport_list->addItem(log_transports.at(i));
        if (configValues->flash_protocol_selected_flash_transport == log_transports.at(i))
        {
            log_transport_list->setCurrentIndex(i);
            protocol = "SSM";
        }
    }

    //if (car_model_list->currentText() == "Subaru")
        //protocol = "SSM";

    return log_transports;
}

void MainWindow::select_protocol()
{
    //emit LOG_D("Select protocol", true, true);
    ProtocolSelect protocolSelect(configValues);
    connect(&protocolSelect, SIGNAL(finished (int)), this, SLOT(select_protocol_finished(int)));
    protocolSelect.exec();

    //QRect screenGeometry = this->geometry();
    //protocolSelect.move(screenGeometry.center() - protocolSelect.rect().center());
    //QCoreApplication::processEvents(QEventLoop::AllEvents, 10);

    emit LOG_D("Selected protocol: " + configValues->flash_protocol_selected_id, true, true);
    //status_bar_ecu_label->setText(configValues->flash_protocol_selected_description + " ");

}

void MainWindow::select_protocol_finished(int result)
{
    if(result == QDialog::Accepted)
    {
        create_flash_transports_list();
        create_log_transports_list();
        fileActions->save_config_file(configValues);

        set_flash_arrow_state();
    }
    else
    {
        //emit LOG_D("Dialog is rejected";
    }

    status_bar_ecu_label->setText(configValues->flash_protocol_selected_description + " ");
}

void MainWindow::select_vehicle()
{
    //emit LOG_D("Select protocol";
    VehicleSelect vehicleSelect(configValues);
    connect(&vehicleSelect, SIGNAL(finished (int)), this, SLOT(select_vehicle_finished(int)));
    vehicleSelect.exec();

    //QRect screenGeometry = this->geometry();
    //vehicleSelect.move(screenGeometry.center() - vehicleSelect.rect().center());
    //QCoreApplication::processEvents(QEventLoop::AllEvents, 10);

    emit LOG_D("Selected protocol: " + configValues->flash_protocol_selected_id, true, true);
    //status_bar_ecu_label->setText(configValues->flash_protocol_selected_description + " ");

}

void MainWindow::select_vehicle_finished(int result)
{
   if(result == QDialog::Accepted)
   {
        create_flash_transports_list();
        create_log_transports_list();
        fileActions->save_config_file(configValues);

        set_flash_arrow_state();
   }
   else
   {
       //emit LOG_D("Dialog is rejected";
   }

   status_bar_ecu_label->setText(configValues->flash_protocol_selected_description + " ");
}

void MainWindow::update_protocol_info(int rom_number)
{
    bool info_updated = false;

    emit LOG_D("Update protocol info by selected ROM with FlashMethod: " + ecuCalDef[rom_number]->RomInfo.at(fileActions->FlashMethod), true, true);
    for (int i = 0; i < configValues->flash_protocol_id.length(); i++)
    {
        if (configValues->flash_protocol_protocol_name.at(i) == ecuCalDef[rom_number]->RomInfo.at(fileActions->FlashMethod))
        {
            info_updated = true;
            emit LOG_D("Protocol name for selected ROM found: " + ecuCalDef[rom_number]->RomInfo.at(fileActions->FlashMethod) + " == " + configValues->flash_protocol_protocol_name.at(i), true, true);
            configValues->flash_protocol_selected_id = configValues->flash_protocol_id.at(i);
            configValues->flash_protocol_selected_make = configValues->flash_protocol_make.at(i);
            configValues->flash_protocol_selected_model = configValues->flash_protocol_model.at(i);
            configValues->flash_protocol_selected_version = configValues->flash_protocol_version.at(i);
            configValues->flash_protocol_selected_protocol_name = configValues->flash_protocol_protocol_name.at(i);
            configValues->flash_protocol_selected_description = configValues->flash_protocol_description.at(i);
            configValues->flash_protocol_selected_log_protocol = configValues->flash_protocol_log_protocol.at(i);
            configValues->flash_protocol_selected_mcu = configValues->flash_protocol_mcu.at(i);
            configValues->flash_protocol_selected_checksum = configValues->flash_protocol_checksum.at(i);
        }
    }
    if (info_updated)
        emit LOG_D("Protocol info for selected ROM updated", true, true);
    else
        emit LOG_D("Could not find protocol for selected ROM!", true, true);
    status_bar_ecu_label->setText(configValues->flash_protocol_selected_description + " ");
}

void MainWindow::set_flash_arrow_state()
{
    QList<QMenu*> menus = ui->menubar->findChildren<QMenu*>();
    foreach (QMenu *menu, menus) {
        foreach (QAction *action, menu->actions()) {
            if (action->isSeparator()) {

            } else if (action->menu()) {

            } else {
                if (action->text() == "Read from ecu")
                {
                    if (configValues->flash_protocol_read.at(configValues->flash_protocol_selected_id.toInt()) == "yes")
                        action->setEnabled(true);
                    else
                        action->setEnabled(false);
                }
                if (action->text() == "Test write to ecu")
                {
                    if (configValues->flash_protocol_test_write.at(configValues->flash_protocol_selected_id.toInt()) == "yes")
                        action->setEnabled(true);
                    else
                        action->setEnabled(false);
                }
                if (action->text() == "Write to ecu")
                {
                    if (configValues->flash_protocol_write.at(configValues->flash_protocol_selected_id.toInt()) == "yes")
                        action->setEnabled(true);
                    else
                        action->setEnabled(false);
                }

            }
        }
    }
}

void MainWindow::log_transport_changed()
{
    //emit LOG_D("Change log transport";
    QComboBox *log_transport_list = ui->toolBar->findChild<QComboBox*>("log_transport_list");

    serial->set_is_can_connection(false);
    serial->set_is_iso15765_connection(false);
    if (log_transport_list->currentText() == "CAN")
    {
        serial->set_is_can_connection(true);
        serial->set_is_iso15765_connection(false);
        serial->set_is_29_bit_id(false);
        serial->set_can_speed("500000");
    }
    else if (log_transport_list->currentText() == "iso15765")
    {
        serial->set_is_can_connection(false);
        serial->set_is_iso15765_connection(true);
        serial->set_is_29_bit_id(true);
        serial->set_can_speed("500000");
    }
    else if (log_transport_list->currentText() == "K-Line")
    {
        if (configValues->flash_protocol_selected_log_protocol == "SSM")
            serial->change_port_speed("4800");
    }

    protocol = configValues->flash_protocol_selected_log_protocol;
    configValues->flash_protocol_selected_log_transport = log_transport_list->currentText();
    fileActions->save_config_file(configValues);

    serial->reset_connection();
    ecuid.clear();
    ecu_init_complete = false;
    //ssm_init_poll_timer->start();
}

void MainWindow::flash_transport_changed()
{
    //emit LOG_D("Change flash transport";
    QComboBox *flash_transport_list = ui->toolBar->findChild<QComboBox*>("flash_transport_list");

    configValues->flash_protocol_selected_flash_transport = flash_transport_list->currentText();
    fileActions->save_config_file(configValues);
}

void MainWindow::check_serial_ports()
{
    QComboBox *serial_port_list = ui->toolBar->findChild<QComboBox*>("serial_port_list");
    QString prev_serial_port = serial_port_list->currentText();
    int index = 0;

    //serial_poll_timer->stop();
    //ssm_init_poll_timer->stop();

    serial->reset_connection();
    ecuid.clear();
    ecu_init_complete = false;
    serial->set_is_iso14230_connection(false);
    serial->set_is_29_bit_id(false);
    serial->set_add_iso14230_header(false);
    serial->set_is_can_connection(false);
    serial->set_is_iso15765_connection(false);
    serial->set_serial_port_baudrate("4800");
    emit log_transport_list->currentIndexChanged(log_transport_list->currentIndex());
    //if(configValues->flash_method != "subarucan" && configValues->flash_method != "subarucan_iso")

    //QStringList j2534_list = serial->getAvailableJ2534Libs();
    //emit LOG_D("J2534 Vehicle PassThru Interfaces:" << j2534_list;

    serial_ports = serial->check_serial_ports();
    serial_port_list->clear();

    for (int i = 0; i < serial_ports.length(); i++)
    {
        serial_port_list->addItem(serial_ports.at(i));
        if (prev_serial_port == serial_ports.at(i))
            serial_port_list->setCurrentIndex(index);
        index++;
    }

    //emit LOG_D("Start serial and ssm poll timers";
    //serial_poll_timer->start();
    //ssm_init_poll_timer->start();
}

void MainWindow::open_serial_port()
{
    if (serial_ports.length() > 0)
    {
        //QStringList serial_port = serial_ports.at(serial_port_list->currentIndex()).split(" - ");
        QStringList serial_port;
        serial_port.append(serial_ports.at(serial_port_list->currentIndex()));

        //emit LOG_D("Serial ports" << serial_ports;

        //if (serial_port.length() < 2)
        //    serial_port.append("Unknown");

        //emit LOG_D("Serial port" << serial_port;
        serial->set_serial_port_list(serial_port);
        QString opened_serial_port = serial->open_serial_port();
        if (opened_serial_port != "")
        {
            if (opened_serial_port != previous_serial_port)
            {
                ecuid.clear();
                ecu_init_complete = false;
            }
            //emit LOG_D("Serial port" << opened_serial_port << "opened" << previous_serial_port;
            previous_serial_port = opened_serial_port;
            configValues->serial_port = serial_port.at(0);
            fileActions->save_config_file(configValues);
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
}

int MainWindow::can_listener()
{
    QByteArray received;
    QByteArray output;

    emit LOG_D("Listen CANbus", true, true);
    QComboBox *serial_port_list = ui->toolBar->findChild<QComboBox*>("serial_port_list");
    if (serial_port_list->currentText() == "")
    {
        emit LOG_D("No serial port selected!", true, true);
        QMessageBox::warning(this, tr("Serial port"), "No serial port selected!");
        return 0;
    }

    // Stop serial timers
    //serial_poll_timer->stop();
    //ssm_init_poll_timer->stop();
    logging_poll_timer->stop();

    //serial->serial_port_list.clear();
    //serial->serial_port_list.append(serial_ports.at(serial_port_list->currentIndex()).split(" - ").at(0));
    //serial->serial_port_list.append(serial_ports.at(serial_port_list->currentIndex()).split(" - ").at(1));
    //QStringList spl = serial->get_serial_port_list();
    //spl.clear();
    //spl.append(serial_ports.at(serial_port_list->currentIndex()).split(" - ").at(0));
    //spl.append(serial_ports.at(serial_port_list->currentIndex()).split(" - ").at(1));

    QStringList spl;
    spl.append(serial_ports.at(serial_port_list->currentIndex()));

    serial->set_serial_port_list(spl);

    if (flash_transport_list->currentText() == "CAN")
    {
        serial->set_is_can_connection(true);
        serial->set_is_iso15765_connection(false);
        serial->set_is_29_bit_id(true);
        serial->set_can_speed("500000");
    }
    else if (flash_transport_list->currentText() == "iso15765")
    {
        serial->set_is_can_connection(false);
        serial->set_is_iso15765_connection(true);
        serial->set_is_29_bit_id(false);
        serial->set_can_speed("500000");
    }

    serial->set_iso15765_source_address(0x00);//0xFFFFFFFF;
    serial->set_iso15765_destination_address(0x00);//0xFFFFFFFF;
    serial->set_can_source_address(serial->get_iso15765_source_address());
    serial->set_can_destination_address(serial->get_iso15765_destination_address());
    // Open serial port
    serial->reset_connection();
    ecuid.clear();
    ecu_init_complete = false;
    serial->set_add_iso14230_header(false);
    open_serial_port();

    uint8_t id = 0xE0;

    output.clear();
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x07);
    output.append((uint8_t)id);
    output.append((uint8_t)0x01);
    output.append((uint8_t)0x00);

    while (can_listener_on)
    {
        serial->write_serial_data_echo_check(output);
        delay(200);
        received = serial->read_serial_data(receive_timeout);

        id++;
        if (id > 0xE8)
            id = 0xE0;
        output[3] = (uint8_t)id;
    }
    return 1;
}

int MainWindow::start_ecu_operations(QString cmd_type)
{
    set_realtime_state(false);
    toggle_realtime();

    int rom_number = 0;

    QTreeWidgetItem *selectedItem = NULL;
    int item_count = ui->calibrationFilesTreeWidget->selectedItems().count();

    QComboBox *serial_port_list = ui->toolBar->findChild<QComboBox*>("serial_port_list");
    if (serial_port_list->currentText() == "")
    {
        emit LOG_D("No serial port selected!", true, true);
        QMessageBox::warning(this, tr("Serial port"), "No serial port selected!");
        return 0;
    }

    // Stop serial timers
    logging_poll_timer->stop();

    QStringList spl;
    spl.append(serial_ports.at(serial_port_list->currentIndex()));

    serial->set_serial_port_list(spl);

    if (configValues->kernel_files_directory.at(configValues->kernel_files_directory.length() - 1) != '/')
        configValues->kernel_files_directory.append("/");

    if (configValues->flash_protocol_selected_make == "Subaru")
    {
        serial->reset_connection();
        ecuid.clear();
        ecu_init_complete = false;
        serial->set_is_iso14230_connection(false);
        serial->set_is_29_bit_id(false);
        serial->set_add_iso14230_header(false);
        serial->set_is_can_connection(false);
        serial->set_is_iso15765_connection(false);
        serial->set_serial_port_parity(QSerialPort::NoParity);
        serial->set_serial_port_baudrate("4800");

        update_vbatt();
        vbatt_timer->start();

        if (configValues->kernel_files_directory.at(configValues->kernel_files_directory.length() - 1) != '/')
            configValues->kernel_files_directory.append("/");

        QByteArray fullRomDataTmp;

        if (cmd_type == "test_write" || cmd_type == "write")
        {
            if (item_count > 0)
            {
                selectedItem = ui->calibrationFilesTreeWidget->selectedItems().at(0);
                rom_number = ui->calibrationFilesTreeWidget->indexOfTopLevelItem(selectedItem);
            }
            else
            {
                QMessageBox::warning(this, tr("Write ROM"), "No file selected!");
                return 0;
            }

            fullRomDataTmp = ecuCalDef[rom_number]->FullRomData;
            if (configValues->flash_protocol_selected_checksum == "n/a")
            {
                QMessageBox msgBox;
                msgBox.setIcon(QMessageBox::Warning);
                msgBox.setWindowTitle("Checksum warning");
                //msgBox.setDetailedText("Write Flash - Checksum Warning");
                msgBox.setText("WARNING! There is no checksum module for this ROM!\
                                    Be aware that if this ROM need checksum correction it must be done with another software!");
                QPushButton *cancelButton = msgBox.addButton(QMessageBox::Cancel);
                QPushButton *okButton = msgBox.addButton(QMessageBox::Ok);
                msgBox.exec();

                if (msgBox.clickedButton() == cancelButton)
                {
                    emit LOG_D("Write canceled!", true, true);
                    ecuCalDef[rom_number]->FullRomData = fullRomDataTmp;
                    return 0;
                }
            }
            if (ecuCalDef[rom_number]->RomInfo.at(fileActions->FlashMethod) == "")
            {
                ecuCalDef[rom_number]->RomInfo.replace(fileActions->FlashMethod, configValues->flash_protocol_selected_protocol_name);
                update_protocol_info(rom_number);
            }
            ecuCalDef[rom_number]->FlashMethod = configValues->flash_protocol_selected_protocol_name;
            ecuCalDef[rom_number]->Kernel = configValues->kernel_files_directory + configValues->flash_protocol_kernel.at(configValues->flash_protocol_selected_id.toInt()); //check_kernel(ecuCalDef[rom_number]->RomInfo.at(fileActions->FlashMethod));
            ecuCalDef[rom_number]->KernelStartAddr = configValues->flash_protocol_kernel_addr.at(configValues->flash_protocol_selected_id.toInt());
            ecuCalDef[rom_number]->McuType = configValues->flash_protocol_selected_mcu;

            if (configValues->flash_protocol_selected_checksum != "n/a")
                ecuCalDef[rom_number] = fileActions->checksum_correction(ecuCalDef[rom_number]);

            if (ecuCalDef[rom_number] == NULL)
            {
                ecuCalDef[rom_number]->FullRomData = fullRomDataTmp;
                return 0;
            }
        }
        else
        {
            rom_number = ecuCalDefIndex;
            ecuCalDef[rom_number] = new FileActions::EcuCalDefStructure;
            while (ecuCalDef[rom_number]->RomInfo.length() < ecuCalDef[rom_number]->RomInfoStrings.length()){
                ecuCalDef[rom_number]->RomInfo.append(" ");
            }
            ecuCalDef[rom_number]->RomInfo.replace(fileActions->FlashMethod, configValues->flash_protocol_selected_protocol_name);
            update_protocol_info(rom_number);
            ecuCalDef[rom_number]->FlashMethod = configValues->flash_protocol_selected_protocol_name;
            ecuCalDef[rom_number]->Kernel = configValues->kernel_files_directory + configValues->flash_protocol_kernel.at(configValues->flash_protocol_selected_id.toInt()); //check_kernel(ecuCalDef[rom_number]->RomInfo.at(fileActions->FlashMethod));
            ecuCalDef[rom_number]->KernelStartAddr = configValues->flash_protocol_kernel_addr.at(configValues->flash_protocol_selected_id.toInt());
            ecuCalDef[rom_number]->McuType = configValues->flash_protocol_selected_mcu;

        }

        emit LOG_D("Protocol to use: " + configValues->flash_protocol_selected_protocol_name, true, true);

        /*
        * Denso CAN
        */
        if (configValues->flash_protocol_selected_protocol_name.endsWith("_densocan") && configValues->flash_protocol_selected_protocol_name.contains("eeprom"))
        {
            EepromEcuSubaruDensoSH705xCan flash_module(serial, ecuCalDef[rom_number], cmd_type, this);
            connect_signals_and_run_module(&flash_module);
        }
        else if (configValues->flash_protocol_selected_protocol_name.endsWith("_densocan"))
        {
            FlashEcuSubaruDensoSH705xDensoCan flash_module(serial, ecuCalDef[rom_number], cmd_type, this);
            connect_signals_and_run_module(&flash_module);
        }
        /*
        * Denso ECU Boot Mode
        */

        /*
        * Denso ECU BDM
        */
        else if (configValues->flash_protocol_selected_protocol_name.startsWith("sub_ecu_denso_mc68hc16y5_02_bdm"))
        {
            FlashEcuSubaruDensoMC68HC16Y5_02_BDM flash_module(serial, ecuCalDef[rom_number], cmd_type, this);
            connect_signals_and_run_module(&flash_module);
        }
        /*
        * Denso ECU
        */
        else if (configValues->flash_protocol_selected_protocol_name.startsWith("sub_ecu_denso_mc68hc16y5_02"))
        {
            FlashEcuSubaruDensoMC68HC16Y5_02 flash_module(serial, ecuCalDef[rom_number], cmd_type, this);
            connect_signals_and_run_module(&flash_module);
        }
        else if (configValues->flash_protocol_selected_protocol_name.startsWith("sub_ecu_denso_mc68hc16y5_04"))
        {
            FlashEcuSubaruDensoMC68HC16Y5_02 flash_module(serial, ecuCalDef[rom_number], cmd_type, this);
            connect_signals_and_run_module(&flash_module);
        }
        else if (configValues->flash_protocol_selected_protocol_name.startsWith("sub_ecu_denso_sh7055_02"))
        {
            FlashEcuSubaruDensoSH7055_02 flash_module(serial, ecuCalDef[rom_number], cmd_type, this);
            connect_signals_and_run_module(&flash_module);
        }
        else if (configValues->flash_protocol_selected_protocol_name.startsWith("sub_ecu_denso_sh7055_04"))
        {
            FlashEcuSubaruDensoSH705xKline flash_module(serial, ecuCalDef[rom_number], cmd_type, this);
            connect_signals_and_run_module(&flash_module);
        }
        else if (configValues->flash_protocol_selected_protocol_name.startsWith("sub_ecu_denso_sh72543_can_diesel"))
        {
            FlashEcuSubaruDensoSH72543CanDiesel flash_module(serial, ecuCalDef[rom_number], cmd_type, this);
            connect_signals_and_run_module(&flash_module);
        }
        else if (configValues->flash_protocol_selected_protocol_name.startsWith("sub_ecu_denso_sh7058_can_diesel"))
        {
            FlashEcuSubaruDensoSH7058CanDiesel flash_module(serial, ecuCalDef[rom_number], cmd_type, this);
            connect_signals_and_run_module(&flash_module);
        }
        else if (configValues->flash_protocol_selected_protocol_name.startsWith("sub_ecu_denso_sh7059_can_diesel"))
        {
            FlashEcuSubaruDensoSH7058CanDiesel flash_module(serial, ecuCalDef[rom_number], cmd_type, this);
            connect_signals_and_run_module(&flash_module);
        }
        else if (configValues->flash_protocol_selected_protocol_name.startsWith("sub_ecu_denso_sh7058_can"))
        {
            FlashEcuSubaruDensoSH7058Can flash_module(serial, ecuCalDef[rom_number], cmd_type, this);
            connect_signals_and_run_module(&flash_module);
        }
        else if (configValues->flash_protocol_selected_protocol_name.startsWith("sub_ecu_denso_sh7058"))
        {
            FlashEcuSubaruDensoSH705xKline flash_module(serial, ecuCalDef[rom_number], cmd_type, this);
            connect_signals_and_run_module(&flash_module);
        }
        else if (configValues->flash_protocol_selected_protocol_name.startsWith("sub_ecu_denso_sh72531_can"))
        {
            FlashEcuSubaruDensoSH72531Can flash_module(serial, ecuCalDef[rom_number], cmd_type, this);
            connect_signals_and_run_module(&flash_module);
        }
        else if (configValues->flash_protocol_selected_protocol_name.startsWith("sub_ecu_denso_1n83m_4m_can"))
        {
            FlashEcuSubaruDenso1N83M_4MCan flash_module(serial, ecuCalDef[rom_number], cmd_type, this);
            connect_signals_and_run_module(&flash_module);
        }
        else if (configValues->flash_protocol_selected_protocol_name.startsWith("sub_ecu_denso_1n83m_1_5m_can"))
        {
            FlashEcuSubaruDenso1N83M_1_5MCan flash_module(serial, ecuCalDef[rom_number], cmd_type, this);
            connect_signals_and_run_module(&flash_module);
        }
        /*
        * Denso TCU
        */
        else if (configValues->flash_protocol_selected_protocol_name.startsWith("sub_tcu_denso_sh7055_can"))
        {
            FlashTcuSubaruDensoSH705xCan flash_module(serial, ecuCalDef[rom_number], cmd_type, this);
            connect_signals_and_run_module(&flash_module);
        }
        else if (configValues->flash_protocol_selected_protocol_name.startsWith("sub_tcu_denso_sh7058_can"))
        {
            FlashTcuSubaruDensoSH705xCan flash_module(serial, ecuCalDef[rom_number], cmd_type, this);
            connect_signals_and_run_module(&flash_module);
        }
        /*
        * Denso EEPROM
        */
        else if (configValues->flash_protocol_selected_protocol_name.startsWith("sub_ecu_eeprom_denso_sh7055_kline"))
        {
            EepromEcuSubaruDensoSH705xKline flash_module(serial, ecuCalDef[rom_number], cmd_type, this);
            connect_signals_and_run_module(&flash_module);
        }
        else if (configValues->flash_protocol_selected_protocol_name.startsWith("sub_ecu_eeprom_denso_sh7058_kline"))
        {
            EepromEcuSubaruDensoSH705xKline flash_module(serial, ecuCalDef[rom_number], cmd_type, this);
            connect_signals_and_run_module(&flash_module);
        }
        else if (configValues->flash_protocol_selected_protocol_name.startsWith("sub_ecu_eeprom_denso_sh7055_densocan"))
        {
            EepromEcuSubaruDensoSH705xCan flash_module(serial, ecuCalDef[rom_number], cmd_type, this);
            connect_signals_and_run_module(&flash_module);
        }
        else if (configValues->flash_protocol_selected_protocol_name.startsWith("sub_ecu_eeprom_denso_sh7058_densocan"))
        {
            EepromEcuSubaruDensoSH705xCan flash_module(serial, ecuCalDef[rom_number], cmd_type, this);
            connect_signals_and_run_module(&flash_module);
        }
        else if (configValues->flash_protocol_selected_protocol_name.startsWith("sub_ecu_eeprom_denso_sh7058_can"))
        {
            EepromEcuSubaruDensoSH705xCan flash_module(serial, ecuCalDef[rom_number], cmd_type, this);
            connect_signals_and_run_module(&flash_module);
        }
        else if (configValues->flash_protocol_selected_protocol_name.startsWith("sub_ecu_eeprom_denso_sh7058_can_diesel"))
        {
            EepromEcuSubaruDensoSH705xCan flash_module(serial, ecuCalDef[rom_number], cmd_type, this);
            connect_signals_and_run_module(&flash_module);
        }
        /*
        * Unisia Jecs ECU Bootmode
        */
        else if (configValues->flash_protocol_selected_protocol_name.startsWith("sub_ecu_unisia_jecs_20_bootmode"))
        {
            FlashEcuSubaruUnisiaJecsM32rBootMode flash_module(serial, ecuCalDef[rom_number], cmd_type, this);
            connect_signals_and_run_module(&flash_module);
        }
        else if (configValues->flash_protocol_selected_protocol_name.startsWith("sub_ecu_unisia_jecs_30_bootmode"))
        {
            FlashEcuSubaruUnisiaJecsM32rBootMode flash_module(serial, ecuCalDef[rom_number], cmd_type, this);
            connect_signals_and_run_module(&flash_module);
        }

        /*
        * Unisia Jecs ECU
        */
        else if (configValues->flash_protocol_selected_protocol_name.startsWith("sub_ecu_unisia_jecs_m3779x"))
        {
            FlashEcuSubaruUnisiaJecs flash_module(serial, ecuCalDef[rom_number], cmd_type, this);
            connect_signals_and_run_module(&flash_module);
        }
        else if (configValues->flash_protocol_selected_protocol_name.startsWith("sub_ecu_unisia_jecs_m3775x"))
        {
            FlashEcuSubaruUnisiaJecs flash_module(serial, ecuCalDef[rom_number], cmd_type, this);
            connect_signals_and_run_module(&flash_module);
        }
        else if (configValues->flash_protocol_selected_protocol_name.startsWith("sub_ecu_unisia_jecs_20"))
        {
            FlashEcuSubaruUnisiaJecsM32r flash_module(serial, ecuCalDef[rom_number], cmd_type, this);
            connect_signals_and_run_module(&flash_module);
        }
        else if (configValues->flash_protocol_selected_protocol_name.startsWith("sub_ecu_unisia_jecs_30"))
        {
            FlashEcuSubaruUnisiaJecsM32r flash_module(serial, ecuCalDef[rom_number], cmd_type, this);
            connect_signals_and_run_module(&flash_module);
        }
        else if (configValues->flash_protocol_selected_protocol_name.startsWith("sub_ecu_unisia_jecs_40"))
        {
            FlashEcuSubaruUnisiaJecsM32r flash_module(serial, ecuCalDef[rom_number], cmd_type, this);
            connect_signals_and_run_module(&flash_module);
        }
        else if (configValues->flash_protocol_selected_protocol_name.startsWith("sub_ecu_unisia_jecs_70"))
        {
            FlashEcuSubaruUnisiaJecsM32r flash_module(serial, ecuCalDef[rom_number], cmd_type, this);
            connect_signals_and_run_module(&flash_module);
        }
        /*
        * Hitachi ECU Boot Mode
        */

        /*
        * Hitachi ECU JTAG
        */
        else if (configValues->flash_protocol_selected_protocol_name.startsWith("sub_ecu_hitachi_m32r_jtag"))
        {
            FlashEcuSubaruHitachiM32rJtag flash_module(serial, ecuCalDef[rom_number], cmd_type, this);
            connect_signals_and_run_module(&flash_module);
        }
        /*
        * Hitachi ECU
        */
        else if (configValues->flash_protocol_selected_protocol_name.startsWith("sub_ecu_hitachi_m32r_kline"))
        {
            FlashEcuSubaruHitachiM32rKline flash_module(serial, ecuCalDef[rom_number], cmd_type, this);
            connect_signals_and_run_module(&flash_module);
        }
        else if (configValues->flash_protocol_selected_protocol_name.startsWith("sub_ecu_hitachi_m32r_can"))
        {
            FlashEcuSubaruHitachiM32rCan flash_module(serial, ecuCalDef[rom_number], cmd_type, this);
            connect_signals_and_run_module(&flash_module);
        }
        else if (configValues->flash_protocol_selected_protocol_name.startsWith("sub_ecu_mitsu_m32r_kline"))
        {
            FlashEcuSubaruMitsuM32rKline flash_module(serial, ecuCalDef[rom_number], cmd_type, this);
            connect_signals_and_run_module(&flash_module);
        }
        else if (configValues->flash_protocol_selected_protocol_name.startsWith("sub_ecu_hitachi_sh7058_can"))
        {
            FlashEcuSubaruHitachiSH7058Can flash_module(serial, ecuCalDef[rom_number], cmd_type, this);
            connect_signals_and_run_module(&flash_module);
        }
        else if (configValues->flash_protocol_selected_protocol_name.startsWith("sub_ecu_hitachi_sh72543r_can"))
        {
            FlashEcuSubaruHitachiSH72543rCan flash_module(serial, ecuCalDef[rom_number], cmd_type, this);
            connect_signals_and_run_module(&flash_module);
        }
        /*
        * Hitachi TCU
        */
        else if (configValues->flash_protocol_selected_protocol_name.startsWith("sub_tcu_hitachi_m32r_kline"))
        {
            FlashTcuSubaruHitachiM32rKline flash_module(serial, ecuCalDef[rom_number], cmd_type, this);
            connect_signals_and_run_module(&flash_module);
        }
        else if (configValues->flash_protocol_selected_protocol_name.startsWith("sub_tcu_hitachi_m32r_can"))
        {
            FlashTcuSubaruHitachiM32rCan flash_module(serial, ecuCalDef[rom_number], cmd_type, this);
            connect_signals_and_run_module(&flash_module);
        }
        else if (configValues->flash_protocol_selected_protocol_name.startsWith("sub_tcu_cvt_hitachi_m32r_can"))
        {
            FlashTcuCvtSubaruHitachiM32rCan flash_module(serial, ecuCalDef[rom_number], cmd_type, this);
            connect_signals_and_run_module(&flash_module);
        }
        /*
        * Mitsu TCU
        */
        else if (configValues->flash_protocol_selected_protocol_name.startsWith("sub_tcu_cvt_mitsu_mh8104_can"))
        {
            FlashTcuCvtSubaruMitsuMH8104Can flash_module(serial, ecuCalDef[rom_number], cmd_type, this);
            connect_signals_and_run_module(&flash_module);
        }
        else if (configValues->flash_protocol_selected_protocol_name.startsWith("sub_tcu_cvt_mitsu_mh8111_can"))
        {
            FlashTcuCvtSubaruMitsuMH8111Can flash_module(serial, ecuCalDef[rom_number], cmd_type, this);
            connect_signals_and_run_module(&flash_module);
        }

        /*
        * Unknown flashmethod
        */
        else
            QMessageBox::warning(this, tr("Unknown flashmethod"), "Unknown flashmethod! Flashmethod \"" + configValues->flash_protocol_selected_protocol_name + "\" not yet implemented!");
            //ecuOperationsSubaru = new EcuOperationsSubaru(serial, ecuCalDef[rom_number], cmd_type, this);

        if (cmd_type == "read")
        {
            if (ecuCalDef[ecuCalDefIndex]->FullRomData.length())
            {
                QDateTime dateTime = dateTime.currentDateTime();
                QString dateTimeString = dateTime.toString("yyyy-MM-dd_hh'h'mm'm'ss's'");

                if (ecuCalDef[ecuCalDefIndex]->RomId.length())
                    ecuCalDef[ecuCalDefIndex]->FileName = ecuCalDef[ecuCalDefIndex]->RomId + dateTimeString + ".bin";
                else
                    ecuCalDef[ecuCalDefIndex]->FileName = "read_image_" + dateTimeString + ".bin";

                //emit LOG_D("Checking definitions, please wait...";
                fileActions->open_subaru_rom_file(ecuCalDef[ecuCalDefIndex], ecuCalDef[ecuCalDefIndex]->FileName);
                update_protocol_info(ecuCalDefIndex);

                //emit LOG_D("Building treewidget, please wait...";
                calibrationTreeWidget->buildCalibrationFilesTree(ecuCalDefIndex, ui->calibrationFilesTreeWidget, ecuCalDef[ecuCalDefIndex]);
                calibrationTreeWidget->buildCalibrationDataTree(ui->calibrationDataTreeWidget, ecuCalDef[ecuCalDefIndex]);

                ecuCalDefIndex++;
                save_calibration_file_as();
            }
        }
        else
            ecuCalDef[rom_number]->FullRomData = fullRomDataTmp;

    }
    vbatt_timer->stop();

    serial->reset_connection();
    ecuid.clear();
    ecu_init_complete = false;
    serial->set_is_iso14230_connection(false);
    serial->set_is_29_bit_id(false);
    serial->set_add_iso14230_header(false);
    serial->set_is_can_connection(false);
    serial->set_is_iso15765_connection(false);
    serial->set_serial_port_parity(QSerialPort::NoParity);
    serial->set_serial_port_baudrate("4800");
    emit log_transport_list->currentIndexChanged(log_transport_list->currentIndex());
    serial->change_port_speed("4800");

    return 0;
}

void MainWindow::custom_menu_requested(QPoint pos)
{
    if (ecuCalDefIndex == 0){
        emit LOG_D("No calibration to select!", true, true);
        return;
    }
    QModelIndex index = ui->calibrationFilesTreeWidget->indexAt(pos);
    emit ui->calibrationFilesTreeWidget->clicked(index);

    QMenu *menu = new QMenu(this);
    //menu->addAction("Sync with ECU", this, SLOT(syncCalWithEcu()));
    menu->addAction("Close selected ROM", this, SLOT(close_calibration()));
    menu->popup(ui->calibrationFilesTreeWidget->viewport()->mapToGlobal(pos));

}

bool MainWindow::open_calibration_file(QString filename)
{
    ecuCalDef[ecuCalDefIndex] = new FileActions::EcuCalDefStructure;

    while (ecuCalDef[ecuCalDefIndex]->RomInfo.length() < ecuCalDef[ecuCalDefIndex]->RomInfoStrings.length())
        ecuCalDef[ecuCalDefIndex]->RomInfo.append(" ");

    ecuCalDef[ecuCalDefIndex] = fileActions->open_subaru_rom_file(ecuCalDef[ecuCalDefIndex], filename);
    if(ecuCalDef[ecuCalDefIndex] != NULL)
    {
        update_protocol_info(ecuCalDefIndex);

        calibrationTreeWidget->buildCalibrationFilesTree(ecuCalDefIndex, ui->calibrationFilesTreeWidget, ecuCalDef[ecuCalDefIndex]);
        calibrationTreeWidget->buildCalibrationDataTree(ui->calibrationDataTreeWidget, ecuCalDef[ecuCalDefIndex]);

        ecuCalDefIndex++;
    }
    else
    {
        //emit LOG_D("Failed!!";
        ecuCalDef[ecuCalDefIndex] = {};
        return 1;
    }
    return 0;
}

void MainWindow::save_calibration_file()
{
    if (ecuCalDefIndex == 0){
        QMessageBox::information(this, tr("Calibration file"), "No calibration to save!");
        return;
    }

    QTreeWidgetItem *selectedItem = ui->calibrationFilesTreeWidget->selectedItems().at(0);

    int rom_number = ui->calibrationFilesTreeWidget->indexOfTopLevelItem(selectedItem);

    QByteArray fullRomDataTmp = ecuCalDef[rom_number]->FullRomData;

    //update_protocol_info(rom_number);
/*
    if (!ecuCalDef[rom_number]->use_romraider_definition && !ecuCalDef[rom_number]->use_ecuflash_definition)
    {
        QMessageBox msgBox;
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setWindowTitle("Calibration file");
        msgBox.setText("WARNING! No definition file linked to selected ROM, checksums are not calculated!\n\n"
                        "If you are sure that right protocol is selected and want to correct checksums anyway, press 'DO IT!' -button");
        QPushButton *okButton = msgBox.addButton(QMessageBox::Ok);
        QPushButton *doItButton = msgBox.addButton(tr("DO IT!"), QMessageBox::NoRole);
        msgBox.exec();

        if (msgBox.clickedButton() == doItButton)
            ecuCalDef[rom_number] = fileActions->checksum_correction(ecuCalDef[rom_number]);
    }
    else
        ecuCalDef[rom_number] = fileActions->checksum_correction(ecuCalDef[rom_number]);
*/
    ecuCalDef[rom_number] = fileActions->checksum_correction(ecuCalDef[rom_number]);

    if (ecuCalDef[rom_number] != NULL)
        fileActions->save_subaru_rom_file(ecuCalDef[rom_number], ecuCalDef[rom_number]->FullFileName);

    emit LOG_D("ecuCalDef->FileName: " + ecuCalDef[rom_number]->FileName, true, true);
    emit LOG_D("ecuCalDef->FullFileName: " + ecuCalDef[rom_number]->FullFileName, true, true);

    ecuCalDef[rom_number]->FullRomData = fullRomDataTmp;
}

void MainWindow::save_calibration_file_as()
{
    if (ecuCalDefIndex == 0){
        QMessageBox::information(this, tr("Calibration file"), "No calibration to save!");
        return;
    }

    QTreeWidgetItem *selectedItem = ui->calibrationFilesTreeWidget->selectedItems().at(0);

    emit LOG_D("Save as: Check selected ROM number", true, true);
    int rom_number = ui->calibrationFilesTreeWidget->indexOfTopLevelItem(selectedItem);

    QByteArray fullRomDataTmp = ecuCalDef[rom_number]->FullRomData;

    //update_protocol_info(rom_number);
/*
    if (!ecuCalDef[rom_number]->use_romraider_definition && !ecuCalDef[rom_number]->use_ecuflash_definition)
    {
        QMessageBox msgBox;
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setWindowTitle("Calibration file");
        msgBox.setText("WARNING! No definition file linked to selected ROM, checksums are not calculated!\n\n"
                            "If you are sure that right protocol is selected and want to correct checksums anyway, press 'DO IT!' -button");
        QPushButton *okButton = msgBox.addButton(QMessageBox::Ok);
        QPushButton *doItButton = msgBox.addButton(tr("DO IT!"), QMessageBox::NoRole);
        msgBox.exec();

        if (msgBox.clickedButton() == doItButton)
            ecuCalDef[rom_number] = fileActions->checksum_correction(ecuCalDef[rom_number]);
    }
    else
        ecuCalDef[rom_number] = fileActions->checksum_correction(ecuCalDef[rom_number]);
*/
    ecuCalDef[rom_number] = fileActions->checksum_correction(ecuCalDef[rom_number]);

    if (ecuCalDef[rom_number] != NULL)
    {
        QString filename = ui->calibrationFilesTreeWidget->selectedItems().at(0)->text(0);
        //QString filename = "";

        QFileDialog saveDialog;
        saveDialog.setDefaultSuffix("bin");
        emit LOG_D("Save as: Check if OEM ECU file", true, true);

        filename = QFileDialog::getSaveFileName(this, tr("Save calibration file"), configValues->calibration_files_directory + filename, tr("Calibration file (*.bin)"));

        if (filename.isEmpty()){
            //ecuCalDef[rom_number]->FileName = "No name.bin";
            ui->calibrationFilesTreeWidget->selectedItems().at(0)->setText(0, ecuCalDef[rom_number]->FileName);
            QMessageBox::information(this, tr("Calibration file"), "No file name selected");
            ecuCalDef[rom_number]->FullRomData = fullRomDataTmp;
            return;
        }

        if(filename.endsWith(QString(".")))
            filename.remove(filename.length() - 1, 1);
        if(!filename.endsWith(QString(".bin")))
             filename.append(QString(".bin"));

        fileActions->save_subaru_rom_file(ecuCalDef[rom_number], filename);
        ui->calibrationFilesTreeWidget->selectedItems().at(0)->setText(0, ecuCalDef[rom_number]->FileName);
        emit LOG_D("ecuCalDef->FileName: " + ecuCalDef[rom_number]->FileName, true, true);
        emit LOG_D("ecuCalDef->FullFileName: " + ecuCalDef[rom_number]->FullFileName, true, true);
    }
    ecuCalDef[rom_number]->FullRomData = fullRomDataTmp;
}

void MainWindow::selectable_combobox_item_changed(QString item)
{
    int mapRomNumber = 0;
    int mapNumber = 0;
    bool bStatus = false;

    //bool ok = false;

    QMdiSubWindow* w = ui->mdiArea->activeSubWindow();
    if (w)
    {
        QStringList mapWindowString = w->objectName().split(",");
        mapRomNumber = mapWindowString.at(0).toInt();
        mapNumber = mapWindowString.at(1).toInt();

        QTableWidget* mapTableWidget = w->findChild<QTableWidget*>(w->objectName());
        if (mapTableWidget)
        {
            QStringList selectionsNameList = ecuCalDef[mapRomNumber]->SelectionsNameList.at(mapNumber).split(",");
            QStringList selectionsValueList = ecuCalDef[mapRomNumber]->SelectionsValueList.at(mapNumber).split(",");

            for (int j = 0; j < selectionsNameList.length(); j++){
                if (selectionsNameList.at(j) == item)
                {
                    //emit LOG_D("Old selectable value was: " + ecuCalDef[mapRomNumber]->MapData.at(mapNumber), true, true);
                    ecuCalDef[mapRomNumber]->MapData.replace(mapNumber, selectionsValueList.at(j));
                    //emit LOG_D("New selectable value is: " + ecuCalDef[mapRomNumber]->MapData.at(mapNumber), true, true);

                    if (ecuCalDef[mapRomNumber]->StorageTypeList.at(mapNumber) == "bloblist")
                    {
                        uint8_t storagesize = 0;
                        uint8_t dataByte = 0;
                        uint32_t byteAddress = ecuCalDef[mapRomNumber]->AddressList.at(mapNumber).toUInt(&bStatus,16);
                        storagesize = ecuCalDef[mapRomNumber]->SelectionsValueList.at(mapNumber).split(",").at(0).length() / 2;
                        for (int k = 0; k < storagesize; k++)
                        {
                            dataByte = ecuCalDef[mapRomNumber]->MapData.at(mapNumber).mid(k*2, 2).toUInt(&bStatus, 16);
                            ecuCalDef[mapRomNumber]->FullRomData[byteAddress+k] = dataByte;
                        }
                    }
                }
            }
        }
    }
}

void MainWindow::checkbox_state_changed(int state)
{

    bool bStatus;

    int map_rom_number = 0;
    int map_number = 0;

    QMdiSubWindow* w = ui->mdiArea->activeSubWindow();
    if (w)
    {
        QStringList mapWindowString = w->objectName().split(",");
        map_rom_number = mapWindowString.at(0).toInt();
        map_number = mapWindowString.at(1).toInt();

        QTableWidget* mapTableWidget = w->findChild<QTableWidget*>(w->objectName());
        if (mapTableWidget)
        {
            QStringList switch_states = ecuCalDef[map_rom_number]->StateList.at(map_number).split(",");
            int switch_states_length = (switch_states.length() - 1);
            for (int i = 0; i < switch_states_length; i += 2)
            {
                QStringList switch_data = switch_states.at(i + 1).split(" ");
                uint32_t byte_address = ecuCalDef[map_rom_number]->AddressList.at(map_number).toUInt(&bStatus, 16);
                if (ecuCalDef[map_rom_number]->RomInfo.at(fileActions->FlashMethod) == "wrx02" && ecuCalDef[map_rom_number]->FileSize.toUInt() < (170 * 1024) && byte_address > 0x27FFF)
                    byte_address -= 0x8000;
                if ((switch_states.at(i) == "off" || switch_states.at(i) == "disabled") && state == 0)
                {
                    for (int j = 0; j < switch_data.length(); j++)
                    {
                        emit LOG_D("Old switch value " + QString::number(j) + " is: " + ecuCalDef[map_rom_number]->FullRomData[byte_address + j], true, true);
                        ecuCalDef[map_rom_number]->FullRomData[byte_address + j] = (uint8_t)switch_data.at(j).toUInt();
                        emit LOG_D("New switch value " + QString::number(j) + " is: " + ecuCalDef[map_rom_number]->FullRomData[byte_address + j], true, true);
                    }
                }
                if ((switch_states.at(i) == "on" || switch_states.at(i) == "enabled") && state == 2)
                {
                    for (int j = 0; j < switch_data.length(); j++)
                    {
                        emit LOG_D("Old switch value" + QString::number(j) + " is: " + ecuCalDef[map_rom_number]->FullRomData[byte_address + j], true, true);
                        ecuCalDef[map_rom_number]->FullRomData[byte_address + j] = (uint8_t)switch_data.at(j).toUInt();
                        emit LOG_D("New switch value" + QString::number(j) + " is: " + ecuCalDef[map_rom_number]->FullRomData[byte_address + j], true, true);
                    }
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

    int rom_number = ui->calibrationFilesTreeWidget->indexOfTopLevelItem(selectedItem);

    selectedItem->setCheckState(0, Qt::Checked);

    QString romId = selectedItem->text(1);

    calibrationTreeWidget->buildCalibrationDataTree(ui->calibrationDataTreeWidget, ecuCalDef[rom_number]);
    update_protocol_info(rom_number);
/*
    QComboBox *flash_method_list = ui->toolBar->findChild<QComboBox*>("flash_method_list");
    for (int i = 0; i < flash_method_list->count(); i++)
    {
        if(ecuCalDef[romNumber]->RomInfo.at(fileActions->FlashMethod) == flash_method_list->itemText(i))
        {
            flash_method_list->setCurrentIndex(i);
        }
    }
    */
}

void MainWindow::calibration_data_treewidget_item_selected(QTreeWidgetItem* item)
{
    const QModelIndex index = ui->calibrationDataTreeWidget->selectionModel()->currentIndex();
    QString selectedText = index.data(Qt::DisplayRole).toString();
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
        hierarchyLevel = 1;
    }
    else if (ui->calibrationDataTreeWidget->indexOfTopLevelItem(item->parent()) > -1){
        hierarchyLevel = 2;

        QTreeWidgetItem *selectedFilesTreeItem = ui->calibrationFilesTreeWidget->selectedItems().at(0);
        QTreeWidgetItem *selectedDataTreeItem = item;
        int romNumber = ui->calibrationFilesTreeWidget->indexOfTopLevelItem(selectedFilesTreeItem);
        int mapIndex = selectedDataTreeItem->text(1).toInt();
        int romIndex = selectedFilesTreeItem->text(2).toInt();
        for (int i = 0; i < ecuCalDef[romNumber]->NameList.count(); i++)
        {
            if (ecuCalDef[romNumber]->NameList.at(i) == selectedText && i == mapIndex)
            {
                if (ecuCalDef[romNumber]->VisibleList.at(i) == "1")
                {
                    int map_index = 0;
                    QList<QMdiSubWindow *> list = ui->mdiArea->findChildren<QMdiSubWindow *>();
                    foreach(QMdiSubWindow *w, list)
                    {
                        map_index++;
                        if (w->objectName().startsWith(QString::number(romIndex) + "," + QString::number(i) + "," + ecuCalDef[romNumber]->NameList.at(i)))
                        {
                            if (w->objectName() == ui->mdiArea->activeSubWindow()->objectName())
                            {
                                ui->mdiArea->removeSubWindow(w);
                                ecuCalDef[romNumber]->VisibleList.replace(i, "0");
                                item->setCheckState(0, Qt::Unchecked);
                            }
                            else
                            {
                                w->setFocus();
                                item->setCheckState(0, Qt::Checked);

                            }
                        }
                    }
                }
                else
                {
                    ecuCalDef[romNumber]->VisibleList.replace(i, "1");
                    item->setCheckState(0, Qt::Checked);

                    CalibrationMaps *calibrationMaps = new CalibrationMaps(ecuCalDef[romNumber], romIndex, i, ui->mdiArea->contentsRect());
                    QMdiSubWindow *subWindow = ui->mdiArea->addSubWindow(calibrationMaps);
                    if (subWindow)
                    {
                        subWindow->setAttribute(Qt::WA_DeleteOnClose, true);
                        subWindow->setObjectName(calibrationMaps->objectName());
                        subWindow->show();
                        subWindow->adjustSize();
                        subWindow->move(0,0);
                        //subWindow->setFixedWidth(subWindow->width());
                        //subWindow->setFixedHeight(subWindow->height());

                        connect(calibrationMaps, SIGNAL(selectable_combobox_item_changed(QString)), this, SLOT(selectable_combobox_item_changed(QString)));
                        connect(calibrationMaps, SIGNAL(checkbox_state_changed(int)), this, SLOT(checkbox_state_changed(int)));
                        connect(subWindow, SIGNAL(destroyed(QObject*)), this, SLOT(close_calibration_map(QObject*)));
                    }
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

    calibrationTreeWidget->calibrationDataTreeWidgetItemExpanded(ecuCalDef[romNumber], categoryName);
}

void MainWindow::calibration_data_treewidget_item_collapsed(QTreeWidgetItem* item)
{
    QTreeWidgetItem *selectedItem = ui->calibrationFilesTreeWidget->selectedItems().at(0);

    int itemIndex = ui->calibrationDataTreeWidget->indexOfTopLevelItem(item);
    int romNumber = ui->calibrationFilesTreeWidget->indexOfTopLevelItem(selectedItem);
    QString categoryName = ui->calibrationDataTreeWidget->topLevelItem(itemIndex)->text(0);

    calibrationTreeWidget->calibrationDataTreeWidgetItemCollapsed(ecuCalDef[romNumber], categoryName);
}

void MainWindow::close_calibration()
{
    QTreeWidgetItem *selectedItem = ui->calibrationFilesTreeWidget->selectedItems().at(0);

    int romNumber = ui->calibrationFilesTreeWidget->indexOfTopLevelItem(selectedItem);
    int romIndex = ui->calibrationFilesTreeWidget->selectedItems().at(0)->text(2).toInt();

    for (int i = 0; i < ui->calibrationDataTreeWidget->topLevelItemCount(); i++)
    {
        for (int j = 0; j < ui->calibrationDataTreeWidget->topLevelItem(i)->childCount(); j++)
        {
            for (int k = 0; k < ui->mdiArea->subWindowList().count(); k++)
            {
                int mapNumber = ui->calibrationDataTreeWidget->topLevelItem(i)->child(j)->text(1).toInt();
                QString mapName = ui->calibrationDataTreeWidget->topLevelItem(i)->child(j)->text(0);
                QString mapType = ecuCalDef[romNumber]->TypeList.at(mapNumber);

                QString calMapWindowName = QString::number(romIndex) + "," + QString::number(mapNumber) + "," + mapName;
                QMdiSubWindow *w = ui->mdiArea->subWindowList().at(k);
                if (w->objectName().startsWith(calMapWindowName))
                {
                    ui->mdiArea->removeSubWindow(w);
                }

            }

        }
    }
    delete ui->calibrationFilesTreeWidget->takeTopLevelItem(romNumber);

    ecuCalDefIndex--;
    for (int i = romNumber; i < ecuCalDefIndex; i++)
    {
        if (ecuCalDefIndex > romNumber)
        {
            ecuCalDef[i] = ecuCalDef[i + 1];
            int rom_index = ui->calibrationFilesTreeWidget->topLevelItem(i)->text(2).toInt();
            ui->calibrationFilesTreeWidget->topLevelItem(i)->setText(2, QString::number(rom_index - 1));
        }
    }
    ecuCalDef[ecuCalDefIndex] = {};

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
    //configValues->calibration_files.removeAt(romNumber);
    //fileActions->save_config_file(configValues);
}

void MainWindow::close_calibration_map(QObject* obj)
{
    QStringList mapWindowString = obj->objectName().split(",");
    int mapRomNumber = mapWindowString.at(0).toInt();
    QString mapName = mapWindowString.at(2);

    for (int i = 0; i < ui->calibrationFilesTreeWidget->topLevelItemCount(); i++)
    {
        ui->calibrationFilesTreeWidget->topLevelItem(i)->setSelected(false);
    }
    QTreeWidgetItem *currentItem = ui->calibrationFilesTreeWidget->topLevelItem(mapRomNumber);
    currentItem->setSelected(true);
    const QModelIndex index = ui->calibrationFilesTreeWidget->selectionModel()->currentIndex();
    emit ui->calibrationFilesTreeWidget->clicked(index);

    QTreeWidgetItem *selectedItem = ui->calibrationFilesTreeWidget->selectedItems().at(0);
    int romIndex = selectedItem->text(2).toInt();

    QTreeWidgetItem *item;
    for(int i = 0 ; i < ui->calibrationDataTreeWidget->topLevelItemCount(); i++)
    {
        for(int j = 0 ; j < ui->calibrationDataTreeWidget->topLevelItem(i)->childCount(); j++)
        {
            item = ui->calibrationDataTreeWidget->topLevelItem(i)->child(j);
            if (item->text(0) == mapName)
            {
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
    close_app();
}

void MainWindow::close_app()
{
    FileActions::ConfigValuesStructure *configValues = &fileActions->ConfigValuesStruct;

    qApp->exit();
}

void MainWindow::change_gauge_values()
{
    change_log_values(0, protocol);
    //if (ecu_init_complete)
    //    fileActions->read_logger_conf(logValues, ecuid, true);
}

void MainWindow::change_digital_values()
{
    change_log_values(1, protocol);
}

void MainWindow::change_switch_values()
{
    change_log_values(2, protocol);
}

void MainWindow::update_logboxes(QString protocol)
{
    int switchBoxCount = 20;
    int logBoxCount = 12;

    emit LOG_D("Update logboxes with protocol: " + protocol, true, true);

    while(!ui->switchBoxLayout->isEmpty()) {
        QWidget *wg = ui->switchBoxLayout->takeAt(0)->widget();
        delete wg;
    }
    while(!ui->logBoxLayout->isEmpty()) {
        QWidget *wg = ui->logBoxLayout->takeAt(0)->widget();
        delete wg;
    }

    for (int i = 0; i < logValues->lower_panel_switch_id.count(); i++)
    {
        for (int j = 0; j < logValues->log_switch_id.length(); j++)
        {
            if (logValues->lower_panel_switch_id.at(i) == logValues->log_switch_id.at(j) && logValues->log_switch_protocol.at(j) == protocol)
            {
                //emit LOG_D("Switch:" << logValues->log_switch_name.at(j);
                QGroupBox *switchBox = logBoxes->drawLogBoxes("switch", i, switchBoxCount, logValues->log_switch_name.at(j), logValues->log_switch_name.at(j), logValues->log_switch_state.at(j));
                switchBox->setAttribute(Qt::WA_TransparentForMouseEvents);
                ui->switchBoxLayout->addWidget(switchBox);
            }
        }
    }
    for (int i = 0; i < logValues->lower_panel_log_value_id.count(); i++)
    {
        for (int j = 0; j < logValues->log_value_id.length(); j++)
        {
            if (logValues->lower_panel_log_value_id.at(i) == logValues->log_value_id.at(j) && logValues->log_value_protocol.at(j) == protocol)
            {
                //emit LOG_D("Log value:" << logValues->log_value_name.at(j);
                QStringList value_unit = logValues->log_value_units.at(j).split(",");
                QGroupBox *logBox = logBoxes->drawLogBoxes("log", i, logBoxCount, logValues->log_value_name.at(j), value_unit.at(1), logValues->log_value.at(j));
                logBox->setAttribute(Qt::WA_TransparentForMouseEvents);
                ui->logBoxLayout->addWidget(logBox);
            }
        }
    }
}

void MainWindow::update_logbox_values(QString protocol)
{
    int index = 0;
    QString warningMin;
    QString warningMax;
    QString labelText;
    QString label;
    QString unit;

    QScreen *screen = QGuiApplication::primaryScreen();
    QRect size = screen->geometry();

    for (int i = 0; i < logValues->lower_panel_log_value_id.length(); i++){
        QWidget *wg = ui->logBoxLayout->itemAt(i)->widget();
        QVBoxLayout* logVBoxLayout = wg->findChild<QVBoxLayout*>();
        if (logVBoxLayout){
            QLabel* log_label = wg->findChild<QLabel*>("log_label" + QString::number(i));
            if (log_label){

                for (int j = 0; j < logValues->log_value_id.count(); j++)
                {
                    if (logValues->log_value_id.at(j) == logValues->lower_panel_log_value_id.at(i) && logValues->log_value_protocol.at(j) == protocol)
                        index = j;
                }

                unit = logValues->log_value_units.at(index).split(",").at(1);

                labelText = logValues->log_value.at(index);
                labelText.append(" <font size=1px color=grey>");
                labelText.append(unit);
                labelText.append("</font>");

                log_label->setAlignment(Qt::AlignRight);
                log_label->setText(labelText);
                int labelFontSize = size.width() / 90;
                QFont f("Arial",labelFontSize);
                log_label->setFont(f);
            }
        }
    }
    delay(1);
}

void MainWindow::setSplashScreenProgress(QString text, int incValue)
{
    startUpSplashLabel->setText(text);
    int startUpSplashProgressBarValue = startUpSplashProgressBar->value();
    startUpSplashProgressBar->setValue(startUpSplashProgressBarValue += incValue);
    QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
}

bool MainWindow::event(QEvent *event)
{
    if (event->type() == QEvent::WindowStateChange)
    {
        if (this->isMaximized())
        {
            //emit LOG_D("Maximize event";
            configValues->window_width = "maximized";
            configValues->window_height = "maximized";
        }
        else
        {
            QString window_width = QString::number(MainWindow::size().width());
            QString window_height = QString::number(MainWindow::size().height());

            configValues->window_width = window_width;
            configValues->window_height = window_height;
        }
        fileActions->save_config_file(configValues);
    }
    else if (event->type() == QEvent::Resize)
    {
        //emit LOG_D("Screen resize event";

        if (isMaximized())
        {
            //emit LOG_D("Window is maximized";
            configValues->window_width = "maximized";
            configValues->window_height = "maximized";
        }
        else
        {
            QString window_width = QString::number(MainWindow::size().width());
            QString window_height = QString::number(MainWindow::size().height());

            configValues->window_width = window_width;
            configValues->window_height = window_height;
        }

        fileActions->save_config_file(configValues);
    }


    return QWidget::event(event);
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    //QScreen *screen = QGuiApplication::primaryScreen();
    //QRect size = screen->geometry();
    QWidget::resizeEvent(event);

    //QString window_width = QString::number(MainWindow::size().width());
    //QString window_height = QString::number(MainWindow::size().height());

    //emit LOG_D("Screen resize event 2";

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
        status_bar_connection_label->setStyleSheet("QLabel { background-color : green; color : white; color: white;}");
    }
    else if(serialConnectionState){
        connectionStatusString = "ECU not connected";
        status_bar_connection_label->setStyleSheet("QLabel { background-color : yellow; color : white; color: black;}");
    }
    else{
        connectionStatusString = "Serial port unavailable";
        status_bar_connection_label->setStyleSheet("QLabel { background-color : red; color : white; color: white;}");
    }
    QString romIdString = "ECU ID: " + romId;
    QString statusBarString = softwareVersionString + " | " + connectionStatusString + " | " + romIdString;
    status_bar_connection_label->setText(statusBarString);
}

void MainWindow::delay(int n)
{
    QTime dieTime = QTime::currentTime().addMSecs(n);
    while (QTime::currentTime() < dieTime)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
}

void MainWindow::add_new_ecu_definition_file()
{
    QString filename;
    QObject* obj = sender();
    QListWidget* definition_files = obj->parent()->parent()->findChild<QListWidget*>("ecu_definition_files_list");

    QFileDialog openDialog;
    openDialog.setDefaultSuffix("xml");
    filename = QFileDialog::getOpenFileName(this, tr("Select definition file"), configValues->definition_files_directory, tr("ECU definition file (*.xml)"));

    if (filename.isEmpty())
    {
        QMessageBox::information(this, tr("ECU definition file"), "No file selected");
    }
    else
    {
        definition_files->addItem(filename);
        configValues->romraider_definition_files.append(filename);
        fileActions->save_config_file(configValues);
    }

}

void MainWindow::remove_ecu_definition_file()
{
    QObject* obj = sender();
    QListWidget* definition_files = obj->parent()->parent()->findChild<QListWidget*>("ecu_definition_files_list");
    QList<QModelIndex> index = definition_files->selectionModel()->selectedIndexes();

    int row = 0;
    for (int i = index.length() - 1; i >= 0; i--)
    {
        row = index.at(i).row();
        definition_files->model()->removeRow(row);
        configValues->romraider_definition_files.removeAt(row);
    }
    if (index.length() > 0)
        fileActions->save_config_file(configValues);
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

    for (int i = 0; i < received.length(); i++)
    {
        msg.append(QString("%1 ").arg((uint8_t)received.at(i),2,16,QLatin1Char('0')));
    }

    return msg;
}

QString MainWindow::parse_ecuid(QByteArray received)
{
    QString msg;
    received.remove(0, 8);
    received.remove(5, received.length() - 5);

    for (int i = 0; i < received.length(); i++)
    {
        msg.append(QString("%1").arg((uint8_t)received.at(i),2,16,QLatin1Char('0')).toUpper());
    }

    return msg;
}

bool MainWindow::eventFilter(QObject *target, QEvent *event)
{
    Q_UNUSED(target)
    //Filter all mouse and keyboard events for splashscreen
    if (target == netSplash)
        if(     (event->type() == QEvent::MouseButtonPress) ||
                (event->type() == QEvent::MouseButtonDblClick) ||
                (event->type() == QEvent::MouseButtonRelease) ||
                (event->type() == QEvent::KeyPress) ||
                (event->type() == QEvent::KeyRelease))
            return true;

    return false;
}

template <typename FLASH_CLASS>
FLASH_CLASS* MainWindow::connect_signals_and_run_module(FLASH_CLASS *object)
{
    //To successfully connect an overloaded signal,
    //one should provide a suitable template parameter to connect()
    QObject::connect<void (FLASH_CLASS::*)(QString)>(object, &FLASH_CLASS::external_logger,
                                                     this, &MainWindow::external_logger);
    QObject::connect<void (FLASH_CLASS::*)(int)>(object, &FLASH_CLASS::external_logger,
                                                 this, &MainWindow::external_logger_set_progressbar_value);

    //If signal is not overloaded, QObject::connect<> template will deduce type automatically
    QObject::connect(object, &FLASH_CLASS::LOG_E, syslogger, &SystemLogger::log_messages);
    QObject::connect(object, &FLASH_CLASS::LOG_W, syslogger, &SystemLogger::log_messages);
    QObject::connect(object, &FLASH_CLASS::LOG_I, syslogger, &SystemLogger::log_messages);
    QObject::connect(object, &FLASH_CLASS::LOG_D, syslogger, &SystemLogger::log_messages);

    object->run();
    return object;
}

//External logger slot for string messages
void MainWindow::external_logger(QString message)
{
    emit LOG_D(Q_FUNC_INFO, true, false);
    emit LOG_D(" " + message, false, true);
    if (remote_utility->isValid())
        remote_utility->send_log_window_message(message);
}

//External progress bar slot
void MainWindow::external_logger_set_progressbar_value(int value)
{
    emit LOG_D(Q_FUNC_INFO, true, false);
    emit LOG_D(" " + QString::number(value), false, true);
    if (remote_utility->isValid())
        remote_utility->set_progressbar_value(value);
}

void MainWindow::send_message_to_log_window(QString msg)
{
    // EcuOperationsWindow
    QDialog *ecuOperationsWindow = this->findChild<QDialog*>("EcuOperationsWindow");
    if (ecuOperationsWindow)
    {
        //emit LOG_D("Found ecuOperationsWindow", true, true);
        QTextEdit *textEdit = ecuOperationsWindow->findChild<QTextEdit*>("text_edit");
        if (textEdit)
        {
            //emit LOG_D("Found ecuOperationsWindow->textEdit", true, true);
            textEdit->insertPlainText(msg);
            textEdit->ensureCursorVisible();
        }
    }
    QDialog *biuOperationsSubaruWindow = this->findChild<QDialog*>("BiuOperationsSubaruWindow");
    if (biuOperationsSubaruWindow)
    {
        //emit LOG_D("Found biuOperationsSubaruWindow", true, true);
        QTextEdit *textEdit = biuOperationsSubaruWindow->findChild<QTextEdit*>("text_edit");
        if (textEdit)
        {
            //emit LOG_D("Found biuOperationsSubaruWindow->textEdit", true, true);
            textEdit->insertPlainText(msg);
            textEdit->ensureCursorVisible();
        }
    }
    QDialog *dtcOperationsWindow = this->findChild<QDialog*>("DtcOperationsWindow");
    if (dtcOperationsWindow)
    {
        //emit LOG_D("Found dtcOperationsWindow", true, true);
        QTextEdit *textEdit = dtcOperationsWindow->findChild<QTextEdit*>("text_edit");
        if (textEdit)
        {
            //emit LOG_D("Found dtcOperationsWindow->textEdit", true, true);
            textEdit->insertPlainText(msg);
            textEdit->ensureCursorVisible();
        }
    }
}

void MainWindow::update_vbatt()
{
    unsigned long vBatt = 0;

    if (!serial->get_use_openport2_adapter())
        return;

    vBatt = serial->read_vbatt();

    QDialog *ecuOperationsWindow = this->findChild<QDialog*>("EcuOperationsWindow");
    if (ecuOperationsWindow)
    {
        //emit LOG_D("Found ecuOperationsWindow", true, true);
        QLabel *vBattLabel = ecuOperationsWindow->findChild<QLabel*>("vBattLabel");
        if (vBattLabel)
        {
            //emit LOG_D("Found ecuOperationsWindow->vBattLabel", true, true);
            QString vBattText = QString("Battery: %1").arg(vBatt/1000.0, 0, 'f', 3) + " V";
            vBattLabel->setText(vBattText);
            emit LOG_D(vBattText, true, true);
        }
    }
    QDialog *biuOperationsSubaruWindow = this->findChild<QDialog*>("BiuOperationsSubaruWindow");
    if (biuOperationsSubaruWindow)
    {
        //emit LOG_D("Found biuOperationsSubaruWindow", true, true);
        QLabel *vBattLabel = biuOperationsSubaruWindow->findChild<QLabel*>("vBattLabel");
        if (vBattLabel)
        {
            //emit LOG_D("Found biuOperationsSubaruWindow->vBattLabel", true, true);
            QString vBattText = QString("Battery: %1").arg(vBatt/1000.0, 0, 'f', 3) + " V";
            vBattLabel->setText(vBattText);
            emit LOG_D(vBattText, true, true);
        }
    }
    QDialog *dtcOperationsWindow = this->findChild<QDialog*>("DtcOperationsWindow");
    if (dtcOperationsWindow)
    {
        //emit LOG_D("Found dtcOperationsWindow", true, true);
        QLabel *vBattLabel = dtcOperationsWindow->findChild<QLabel*>("vBattLabel");
        if (vBattLabel)
        {
            //emit LOG_D("Found dtcOperationsWindow->vBattLabel", true, true);
            QString vBattText = QString("Battery: %1").arg(vBatt/1000.0, 0, 'f', 3) + " V";
            vBattLabel->setText(vBattText);
            emit LOG_D(vBattText, true, true);
        }
    }
}
