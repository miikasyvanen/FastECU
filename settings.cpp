#include "settings.h"
#include "ui_settings.h"

Settings::Settings(FileActions::ConfigValuesStructure *configValues, QWidget *parent)
    : QWidget(parent),
      ui(new Ui::Settings)
{
    ui->setupUi(this);

    this->configValues = configValues;

    ui->list_widget->setViewMode(QListView::IconMode);
    //ui->list_widget->setIconSize(QSize(96, 84));
    ui->list_widget->setMovement(QListView::Static);
    ui->list_widget->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
    //ui->list_widget->setFixedWidth(160);
    ui->list_widget->setSpacing(10);

    ui->dir_page->setLayout(create_files_config_page(configValues));
    ui->ui_page->setLayout(create_ui_config_page(configValues));

    ui->save_button->hide();
    //connect(ui->save_button, SIGNAL (clicked()), this, SLOT (save_config_file()));

    connect(ui->close_button, SIGNAL (clicked()), this, SLOT (close()));

    create_list_icons();
    ui->list_widget->setCurrentRow(0);

    QHBoxLayout *buttonsLayout = new QHBoxLayout;
    buttonsLayout->addStretch(1);


}

Settings::~Settings()
{
    qDebug() << "Save config file before exit, bye bye!";
    save_config_file();

    delete ui;
}

void Settings::closeEvent(QCloseEvent *bar)
{
    qDebug() << "Save config file before exit, bye bye!";
    save_config_file();
}

int Settings::save_config_file()
{
    qDebug() << "Save config file";
    fileActions = new FileActions();
    fileActions->save_config_file(configValues);
    qDebug() << "Config file saved";

    return 0;
}

QVBoxLayout *Settings::create_files_config_page(FileActions::ConfigValuesStructure *configValues)
{
    QGroupBox *romraider_definitions_group = new QGroupBox(tr("RomRaider Definition Files"));
/*
    qDebug() << "Calibration path:" << configValues->calibration_files_directory;
    qDebug() << "Romraider definition files:" << configValues->romraider_definition_files;
    qDebug() << "EcuFlash definition path:" << configValues->ecuflash_definition_files_directory;
    qDebug() << "Log files path:" << configValues->log_files_directory;
*/

    QCheckBox *romraider_defs_enabled = new QCheckBox("Enabled");
    if (configValues->use_romraider_definitions == "enabled")
        romraider_defs_enabled->setChecked(true);
    connect(romraider_defs_enabled, SIGNAL(stateChanged(int)), this, SLOT(romraider_defs_enabled_checkbox(int)));

    romraider_definition_files_list = new QListWidget;

    for (int i = 0; i < configValues->romraider_definition_files.length(); i++)
    {
        new QListWidgetItem(configValues->romraider_definition_files.at(i), romraider_definition_files_list);
    }

    QCheckBox *romraider_as_primary_def_base = new QCheckBox("Use as primary");
    if (configValues->primary_definition_base == "romraider")
        romraider_as_primary_def_base->setChecked(true);
    connect(romraider_as_primary_def_base, SIGNAL(stateChanged(int)), this, SLOT(romraider_as_primary_def_base_checkbox(int)));

    QPushButton *romraider_def_files_add_button = new QPushButton;
    romraider_def_files_add_button->setFixedWidth(100);
    romraider_def_files_add_button->setText("Add");
    connect(romraider_def_files_add_button, &QAbstractButton::clicked, this, &Settings::add_definition_files);

    QPushButton *romraider_def_files_remove_button = new QPushButton;
    romraider_def_files_remove_button->setFixedWidth(100);
    romraider_def_files_remove_button->setText("Remove");
    connect(romraider_def_files_remove_button, &QAbstractButton::clicked, this, &Settings::remove_definition_files);

    QHBoxLayout *romraider_def_files_buttons_layout = new QHBoxLayout;
    romraider_def_files_buttons_layout->addWidget(romraider_as_primary_def_base);
    romraider_def_files_buttons_layout->addStretch(1);
    //romraider_def_files_buttons_layout->setAlignment(Qt::AlignRight);
    romraider_def_files_buttons_layout->addWidget(romraider_def_files_add_button);
    romraider_def_files_buttons_layout->addWidget(romraider_def_files_remove_button);

    QVBoxLayout *romraider_def_files_layout = new QVBoxLayout;
    romraider_def_files_layout->addWidget(romraider_defs_enabled);
    romraider_def_files_layout->addWidget(romraider_definition_files_list);
    romraider_def_files_layout->addLayout(romraider_def_files_buttons_layout);

    QCheckBox *ecuflash_defs_enabled = new QCheckBox("Enabled");
    if (configValues->use_ecuflash_definitions == "enabled")
        ecuflash_defs_enabled->setChecked(true);
    connect(ecuflash_defs_enabled, SIGNAL(stateChanged(int)), this, SLOT(ecuflash_defs_enabled_checkbox(int)));

    QGroupBox *ecuflash_def_dir_group = new QGroupBox(tr("EcuFlash Definition Files Directory"));
    ecuflash_def_dir_lineedit = new QLineEdit;
    ecuflash_def_dir_lineedit->setText(configValues->ecuflash_definition_files_directory);

    QPushButton *ecuflash_def_dir_browse_button = new QPushButton;
    ecuflash_def_dir_browse_button->setIcon(QIcon(":/icons/document-open.png"));
    connect(ecuflash_def_dir_browse_button, &QAbstractButton::clicked, this, &Settings::set_ecuflash_def_dir);

    QHBoxLayout *ecuflash_def_dir_layout = new QHBoxLayout;
    ecuflash_def_dir_layout->addWidget(ecuflash_def_dir_lineedit);
    ecuflash_def_dir_layout->addWidget(ecuflash_def_dir_browse_button);

    QGroupBox *romraider_logger_file_group = new QGroupBox(tr("RomRaider Logger File"));
    romraider_logger_file_lineedit = new QLineEdit;
    romraider_logger_file_lineedit->setText(configValues->romraider_logger_definition_file);

    QPushButton *romraider_logger_file_browse_button = new QPushButton;
    romraider_logger_file_browse_button->setIcon(QIcon(":/icons/document-open.png"));
    connect(romraider_logger_file_browse_button, &QAbstractButton::clicked, this, &Settings::set_romraider_logger_file);

    QHBoxLayout *romraider_logger_file_layout = new QHBoxLayout;
    romraider_logger_file_layout->addWidget(romraider_logger_file_lineedit);
    romraider_logger_file_layout->addWidget(romraider_logger_file_browse_button);

    QGroupBox *ecu_cal_dir_group = new QGroupBox(tr("Calibrations Directory"));
    ecu_cal_dir_lineedit = new QLineEdit;
    ecu_cal_dir_lineedit->setText(configValues->calibration_files_directory);

    QPushButton *ecu_cal_dir_browse_button = new QPushButton;
    ecu_cal_dir_browse_button->setIcon(QIcon(":/icons/document-open.png"));
    connect(ecu_cal_dir_browse_button, &QAbstractButton::clicked, this, &Settings::set_ecu_cal_dir);

    QHBoxLayout *ecu_cal_dir_layout = new QHBoxLayout;
    ecu_cal_dir_layout->addWidget(ecu_cal_dir_lineedit);
    ecu_cal_dir_layout->addWidget(ecu_cal_dir_browse_button);

    QGroupBox *log_files_group = new QGroupBox(tr("Logfiles Directory"));

    log_files_dir_lineedit = new QLineEdit;
    log_files_dir_lineedit->setText(configValues->datalog_files_directory);

    QPushButton *log_files_dir_browse_button = new QPushButton;
    log_files_dir_browse_button->setIcon(QIcon(":/icons/document-open.png"));
    connect(log_files_dir_browse_button, &QAbstractButton::clicked, this, &Settings::set_log_files_dir);

    QHBoxLayout *log_files_dir_layout = new QHBoxLayout;
    log_files_dir_layout->addWidget(log_files_dir_lineedit);
    log_files_dir_layout->addWidget(log_files_dir_browse_button);

    QVBoxLayout *definition_layout = new QVBoxLayout;
    definition_layout->addLayout(romraider_def_files_layout);
    romraider_definitions_group->setLayout(definition_layout);

    QVBoxLayout *ecuflash_def_layout = new QVBoxLayout;
    ecuflash_def_layout->addWidget(ecuflash_defs_enabled);
    ecuflash_def_layout->addLayout(ecuflash_def_dir_layout);
    ecuflash_def_dir_group->setLayout(ecuflash_def_layout);

    QVBoxLayout *romraider_logger_layout = new QVBoxLayout;
    romraider_logger_layout->addLayout(romraider_logger_file_layout);
    romraider_logger_file_group->setLayout(romraider_logger_layout);

    QVBoxLayout *calibration_layout = new QVBoxLayout;
    calibration_layout->addLayout(ecu_cal_dir_layout);
    ecu_cal_dir_group->setLayout(calibration_layout);

    QVBoxLayout *log_files_layout = new QVBoxLayout;
    log_files_layout->addLayout(log_files_dir_layout);
    log_files_group->setLayout(log_files_layout);

    QVBoxLayout *directory_layout = new QVBoxLayout;
    directory_layout->addWidget(romraider_definitions_group);
    directory_layout->addWidget(romraider_logger_file_group);
    directory_layout->addWidget(ecuflash_def_dir_group);
    directory_layout->addWidget(ecu_cal_dir_group);
    directory_layout->addWidget(log_files_group);
    directory_layout->addStretch(1);

    return directory_layout;
}

QVBoxLayout *Settings::create_ui_config_page(FileActions::ConfigValuesStructure *configValues)
{
    QGroupBox *toolbar_group = new QGroupBox(tr("Toolbar settings"));
    QLabel *toolbar_iconsize_label = new QLabel("Toolbar icon size:");
    toolbar_iconsize_spinbox = new QSpinBox();
    toolbar_iconsize_spinbox->setValue(configValues->toolbar_iconsize.toInt());
    connect(toolbar_iconsize_spinbox, SIGNAL(valueChanged(int)), this, SLOT(toolbar_iconsize_value_changed(int)));

    QHBoxLayout *toolbar_layout = new QHBoxLayout;
    toolbar_layout->setAlignment(Qt::AlignLeft);
    toolbar_layout->addWidget(toolbar_iconsize_label);
    toolbar_layout->addWidget(toolbar_iconsize_spinbox);
    toolbar_group->setLayout(toolbar_layout);

    QVBoxLayout *ui_config_layout = new QVBoxLayout;
    ui_config_layout->addWidget(toolbar_group);
    ui_config_layout->addStretch(1);

    return ui_config_layout;
}

void Settings::create_list_icons()
{
    QListWidgetItem *file_config_item = new QListWidgetItem(ui->list_widget);
    file_config_item->setIcon(QIcon(":/icons/document-open.png"));
    file_config_item->setText(tr("Files"));
    file_config_item->setSizeHint(QSize(64, 64));
    //fileConfigButton->setTextAlignment(Qt::AlignHCenter);
    file_config_item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

    QListWidgetItem *ui_config_item = new QListWidgetItem(ui->list_widget);
    ui_config_item->setIcon(QIcon(":/icons/preferences-system.png"));
    ui_config_item->setText(tr("Ui config"));
    ui_config_item->setSizeHint(QSize(64, 64));
    //uiConfigButton->setTextAlignment(Qt::AlignHCenter);
    ui_config_item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

    connect(ui->list_widget, &QListWidget::currentItemChanged, this, &Settings::change_page);
}

void Settings::change_page(QListWidgetItem *current, QListWidgetItem *previous)
{
    if (!current)
        current = previous;

    ui->pages_widget->setCurrentIndex(ui->list_widget->row(current));
}

void Settings::romraider_defs_enabled_checkbox(int state)
{
    if (state)
        configValues->use_romraider_definitions = "enabled";
    else
        configValues->use_romraider_definitions = "disabled";
}

void Settings::ecuflash_defs_enabled_checkbox(int state)
{
    if (state)
        configValues->use_ecuflash_definitions = "enabled";
    else
        configValues->use_ecuflash_definitions = "disabled";
}

void Settings::romraider_as_primary_def_base_checkbox(int state)
{
    if (state)
        configValues->primary_definition_base = "romraider";
    else
        configValues->primary_definition_base = "ecuflash";
}

void Settings::toolbar_iconsize_value_changed(int value)
{
    configValues->toolbar_iconsize = QString::number(value);
}

void Settings::set_ecuflash_def_dir()
{

    QString ecuflash_definition_dir = QFileDialog::getExistingDirectory(this, tr("Select EcuFlash definition directory"),
                                                 configValues->ecuflash_definition_files_directory,
                                                 QFileDialog::ShowDirsOnly
                                                 | QFileDialog::DontResolveSymlinks);

    qDebug() << "Selected path:" << ecuflash_definition_dir;
    if (!ecuflash_definition_dir.endsWith("/") && !ecuflash_definition_dir.endsWith("\\"))
        ecuflash_definition_dir.append("/");
    if (!ecuflash_definition_dir.isEmpty()){
        configValues->ecuflash_definition_files_directory = ecuflash_definition_dir;
        ecuflash_def_dir_lineedit->clear();
        ecuflash_def_dir_lineedit->setText(configValues->ecuflash_definition_files_directory);
    }
    else
        QMessageBox::information(this, tr("EcuFlash definition directory"), "No directory selected");
}

void Settings::set_romraider_logger_file()
{
    QString file_dir;

    QDir dir;
    if (configValues->romraider_logger_definition_file.length() > 0){
        QFileInfo def_file_name(configValues->romraider_logger_definition_file);
        QString def_file_dir = def_file_name.absoluteFilePath();
        file_dir.append(def_file_dir);
    }
    else
        file_dir.append("./");
    QString filename = QFileDialog::getOpenFileName(this, tr("Add RomRaider logger file"), file_dir, tr("RomRaider logger file (*.xml)"));
    qDebug() << "Filename:" << filename;
    if (!filename.isEmpty())
    {
        configValues->romraider_logger_definition_file = filename;
        romraider_logger_file_lineedit->clear();
        romraider_logger_file_lineedit->setText(configValues->romraider_logger_definition_file);
    }
    else
        QMessageBox::information(this, tr("RomRaider logger file"), "No logger file selected");
}

void Settings::set_ecu_cal_dir()
{

    QString calibration_dir = QFileDialog::getExistingDirectory(this, tr("Select calibrations directory"),
                                                 configValues->calibration_files_directory,
                                                 QFileDialog::ShowDirsOnly
                                                 | QFileDialog::DontResolveSymlinks);

    qDebug() << "Selected path:" << calibration_dir;
    if (!calibration_dir.endsWith("/") && !calibration_dir.endsWith("\\"))
        calibration_dir.append("/");
    if (!calibration_dir.isEmpty()){
        configValues->calibration_files_directory = calibration_dir;
        ecu_cal_dir_lineedit->clear();
        ecu_cal_dir_lineedit->setText(configValues->calibration_files_directory);
    }
    else
        QMessageBox::information(this, tr("Calibration directory"), "No directory selected");
}

void Settings::set_log_files_dir()
{

    QString logfiles_dir = QFileDialog::getExistingDirectory(this, tr("Select logfiles directory"),
                                                 configValues->datalog_files_directory,
                                                 QFileDialog::ShowDirsOnly
                                                 | QFileDialog::DontResolveSymlinks);


    qDebug() << "Selected path:" << logfiles_dir;
    if (!logfiles_dir.endsWith("/") && !logfiles_dir.endsWith("\\"))
        logfiles_dir.append("/");
    if (!logfiles_dir.isEmpty()){
        configValues->datalog_files_directory = logfiles_dir;
        log_files_dir_lineedit->clear();
        log_files_dir_lineedit->setText(configValues->datalog_files_directory);
    }
    else
        QMessageBox::information(this, tr("Logger files directory"), "No directory selected");
}

void Settings::add_definition_files()
{
    QString file_dir;

    QDir dir;
    if (configValues->romraider_definition_files.count() > 0){
        QFileInfo def_file_name(configValues->romraider_definition_files[configValues->romraider_definition_files.count() - 1]);
        QString def_file_dir = def_file_name.absoluteFilePath();
        file_dir.append(def_file_dir);
    }
    else
        file_dir.append("./");
    QString filename = QFileDialog::getOpenFileName(this, tr("Add RomRaider definition file"), file_dir, tr("RomRaider definition file (*.xml)"));

    if (!filename.isEmpty()){
        romraider_definition_files_list->addItem(filename);
        romraider_definition_files_list->update();
        configValues->romraider_definition_files.append(filename);
    }
    else
        QMessageBox::information(this, tr("RomRaider definition file"), "No definition file selected");
}

void Settings::remove_definition_files()
{

    QList<QListWidgetItem*> items = romraider_definition_files_list->selectedItems();
    foreach(QListWidgetItem * item, items)
    {
        delete romraider_definition_files_list->takeItem(romraider_definition_files_list->row(item));
    }

    configValues->romraider_definition_files.clear();

    for(int i = 0; i < romraider_definition_files_list->count(); ++i)
    {
        configValues->romraider_definition_files.append(romraider_definition_files_list->item(i)->text());
    }
}
