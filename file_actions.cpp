#include "file_actions.h"
#include "error_codes.h"

FileActions::FileActions(QWidget *parent)
    : QWidget(parent)
{

}

FileActions::ConfigValuesStructure *FileActions::set_base_dirs(ConfigValuesStructure *configValues)
{
    QDir currentPath(QDir::currentPath());
    QStringList isDevPath = currentPath.absolutePath().split("build");
    bool isDevFile = false;

    QString AppFilePath = QApplication::applicationFilePath();
#if defined Q_OS_UNIX
    QString AppRootPath = AppFilePath.split("usr").at(0);
    if (AppRootPath.contains("FastECU"))
        AppRootPath = "/config";
#elif defined Q_OS_WIN32
    QString AppRootPath = currentPath.absolutePath();
#endif
    QString filename;
    QDirIterator it(AppRootPath, QStringList() << "*.*", QDir::Files, QDirIterator::Subdirectories);

    if(QFileInfo::exists("./build.txt"))
    {
        isDevFile = true;
    }

    if (isDevPath.length() > 1 || isDevFile)
    {
        emit LOG_D("App is started on dev path, base path set to:" + currentPath.absolutePath(), true, true);//isDevPath.at(0);
        configValues->base_config_directory = currentPath.absolutePath();
        configValues->version_config_directory = currentPath.absolutePath();
        configValues->calibration_files_directory = configValues->base_config_directory + "/calibrations/";
        configValues->config_files_directory = configValues->base_config_directory + "/config/";
        configValues->definition_files_directory = configValues->base_config_directory + "/definitions/";
        configValues->kernel_files_directory = configValues->base_config_directory + "/kernels/";
        configValues->datalog_files_directory = configValues->base_config_directory + "/datalogs/";
        configValues->syslog_files_directory = configValues->base_config_directory + "/syslogs/";
        configValues->config_file = configValues->config_files_directory + "fastecu.cfg";
        configValues->menu_file = configValues->config_files_directory + "menu.cfg";
        configValues->protocols_file = configValues->config_files_directory + "protocols.cfg";
        configValues->logger_file = configValues->config_files_directory + "logger.cfg";
        copyConfigFromDirectory.setPath(isDevPath.at(0) + "/" + configValues->config_files_directory);
        copyKernelsFromDirectory.setPath(isDevPath.at(0) + "/" + configValues->kernel_files_directory);
    }
    else
    {
        configValues->version_config_directory = configValues->base_config_directory + "/" + configValues->software_version + "/";
        configValues->calibration_files_directory = configValues->base_config_directory + configValues->software_version + "/calibrations/";
        configValues->config_files_directory = configValues->base_config_directory + configValues->software_version + "/config/";
        configValues->definition_files_directory = configValues->base_config_directory + configValues->software_version + "/definitions/";
        configValues->kernel_files_directory = configValues->base_config_directory + configValues->software_version + "/kernels/";
        configValues->datalog_files_directory = configValues->base_config_directory + configValues->software_version + "/datalogs/";
        configValues->syslog_files_directory = configValues->base_config_directory + configValues->software_version + "/syslogs/";
        configValues->config_file = configValues->config_files_directory + "fastecu.cfg";
        configValues->menu_file = configValues->config_files_directory + "menu.cfg";
        configValues->protocols_file = configValues->config_files_directory + "protocols.cfg";
        configValues->logger_file = configValues->config_files_directory + "logger.cfg";
        copyConfigFromDirectory.setPath(AppRootPath + "./config");
        copyKernelsFromDirectory.setPath(AppRootPath + "./kernels");
    }

    return configValues;
}

FileActions::ConfigValuesStructure *FileActions::check_config_dirs(ConfigValuesStructure *configValues)
{
    QDir currentPath(QDir::currentPath());
    QStringList isDevPath = currentPath.absolutePath().split("build");
    //bool isDevFile = false;

    QString AppFilePath = QApplication::applicationFilePath();
#if defined Q_OS_UNIX
    QString AppRootPath = AppFilePath.split("usr").at(0);
    if (AppRootPath.contains("FastECU"))
        AppRootPath = "/config";
#elif defined Q_OS_WIN32
    QString AppRootPath = currentPath.absolutePath();
#endif
    //QString filename;
    QDirIterator it(AppRootPath, QStringList() << "*.*", QDir::Files, QDirIterator::Subdirectories);

    emit LOG_D("App path: " + AppFilePath, true, true);
    emit LOG_D("App root path: " + AppRootPath, true, true);
    emit LOG_D("APP DIRECTORY: " + currentPath.absolutePath(), true, true);
    if(QFileInfo::exists("./build.txt"))
    {
        //isDevFile = true;
        emit LOG_D("build.txt found", true, true);
    }

    emit LOG_D(copyConfigFromDirectory.absolutePath(), true, true);
    emit LOG_D(copyKernelsFromDirectory.absolutePath(), true, true);

    QStringList directories_sorted;
    QString latest_config_dir;

    // Check if fastecu directory exists in users home config folder
    emit LOG_D("Config base dir " + configValues->base_config_directory, true, true);
    if (!QDir(configValues->base_config_directory).exists()){
        QDir().mkdir(configValues->base_config_directory);
    }

    if (isDevPath.length() < 2)
    {
        // Check if fastecu earlier version directories exists in fastecu config folder
        QDir configDir(configValues->base_config_directory);
        QFileInfoList configDirList = configDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Time);
        for (int i = 0; i < configDirList.length(); i++){
            emit LOG_D("Sorted dir by date: " + configDirList.at(i).absoluteFilePath() + " " + configDirList.at(i).lastModified().toString(), true, true);
        }
        // Copy latest version directory path
        if (configDirList.length())
            latest_config_dir = configDirList.at(0).absoluteFilePath();
        //else
        //    latest_config_dir = configDirList.at(configDirList.length() - 1).absoluteFilePath();

        // Check if current fastecu version directory exists in users home config folder
        if (!QDir(configValues->version_config_directory).exists()){
            QDir().mkdir(configValues->version_config_directory);
        }
    }
    emit LOG_D("Cal dir: " + configValues->calibration_files_directory, true, true);
    // Check if fastecu calibration files directory exists
    if (!QDir(configValues->calibration_files_directory).exists()){
        QDir().mkdir(configValues->calibration_files_directory);
    }

    // Check if fastecu config files directory exists
    if (!QDir(configValues->config_files_directory).exists()){
        QDir().mkdir(configValues->config_files_directory);
        // Copy fastecu.cfg from latest previous config
        LOG_D("Copy " + latest_config_dir + "/config/fastecu.cfg --> " + configValues->config_files_directory + "fastecu.cfg", true, true);
        QFile configFile(latest_config_dir + "/config/fastecu.cfg");
        configFile.copy(configValues->config_files_directory + "fastecu.cfg");
    }
    QDirIterator configs(":/config/", QDirIterator::Subdirectories);
    while (configs.hasNext()) {
        emit LOG_D("Check file: " + configs.next(), true, true);
        if(!QFileInfo::exists(configValues->config_files_directory + configs.fileName()))
        {
            emit LOG_D("File: " + configs.fileName() + " doesn't exists, copy file...", true, true);
            QFile::copy(":/config/" + configs.fileName(), configValues->config_files_directory + configs.fileName());
            QFile file(configValues->config_files_directory + configs.fileName());
            file.setPermissions(QFile::ReadUser | QFile::WriteUser);
        }
    }
    // Check if fastecu definition files directory exists
    if (!QDir(configValues->definition_files_directory).exists()){
        QDir().mkdir(configValues->definition_files_directory);
    }

    // Check if fastecu kernel files directory exists
    if (!QDir(configValues->kernel_files_directory).exists()){
        QDir().mkdir(configValues->kernel_files_directory);
    }
    QDirIterator kernels(":/kernels/", QDirIterator::Subdirectories);
    while (kernels.hasNext()) {
        emit LOG_D("Check file: " + kernels.next(), true, true);
        if(!QFileInfo::exists(configValues->kernel_files_directory + kernels.fileName()))
        {
            emit LOG_D("File: " + kernels.fileName() + " doesn't exists, copy file...", true, true);
            QFile::copy(":/kernels/" + kernels.fileName(), configValues->kernel_files_directory + kernels.fileName());
            QFile file(configValues->kernel_files_directory + kernels.fileName());
            file.setPermissions(QFile::ReadUser | QFile::WriteUser);
        }
    }

    // Check if fastecu datalog files directory exists
    if (!QDir(configValues->datalog_files_directory).exists()){
        QDir().mkdir(configValues->datalog_files_directory);
    }

    // Check if fastecu syslog files directory exists
    QDir syslog_dir(configValues->syslog_files_directory);
    if (!QDir(configValues->syslog_files_directory).exists()){
        QDir().mkdir(configValues->syslog_files_directory);
    }
    if (QDir(configValues->syslog_files_directory).exists())
    {
        qDebug() << "Check syslog file count";
        QFileInfoList syslog_dir_list = syslog_dir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot, QDir::Time);
        qDebug() << "Syslog files count:" << syslog_dir_list.length();
        for (int i = 20; i < syslog_dir_list.length(); i++){
            qDebug() << "Sorted dir by date:" << syslog_dir_list.at(i).absoluteFilePath() << syslog_dir_list.at(i).lastModified();
            syslog_dir.remove(syslog_dir_list.at(i).absoluteFilePath());
        }
    }

    return configValues;
}

bool FileActions::copy_directory_files(const QString &source_dir, const QString &target_dir, bool cover_file_if_exist)
{
    QDir sourceDir(source_dir);
    QDir targetDir(target_dir);
    if(!targetDir.exists()){    /* if directory don't exists, build it */
        if(!targetDir.mkdir(targetDir.absolutePath()))
            return false;
    }

    QFileInfoList fileInfoList = sourceDir.entryInfoList();
    foreach(QFileInfo fileInfo, fileInfoList){
        if(fileInfo.fileName() == "." || fileInfo.fileName() == "..")
            continue;

        if(fileInfo.isDir()){    /* if it is directory, copy recursively*/
            if(!copy_directory_files(fileInfo.filePath(),
                targetDir.filePath(fileInfo.fileName()),
                cover_file_if_exist))
                return false;
        }
        else{            /* if coverFileIfExist == true, remove old file first */
            if(cover_file_if_exist && targetDir.exists(fileInfo.fileName())){
                targetDir.remove(fileInfo.fileName());
            }

            // files copy
            if(!QFile::copy(fileInfo.filePath(),
                targetDir.filePath(fileInfo.fileName()))){
                    return false;
            }
        }
    }
    return true;
}

FileActions::ConfigValuesStructure *FileActions::read_config_file(ConfigValuesStructure *configValues)
{
    //ConfigValuesStructure *configValues = &ConfigValuesStruct;

    QDomDocument xmlBOM;
    QFile file(configValues->config_file);
    emit LOG_D("Looking config file from: " + configValues->config_file, true, true);
    if (!file.open(QIODevice::ReadOnly ))
    {
        QMessageBox::warning(this, tr("Config file"), "Unable to open application config file '" + file.fileName() + "' for reading");
        return configValues;
    }

    QXmlStreamReader reader;
    reader.setDevice(&file);

    if (reader.readNextStartElement())
    {
        if (reader.name().toUtf8() == "config" && reader.attributes().value("name").toUtf8() == "FastECU")
        {
            if (reader.readNextStartElement())
            {
                if (reader.name().toUtf8() == "software_settings")
                {
                    while (reader.readNextStartElement())
                    {
                        if (reader.name().toUtf8() == "setting" && reader.attributes().value("name").toUtf8() == "window_size")
                        {
                            while(reader.readNextStartElement())
                            {
                                if (reader.name().toUtf8() == "value" && reader.attributes().value("width").toUtf8() != "")
                                {
                                    configValues->window_width = reader.attributes().value("width").toString();
                                    reader.skipCurrentElement();
                                }
                                else if (reader.name().toUtf8() == "value" && reader.attributes().value("height").toUtf8() != "")
                                {
                                    configValues->window_height = reader.attributes().value("height").toString();
                                    reader.skipCurrentElement();
                                }
                                else
                                    reader.skipCurrentElement();
                            }
                            emit LOG_D("Width: " + configValues->window_width + " Height: " + configValues->window_height, true, true);
                        }
                        else if (reader.name().toUtf8() == "setting" && reader.attributes().value("name").toUtf8() == "toolbar_iconsize")
                        {
                            while(reader.readNextStartElement())
                            {
                                if (reader.name().toUtf8() == "value")
                                {
                                    configValues->toolbar_iconsize = reader.attributes().value("data").toString();
                                    reader.skipCurrentElement();
                                }
                                else
                                    reader.skipCurrentElement();
                            }
                            emit LOG_D("Serial port: " + configValues->serial_port, true, true);
                        }
                        else if (reader.name().toUtf8() == "setting" && reader.attributes().value("name").toUtf8() == "serial_port")
                        {
                            while(reader.readNextStartElement())
                            {
                                if (reader.name().toUtf8() == "value")
                                {
                                    configValues->serial_port = reader.attributes().value("data").toString();
                                    reader.skipCurrentElement();
                                }
                                else
                                    reader.skipCurrentElement();
                            }
                            emit LOG_D("Serial port: " + configValues->serial_port, true, true);
                        }
                        else if (reader.name().toUtf8() == "setting" && reader.attributes().value("name").toUtf8() == "protocol_id")
                        {
                            while(reader.readNextStartElement())
                            {
                                if (reader.name().toUtf8() == "value")
                                {
                                    configValues->flash_protocol_selected_id = reader.attributes().value("data").toString();
                                    reader.skipCurrentElement();
                                }
                                else
                                    reader.skipCurrentElement();
                            }
                            emit LOG_D("Protocol ID: " + configValues->flash_protocol_selected_id, true, true);
                        }
                        else if (reader.name().toUtf8() == "setting" && reader.attributes().value("name").toUtf8() == "flash_transport")
                        {
                            while(reader.readNextStartElement())
                            {
                                if (reader.name().toUtf8() == "value")
                                {
                                    configValues->flash_protocol_selected_flash_transport = reader.attributes().value("data").toString();
                                    reader.skipCurrentElement();
                                }
                                else
                                    reader.skipCurrentElement();
                            }
                            emit LOG_D("Flash transport: " + configValues->flash_protocol_selected_flash_transport, true, true);
                        }
                        else if (reader.name().toUtf8() == "setting" && reader.attributes().value("name").toUtf8() == "log_transport")
                        {
                            while(reader.readNextStartElement())
                            {
                                if (reader.name().toUtf8() == "value")
                                {
                                    configValues->flash_protocol_selected_log_transport = reader.attributes().value("data").toString();
                                    reader.skipCurrentElement();
                                }
                                else
                                    reader.skipCurrentElement();
                            }
                            emit LOG_D("Log transport: " + configValues->flash_protocol_selected_log_transport, true, true);
                        }
                        else if (reader.name().toUtf8() == "setting" && reader.attributes().value("name").toUtf8() == "log_protocol")
                        {
                            while(reader.readNextStartElement())
                            {
                                if (reader.name().toUtf8() == "value")
                                {
                                    configValues->flash_protocol_selected_log_protocol = reader.attributes().value("data").toString();
                                    reader.skipCurrentElement();
                                }
                                else
                                    reader.skipCurrentElement();
                            }
                            emit LOG_D("Log protocol: " + configValues->flash_protocol_selected_log_protocol, true, true);
                        }
                        else if (reader.name().toUtf8() == "setting" && reader.attributes().value("name").toUtf8() == "primary_definition_base")
                        {
                            while(reader.readNextStartElement())
                            {
                                if (reader.name().toUtf8() == "value")
                                {
                                    QString primary_definition_base = reader.attributes().value("data").toString();
                                    if (primary_definition_base == "romraider" || primary_definition_base == "ecuflash")
                                        configValues->primary_definition_base = primary_definition_base;
                                    reader.skipCurrentElement();
                                }
                                else
                                    reader.skipCurrentElement();
                            }
                            emit LOG_D("Primary def base: " + configValues->primary_definition_base, true, true);
                        }
                        else if (reader.name().toUtf8() == "setting" && reader.attributes().value("name").toUtf8() == "calibration_files")
                        {
                            while(reader.readNextStartElement())
                            {
                                if (reader.name().toUtf8() == "value")
                                {
                                    configValues->calibration_files.append(reader.attributes().value("data").toString());
                                    reader.skipCurrentElement();
                                }
                                else
                                    reader.skipCurrentElement();
                            }
                            emit LOG_D("Calibration files: " + configValues->calibration_files.join(", "), true, true);
                        }
                        else if (reader.name().toUtf8() == "setting" && reader.attributes().value("name").toUtf8() == "calibration_files_directory")
                        {
                            while(reader.readNextStartElement())
                            {
                                if (reader.name().toUtf8() == "value")
                                {
                                    configValues->calibration_files_directory = reader.attributes().value("data").toString();
                                    reader.skipCurrentElement();
                                }
                                else
                                    reader.skipCurrentElement();
                            }
                            emit LOG_D("Calibration files directory: " + configValues->calibration_files_directory, true, true);
                        }
                        else if (reader.name().toUtf8() == "setting" && reader.attributes().value("name").toUtf8() == "romraider_definition_files")
                        {
                            while(reader.readNextStartElement())
                            {
                                if (reader.name().toUtf8() == "value")
                                {
                                    QString filename = reader.attributes().value("data").toString();
                                    if (filename != "")
                                        configValues->romraider_definition_files.append(reader.attributes().value("data").toString());
                                    reader.skipCurrentElement();
                                }
                                else
                                    reader.skipCurrentElement();
                            }
                            emit LOG_D("RomRaider def files: " + configValues->romraider_definition_files.join(", "), true, true);
                        }
                        else if (reader.name().toUtf8() == "setting" && reader.attributes().value("name").toUtf8() == "use_romraider_definitions")
                        {
                            while(reader.readNextStartElement())
                            {
                                if (reader.name().toUtf8() == "value")
                                {
                                    configValues->use_romraider_definitions = reader.attributes().value("data").toString();
                                    reader.skipCurrentElement();
                                }
                                else
                                    reader.skipCurrentElement();
                            }
                            emit LOG_D("Use RomRaider definitions: " + configValues->use_romraider_definitions, true, true);
                        }
                        else if (reader.name().toUtf8() == "setting" && reader.attributes().value("name").toUtf8() == "ecuflash_definition_files_directory")
                        {
                            while(reader.readNextStartElement())
                            {
                                if (reader.name().toUtf8() == "value")
                                {
                                    configValues->ecuflash_definition_files_directory = reader.attributes().value("data").toString();
                                    reader.skipCurrentElement();
                                }
                                else
                                    reader.skipCurrentElement();
                            }
                            emit LOG_D("EcuFlash def files directory: " + configValues->ecuflash_definition_files_directory, true, true);
                        }
                        else if (reader.name().toUtf8() == "setting" && reader.attributes().value("name").toUtf8() == "use_ecuflash_definitions")
                        {
                            while(reader.readNextStartElement())
                            {
                                if (reader.name().toUtf8() == "value")
                                {
                                    configValues->use_ecuflash_definitions = reader.attributes().value("data").toString();
                                    reader.skipCurrentElement();
                                }
                                else
                                    reader.skipCurrentElement();
                            }
                            emit LOG_D("Use EcuFlash definitions: " + configValues->use_ecuflash_definitions, true, true);
                        }
                        else if (reader.name().toUtf8() == "setting" && reader.attributes().value("name").toUtf8() == "logger_definition_file")
                        {
                            while(reader.readNextStartElement())
                            {
                                if (reader.name().toUtf8() == "value")
                                {
                                    configValues->romraider_logger_definition_file = reader.attributes().value("data").toString();
                                    reader.skipCurrentElement();
                                }
                                else
                                    reader.skipCurrentElement();
                            }
                            emit LOG_D("Logger def file: " + configValues->romraider_logger_definition_file, true, true);
                        }
                        /*
                        else if (reader.name().toUtf8() == "setting" && reader.attributes().value("name").toUtf8() == "kernel_files_directory")
                        {
                            while(reader.readNextStartElement())
                            {
                                if (reader.name().toUtf8() == "value")
                                {
                                    configValues->kernel_files_directory = reader.attributes().value("data").toString();
                                    reader.skipCurrentElement();
                                }
                                else
                                    reader.skipCurrentElement();
                            }
                            emit LOG_D("Kernel files directory: " + configValues->kernel_files_directory;
                        }
*/
                        else if (reader.name().toUtf8() == "setting" && reader.attributes().value("name").toUtf8() == "datalog_files_directory")
                        {
                            while(reader.readNextStartElement())
                            {
                                if (reader.name().toUtf8() == "value")
                                {
                                    configValues->datalog_files_directory = reader.attributes().value("data").toString();
                                    reader.skipCurrentElement();
                                }
                                else
                                    reader.skipCurrentElement();
                            }
                            emit LOG_D("Logfiles firectory: " + configValues->datalog_files_directory, true, true);
                        }
                        else
                            reader.skipCurrentElement();
                    }
                }
            }
        }
    }
    file.close();

    save_config_file(configValues);

    return configValues;
}

FileActions::ConfigValuesStructure *FileActions::save_config_file(FileActions::ConfigValuesStructure *configValues)
{
    //ConfigValuesStructure *configValues = &ConfigValuesStruct;

    QFile file(configValues->config_file);
    if (!file.open(QIODevice::ReadWrite)) {
        QMessageBox::warning(this, tr("Config file"), "Unable to open config file '" + file.fileName() + "' for writing");
        return 0;
    }

    //QTextStream outStream(&configFile);

    QXmlStreamWriter stream(&file);
    file.resize(0);
    stream.setAutoFormatting(true);
    stream.writeStartDocument();
    stream.writeStartElement("config");
    stream.writeAttribute("name", configValues->software_name);
    stream.writeAttribute("version", configValues->software_version);
    stream.writeStartElement("software_settings");

    stream.writeStartElement("setting");
    stream.writeAttribute("name", "window_size");
    stream.writeStartElement("value");
    stream.writeAttribute("width", configValues->window_width);
    stream.writeEndElement();
    stream.writeStartElement("value");
    stream.writeAttribute("height", configValues->window_height);
    stream.writeEndElement();
    stream.writeEndElement();

    stream.writeStartElement("setting");
    stream.writeAttribute("name", "toolbar_iconsize");
    stream.writeStartElement("value");
    stream.writeAttribute("data", configValues->toolbar_iconsize);
    stream.writeEndElement();
    stream.writeEndElement();

    stream.writeStartElement("setting");
    stream.writeAttribute("name", "serial_port");
    stream.writeStartElement("value");
    stream.writeAttribute("data", configValues->serial_port);
    stream.writeEndElement();
    stream.writeEndElement();

    stream.writeStartElement("setting");
    stream.writeAttribute("name", "protocol_id");
    stream.writeStartElement("value");
    stream.writeAttribute("data", configValues->flash_protocol_selected_id);
    stream.writeEndElement();
    stream.writeEndElement();

    stream.writeStartElement("setting");
    stream.writeAttribute("name", "flash_transport");
    stream.writeStartElement("value");
    stream.writeAttribute("data", configValues->flash_protocol_selected_flash_transport);
    stream.writeEndElement();
    stream.writeEndElement();

    stream.writeStartElement("setting");
    stream.writeAttribute("name", "log_transport");
    stream.writeStartElement("value");
    stream.writeAttribute("data", configValues->flash_protocol_selected_log_transport);
    stream.writeEndElement();
    stream.writeEndElement();

    stream.writeStartElement("setting");
    stream.writeAttribute("name", "log_protocol");
    stream.writeStartElement("value");
    stream.writeAttribute("data", configValues->flash_protocol_selected_log_protocol);
    stream.writeEndElement();
    stream.writeEndElement();

    stream.writeStartElement("setting");
    stream.writeAttribute("name", "primary_definition_base");
    stream.writeStartElement("value");
    stream.writeAttribute("data", configValues->primary_definition_base);
    stream.writeEndElement();
    stream.writeEndElement();

    stream.writeStartElement("setting");
    stream.writeAttribute("name", "calibration_files");
    for (int i = 0; i < configValues->calibration_files.length(); i++)
    {
        stream.writeStartElement("value");
        stream.writeAttribute("data", configValues->calibration_files.at(i));
        stream.writeEndElement();
    }
    stream.writeEndElement();

    stream.writeStartElement("setting");
    stream.writeAttribute("name", "calibration_files_directory");
    stream.writeStartElement("value");
    stream.writeAttribute("data", configValues->calibration_files_directory);
    stream.writeEndElement();
    stream.writeEndElement();

    stream.writeStartElement("setting");
    stream.writeAttribute("name", "use_romraider_definitions");
    stream.writeStartElement("value");
    stream.writeAttribute("data", configValues->use_romraider_definitions);
    stream.writeEndElement();
    stream.writeEndElement();

    stream.writeStartElement("setting");
    stream.writeAttribute("name", "romraider_definition_files");
    for (int i = 0; i < configValues->romraider_definition_files.length(); i++)
    {
        stream.writeStartElement("value");
        stream.writeAttribute("data", configValues->romraider_definition_files.at(i));
        stream.writeEndElement();
    }
    stream.writeEndElement();

    stream.writeStartElement("setting");
    stream.writeAttribute("name", "use_ecuflash_definitions");
    stream.writeStartElement("value");
    stream.writeAttribute("data", configValues->use_ecuflash_definitions);
    stream.writeEndElement();
    stream.writeEndElement();

    stream.writeStartElement("setting");
    stream.writeAttribute("name", "ecuflash_definition_files_directory");
    stream.writeStartElement("value");
    stream.writeAttribute("data", configValues->ecuflash_definition_files_directory);
    stream.writeEndElement();
    stream.writeEndElement();

    stream.writeStartElement("setting");
    stream.writeAttribute("name", "logger_definition_file");
    stream.writeStartElement("value");
    stream.writeAttribute("data", configValues->romraider_logger_definition_file);
    stream.writeEndElement();
    stream.writeEndElement();
/*
    stream.writeStartElement("setting");
    stream.writeAttribute("name", "kernel_files_directory");
    stream.writeStartElement("value");
    stream.writeAttribute("data", configValues->kernel_files_directory);
    stream.writeEndElement();
    stream.writeEndElement();
*/
    stream.writeStartElement("setting");
    stream.writeAttribute("name", "logfiles_directory");
    stream.writeStartElement("value");
    stream.writeAttribute("data", configValues->datalog_files_directory);
    stream.writeEndElement();
    stream.writeEndElement();

    stream.writeEndElement();
    stream.writeEndElement();
    stream.writeEndDocument();

    file.close();

    return 0;
}

FileActions::ConfigValuesStructure *FileActions::read_protocols_file(FileActions::ConfigValuesStructure *configValues)
{
    QDomDocument xmlBOM;

    QString filename = configValues->protocols_file;

    QStringList flash_protocol_name;
    QStringList flash_protocol_alias;
    QStringList flash_protocol_ecu;
    QStringList flash_protocol_mcu;
    QStringList flash_protocol_mode;
    QStringList flash_protocol_checksum;
    QStringList flash_protocol_read;
    QStringList flash_protocol_test_write;
    QStringList flash_protocol_write;
    QStringList flash_protocol_flash_transport;
    QStringList flash_protocol_log_transport;
    QStringList flash_protocol_log_protocol;
    QStringList flash_protocol_ecu_id_ascii;
    QStringList flash_protocol_ecu_id_addr;
    QStringList flash_protocol_ecu_id_length;
    QStringList flash_protocol_cal_id_ascii;
    QStringList flash_protocol_cal_id_addr;
    QStringList flash_protocol_cal_id_length;
    QStringList flash_protocol_kernel;
    QStringList flash_protocol_kernel_addr;
    QStringList flash_protocol_description;
    QStringList flash_protocol_protocol_name;

    QFile file(filename);
    if(!file.open(QFile::ReadWrite | QFile::Text)) {
        QMessageBox::warning(this, tr("Protocols file"), "Unable to open protocols file '" + file.fileName() + "' for reading");
        return NULL;
    }
    xmlBOM.setContent(&file);
    file.close();

    QDomElement root = xmlBOM.documentElement();

    if (root.tagName() == "config")// && root.attribute("name"," ") == configValues->software_version)
    {
        QDomElement root_child = root.firstChild().toElement();
        while (!root_child.isNull())
        {
            if (root_child.tagName() == "protocols")
            {
                //emit LOG_D("Protocols", true, true);
                int index = 0;

                QDomElement protocol = root_child.firstChild().toElement();
                while (!protocol.isNull())
                {
                    if (protocol.tagName() == "protocol")
                    {
                        //emit LOG_D("Protocol", true, true);
                        flash_protocol_name.append(" ");
                        flash_protocol_name.replace(index, protocol.attribute("name","No name"));
                        flash_protocol_alias.append(" ");
                        flash_protocol_alias.replace(index, protocol.attribute("alias","No alias"));

                        flash_protocol_ecu.append(" ");
                        flash_protocol_mcu.append(" ");
                        flash_protocol_mode.append(" ");
                        flash_protocol_checksum.append(" ");
                        flash_protocol_read.append(" ");
                        flash_protocol_test_write.append(" ");
                        flash_protocol_write.append(" ");
                        flash_protocol_flash_transport.append(" ");
                        flash_protocol_log_transport.append(" ");
                        flash_protocol_log_protocol.append(" ");
                        flash_protocol_ecu_id_ascii.append(" ");
                        flash_protocol_ecu_id_addr.append(" ");
                        flash_protocol_ecu_id_length.append(" ");
                        flash_protocol_cal_id_ascii.append(" ");
                        flash_protocol_cal_id_addr.append(" ");
                        flash_protocol_cal_id_length.append(" ");
                        flash_protocol_kernel.append(" ");
                        flash_protocol_kernel_addr.append(" ");
                        flash_protocol_description.append(" ");
                        flash_protocol_protocol_name.append(" ");
                        flash_protocol_protocol_name.replace(index, protocol.attribute("name","No name"));

                        QDomElement protocol_data = protocol.firstChild().toElement();
                        while (!protocol_data.isNull())
                        {
                            if (protocol_data.tagName() == "ecu")
                                flash_protocol_ecu.replace(index, protocol_data.text());
                            if (protocol_data.tagName() == "mcu")
                                flash_protocol_mcu.replace(index, protocol_data.text());
                            if (protocol_data.tagName() == "mode")
                                flash_protocol_mode.replace(index, protocol_data.text());
                            if (protocol_data.tagName() == "checksum")
                                flash_protocol_checksum.replace(index, protocol_data.text());
                            if (protocol_data.tagName() == "read")
                                flash_protocol_read.replace(index, protocol_data.text());
                            if (protocol_data.tagName() == "test_write")
                                flash_protocol_test_write.replace(index, protocol_data.text());
                            if (protocol_data.tagName() == "write")
                                flash_protocol_write.replace(index, protocol_data.text());
                            if (protocol_data.tagName() == "flash_transport")
                                flash_protocol_flash_transport.replace(index, protocol_data.text());
                            if (protocol_data.tagName() == "log_transport")
                                flash_protocol_log_transport.replace(index, protocol_data.text());
                            if (protocol_data.tagName() == "log_protocol")
                                flash_protocol_log_protocol.replace(index, protocol_data.text());
                            if (protocol_data.tagName() == "ecu_id_ascii")
                                flash_protocol_ecu_id_ascii.replace(index, protocol_data.text());
                            if (protocol_data.tagName() == "ecu_id_addr")
                                flash_protocol_ecu_id_addr.replace(index, protocol_data.text());
                            if (protocol_data.tagName() == "ecu_id_length")
                                flash_protocol_ecu_id_length.replace(index, protocol_data.text());
                            if (protocol_data.tagName() == "cal_id_ascii")
                                flash_protocol_cal_id_ascii.replace(index, protocol_data.text());
                            if (protocol_data.tagName() == "cal_id_addr")
                                flash_protocol_cal_id_addr.replace(index, protocol_data.text());
                            if (protocol_data.tagName() == "cal_id_length")
                                flash_protocol_cal_id_length.replace(index, protocol_data.text());
                            if (protocol_data.tagName() == "kernel")
                                flash_protocol_kernel.replace(index, protocol_data.text());
                            if (protocol_data.tagName() == "kernel_addr")
                                flash_protocol_kernel_addr.replace(index, protocol_data.text());
                            if (protocol_data.tagName() == "description")
                                flash_protocol_description.replace(index, protocol_data.text());
                            //if (protocol_data.tagName() == "protocol_name")
                                //flash_protocol_protocol_name.replace(index, protocol_data.text());

                            protocol_data = protocol_data.nextSibling().toElement();

                        }
                        //emit LOG_D("Flash protocol name: " + flash_protocol_name.at(index) + " and family: " + flash_protocol_protocol_name.at(index), true, true);
                        index++;
                    }
                    protocol = protocol.nextSibling().toElement();
                }
            }
            if (root_child.tagName() == "car_models")
            {
                int index = 0;
                int id = 0;
                QDomElement car_model = root_child.firstChild().toElement();
                while (!car_model.isNull())
                {
                    if (car_model.tagName() == "car_model")
                    {
                        //emit LOG_D("Add new vehicle", true, true);
                        configValues->flash_protocol_id.append(QString::number(id));//car_model.attribute("id","No id"));
                        configValues->flash_protocol_alias.append(" ");
                        configValues->flash_protocol_make.append(" ");
                        configValues->flash_protocol_model.append(" ");
                        configValues->flash_protocol_version.append(" ");
                        configValues->flash_protocol_type.append(" ");
                        configValues->flash_protocol_kw.append(" ");
                        configValues->flash_protocol_hp.append(" ");
                        configValues->flash_protocol_fuel.append(" ");
                        configValues->flash_protocol_year.append(" ");
                        configValues->flash_protocol_ecu.append(" ");
                        configValues->flash_protocol_mcu.append(" ");
                        configValues->flash_protocol_mode.append(" ");
                        configValues->flash_protocol_checksum.append(" ");
                        configValues->flash_protocol_read.append(" ");
                        configValues->flash_protocol_test_write.append(" ");
                        configValues->flash_protocol_write.append(" ");
                        configValues->flash_protocol_flash_transport.append(" ");
                        configValues->flash_protocol_log_transport.append(" ");
                        configValues->flash_protocol_log_protocol.append(" ");
                        configValues->flash_protocol_ecu_id_ascii.append(" ");
                        configValues->flash_protocol_ecu_id_addr.append(" ");
                        configValues->flash_protocol_ecu_id_length.append(" ");
                        configValues->flash_protocol_cal_id_ascii.append(" ");
                        configValues->flash_protocol_cal_id_addr.append(" ");
                        configValues->flash_protocol_cal_id_length.append(" ");
                        configValues->flash_protocol_kernel.append(" ");
                        configValues->flash_protocol_kernel_addr.append(" ");
                        configValues->flash_protocol_description.append(" ");
                        configValues->flash_protocol_protocol_name.append(" ");

                        id++;
                        QDomElement car_model_data = car_model.firstChild().toElement();
                        while (!car_model_data.isNull())
                        {
                            //emit LOG_D(flash_protocol_data.tagName(), true, true);
                            if (car_model_data.tagName() == "make")
                                configValues->flash_protocol_make.replace(index, car_model_data.text());
                            if (car_model_data.tagName() == "model")
                                configValues->flash_protocol_model.replace(index, car_model_data.text());
                            if (car_model_data.tagName() == "version")
                                configValues->flash_protocol_version.replace(index, car_model_data.text());
                            if (car_model_data.tagName() == "type")
                                configValues->flash_protocol_type.replace(index, car_model_data.text());
                            if (car_model_data.tagName() == "kw")
                                configValues->flash_protocol_kw.replace(index, car_model_data.text());
                            if (car_model_data.tagName() == "hp")
                                configValues->flash_protocol_hp.replace(index, car_model_data.text());
                            if (car_model_data.tagName() == "fuel")
                                configValues->flash_protocol_fuel.replace(index, car_model_data.text());
                            if (car_model_data.tagName() == "year")
                                configValues->flash_protocol_year.replace(index, car_model_data.text());
                            if (car_model_data.tagName() == "protocol")
                            {
                                configValues->flash_protocol_protocol_name.replace(index, car_model_data.text());
                                for (int i = 0; i < flash_protocol_protocol_name.length(); i++)
                                {
                                    if (flash_protocol_protocol_name.at(i) == configValues->flash_protocol_protocol_name.at(index))
                                    {
                                        configValues->flash_protocol_alias.replace(index, flash_protocol_alias.at(i));
                                        configValues->flash_protocol_ecu.replace(index, flash_protocol_ecu.at(i));
                                        configValues->flash_protocol_mcu.replace(index, flash_protocol_mcu.at(i));
                                        configValues->flash_protocol_mode.replace(index, flash_protocol_mode.at(i));
                                        configValues->flash_protocol_checksum.replace(index, flash_protocol_checksum.at(i));
                                        configValues->flash_protocol_read.replace(index, flash_protocol_read.at(i));
                                        configValues->flash_protocol_test_write.replace(index, flash_protocol_test_write.at(i));
                                        configValues->flash_protocol_write.replace(index, flash_protocol_write.at(i));
                                        configValues->flash_protocol_flash_transport.replace(index, flash_protocol_flash_transport.at(i));
                                        configValues->flash_protocol_log_transport.replace(index, flash_protocol_log_transport.at(i));
                                        configValues->flash_protocol_log_protocol.replace(index, flash_protocol_log_protocol.at(i));
                                        configValues->flash_protocol_ecu_id_ascii.replace(index, flash_protocol_ecu_id_ascii.at(i));
                                        configValues->flash_protocol_ecu_id_addr.replace(index, flash_protocol_ecu_id_addr.at(i));
                                        configValues->flash_protocol_ecu_id_length.replace(index, flash_protocol_ecu_id_length.at(i));
                                        configValues->flash_protocol_cal_id_ascii.replace(index, flash_protocol_cal_id_ascii.at(i));
                                        configValues->flash_protocol_cal_id_addr.replace(index, flash_protocol_cal_id_addr.at(i));
                                        configValues->flash_protocol_cal_id_length.replace(index, flash_protocol_cal_id_length.at(i));
                                        configValues->flash_protocol_kernel.replace(index, flash_protocol_kernel.at(i));
                                        configValues->flash_protocol_kernel_addr.replace(index, flash_protocol_kernel_addr.at(i));
                                        configValues->flash_protocol_description.replace(index, flash_protocol_description.at(i));
                                    }
                                }
                            }
                            //if (car_model_data.tagName() == "description")
                                //configValues->flash_protocol_description.replace(index, car_model_data.text());

                            car_model_data = car_model_data.nextSibling().toElement();
                        }
                        //emit LOG_D("Flash protocol ID: " + configValues->flash_protocol_id.at(index) + " make: " + configValues->flash_protocol_make.at(index) + " model: " + configValues->flash_protocol_model.at(index) + " flash method: " + configValues->flash_protocol_protocol_name.at(index), true, true);
                        index++;
                    }
                    car_model = car_model.nextSibling().toElement();
                }
            }
            root_child = root_child.nextSibling().toElement();
        }
    }

    return configValues;
}

FileActions::LogValuesStructure *FileActions::read_logger_conf(FileActions::LogValuesStructure *logValues, QString ecu_id, bool modify)
{
    ConfigValuesStructure *configValues = &ConfigValuesStruct;

    QDomDocument xmlBOM;

    QString filename = configValues->logger_file;

    emit LOG_D("Looking for ECU ID: " + ecu_id + " in logger def file: " + configValues->logger_file, true, true);

    QFile file(filename);
    if(!file.open(QFile::ReadWrite | QFile::Text)) {
        QMessageBox::warning(this, tr("Logger file"), "Unable to open logger config file '" + file.fileName() + "' for reading");
        return NULL;
    }
    xmlBOM.setContent(&file);

    if (!modify)
    {
        logValues->dashboard_log_value_id.clear();
        logValues->lower_panel_log_value_id.clear();
        logValues->lower_panel_switch_id.clear();
    }

    bool ecu_id_found = false;
    int index = 0;

    QDomElement root = xmlBOM.documentElement();

    if (root.tagName() == "config")
    {
        QDomElement logger = root.firstChild().toElement();
        if (logger.tagName() == "logger")
        {
            QDomElement ecu = logger.firstChild().toElement();
            while (!ecu.isNull())
            {
                if (ecu.tagName() == "ecu")
                {
                    QString file_ecu_id = ecu.attribute("id","No id");
                    QString ecu_id_active = ecu.attribute("active","false");
                    if (ecu_id == file_ecu_id)
                    {
                        ecu_id_found = true;
                        emit LOG_D("Found ECU ID " + file_ecu_id, true, true);
                        QDomElement protocol = ecu.firstChild().toElement();
                        while (!protocol.isNull())
                        {
                            if (protocol.tagName() == "protocol")
                            {
                                emit LOG_D("Found protocol " + protocol.attribute("id","No id"), true, true);
                                logValues->logging_values_protocol = protocol.attribute("id","No id");
                                QDomElement parameters = protocol.firstChild().toElement();
                                while(!parameters.isNull())
                                {
                                    if (parameters.tagName() == "parameters")
                                    {
                                        QDomElement parameter_type = parameters.firstChild().toElement();
                                        while(!parameter_type.isNull())
                                        {
                                            if (parameter_type.tagName() == "gauges")
                                            {
                                                index = 0;
                                                QDomElement gauges = parameter_type.firstChild().toElement();
                                                while(!gauges.isNull())
                                                {
                                                    if (gauges.tagName() == "parameter")
                                                    {
                                                        if (!modify)
                                                            logValues->dashboard_log_value_id.append(gauges.attribute("id","No id"));
                                                        else
                                                            gauges.setAttribute("id", logValues->dashboard_log_value_id.at(index));
                                                    }
                                                    gauges = gauges.nextSibling().toElement();
                                                    index++;
                                                }
                                            }
                                            if (parameter_type.tagName() == "lower_panel")
                                            {
                                                index = 0;
                                                QDomElement lower_panel = parameter_type.firstChild().toElement();
                                                while(!lower_panel.isNull())
                                                {
                                                    if (lower_panel.tagName() == "parameter")
                                                    {
                                                        if (!modify)
                                                            logValues->lower_panel_log_value_id.append(lower_panel.attribute("id","No id"));
                                                        else
                                                        {
                                                            QDomElement parameter = xmlBOM.createElement("parameter");
                                                            parameter.setAttribute("id", logValues->lower_panel_log_value_id.at(index));
                                                            parameter.setAttribute("name", "");

                                                            lower_panel.setAttribute("id", logValues->lower_panel_log_value_id.at(index));
                                                        }
                                                    }
                                                    lower_panel = lower_panel.nextSibling().toElement();
                                                    index++;
                                                }
                                            }
                                            parameter_type = parameter_type.nextSibling().toElement();
                                        }
                                    }
                                    if (parameters.tagName() == "switches")
                                    {
                                        index = 0;
                                        QDomElement switches = parameters.firstChild().toElement();
                                        while(!switches.isNull())
                                        {
                                            if (switches.tagName() == "switch")
                                            {
                                                if (!modify)
                                                    logValues->lower_panel_switch_id.append(switches.attribute("id","No id"));
                                                else
                                                    switches.setAttribute("id", logValues->lower_panel_switch_id.at(index));
                                            }
                                            switches = switches.nextSibling().toElement();
                                            index++;
                                        }
                                    }
                                    parameters = parameters.nextSibling().toElement();
                                }
                            }
                            protocol = protocol.nextSibling().toElement();
                        }
                    }
                }
                ecu = ecu.nextSibling().toElement();
            }
        }
        if (!ecu_id_found)
        {
            if (!logValues->log_value_protocol.length())
            {
                QMessageBox::warning(this, tr("Logger definition file"), "No logger definition file selected, returning without initializing log parameters!");
                emit LOG_D("No logger definition file selected, returning without initializing log parameters!", true, true);
                return 0;
            }
            emit LOG_D("ECU ID not found, initializing log parameters", true, true);
            logValues->logging_values_protocol = logValues->log_value_protocol.at(0);
            emit LOG_D("Initializing gauge parameters", true, true);
            for (int i = 0; i < logValues->log_value_id.length(); i++)
            {
                if (logValues->log_value_enabled.at(i) == "1" && logValues->dashboard_log_value_id.length() < 15)
                    logValues->dashboard_log_value_id.append(logValues->log_value_id.at(i));
            }
            emit LOG_D("Initializing lower panel parameters", true, true);
            for (int i = 0; i < logValues->log_value_id.length(); i++)
            {
                if (logValues->log_value_enabled.at(i) == "1" && logValues->lower_panel_log_value_id.length() < 12)
                logValues->lower_panel_log_value_id.append(logValues->log_value_id.at(i));
            }
            emit LOG_D("Initializing switch parameters", true, true);
            for (int i = 0; i < logValues->log_switch_id.length(); i++)
            {
                if (logValues->log_switch_enabled.at(i) == "1" && logValues->lower_panel_switch_id.length() < 20)
                logValues->lower_panel_switch_id.append(logValues->log_switch_id.at(i));
            }
            emit LOG_D("Values initialized, creating xml data", true, true);
            //save_logger_conf(logValues, ecu_id);
            QDomElement ecu = xmlBOM.createElement("ecu");
            ecu.setAttribute("id", ecu_id);
            logger.appendChild(ecu);
            QDomElement protocol = xmlBOM.createElement("protocol");
            protocol.setAttribute("id", logValues->logging_values_protocol);
            ecu.appendChild(protocol);
            QDomElement parameters = xmlBOM.createElement("parameters");
            protocol.appendChild(parameters);
            QDomElement gauges = xmlBOM.createElement("gauges");
            parameters.appendChild(gauges);
            for (int i = 0; i < logValues->dashboard_log_value_id.length(); i++)
            {
                QDomElement parameter = xmlBOM.createElement("parameter");
                gauges.appendChild(parameter);
                parameter.setAttribute("id", logValues->dashboard_log_value_id.at(i));
                parameter.setAttribute("name", "");
            }
            QDomElement lower_panel = xmlBOM.createElement("lower_panel");
            parameters.appendChild(lower_panel);
            for (int i = 0; i < logValues->lower_panel_log_value_id.length(); i++)
            {
                QDomElement parameter = xmlBOM.createElement("parameter");
                lower_panel.appendChild(parameter);
                parameter.setAttribute("id", logValues->lower_panel_log_value_id.at(i));
                parameter.setAttribute("name", "");
            }
            QDomElement switches = xmlBOM.createElement("switches");
            protocol.appendChild(switches);
            for (int i = 0; i < logValues->lower_panel_switch_id.length(); i++)
            {
                QDomElement parameter = xmlBOM.createElement("switch");
                switches.appendChild(parameter);
                parameter.setAttribute("id", logValues->lower_panel_switch_id.at(i));
                parameter.setAttribute("name", "");
            }
            //emit LOG_D("Saving log parameters", true, true);
            file.resize(0);
            QTextStream output(&file);
            xmlBOM.save(output, 4);
            file.close();
        }
    }
    if (modify)
    {
        file.resize(0);
        QTextStream output(&file);
        xmlBOM.save(output, 4);
        //output << xmlBOM.toString();
    }
    file.close();

    return logValues;
}
/*
void *FileActions::save_logger_conf(FileActions::LogValuesStructure *logValues, QString ecu_id)
{
    ConfigValuesStructure *configValues = &ConfigValuesStruct;

    QFile file(configValues->logger_file);
    if (!file.open(QIODevice::ReadWrite)) {
        QMessageBox::warning(this, tr("Config file"), "Unable to open logger config file '" + file.fileName() + "' for writing");
        return 0;
    }
    QXmlStreamReader reader;
    reader.setDevice(&file);

    QXmlStreamWriter stream(&file);
    //file.resize(0);
    stream.setAutoFormatting(true);
    stream.writeStartDocument();
    stream.writeStartElement("config");
    stream.writeAttribute("name", configValues->software_title);
    stream.writeAttribute("version", configValues->software_version);
    emit LOG_D("Software version: " + configValues->software_version;
    stream.writeStartElement("logger");
    stream.writeStartElement("ecu");
    stream.writeAttribute("id", ecu_id);
    stream.writeStartElement("protocol");
    stream.writeAttribute("id", logValues->logging_values_protocol);
    stream.writeStartElement("parameters");
    stream.writeStartElement("gauges");
    for (int i = 0; i < 15; i++)
    {
        stream.writeStartElement("parameter");
        stream.writeAttribute("id", logValues->dashboard_log_value_id.at(i));
        stream.writeAttribute("name", "");
        stream.writeEndElement();
    }
    stream.writeEndElement();
    stream.writeStartElement("lower_panel");
    for (int i = 0; i < 15; i++)
    {
        stream.writeStartElement("parameter");
        stream.writeAttribute("id", logValues->dashboard_log_value_id.at(i));
        stream.writeAttribute("name", "");
        stream.writeEndElement();
    }
    stream.writeEndElement();
    stream.writeEndElement();
    stream.writeStartElement("switches");
    for (int i = 0; i < 15; i++)
    {
        stream.writeStartElement("switch");
        stream.writeAttribute("id", logValues->dashboard_log_value_id.at(i));
        stream.writeAttribute("name", "");
        stream.writeEndElement();
    }
    stream.writeEndElement();
    stream.writeEndElement();
    stream.writeEndElement();
    stream.writeEndElement();
    stream.writeEndElement();

    return 0;
}
*/
FileActions::LogValuesStructure *FileActions::read_logger_definition_file()
{
    LogValuesStructure *logValues = &LogValuesStruct;
    ConfigValuesStructure *configValues = &ConfigValuesStruct;

    //The QDomDocument class represents an XML document.
    QDomDocument xmlBOM;

    QString filename = configValues->romraider_logger_definition_file;
    //emit LOG_D("Logger filename = " + filename;
    QFile file(filename);
    if(!file.open(QFile::ReadOnly | QFile::Text)) {
        QMessageBox::warning(this, tr("Logger file"), "Unable to open logger definition file '" + file.fileName() + "' for reading");
        return logValues;
    }
    xmlBOM.setContent(&file);
    file.close();

    // Extract the root markup
    QDomElement root = xmlBOM.documentElement();

    // Get root names and attributes
    //QString Type = root.tagName();
    //QString Name = root.attribute("name","No name");

    int log_value_index = 0;
    int log_switch_index = 0;

    if (root.tagName() == "logger")
    {
        //emit LOG_D("Logger start element";
        QDomElement protocols = root.firstChild().toElement();

        while (!protocols.isNull())
        {
            //emit LOG_D("Protocols start element";
            if (protocols.tagName() == "protocols")
            {
                QDomElement protocol = protocols.firstChild().toElement();
                while(!protocol.isNull())
                {
                    if (protocol.tagName() == "protocol")
                    {
                        QString log_value_protocol = protocol.attribute("id","No protocol id");
                        //emit LOG_D("Protocol = " + protocol.attribute("id","No id");
                        QDomElement transports = protocol.firstChild().toElement();
                        while(!transports.isNull())
                        {
                            if (transports.tagName() == "transports")
                            {
                                //emit LOG_D("Transports for protocol " + protocol.attribute("id","No id");
                                QDomElement transport = transports.firstChild().toElement();
                                while(!transport.isNull())
                                {
                                    if (transport.tagName() == "transport")
                                    {
                                        //emit LOG_D("Transport = " + transport.attribute("id","No id") + " " + transport.attribute("name","No name") + " " + transport.attribute("desc","No description");
                                    }
                                    QDomElement module = transport.firstChild().toElement();
                                    while(!module.isNull())
                                    {
                                        if (module.tagName() == "module")
                                        {
                                            //emit LOG_D("Module = " + module.attribute("id","No id") + " " + module.attribute("address","No address") + " " + module.attribute("desc","No description") + " " + module.attribute("tester","No tester");
                                        }
                                        module = module.nextSibling().toElement();
                                    }
                                    transport = transport.nextSibling().toElement();
                                }
                            }
                            if (transports.tagName() == "parameters")
                            {
                                QDomElement parameter = transports.firstChild().toElement();
                                while(!parameter.isNull())
                                {
                                    if (parameter.tagName() == "parameter")
                                    {
                                        QString log_value_id = parameter.attribute("id","No id");
                                        //emit LOG_D("Parameter = " + parameter.attribute("id","No id") + " " + parameter.attribute("name","No name") + " " + parameter.attribute("desc","No description");
                                        logValues->log_value_protocol.append(log_value_protocol);
                                        logValues->log_value_id.append(parameter.attribute("id","No id"));
                                        logValues->log_value_name.append(parameter.attribute("name","No name"));
                                        logValues->log_value_description.append(parameter.attribute("desc","No desc"));
                                        logValues->log_value_ecu_byte_index.append(parameter.attribute("ecubyteindex","No byte index"));
                                        logValues->log_value_ecu_bit.append(parameter.attribute("ecubit","No ecu bit"));
                                        logValues->log_value_target.append(parameter.attribute("target","No target"));
                                        logValues->log_value_enabled.append("0");
                                        logValues->log_value.append("0.00");
                                        if (log_value_index < 12)
                                            logValues->lower_panel_log_value_id.append(log_value_id);
                                        if (log_value_index < 15)
                                            logValues->dashboard_log_value_id.append(log_value_id);

                                        QDomElement param_options = parameter.firstChild().toElement();
                                        while (!param_options.isNull())
                                        {
                                            if (param_options.tagName() == "address")
                                            {
                                                //emit LOG_D("Address = " + param_options.text();
                                                logValues->log_value_length.append(parameter.attribute("length","1"));
                                                logValues->log_value_address.append(param_options.text());
                                            }
                                            if (param_options.tagName() == "conversions")
                                            {
                                                QDomElement conversion = param_options.firstChild().toElement();
                                                QString param_conversion;
                                                int conversion_count = 0;
                                                while (!conversion.isNull())
                                                {
                                                    if (conversion.tagName() == "conversion")
                                                    {
                                                        //emit LOG_D("Conversion = " + conversion.attribute("units","No units") + " " + conversion.attribute("expr","No expr") + " " + conversion.attribute("format","No format") + " " + conversion.attribute("gauge_min","0") + " " + conversion.attribute("gauge_max","0") + " " + conversion.attribute("gauge_step","0");
                                                        param_conversion.append("conversion " + QString::number(conversion_count) + ",");
                                                        param_conversion.append(conversion.attribute("units","#") + ",");
                                                        param_conversion.append(conversion.attribute("expr","x") + ",");
                                                        param_conversion.append(conversion.attribute("format","0.00") + ",");
                                                        param_conversion.append(conversion.attribute("gauge_min","No gauge_min") + ",");
                                                        param_conversion.append(conversion.attribute("gauge_max","No gauge_max") + ",");
                                                        param_conversion.append(conversion.attribute("gauge_step","No gauge_step"));
                                                    }
                                                    conversion_count++;
                                                    conversion = conversion.nextSibling().toElement();
                                                    if (!conversion.isNull())
                                                        param_conversion.append(",");
                                                }
                                                logValues->log_value_units.append(param_conversion);
                                            }
                                            param_options = param_options.nextSibling().toElement();
                                        }
                                        log_value_index++;
                                    }
                                    parameter = parameter.nextSibling().toElement();
                                }
                            }
                            if (transports.tagName() == "switches")
                            {
                                QDomElement paramswitch = transports.firstChild().toElement();
                                while(!paramswitch.isNull())
                                {
                                    if (paramswitch.tagName() == "switch")
                                    {
                                        QString log_switch_id = paramswitch.attribute("id","No id");
                                        logValues->log_switch_protocol.append(log_value_protocol);
                                        logValues->log_switch_id.append(paramswitch.attribute("id","No id"));
                                        logValues->log_switch_name.append(paramswitch.attribute("name","No name"));
                                        logValues->log_switch_description.append(paramswitch.attribute("desc","No desc"));
                                        logValues->log_switch_address.append(paramswitch.attribute("byte","No address"));
                                        logValues->log_switch_ecu_byte_index.append(paramswitch.attribute("ecubyteindex","No ecu byte index"));
                                        logValues->log_switch_ecu_bit.append(paramswitch.attribute("bit","No ecu bit"));
                                        logValues->log_switch_target.append(paramswitch.attribute("target","No target"));
                                        logValues->log_switch_enabled.append("0");
                                        logValues->log_switch_state.append("0");
                                        if (log_switch_index < 20)
                                            logValues->lower_panel_switch_id.append(log_switch_id);
                                        log_switch_index++;
                                        //emit LOG_D("Switch = " + paramswitch.attribute("id","No id") + " " + paramswitch.attribute("name","No name") + " " + paramswitch.attribute("desc","No description");
                                    }
                                    paramswitch = paramswitch.nextSibling().toElement();
                                }
                            }
                            if (transports.tagName() == "dtcodes")
                            {
                                QDomElement dtcode = transports.firstChild().toElement();
                                while(!dtcode.isNull())
                                {
                                    if (dtcode.tagName() == "dtcode")
                                    {
                                        //emit LOG_D("DT code = " + dtcode.attribute("id","No id") + " " + dtcode.attribute("name","No name") + " " + dtcode.attribute("desc","No description");
                                    }
                                    dtcode = dtcode.nextSibling().toElement();
                                }
                            }
                            if (transports.tagName() == "ecuparams")
                            {
                                QDomElement ecuparam = transports.firstChild().toElement();
                                while(!ecuparam.isNull())
                                {
                                    if (ecuparam.tagName() == "ecuparam")
                                    {
                                        //emit LOG_D("ECU param = " + ecuparam.attribute("id","No id") + " " + ecuparam.attribute("name","No name") + " " + ecuparam.attribute("desc","No description");
                                    }
                                    ecuparam = ecuparam.nextSibling().toElement();
                                }
                            }
                            transports = transports.nextSibling().toElement();
                        }
                    }
                    protocol = protocol.nextSibling().toElement();
                }
            }
            protocols = protocols.nextSibling().toElement();
        }
    }

    //log_values_names_sorted
    //log_switch_names_sorted
    return logValues;
}

QSignalMapper *FileActions::read_menu_file(QMenuBar *menubar, QToolBar *toolBar)
{
    ConfigValuesStructure *configValues = &ConfigValuesStruct;

    QMenu *mainWindowMenu;
    QMenu *subMenu;

    QSignalMapper *mapper = new QSignalMapper(this);
    bool toolBarIconSet = false;

    //The QDomDocument class represents an XML document.
    QDomDocument xmlBOM;
    // Load xml file as raw data
    QFile file(configValues->menu_file);
    if (!file.open(QIODevice::ReadOnly ))
    {
        // Error while loading file
        QMessageBox::warning(this, tr("Ecu menu file"), "Unable to open menu config file '" + file.fileName() + "' for reading");
        return mapper;
    }
    // Set data into the QDomDocument before processing
    xmlBOM.setContent(&file);
    file.close();

    // Extract the root markup
    QDomElement root = xmlBOM.documentElement();

    // Get root names and attributes
    QString Type = root.tagName();
    QString Name = root.attribute("name","No name");

    QDomElement TagType = root.firstChild().toElement();

    while (!TagType.isNull())
    {
        if (TagType.tagName() == "ecu_menu_definitions")
        {
            QDomElement Component = TagType.firstChild().toElement();
            while(!Component.isNull())
            {
                // Check if the child tag name is categories
                if (Component.tagName() == "menu")
                {
                    QString menuName = Component.attribute("name","No name");
                    mainWindowMenu = menubar->addMenu(menuName);
                    //emit LOG_D("Menu:  " + menuName;

                    QDomElement menu_item = Component.firstChild().toElement();
                    while(!menu_item.isNull())
                    {
                        // Check if the child tag name is table
                        if (menu_item.tagName() == "menu")
                        {
                            QString subMenuName = menu_item.attribute("name","No name");
                            subMenu = mainWindowMenu->addMenu(subMenuName);
                            QDomElement sub_menu_item = menu_item.firstChild().toElement();
                            while(!sub_menu_item.isNull())
                            {
                                if (sub_menu_item.tagName() == "menuitem")
                                {
                                    QString menuItemName = sub_menu_item.attribute("name","No name");
                                    if (menuItemName == "Separator"){
                                        subMenu->addSeparator();
                                    }
                                    else{
                                        QAction* action = new QAction(menuItemName, this);
                                        QString menuItemActionName = sub_menu_item.attribute("id","No id");;
                                        QString menuItemCheckable = sub_menu_item.attribute("checkable","No checkable");;
                                        QString menuItemShortcut = sub_menu_item.attribute("shortcut","No shortcut");;
                                        QString menuItemToolbar = sub_menu_item.attribute("toolbar","No toolbar");;
                                        QString menuItemIcon = sub_menu_item.attribute("icon","No icon");;
                                        QString menuItemTooltip = sub_menu_item.attribute("tooltip","No tooltip");;

                                        action->setObjectName(menuItemActionName);
                                        //emit LOG_D(menuItemShortcut;
                                        action->setShortcut(menuItemShortcut);
                                        action->setIcon(QIcon(menuItemIcon));
                                        action->setIconVisibleInMenu(true);
                                        action->setToolTip(subMenuName + "\n\n" + menuItemTooltip);
                                        if (menuItemCheckable == "true")
                                            action->setCheckable(true);
                                        else
                                            action->setCheckable(false);
                                        if (menuItemToolbar == "true"){
                                            toolBar->addAction(action);
                                            toolBarIconSet = true;
                                        }

                                        subMenu->addAction(action);
                                        mapper->setMapping(action, action->objectName());
                                        connect(action, SIGNAL(triggered(bool)), mapper, SLOT(map()));

                                    }

                                }

                                sub_menu_item = sub_menu_item.nextSibling().toElement();
                            }
                        }
                        if (menu_item.tagName() == "menuitem")
                        {
                            QString menuItemName = menu_item.attribute("name","No name");
                            if (menuItemName == "Separator"){
                                mainWindowMenu->addSeparator();
                            }
                            else{
                                QAction* action = new QAction(menuItemName, this);
                                QString menuItemActionName = menu_item.attribute("id","No id");;
                                QString menuItemCheckable = menu_item.attribute("checkable","No checkable");;
                                QString menuItemShortcut = menu_item.attribute("shortcut","No shortcut");;
                                QString menuItemToolbar = menu_item.attribute("toolbar","No toolbar");;
                                QString menuItemIcon = menu_item.attribute("icon","No icon");;
                                QString menuItemTooltip = menu_item.attribute("tooltip","No tooltip");;

                                action->setObjectName(menuItemActionName);
                                //emit LOG_D(menuItemShortcut;
                                action->setShortcut(menuItemShortcut);
                                action->setIcon(QIcon(menuItemIcon));
                                action->setIconVisibleInMenu(true);
                                action->setToolTip(menuItemName + "\n\n" + menuItemTooltip);
                                if (menuItemCheckable == "true")
                                    action->setCheckable(true);
                                else
                                    action->setCheckable(false);
                                if (menuItemToolbar == "true"){
                                    toolBar->addAction(action);
                                    toolBarIconSet = true;
                                }

                                mainWindowMenu->addAction(action);
                                mapper->setMapping(action, action->objectName());
                                connect(action, SIGNAL(triggered(bool)), mapper, SLOT(map()));

                            }
                        }
                        menu_item = menu_item.nextSibling().toElement();
                    }
                    if (toolBarIconSet)
                        toolBar->addSeparator();
                    toolBarIconSet = false;
                }
                Component = Component.nextSibling().toElement();
            }
        }
/*
        if (TagType.tagName() == "popup_menu_definitions")
        {
            QDomElement Component = TagType.firstChild().toElement();
            while(!Component.isNull())
            {
                if (Component.tagName() == "menu")
                {
                    QDomElement Child = Component.firstChild().toElement();
                    while(!Child.isNull())
                    {
                        if (Child.tagName() == "menuitem")
                        {
                            popupMenuItemName.append(Child.attribute("name","No name"));
                            popupMenuItemActionName.append(Child.attribute("id","No id"));
                            popupMenuItemCheckable.append(Child.attribute("checkable","No checkable"));
                            popupMenuItemShortcut.append(Child.attribute("shortcut","No shortcut"));
                            popupMenuItemToolbar.append(Child.attribute("toolbar","No toolbar"));
                            popupMenuItemIcon.append(Child.attribute("icon","No icon"));
                            popupMenuItemTooltip.append(Child.attribute("tooltip","No tooltip"));
                        }
                        Child = Child.nextSibling().toElement();
                    }
                    if (toolBarIconSet)
                        ui->mainToolBar->addSeparator();
                    toolBarIconSet = false;
                }
                Component = Component.nextSibling().toElement();
            }
        }
*/
        TagType = TagType.nextSibling().toElement();
    }
    //connect(mapper, SIGNAL(mapped(QString)), this, SLOT(menuActionTriggered(QString)));

    return mapper;
}

QString FileActions::parse_hex_ecuid(uint8_t byte)
{
    QString ecuid_byte;
    unsigned char chars[] = "0123456789ABCDEF";

    ecuid_byte = (QChar)chars[(byte >> 4) & 0xF];
    ecuid_byte.append((QChar)chars[(byte >> 0) & 0xF]);
    //emit LOG_D("Constructed byte: " + ecuid_byte;

    return ecuid_byte;
}

FileActions::EcuCalDefStructure *FileActions::parse_ecuid_ecuflash_def_files(FileActions::EcuCalDefStructure *ecuCalDef, bool is_ascii)
{
    ConfigValuesStructure *configValues = &ConfigValuesStruct;

    QString cal_id_ascii;
    QString cal_id_hex;

    bool bStatus = false;
    bool cal_id_ascii_confirmed = false;
    bool cal_id_hex_confirmed = false;

    //emit LOG_D("Parse EcuFlash def files " + ecuCalDef->RomId;

    for (int i = 0; i < configValues->ecuflash_def_cal_id.length(); i++)
    {
        cal_id_ascii.clear();
        cal_id_hex.clear();
        int addr = configValues->ecuflash_def_cal_id_addr.at(i).toUInt(&bStatus, 16);
        int ecuid_length = configValues->ecuflash_def_cal_id.at(i).length();
        for (int j = addr; j < addr + ecuid_length; j++)
        {
            if (ecuCalDef->FullRomData.length() > addr + ecuid_length)
            {
                uint8_t byte = (uint8_t)ecuCalDef->FullRomData.at(j);
                if (cal_id_hex.length() < ecuid_length)
                    cal_id_hex.append(parse_hex_ecuid(byte));
                cal_id_ascii.append((QChar)byte);
            }
        }

        if (cal_id_ascii == configValues->ecuflash_def_cal_id.at(i))
            cal_id_ascii_confirmed = true;
        if (cal_id_hex == configValues->ecuflash_def_cal_id.at(i))
            cal_id_hex_confirmed = true;

        //emit LOG_D("EcuFlash cal id: " + i + " " + configValues->ecuflash_def_cal_id.at(i) + " -> " + cal_id_ascii + " -> " + cal_id_hex;

        if (cal_id_ascii_confirmed)
        {
            emit LOG_D("EcuFlash cal id " + cal_id_ascii + " found", true, true);
            ecuCalDef->RomId = cal_id_ascii;
            break;
        }
        if (cal_id_hex_confirmed)
        {
            emit LOG_D("EcuFlash cal id " + cal_id_hex + "found", true, true);
            ecuCalDef->RomId = cal_id_hex;
            break;
        }
    }
    //emit LOG_D("Parse EcuFlash def files no ID found... " + ecuCalDef->RomId;

    return ecuCalDef;
}

FileActions::EcuCalDefStructure *FileActions::parse_ecuid_romraider_def_files(FileActions::EcuCalDefStructure *ecuCalDef, bool is_ascii)
{
    ConfigValuesStructure *configValues = &ConfigValuesStruct;

    QString cal_id_ascii;
    QString cal_id_hex;

    bool bStatus = false;
    bool cal_id_ascii_confirmed = false;
    bool cal_id_hex_confirmed = false;

    //emit LOG_D("Parse RomRaider def files " + ecuCalDef->RomId;

    for (int i = 0; i < configValues->romraider_def_cal_id.length(); i++)
    {
        cal_id_ascii.clear();
        cal_id_hex.clear();
        int addr = configValues->romraider_def_cal_id_addr.at(i).toUInt(&bStatus, 16);
        int ecuid_length = configValues->romraider_def_cal_id.at(i).length();
        for (int j = addr; j < addr + ecuid_length; j++)
        {
            if (ecuCalDef->FullRomData.length() > addr + ecuid_length)
            {
                uint8_t byte = (uint8_t)ecuCalDef->FullRomData.at(j);
                if (cal_id_hex.length() < ecuid_length)
                    cal_id_hex.append(parse_hex_ecuid(byte));
                cal_id_ascii.append((QChar)byte);
            }
        }

        if (cal_id_ascii == configValues->romraider_def_cal_id.at(i))
            cal_id_ascii_confirmed = true;
        if (cal_id_hex == configValues->romraider_def_cal_id.at(i))
            cal_id_hex_confirmed = true;

        //emit LOG_D("RomRaider cal id: " + i + " " + configValues->romraider_def_cal_id.at(i) + " -> " + cal_id_ascii + " -> " + cal_id_hex;

        if (cal_id_ascii_confirmed)
        {
            emit LOG_D("RomRaider cal id " + cal_id_ascii + " found", true, true);
            ecuCalDef->RomId = cal_id_ascii;
            break;
        }
        if (cal_id_hex_confirmed)
        {
            emit LOG_D("RomRaider cal id " + cal_id_hex + " found", true, true);
            ecuCalDef->RomId = cal_id_hex;
            break;
        }
    }
    //emit LOG_D("Parse RomRaider def files no ID found... " + ecuCalDef->RomId;

    return ecuCalDef;
}

FileActions::EcuCalDefStructure *FileActions::create_new_definition_for_rom(FileActions::EcuCalDefStructure *ecuCalDef)
{
    ConfigValuesStructure *configValues = &ConfigValuesStruct;

    QString filename;
    QFileDialog saveDialog;
    bool isFileSelected = false;

    QDialog *definitionDialog = new QDialog(this);
    QVBoxLayout *vBoxLayout = new QVBoxLayout(definitionDialog);
    QLabel *label = new QLabel("Please provide ROM Information:");
    vBoxLayout->addWidget(label);

    QGridLayout *defHeaderGridLayout = new QGridLayout();
    QList<QLineEdit*> lineEditList;
    QList<QTextEdit*> textEditList;
    int index = 0;
    emit LOG_D("Create header", true, true);
    for (int i = 0; i < ecuCalDef->DefHeaderNames.length(); i++)
    {
        QLabel *label = new QLabel(ecuCalDef->DefHeaderStrings.at(index));
        defHeaderGridLayout->addWidget(label, index, 0);

        if (ecuCalDef->DefHeaderNames.at(i) == "notes")
        {
            textEditList.append(new QTextEdit());
            textEditList.at(textEditList.length()-1)->setObjectName(ecuCalDef->DefHeaderNames.at(i));
            //textEditList.at(textEditList.length()-1)->setText(headerData.at(i+1));
            defHeaderGridLayout->addWidget(textEditList.at(textEditList.length()-1), index+1, 0, 1, 2);
        }
        else
        {
            lineEditList.append(new QLineEdit());
            lineEditList.at(lineEditList.length()-1)->setObjectName(ecuCalDef->DefHeaderNames.at(i));
            //lineEditList.at(lineEditList.length()-1)->setText(headerData.at(i+1));
            defHeaderGridLayout->addWidget(lineEditList.at(lineEditList.length()-1), index, 1);
        }
        index++;
    }
    vBoxLayout->addLayout(defHeaderGridLayout);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    vBoxLayout->addWidget(buttonBox);
    connect(buttonBox, &QDialogButtonBox::accepted, definitionDialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, definitionDialog, &QDialog::reject);

    definitionDialog->setMinimumWidth(500);
    int result = definitionDialog->exec();
    if(result == QDialog::Accepted)
    {
        // 0x0C6D57
        while (filename.isEmpty() && !isFileSelected)
        {
            saveDialog.setDefaultSuffix("xml");
            filename = QFileDialog::getSaveFileName(this, tr("Select definition file"), configValues->ecuflash_definition_files_directory, tr("Definition file (*.xml)"));
            if (filename.isEmpty()){
                QDialog *definitionDialog = new QDialog(this);
                QVBoxLayout *vBoxLayout = new QVBoxLayout(definitionDialog);
                QLabel *label = new QLabel("No file selected!\n\nIf you still want to create file click 'Ok'\nIf you want to continue to use ROM without definition, click 'Cancel'");
                QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
                connect(buttonBox, &QDialogButtonBox::accepted, definitionDialog, &QDialog::accept);
                connect(buttonBox, &QDialogButtonBox::rejected, definitionDialog, &QDialog::reject);

                vBoxLayout->addWidget(label);
                vBoxLayout->addWidget(buttonBox);

                int result = definitionDialog->exec();
                if(result == QDialog::Rejected)
                    isFileSelected = true;
            }
        }
        if(filename.endsWith(QString(".")))
            filename.remove(filename.length() - 1, 1);
        if(!filename.endsWith(QString(".xml")))
            filename.append(QString(".xml"));

        QFile file(filename);
        QFileInfo fileInfo(file.fileName());
        //file_name_str = fileInfo.fileName();

        if (!file.open(QIODevice::ReadWrite ))
        {
            QMessageBox::warning(this, tr("Definition file"), "Unable to open definition file for writing");
            return NULL;
        }

        QString rombase;
        QString checksum_module = configValues->flash_protocol_selected_protocol_name;
        checksum_module.remove(0, 3);
        checksum_module.insert(0, "checksum");

        configValues->ecuflash_def_filename.append(filename);

        QXmlStreamWriter stream(&file);
        file.resize(0);
        stream.setAutoFormatting(true);
        stream.setAutoFormattingIndent(2);
        stream.writeStartDocument();
        stream.writeStartElement("rom");
        stream.writeStartElement("romid");

        int index = 0;
        for (int i = 0; i < ecuCalDef->DefHeaderNames.length(); i++)
        {
            if (ecuCalDef->DefHeaderNames.at(i) != "include" && ecuCalDef->DefHeaderNames.at(i) != "notes")
            {
                if(ecuCalDef->DefHeaderNames.at(i) == "internalidstring")
                    configValues->ecuflash_def_cal_id.append(lineEditList.at(i)->text());
                if(ecuCalDef->DefHeaderNames.at(i) == "internalidaddress")
                    configValues->ecuflash_def_cal_id_addr.append(lineEditList.at(i)->text());
                if(ecuCalDef->DefHeaderNames.at(i) == "ecuid")
                    configValues->ecuflash_def_ecu_id.append(lineEditList.at(i)->text());

                emit LOG_D(lineEditList.at(i)->text(), true, true);
                stream.writeTextElement(ecuCalDef->RomInfoNames.at(i), lineEditList.at(i)->text());
                index++;
            }
        }
        stream.writeEndElement();
        stream.writeCharacters("\n\n\t");
        stream.writeTextElement("include", lineEditList.at(index)->text());
        stream.writeCharacters("\n\n\t");
        stream.writeTextElement("notes", textEditList.at(0)->toPlainText());
        stream.writeEndElement();

        file.close();
    }

    return ecuCalDef;
}

FileActions::EcuCalDefStructure *FileActions::use_existing_definition_for_rom(FileActions::EcuCalDefStructure *ecuCalDef)
{
    ConfigValuesStructure *configValues = &ConfigValuesStruct;

    QString filename;
    QFileDialog openDialog;
    QFileDialog saveDialog;
    bool isFileSelected = false;

    while (filename.isEmpty() && !isFileSelected)
    {
        openDialog.setDefaultSuffix("xml");
        filename = QFileDialog::getOpenFileName(this, tr("Select definition file"), configValues->ecuflash_definition_files_directory, tr("Definition file (*.xml)"));
        if (filename.isEmpty()){
            QDialog *definitionDialog = new QDialog(this);
            QVBoxLayout *vBoxLayout = new QVBoxLayout(definitionDialog);
            QLabel *label = new QLabel("No file selected!\n\nIf you still want to select file click 'Ok'\nIf you want to continue to use ROM without definition, click 'Cancel'");
            QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
            connect(buttonBox, &QDialogButtonBox::accepted, definitionDialog, &QDialog::accept);
            connect(buttonBox, &QDialogButtonBox::rejected, definitionDialog, &QDialog::reject);

            vBoxLayout->addWidget(label);
            vBoxLayout->addWidget(buttonBox);

            int result = definitionDialog->exec();
            if(result == QDialog::Rejected)
                isFileSelected = true;
        }
    }

    QFile file(filename);
    QFileInfo fileInfo(file.fileName());
    //file_name_str = fileInfo.fileName();

    if (!file.open(QIODevice::ReadOnly ))
    {
        QMessageBox::warning(this, tr("Definition file"), "Unable to open definition file for reading");
        return NULL;
    }
    QStringList defData;
    while(!file.atEnd())
    {
        defData.append(file.readLine());
    }
    file.close();

    QString xml_id;
    QString xml_id_addr;
    QStringList headerData;
    int endIndex = 0;

    for (int i = 0; i < defData.length(); i++)
    {
        if (!defData.at(i).contains("<") && !defData.at(i).contains(">"))
            continue;
        for (int j = 0; j < ecuCalDef->DefHeaderNames.length(); j++)
        {
            QString parsedHeaderName = defData.at(i).split("<").at(1).split(">").at(0);
            if (parsedHeaderName == ecuCalDef->DefHeaderNames.at(j))
            {
                emit LOG_D(parsedHeaderName, true, true);
                headerData.append(ecuCalDef->DefHeaderNames.at(j));
                if (parsedHeaderName != "notes")
                    headerData.append(defData.at(i).split(">").at(1).split("<").at(0));
                if (parsedHeaderName == "notes")
                {
                    QString lineData;
                    parsedHeaderName.clear();

                    lineData.append(defData.at(i).split(">").at(1).split("<").at(0));
                    if (defData.at(i).contains("</") && defData.at(i).contains(">"))
                        parsedHeaderName = defData.at(i).split("</").at(1).split(">").at(0);
                    while (parsedHeaderName != "notes")
                    {
                        i++;
                        if (defData.at(i).contains("</") && defData.at(i).contains(">"))
                        {
                            parsedHeaderName = defData.at(i).split("</").at(1).split(">").at(0);
                            lineData.append(defData.at(i).split("</").at(0));
                        }
                        else
                            lineData.append(defData.at(i));

                        emit LOG_D("Test: " + parsedHeaderName, true, true);
                    }
                    headerData.append(lineData);
                }
                endIndex = i;
            }
        }
    }
    endIndex++;

    QDialog *definitionDialog = new QDialog(this);
    QVBoxLayout *vBoxLayout = new QVBoxLayout(definitionDialog);
    QLabel *label = new QLabel("Please provide ROM Information:");
    vBoxLayout->addWidget(label);

    QGridLayout *defHeaderGridLayout = new QGridLayout();
    QList<QLineEdit*> lineEditList;
    QList<QTextEdit*> textEditList;
    int index = 0;
    emit LOG_D("Create header", true, true);
    for (int i = 0; i < headerData.length(); i+=2)
    {
        QLabel *label = new QLabel(ecuCalDef->DefHeaderStrings.at(index));
        defHeaderGridLayout->addWidget(label, index, 0);

        if (headerData.at(i) == "notes")
        {
            textEditList.append(new QTextEdit());
            textEditList.at(textEditList.length()-1)->setObjectName(headerData.at(i));
            textEditList.at(textEditList.length()-1)->setText(headerData.at(i+1));
            defHeaderGridLayout->addWidget(textEditList.at(textEditList.length()-1), index+1, 0, 1, 2);
        }
        else
        {
            lineEditList.append(new QLineEdit());
            lineEditList.at(lineEditList.length()-1)->setObjectName(headerData.at(i));
            lineEditList.at(lineEditList.length()-1)->setText(headerData.at(i+1));
            defHeaderGridLayout->addWidget(lineEditList.at(lineEditList.length()-1), index, 1);
        }
        index++;
    }
    vBoxLayout->addLayout(defHeaderGridLayout);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    vBoxLayout->addWidget(buttonBox);
    connect(buttonBox, &QDialogButtonBox::accepted, definitionDialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, definitionDialog, &QDialog::reject);

    definitionDialog->setMinimumWidth(500);
    int result = definitionDialog->exec();
    if(result == QDialog::Accepted)
    {
        filename.clear();
        while (filename.isEmpty() && !isFileSelected)
        {
            saveDialog.setDefaultSuffix("xml");
            filename = QFileDialog::getSaveFileName(this, tr("Select definition file"), configValues->ecuflash_definition_files_directory, tr("Definition file (*.xml)"));
            if (filename.isEmpty()){
                QDialog *definitionDialog = new QDialog(this);
                QVBoxLayout *vBoxLayout = new QVBoxLayout(definitionDialog);
                QLabel *label = new QLabel("No file selected!\n\nIf you still want to create file click 'Ok'\nIf you want to continue to use ROM without definition, click 'Cancel'");
                QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
                connect(buttonBox, &QDialogButtonBox::accepted, definitionDialog, &QDialog::accept);
                connect(buttonBox, &QDialogButtonBox::rejected, definitionDialog, &QDialog::reject);

                vBoxLayout->addWidget(label);
                vBoxLayout->addWidget(buttonBox);

                int result = definitionDialog->exec();
                if(result == QDialog::Rejected)
                    isFileSelected = true;
            }
        }
        if(filename.endsWith(QString(".")))
            filename.remove(filename.length() - 1, 1);
        if(!filename.endsWith(QString(".xml")))
            filename.append(QString(".xml"));

        QFile file(filename);
        QFileInfo fileInfo(file.fileName());
        //file_name_str = fileInfo.fileName();

        if (!file.open(QIODevice::ReadWrite ))
        {
            QMessageBox::warning(this, tr("Definition file"), "Unable to open definition file for writing");
            return NULL;
        }

        QString rombase;
        QString checksum_module = ecuCalDef->RomInfo.at(FlashMethod);
        checksum_module.remove(0, 3);
        checksum_module.insert(0, "checksum");

        configValues->ecuflash_def_filename.append(filename);

        QXmlStreamWriter stream(&file);
        file.resize(0);

        emit LOG_D("Write to file", true, true);
        stream.setAutoFormatting(true);
        stream.setAutoFormattingIndent(2);
        stream.writeStartDocument();
        stream.writeStartElement("rom");
        stream.writeStartElement("romid");
        int index = 0;
        for (int i = 0; i < headerData.length(); i+=2)
        {
            if(headerData.at(i) == "internalidstring")
                configValues->ecuflash_def_cal_id.append(lineEditList.at(index)->text());
            if(headerData.at(i) == "internalidaddress")
                configValues->ecuflash_def_cal_id_addr.append(lineEditList.at(index)->text());
            if(headerData.at(i) == "ecuid")
                configValues->ecuflash_def_ecu_id.append(lineEditList.at(index)->text());

            if (headerData.at(i) != "include" && headerData.at(i) != "notes")
            {
                emit LOG_D(lineEditList.at(index)->text(), true, true);
                stream.writeTextElement(headerData.at(i), lineEditList.at(index)->text());
                index++;
            }
        }
        stream.writeEndElement();
        stream.writeCharacters("\n\n\t");
        stream.writeTextElement("include", lineEditList.at(index)->text());
        stream.writeCharacters("\n\n\t");
        stream.writeTextElement("notes", textEditList.at(0)->toPlainText());
        stream.writeCharacters("\n");

        QTextStream out(&file);
        for (int i = endIndex; i < defData.length(); i++)
            out << defData.at(i);

        stream.writeEndElement();

        file.close();
    }

    return ecuCalDef;
}

FileActions::EcuCalDefStructure *FileActions::open_subaru_rom_file(FileActions::EcuCalDefStructure *ecuCalDef, QString filename)
{
    ConfigValuesStructure *configValues = &ConfigValuesStruct;
    //save_subaru_rom_file(ecuCalDef, "calibrations/temp.bin");

    QString file_name_str;
    QString ecu_id;
    QString cal_id;
    QString selected_id;
    bool cal_id_confirmed = true;
    QStringList cal_id_family_list;
    QStringList cal_id_ascii_list;
    QStringList cal_id_addr_list;
    QStringList cal_id_length_list;
    bool ecu_id_confirmed = true;
    QStringList ecu_id_ascii_list;
    QStringList ecu_id_addr_list;
    QStringList ecu_id_length_list;
    QString selected_id_addr;
    bool id_is_ascii = false;

    bool bStatus = false;

    for (int i = 0; i < configValues->flash_protocol_id.length(); i++)
    {
        if (!cal_id_family_list.contains(configValues->flash_protocol_protocol_name.at(i)))
        {
            cal_id_family_list.append(configValues->flash_protocol_protocol_name.at(i));
            cal_id_ascii_list.append(configValues->flash_protocol_cal_id_ascii.at(i));
            cal_id_addr_list.append(configValues->flash_protocol_cal_id_addr.at(i));
            cal_id_length_list.append(configValues->flash_protocol_cal_id_length.at(i));
            ecu_id_ascii_list.append(configValues->flash_protocol_ecu_id_ascii.at(i));
            ecu_id_addr_list.append(configValues->flash_protocol_ecu_id_addr.at(i));
            ecu_id_length_list.append(configValues->flash_protocol_ecu_id_length.at(i));
        }
    }
    for (int i = 0; i < cal_id_family_list.length(); i++)
    {
        //emit LOG_D("ECU id family/address list: " + cal_id_family_list.at(i) + " - " + ecu_id_addr_list.at(i) + " - " + cal_id_addr_list.at(i), true, true);
    }

    if (!ecuCalDef->FullRomData.length())
    {

        if (filename.isEmpty())
        {
            QFileDialog openDialog;
            openDialog.setDefaultSuffix("bin");
            filename = QFileDialog::getOpenFileName(this, tr("Open ROM file"), configValues->calibration_files_directory, tr("Calibration file (*.bin *.hex)"));

            if (filename.isEmpty()){
                QMessageBox::information(this, tr("Calibration file"), "No file selected");
                free(ecuCalDef);
                return NULL;
            }

        }

        QFile file(filename);
        QFileInfo fileInfo(file.fileName());
        file_name_str = fileInfo.fileName();

        if (!file.open(QIODevice::ReadOnly ))
        {
            QMessageBox::warning(this, tr("Calibration file"), "Unable to open calibration file for reading");
            return NULL;
        }
        ecuCalDef->FullRomData = file.readAll();
    }
    else
    {
        emit LOG_D("Save file to: " + configValues->calibration_files_directory + "read.bin", true, true);
        save_subaru_rom_file(ecuCalDef, configValues->calibration_files_directory + "read.bin");

        if (filename == "")
        {
            QDateTime dateTime = dateTime.currentDateTime();
            QString dateTimeString = dateTime.toString("yyyy-MM-dd_hh'h'mm'm'ss's'");
            filename = "read_image_" + dateTimeString + ".bin";
        }

        QFile file(filename);
        QFileInfo fileInfo(file.fileName());
        file_name_str = fileInfo.fileName();
    }

    def_map_index = 0;

    ecuCalDef->use_ecuflash_definition = false;
    ecuCalDef->use_romraider_definition = false;

    if ((configValues->primary_definition_base == "ecuflash"  || configValues->use_romraider_definitions != "enabled") && configValues->ecuflash_definition_files_directory.length())
    {
        if (configValues->use_ecuflash_definitions == "enabled")
        {
            parse_ecuid_ecuflash_def_files(ecuCalDef, id_is_ascii);
            if (ecuCalDef->RomId != "")
            {
                //emit LOG_D("Parse EcuFlash def files (primary) " + ecuCalDef->RomId;
                read_ecuflash_ecu_def(ecuCalDef, ecuCalDef->RomId);
                parse_ecuflash_def_scalings(ecuCalDef);
            }
        }
        if (!ecuCalDef->use_ecuflash_definition && configValues->use_romraider_definitions == "enabled")
        {
            parse_ecuid_romraider_def_files(ecuCalDef, id_is_ascii);
            if (ecuCalDef->RomId != "")
            {
                //emit LOG_D("Parse RomRaider def files (secondary) " + ecuCalDef->RomId;
                read_romraider_ecu_def(ecuCalDef, ecuCalDef->RomId);
            }
        }
    }
    if (configValues->primary_definition_base == "romraider" && configValues->romraider_definition_files.length())
    {
        if (configValues->use_romraider_definitions == "enabled")
        {
            parse_ecuid_romraider_def_files(ecuCalDef, id_is_ascii);
            if (ecuCalDef->RomId !="")
            {
                //emit LOG_D("Parse RomRaider def files (primary) " + ecuCalDef->RomId;
                read_romraider_ecu_def(ecuCalDef, ecuCalDef->RomId);
            }
        }
        if(!ecuCalDef->use_romraider_definition && configValues->use_ecuflash_definitions == "enabled")
        {
            parse_ecuid_ecuflash_def_files(ecuCalDef, id_is_ascii);
            if (ecuCalDef->RomId !="")
            {
                //emit LOG_D("Parse EcuFlash def files (secondary) " + ecuCalDef->RomId;
                read_ecuflash_ecu_def(ecuCalDef, ecuCalDef->RomId);
                parse_ecuflash_def_scalings(ecuCalDef);
            }
        }
    }

    if (!ecuCalDef->use_romraider_definition && !ecuCalDef->use_ecuflash_definition)
    {
        QDialog *definitionDialog = new QDialog(this);
        QVBoxLayout *vBoxLayout = new QVBoxLayout(definitionDialog);
        QLabel *label = new QLabel("Unable to find definition for selected ROM file!\n\nSelect option:");
        QRadioButton *createNewRadioButton = new QRadioButton("Create new definition file template");
        QRadioButton *useExistingRadioButton = new QRadioButton("Use existing definition file as base");
        QRadioButton *continueWithoutRadioButton = new QRadioButton("Continue without definition file");

        QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        connect(buttonBox, &QDialogButtonBox::accepted, definitionDialog, &QDialog::accept);
        connect(buttonBox, &QDialogButtonBox::rejected, definitionDialog, &QDialog::reject);

        vBoxLayout->addWidget(label);
        vBoxLayout->addWidget(createNewRadioButton);
        vBoxLayout->addWidget(useExistingRadioButton);
        vBoxLayout->addWidget(continueWithoutRadioButton);
        vBoxLayout->addWidget(buttonBox);
        //createNewRadioButton->setChecked(true);
        //useExistingRadioButton->setChecked(true);
        continueWithoutRadioButton->setChecked(true);

        int result = definitionDialog->exec();
        if(result == QDialog::Accepted)
        {
            if(createNewRadioButton->isChecked()){
                emit LOG_D(createNewRadioButton->text(), true, true);
                create_new_definition_for_rom(ecuCalDef);
            }
            else if(useExistingRadioButton->isChecked()){
                emit LOG_D(useExistingRadioButton->text(), true, true);
                use_existing_definition_for_rom(ecuCalDef);
            }
        }
        if(continueWithoutRadioButton->isChecked() || result == QDialog::Rejected)
        {
            emit LOG_D(continueWithoutRadioButton->text(), true, true);


            //QMessageBox::warning(this, tr("Calibration file"), "Unable to find definition for selected ROM file!");
            ecuCalDef->RomInfo.replace(XmlId, "UnknownID");
            ecuCalDef->RomInfo.replace(InternalIdAddress, selected_id_addr);
            ecuCalDef->RomInfo.replace(InternalIdString, selected_id);
            ecuCalDef->RomInfo.replace(EcuId, selected_id);
            ecuCalDef->RomInfo.replace(Make, configValues->flash_protocol_selected_make);
            //ecuCalDef->RomInfo.replace(Model, configValues->flash_protocol_selected_model);
            //ecuCalDef->RomInfo.replace(SubModel, submodel);
            //ecuCalDef->RomInfo.replace(Transmission, transmission);
            //ecuCalDef->RomInfo.replace(MemModel, memmodel);
            //ecuCalDef->RomInfo.replace(ChecksumModule, checksummodule);
            //ecuCalDef->RomInfo.replace(FlashMethod, configValues->flash_protocol_selected_protocol_name);
            ecuCalDef->RomInfo.replace(FileSize, QString::number(ecuCalDef->FullRomData.length() / 1024) + "kb");
            ecuCalDef->RomInfo.replace(DefFile, " ");
        }
    }

    QString checksum_module = ecuCalDef->RomInfo.at(FlashMethod);
    checksum_module.remove(0, 3);
    checksum_module.insert(0, "checksum");
    for (int i = 0; i < configValues->flash_protocol_id.length(); i++)
    {
        if (configValues->flash_protocol_protocol_name.at(i) == ecuCalDef->RomInfo.at(FlashMethod))
        {
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
    if (configValues->flash_protocol_selected_checksum == "yes")
        ecuCalDef->RomInfo.replace(ChecksumModule, checksum_module);
    if (configValues->flash_protocol_selected_checksum == "n/a")
        ecuCalDef->RomInfo.replace(ChecksumModule, "Not implemented yet");
    if (configValues->flash_protocol_selected_checksum == "no")
        ecuCalDef->RomInfo.replace(ChecksumModule, "No checksums");

/*
    if (ecuCalDef == NULL)
    {
        QMessageBox::warning(this, tr("Calibration file"), "Unable to find definition for selected ROM file with ECU ID: " + ecuId);
        ecuCalDef = NULL;
        return NULL;
    }
*/
    if (!file_name_str.length())
        file_name_str = "default.bin";

    ecuCalDef->McuType = configValues->flash_protocol_selected_mcu;
    ecuCalDef->OemEcuFile = true;
    ecuCalDef->FileName = file_name_str;
    ecuCalDef->FullFileName = filename;
    ecuCalDef->FileSize = QString::number(ecuCalDef->FullRomData.length());
    ecuCalDef->RomInfo.replace(FileSize, QString::number(ecuCalDef->FullRomData.length() / 1024) + "kb");

    for (int i = 0; i < ecuCalDef->NameList.length(); i++)
    {
        if (ecuCalDef->AddressList.at(i).toUInt() > ecuCalDef->FullRomData.length() || ecuCalDef->XScaleAddressList.at(i).toUInt() > ecuCalDef->FullRomData.length() || ecuCalDef->YScaleAddressList.at(i).toUInt() > ecuCalDef->FullRomData.length())
        {
            QMessageBox::warning(this, tr("File size error"), "Error in expected ROM size!");
            ecuCalDef->NameList.clear();
            return ecuCalDef;
        }
    }

    QByteArray padding;
    padding.clear();
    if (ecuCalDef->RomInfo.at(FlashMethod).startsWith("sub_ecu_denso_mc68hc16y5_02") && ecuCalDef->FileSize.toUInt() < 190 * 1024)
    {
        for (int i = 0; i < 0x8000; i++)
            ecuCalDef->FullRomData.insert(0x20000, (uint8_t)0xff);
    }
    //emit LOG_D("QByteArray size = " + ecuCalDef->FullRomData.length(), true, true);

    int storagesize = 0;
    QString mapData;

    union mapData{
        uint8_t oneByteValue[4];
        uint16_t twoByteValue[2];
        uint32_t fourByteValue;
        float floatValue;
    } mapDataValue;

    for (int i = 0; i < ecuCalDef->NameList.length(); i++)
    {
        //emit LOG_D("Start parsing map" + " " + i + " " + ecuCalDef->NameList.at(i), true, true);
        storagesize = 1;
        if (ecuCalDef->StorageTypeList.at(i) == "uint16")
            storagesize = 2;
        if (ecuCalDef->StorageTypeList.at(i) == "uint24")
            storagesize = 3;
        if (ecuCalDef->StorageTypeList.at(i) == "uint32" || ecuCalDef->StorageTypeList.at(i) == "float")
            storagesize = 4;
        mapData.clear();
        if (ecuCalDef->StorageTypeList.at(i) == "bloblist")
        {
            //emit LOG_D("Map " + ecuCalDef->NameList.at(i) + " is bloblist", true, true);
            //if (ecuCalDef->SelectionsValueList.at(i) != " ")
                storagesize = ecuCalDef->SelectionsValueList.at(i).split(",").at(0).length() / 2;
            //else
            //    storagesize = ecuCalDef->SelectionsDataList.at(i).split(",").at(0).length() / 2;
            uint8_t dataByte = 0;
            uint32_t byteAddress = ecuCalDef->AddressList.at(i).toUInt(&bStatus,16);
            for (int k = 0; k < storagesize; k++)
            {
                dataByte = (uint8_t)ecuCalDef->FullRomData.at(byteAddress + k);
                mapData.append(QString("%1").arg(dataByte,2,16,QLatin1Char('0')));
            }
            emit LOG_D("Selectable " + ecuCalDef->NameList.at(i) + " -> addr: 0x" + QString::number(byteAddress, 16) + " -> value: 0x" + mapData, true, true);
            ecuCalDef->MapData.replace(i, mapData);
            //emit LOG_D("Mapdata " + ecuCalDef->MapData.at(i), true, true);
        }
        else
        {
            //emit LOG_D("Map " + ecuCalDef->NameList.at(i) + " is normal map", true, true);

            for (unsigned j = 0; j < ecuCalDef->XSizeList.at(i).toUInt() * ecuCalDef->YSizeList.at(i).toUInt(); j++)
            {
                uint32_t dataByte = 0;
                uint32_t startPos = ecuCalDef->StartPosList.at(i).toUInt(&bStatus,16);
                uint32_t interval = ecuCalDef->IntervalList.at(i).toUInt(&bStatus,16);
                uint32_t byteAddress = ecuCalDef->AddressList.at(i).toUInt(&bStatus,16) + (j * storagesize * interval + (startPos - 1) * storagesize);

                //emit LOG_D("Map value address: 0x" QString::number(byteAddress, 16), true, true);
                if (ecuCalDef->RomInfo.at(FlashMethod) == "wrx02" && (uint32_t)ecuCalDef->FullRomData.length() < byteAddress)
                    byteAddress -= 0x8000;
                for (int k = 0; k < storagesize; k++)
                {
                    //emit LOG_D("Check endian";
                    if (ecuCalDef->EndianList.at(i) == "little" || ecuCalDef->StorageTypeList.at(i) == "float")
                    {
                        dataByte = (dataByte << 8) + (uint8_t)ecuCalDef->FullRomData.at(byteAddress + storagesize - 1 - k);
                        mapDataValue.oneByteValue[k] = (uint8_t)ecuCalDef->FullRomData.at(byteAddress + storagesize - 1 - k);
                    }
                    else
                    {
                        dataByte = (dataByte << 8) + (uint8_t)ecuCalDef->FullRomData.at(byteAddress + k);
                        mapDataValue.oneByteValue[k] = (uint8_t)ecuCalDef->FullRomData.at(byteAddress + k);
                    }
                }
                //if (ecuCalDef->NameList.at(i) == "Volumetric Efficiency")
                    //emit LOG_D("dataByte: " + dataByte + " " + mapDataValue.twoByteValue[0] + " " + mapDataValue.twoByteValue[1], true, true);
                double value = 0;
                if (ecuCalDef->TypeList.at(i) != "Selectable")
                {
                    if (ecuCalDef->StorageTypeList.at(i) == "float"){
                        value = calculate_value_from_expression(parse_stringlist_from_expression_string(ecuCalDef->FromByteList.at(i), QString::number(mapDataValue.floatValue, 'g', float_precision)));
                    }
                    else{
                        value = calculate_value_from_expression(parse_stringlist_from_expression_string(ecuCalDef->FromByteList.at(i), QString::number(dataByte)));
                    }
                    //if (ecuCalDef->NameList.at(i) == "Volumetric Efficiency")
                        //emit LOG_D("MapData value " + QString::number(value), true, true);
                }
                mapData.append(QString::number(value, 'g', float_precision) + ",");
            }
            ecuCalDef->MapData.replace(i, mapData);

            if (ecuCalDef->XSizeList.at(i).toUInt() > 1)
            {
                //emit LOG_D("Map " + ecuCalDef->NameList.at(i) + " " + "x scale", true, true);
                if (ecuCalDef->XScaleTypeList.at(i) == "Static Y Axis" || ecuCalDef->XScaleTypeList.at(i) == "Static X Axis")
                {
                    //emit LOG_D("Static X scale", true, true);
                    ecuCalDef->XScaleData.replace(i, ecuCalDef->XScaleStaticDataList.at(i));
                }
                else if (ecuCalDef->XScaleTypeList.at(i) == "X Axis" || (ecuCalDef->XScaleTypeList.at(i) == "Y Axis" && ecuCalDef->TypeList.at(i) == "2D"))
                {
                    storagesize = 1;
                    if (ecuCalDef->XScaleStorageTypeList.at(i) == "uint16")
                        storagesize = 2;
                    if (ecuCalDef->XScaleStorageTypeList.at(i) == "uint24")
                        storagesize = 3;
                    if (ecuCalDef->XScaleStorageTypeList.at(i) == "uint32" || ecuCalDef->XScaleStorageTypeList.at(i) == "float")
                        storagesize = 4;
                    mapData.clear();
                    for (unsigned j = 0; j < ecuCalDef->XSizeList.at(i).toUInt(); j++)
                    {
                        uint32_t dataByte = 0;
                        uint32_t startPos = ecuCalDef->XScaleStartPosList.at(i).toUInt(&bStatus,16);
                        uint32_t interval = ecuCalDef->XScaleIntervalList.at(i).toUInt(&bStatus,16);
                        uint32_t byteAddress = ecuCalDef->XScaleAddressList.at(i).toUInt(&bStatus,16) + (j * storagesize * interval + (startPos - 1) * storagesize);

                        //emit LOG_D("X Scale value address: 0x" + QString::number(byteAddress, 16), true, true);
                        if (ecuCalDef->RomInfo.at(FlashMethod) == "wrx02" && (uint32_t)ecuCalDef->FullRomData.length() < byteAddress)
                            byteAddress -= 0x8000;

                        for (int k = 0; k < storagesize; k++)
                        {
                            if (ecuCalDef->XScaleEndianList.at(i) == "little" || ecuCalDef->XScaleStorageTypeList.at(i) == "float")
                            {
                                dataByte = (dataByte << 8) + (uint8_t)ecuCalDef->FullRomData.at(byteAddress + storagesize - 1 - k);
                                mapDataValue.oneByteValue[k] = (uint8_t)ecuCalDef->FullRomData.at(byteAddress + storagesize - 1 - k);
                            }
                            else
                            {
                                dataByte = (dataByte << 8) + (uint8_t)ecuCalDef->FullRomData.at(byteAddress + k);
                                mapDataValue.oneByteValue[k] = (uint8_t)ecuCalDef->FullRomData.at(byteAddress + k);
                            }
                        }
                        double value = 0;
                        if (ecuCalDef->XScaleTypeList.at(i) != "Selectable")
                        {
                            if (ecuCalDef->XScaleStorageTypeList.at(i) == "float")
                                value = calculate_value_from_expression(parse_stringlist_from_expression_string(ecuCalDef->XScaleFromByteList.at(i), QString::number(mapDataValue.floatValue, 'g', float_precision)));
                            else
                                value = calculate_value_from_expression(parse_stringlist_from_expression_string(ecuCalDef->XScaleFromByteList.at(i), QString::number(dataByte)));
                        }
                        mapData.append(QString::number(value, 'g', float_precision) + ",");
                    }
                    ecuCalDef->XScaleData.replace(i, mapData);
                }
                //emit LOG_D("Map " + ecuCalDef->NameList.at(i) + " x scale ready", true, true);
            }
            else
                ecuCalDef->XScaleData.replace(i, " ");
            if (ecuCalDef->YSizeList.at(i).toUInt() > 1)
            {
                //emit LOG_D("Map " + ecuCalDef->NameList.at(i) + " y scale", true, true);
                storagesize = 1;
                if (ecuCalDef->YScaleStorageTypeList.at(i) == "uint16")
                    storagesize = 2;
                if (ecuCalDef->YScaleStorageTypeList.at(i) == "uint24")
                    storagesize = 3;
                if (ecuCalDef->YScaleStorageTypeList.at(i) == "uint32" || ecuCalDef->YScaleStorageTypeList.at(i) == "float")
                    storagesize = 4;
                mapData.clear();
                for (unsigned j = 0; j < ecuCalDef->YSizeList.at(i).toUInt(); j++)
                {
                    uint32_t dataByte = 0;
                    uint32_t startPos = ecuCalDef->YScaleStartPosList.at(i).toUInt(&bStatus,16);
                    uint32_t interval = ecuCalDef->YScaleIntervalList.at(i).toUInt(&bStatus,16);
                    uint32_t byteAddress = ecuCalDef->YScaleAddressList.at(i).toUInt(&bStatus,16) + (j * storagesize * interval + (startPos - 1) * storagesize);

                    //emit LOG_D("Y Scale value address: " + ecuCalDef->NameList.at(i) + " " + ecuCalDef->YScaleNameList.at(i) + " " + byteAddress, true, true);
                    if (ecuCalDef->RomInfo.at(FlashMethod) == "wrx02" && (uint32_t)ecuCalDef->FullRomData.length() < byteAddress)
                        byteAddress -= 0x8000;
                    for (int k = 0; k < storagesize; k++)
                    {
                        if (ecuCalDef->YScaleEndianList.at(i) == "little" || ecuCalDef->YScaleStorageTypeList.at(i) == "float")
                        {
                            dataByte = (dataByte << 8) + (uint8_t)ecuCalDef->FullRomData.at(byteAddress + storagesize - 1 - k);
                            mapDataValue.oneByteValue[k] = (uint8_t)ecuCalDef->FullRomData.at(byteAddress + storagesize - 1 - k);
                        }
                        else
                        {
                            dataByte = (dataByte << 8) + (uint8_t)ecuCalDef->FullRomData.at(byteAddress + k);
                            mapDataValue.oneByteValue[k] = (uint8_t)ecuCalDef->FullRomData.at(byteAddress + k);
                        }
                    }
                    double value = 0;
                    if (ecuCalDef->YScaleTypeList.at(i) != "Selectable")
                    {
                        if (ecuCalDef->YScaleStorageTypeList.at(i) == "float")
                            value = calculate_value_from_expression(parse_stringlist_from_expression_string(ecuCalDef->YScaleFromByteList.at(i), QString::number(mapDataValue.floatValue, 'g', float_precision)));
                        else
                            value = calculate_value_from_expression(parse_stringlist_from_expression_string(ecuCalDef->YScaleFromByteList.at(i), QString::number(dataByte)));
                    }
                    mapData.append(QString::number(value, 'g', float_precision) + ",");
                }
                ecuCalDef->YScaleData.replace(i, mapData);
                //emit LOG_D("Map " + ecuCalDef->NameList.at(i) + " x scale ready", true, true);
            }
            else
                ecuCalDef->YScaleData.replace(i, " ");
            //emit LOG_D("Map " + i + " parsed", true, true);
        }
    }

    if (ecuCalDef == NULL)
    {
        QMessageBox::warning(this, tr("Calibration file"), "Unable to find definition for selected calibration file with ECU ID: " + selected_id + ".");
        return NULL;
    }

    return ecuCalDef;
}

FileActions::EcuCalDefStructure *FileActions::save_subaru_rom_file(FileActions::EcuCalDefStructure *ecuCalDef, QString filename)
{
    QFile file(filename);
    QFileInfo fileInfo(file.fileName());
    QString file_name_str = fileInfo.fileName();

    if (!file.open(QIODevice::WriteOnly ))
    {
        //emit LOG_D("Unable to open file for writing", true, true);
        QMessageBox::warning(this, tr("Ecu calibration file"), "Unable to open file " + filename + " for writing");
        return NULL;
    }

    file.write(ecuCalDef->FullRomData);
    file.close();

    ecuCalDef->FullFileName = filename;
    ecuCalDef->FileName = file_name_str;

    return 0;
}

FileActions::EcuCalDefStructure *FileActions::checksum_correction(FileActions::EcuCalDefStructure *ecuCalDef)
{
    ConfigValuesStructure *configValues = &ConfigValuesStruct;

    bool chksumModuleAvailable = false;

    QString flashMethod = configValues->flash_protocol_selected_protocol_name;// ecuCalDef->RomInfo[FlashMethod];

    emit LOG_D("Protocol: " + configValues->flash_protocol_selected_protocol_name, true, true);
    emit LOG_D("Make: " + configValues->flash_protocol_selected_make, true, true);
    emit LOG_D("Checksum: " + configValues->flash_protocol_selected_checksum, true, true);

    QString mcu_type_string = ecuCalDef->McuType;
    int mcu_type_index = 0;
    uint32_t fullRomSize = ecuCalDef->FullRomData.length();

    while (flashdevices[mcu_type_index].name != 0)
    {
        if (flashdevices[mcu_type_index].name == mcu_type_string)
            break;
        mcu_type_index++;
    }
    emit LOG_D("ecuCalDef->McuType: " + ecuCalDef->McuType + " " + configValues->flash_protocol_selected_mcu, true, true);
    emit LOG_D("Size: 0x" + QString::number(fullRomSize, 16) + " -> 0x" + QString::number(flashdevices[mcu_type_index].romsize, 16), true, true);

    if (!ecuCalDef->use_romraider_definition && !ecuCalDef->use_ecuflash_definition)
    {
        QMessageBox msgBox;
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setWindowTitle("Calibration file");
        msgBox.setText("WARNING! No definition file linked to selected ROM, checksums are not calculated!\n\n"
                        "If you are sure that right protocol is selected and want to correct checksums anyway, press 'DO IT!' -button");
        QPushButton *okButton = msgBox.addButton(QMessageBox::Ok);
        QPushButton *doItButton = msgBox.addButton(tr("DO IT!"), QMessageBox::NoRole);
        msgBox.exec();

        if (msgBox.clickedButton() == okButton)
            return ecuCalDef;
    }

    if (configValues->flash_protocol_selected_checksum == "yes")
    {
        if (configValues->flash_protocol_selected_make == "Subaru")
        {
            emit LOG_D("ROM memory model is " + ecuCalDef->RomInfo[MemModel], true, true);
            emit LOG_D("Checksum module: " + flashMethod, true, true);

            if (fullRomSize != flashdevices[mcu_type_index].romsize)
            {
                QMessageBox::information(this, tr("Checksum module"), "Bad ROM size! Make sure that you have selected correct flash method!");
                return ecuCalDef;
            }
            /*
            * Denso ECU
            */
            if (flashMethod.startsWith("sub_ecu_denso_sh7055"))
            {
                chksumModuleAvailable = true;
                ecuCalDef->FullRomData = ChecksumEcuSubaruDensoSH7xxx::calculate_checksum(ecuCalDef->FullRomData, 0x07FB80, 17 * 12);

            }
            else if (flashMethod.startsWith("sub_ecu_denso_sh7058_can_diesel"))
            {
                chksumModuleAvailable = true;
                ecuCalDef->FullRomData = ChecksumEcuSubaruDensoSH705xDiesel::calculate_checksum(ecuCalDef->FullRomData, 0x0FFB80, 17 * 12);
            }
            else if (flashMethod.startsWith("sub_ecu_denso_sh7058s_diesel_densocan"))
            {
                chksumModuleAvailable = true;
                ecuCalDef->FullRomData = ChecksumEcuSubaruDensoSH705xDiesel::calculate_checksum(ecuCalDef->FullRomData, 0x0FFB80, 17 * 12);
            }
            else if (flashMethod.startsWith("sub_ecu_denso_sh7058"))
            {
                chksumModuleAvailable = true;
                ecuCalDef->FullRomData = ChecksumEcuSubaruDensoSH7xxx::calculate_checksum(ecuCalDef->FullRomData, 0x0FFB80, 17 * 12);
            }
            else if (flashMethod.startsWith("sub_ecu_denso_sh72531_can"))
            {
                chksumModuleAvailable = true;
                ecuCalDef->FullRomData = ChecksumEcuSubaruDensoSH7xxx::calculate_checksum(ecuCalDef->FullRomData, 0x13F500, 17 * 12);
            }
            else if (flashMethod.startsWith("sub_ecu_denso_1n83m_4m_can"))
            {
                chksumModuleAvailable = true;
                ecuCalDef->FullRomData = ChecksumEcuSubaruDensoSH7xxx::calculate_checksum(ecuCalDef->FullRomData, 0x3E3E00, 17 * 12, -0x8F9C000);
            }
            else if (flashMethod.startsWith("sub_ecu_denso_1n83m_1_5m_can"))
            {
                chksumModuleAvailable = true;
                ecuCalDef->FullRomData = ChecksumEcuSubaruDensoSH7xxx::calculate_checksum(ecuCalDef->FullRomData, 0x183E00, 17 * 12, -0x8F9C000);
            }
            else if (flashMethod.startsWith("sub_ecu_denso_sh7059_can_diesel"))
            {
                chksumModuleAvailable = true;
                ecuCalDef->FullRomData = ChecksumEcuSubaruDensoSH705xDiesel::calculate_checksum(ecuCalDef->FullRomData, 0x17FB80, 17 * 12);
            }
            else if (flashMethod.startsWith("sub_ecu_denso_sh7059_diesel_densocan"))
            {
                chksumModuleAvailable = true;
                ecuCalDef->FullRomData = ChecksumEcuSubaruDensoSH705xDiesel::calculate_checksum(ecuCalDef->FullRomData, 0x17FB80, 17 * 12);
            }
            else if (flashMethod.startsWith("sub_ecu_denso_sh72543_can_diesel"))
            {
                chksumModuleAvailable = true;
                ecuCalDef->FullRomData = ChecksumEcuSubaruDensoSH7xxx::calculate_checksum(ecuCalDef->FullRomData, 0x1FF800, 17 * 12);
            }
            /*
            * Denso TCU
            */
            else if (flashMethod.startsWith("sub_tcu_denso_sh7055_can"))
            {
                chksumModuleAvailable = true;
                ecuCalDef->FullRomData = ChecksumTcuSubaruDensoSH7055::calculate_checksum(ecuCalDef->FullRomData);
            }
            else if (flashMethod.startsWith("sub_tcu_denso_sh7058_can"))
            {
                chksumModuleAvailable = true;
                ecuCalDef->FullRomData = ChecksumEcuSubaruDensoSH7xxx::calculate_checksum(ecuCalDef->FullRomData, 0x0FFB80, 17 * 12);
            }
            /*
            * Hitachi ECU
            */
            else if (flashMethod.startsWith("sub_ecu_hitachi_m32r_kline"))
            {
                chksumModuleAvailable = true;
                if (ecuCalDef->RomId.startsWith("3"))
                    ecuCalDef->FullRomData = ChecksumEcuSubaruHitachiM32rKline::calculate_checksum(ecuCalDef->FullRomData);
                if (ecuCalDef->RomId.startsWith("4"))
                    ecuCalDef->FullRomData = ChecksumEcuSubaruHitachiM32rCan::calculate_checksum(ecuCalDef->FullRomData);
                if (ecuCalDef->RomId.startsWith("6"))
                    ecuCalDef->FullRomData = ChecksumEcuSubaruHitachiM32rCan::calculate_checksum(ecuCalDef->FullRomData);
            }
            else if (flashMethod.startsWith("sub_ecu_hitachi_m32r_can"))
            {
                chksumModuleAvailable = true;
                ecuCalDef->FullRomData = ChecksumEcuSubaruHitachiM32rCan::calculate_checksum(ecuCalDef->FullRomData);
            }
            else if (flashMethod.startsWith("sub_ecu_hitachi_sh7058_can"))
            {
                chksumModuleAvailable = true;
                ecuCalDef->FullRomData = ChecksumEcuSubaruHitachiSH7058::calculate_checksum(ecuCalDef->FullRomData);
            }
            else if (flashMethod.startsWith("sub_ecu_hitachi_sh72543r"))
            {
                chksumModuleAvailable = true;
                ecuCalDef->FullRomData = ChecksumEcuSubaruHitachiSh72543r::calculate_checksum(ecuCalDef->FullRomData);
            }
            /*
            * Hitachi TCU
            */
            else if (flashMethod.startsWith("sub_tcu_hitachi_m32r_can"))
            {
                chksumModuleAvailable = true;
                ecuCalDef->FullRomData = ChecksumTcuSubaruHitachiM32rCan::calculate_checksum(ecuCalDef->FullRomData);
            }
            else if (flashMethod.startsWith("sub_tcu_hitachi_m32r_kline"))
            {
                chksumModuleAvailable = true;
                ecuCalDef->FullRomData = ChecksumTcuSubaruHitachiM32rCan::calculate_checksum(ecuCalDef->FullRomData);
            }
            /*
            * Mitsu TCU
            */
            else if (flashMethod.startsWith("sub_tcu_cvt_mitsu_mh8104_can"))
            {
                chksumModuleAvailable = true;
                ecuCalDef->FullRomData = ChecksumTcuMitsuMH8104Can::calculate_checksum(ecuCalDef->FullRomData);
            }
            else
                chksumModuleAvailable = false;
        }
    }
    if (!chksumModuleAvailable && configValues->flash_protocol_selected_checksum != "no")
    {
        QMessageBox msgBox;
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setWindowTitle("File - Checksum Warning");
        //msgBox.setDetailedText("File - Checksum Warning");
        msgBox.setText("WARNING! There is no checksum module for this ROM!\
                            Be aware that if this ROM need checksum correction it must be done with another software!");
        QPushButton *cancelButton = msgBox.addButton(QMessageBox::Cancel);
        QPushButton *okButton = msgBox.addButton(QMessageBox::Ok);
        msgBox.exec();

        if (msgBox.clickedButton() == cancelButton)
        {
            emit LOG_D("Checksum calculation canceled!", true, true);
            return ecuCalDef;
        }
    }

    return ecuCalDef;
}

QStringList FileActions::parse_stringlist_from_expression_string(QString expression, QString x)
{
    QStringList numbers;
    QStringList operators;
    uint8_t output = 0;
    uint8_t stack = 0;
    bool isOperator = true;

    int i = 0;

    //emit LOG_D("Parse expression stringlist " + expression + " " + expression.length(), true, true);

    while (i < expression.length())
    {
        QString number;
        //emit LOG_D("Expression stringlist index: " + i + " " + expression.at(i), true, true);

        if (expression.at(i) == 'x')
        {
            isOperator = false;
            numbers.append(x);
            output++;
        }
        else if ((isOperator && expression.at(i) == '-' && expression.at(i + 1) == 'x') || (expression.at(i) == '-' && expression.at(i + 1) == 'x' && i == 0))
        {
            isOperator = false;
            number.append(expression.at(i));
            number.append(x);
            numbers.append(number);
            i++;
            output+=2;

        }
        else if (expression.at(i).isNumber() || expression.at(i) == '.' || (isOperator && expression.at(i) == '-'))
        {
            isOperator = false;
            number.append(expression.at(i));
            i++;
            while (i < expression.length() && (expression.at(i).isNumber() || expression.at(i) == '.'))
            {
                number.append(expression.at(i));
                i++;
            }
            i--;
            numbers.append(number);
            output++;
        }
        else if (expression.at(i) == '(')
        {
            isOperator = true;
            operators.append(expression.at(i));
            stack++;
        }
        else if (expression.at(i) == ')')
        {
            stack--;
            while (operators.at(stack) != "(")
            {
                numbers.append(operators.at(stack));
                operators.removeAt(stack);
                stack--;
            }
            operators.removeAt(stack);
        }
        else if (expression.at(i) == '*')
        {
            isOperator = true;
            operators.append(expression.at(i));
            stack++;
        }
        else if (expression.at(i) == '/')
        {
            isOperator = true;
            operators.append(expression.at(i));
            stack++;
        }
        else if (expression.at(i) == '+')
        {
            isOperator = true;
            if ((operators.length() > 0 && i > 0) && (operators.at(stack - 1) == '/' || operators.at(stack - 1) == '*'))
            {
                numbers.append(operators.at(stack - 1));
                output++;
                operators.replace(stack - 1, expression.at(i));
            }
            else
            {
                operators.append(expression.at(i));
                stack++;
            }
        }
        else if (expression.at(i) == '-')
        {
            isOperator = true;
            if ((operators.length() > 0 && i > 0) && (operators.at(stack - 1) == '/' || operators.at(stack - 1) == '*'))
            {
                numbers.append(operators.at(stack - 1));
                output++;
                operators.replace(stack - 1, expression.at(i));
            }
            else
            {
                operators.append(expression.at(i));
                stack++;
            }
        }

        i++;
    }
    while (operators.length() > 0)
    {
        stack--;
        numbers.append(operators.at(stack));
        operators.removeAt(stack);
    }

    //emit LOG_D("Parse expression stringlist end";

    return numbers;
}

double FileActions::calculate_value_from_expression(QStringList expression)
{
    double value = 0;

    //emit LOG_D("Calculate value from expression", true, true);
    if (expression.length() == 1)
    {
        QString valueString = expression.at(0);
        if (valueString.startsWith("--"))
            valueString.remove(0, 2);
        value = valueString.toDouble();
    }

    //emit LOG_D("Calculate value from expression", true, true);

    while (expression.length() > 1)
    {
        for (int i = 0; i < expression.length(); i++)
        {
            if (expression.at(i) == "-") {
                value = expression.at(i - 2).toDouble() - expression.at(i - 1).toDouble();
                expression.replace(i, QString::number(value, 'g', float_precision));
                expression.removeAt(i - 1);
                expression.removeAt(i - 2);
                break;
            }
            if (expression.at(i) == "+") {
                value = expression.at(i - 2).toDouble() + expression.at(i - 1).toDouble();
                expression.replace(i, QString::number(value, 'g', float_precision));
                expression.removeAt(i - 1);
                expression.removeAt(i - 2);
                break;
            }
            if (expression.at(i) == "*") {
                value = expression.at(i - 2).toDouble() * expression.at(i - 1).toDouble();
                expression.replace(i, QString::number(value, 'g', float_precision));
                expression.removeAt(i - 1);
                expression.removeAt(i - 2);
                break;
            }
            if (expression.at(i) == "/") {
                value = expression.at(i - 2).toDouble() / expression.at(i - 1).toDouble();
                expression.replace(i, QString::number(value, 'g', float_precision));
                expression.removeAt(i - 1);
                expression.removeAt(i - 2);
                break;
            }
        }
    }

    if (isnan(value))
        value = 0;
    return value;
}

QString FileActions::parse_nrc_message(QByteArray nrc)
{
    QString ret = "Unknown error code";

    if (nrc.length() > 2 && (uint8_t)nrc.at(0) == 0x7f)
    {
        ret = neg_rsp_codes.value((uint8_t)nrc.at(2), ret);
    }

    return ret;
}

QString FileActions::parse_dtc_message(uint16_t dtc)
{
    QString ret = "Unknown error code";

    ret = dtc_Pxxxx_codes.value(dtc, ret);

    return ret;
}
