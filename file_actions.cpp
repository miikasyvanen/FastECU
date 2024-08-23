#include "file_actions.h"

FileActions::FileActions()
{

}

FileActions::ConfigValuesStructure *FileActions::check_config_dir(ConfigValuesStructure *configValues){
    //ConfigValuesStructure *configValues = &ConfigValuesStruct;

    if (!QDir(configValues->base_directory).exists()){
        QDir().mkdir(configValues->base_directory);
    }

    if (!QDir(configValues->calibration_files_base_directory).exists()){
        QDir().mkdir(configValues->calibration_files_base_directory);
        QDir dir("calibrations/");
        foreach (const QFileInfo& entry, dir.entryInfoList((QStringList() << "*.*", QDir::Files))){
            QFile().copy("calibrations/" + entry.fileName(), configValues->calibration_files_base_directory + "/" + entry.fileName());
        }
    }
    if (!QDir(configValues->config_base_directory).exists()){
        QDir().mkdir(configValues->config_base_directory);
        save_config_file(configValues);
    }
    QDir dir("config/");
    foreach (const QFileInfo& entry, dir.entryInfoList((QStringList() << "*.*", QDir::Files))){
        qDebug() << "Check file" << entry.fileName();
        if(!QFileInfo::exists(configValues->config_base_directory + "/" + entry.fileName()))
        {
            qDebug() << "File" << entry.fileName() << "does not exists, copying...";
            QFile().copy("config/" + entry.fileName(), configValues->config_base_directory + "/" + entry.fileName());
        }
    }
    if (!QDir(configValues->definition_files_base_directory).exists()){
        QDir().mkdir(configValues->definition_files_base_directory);

        QString source_dir = "definitions/";
        QString target_dir = configValues->definition_files_base_directory;
        bool cover_file_if_exist = false;

        copy_directory_files(source_dir, target_dir, cover_file_if_exist);

    }
    if (!QDir(configValues->kernel_files_base_directory).exists()){
        QDir().mkdir(configValues->kernel_files_base_directory);
        QDir dir("kernels/");
        foreach (const QFileInfo& entry, dir.entryInfoList((QStringList() << "*.*", QDir::Files))){
            QFile().copy("kernels/" + entry.fileName(), configValues->kernel_files_base_directory + "/" + entry.fileName());
        }
    }
    if (!QDir(configValues->log_files_base_directory).exists()){
        QDir().mkdir(configValues->log_files_base_directory);
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
    //QFile file(configValues->config_base_directory + "/fastecu.cfg");
    QFile file(configValues->config_file);
    qDebug() << "Looking config file from:" << configValues->config_file;
    if (!file.open(QIODevice::ReadOnly ))
    {
        QMessageBox::warning(this, tr("Config file"), "Unable to open application config file for reading");
        return configValues;
    }

    QXmlStreamReader reader;
    reader.setDevice(&file);

    if (reader.readNextStartElement())
    {
        if (reader.name().toUtf8() == "ecu" && reader.attributes().value("name").toUtf8() == "FastECU")
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
                            qDebug() << "Width:" << configValues->window_width << "Height:" << configValues->window_height;
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
                            qDebug() << "Serial port:" << configValues->serial_port;
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
                            qDebug() << "Serial port:" << configValues->serial_port;
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
                            qDebug() << "Protocol ID:" << configValues->flash_protocol_selected_id;
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
                            qDebug() << "Flash transport:" << configValues->flash_protocol_selected_flash_transport;
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
                            qDebug() << "Log transport:" << configValues->flash_protocol_selected_log_transport;
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
                            qDebug() << "Log protocol:" << configValues->flash_protocol_selected_log_protocol;
                        }
                        else if (reader.name().toUtf8() == "setting" && reader.attributes().value("name").toUtf8() == "primary_definition_base")
                        {
                            while(reader.readNextStartElement())
                            {
                                if (reader.name().toUtf8() == "value")
                                {
                                    configValues->primary_definition_base = reader.attributes().value("data").toString();
                                    reader.skipCurrentElement();
                                }
                                else
                                    reader.skipCurrentElement();
                            }
                            qDebug() << "Primary def base:" << configValues->primary_definition_base;
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
                            qDebug() << "Calibration files:" << configValues->calibration_files;
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
                            qDebug() << "Calibration files directory:" << configValues->calibration_files_directory;
                        }
                        else if (reader.name().toUtf8() == "setting" && reader.attributes().value("name").toUtf8() == "romraider_definition_files")
                        {
                            while(reader.readNextStartElement())
                            {
                                if (reader.name().toUtf8() == "value")
                                {
                                    configValues->romraider_definition_files.append(reader.attributes().value("data").toString());
                                    reader.skipCurrentElement();
                                }
                                else
                                    reader.skipCurrentElement();
                            }
                            qDebug() << "RomRaider def files:" << configValues->romraider_definition_files;
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
                            qDebug() << "Use RomRaider definitions:" << configValues->use_romraider_definitions;
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
                            qDebug() << "EcuFlash def files directory:" << configValues->ecuflash_definition_files_directory;
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
                            qDebug() << "Use EcuFlash definitions:" << configValues->use_ecuflash_definitions;
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
                            qDebug() << "Logger def file:" << configValues->romraider_logger_definition_file;
                        }
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
                            qDebug() << "Kernel files directory:" << configValues->kernel_files_directory;
                        }
                        else if (reader.name().toUtf8() == "setting" && reader.attributes().value("name").toUtf8() == "logfiles_directory")
                        {
                            while(reader.readNextStartElement())
                            {
                                if (reader.name().toUtf8() == "value")
                                {
                                    configValues->log_files_directory = reader.attributes().value("data").toString();
                                    reader.skipCurrentElement();
                                }
                                else
                                    reader.skipCurrentElement();
                            }
                            qDebug() << "Logfiles firectory:" << configValues->log_files_directory;
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
        QMessageBox::warning(this, tr("Config file"), "Unable to open config file for writing");
        return 0;
    }

    //QTextStream outStream(&configFile);

    QXmlStreamWriter stream(&file);
    file.resize(0);
    stream.setAutoFormatting(true);
    stream.writeStartDocument();
    stream.writeStartElement("ecu");
    stream.writeAttribute("name", "FastECU");
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

    stream.writeStartElement("setting");
    stream.writeAttribute("name", "kernel_files_directory");
    stream.writeStartElement("value");
    stream.writeAttribute("data", configValues->kernel_files_directory);
    stream.writeEndElement();
    stream.writeEndElement();

    stream.writeStartElement("setting");
    stream.writeAttribute("name", "logfiles_directory");
    stream.writeStartElement("value");
    stream.writeAttribute("data", configValues->log_files_base_directory);
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
    QStringList flash_protocol_ecu;
    QStringList flash_protocol_mcu;
    QStringList flash_protocol_mode;
    QStringList flash_protocol_checksum;
    QStringList flash_protocol_read;
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
        QMessageBox::warning(this, tr("Protocols file"), "Unable to open protocols file for reading");
        return NULL;
    }
    xmlBOM.setContent(&file);
    file.close();

    QDomElement root = xmlBOM.documentElement();

    if (root.tagName() == "ecu")
    {
        QDomElement root_child = root.firstChild().toElement();
        while (!root_child.isNull())
        {
            if (root_child.tagName() == "protocols")
            {
                //qDebug() << "Protocols";
                int index = 0;

                QDomElement protocol = root_child.firstChild().toElement();
                while (!protocol.isNull())
                {
                    if (protocol.tagName() == "protocol")
                    {
                        //qDebug() << "Protocol";
                        flash_protocol_name.append(" ");
                        flash_protocol_name.replace(index, protocol.attribute("name","No name"));

                        flash_protocol_ecu.append(" ");
                        flash_protocol_mcu.append(" ");
                        flash_protocol_mode.append(" ");
                        flash_protocol_checksum.append(" ");
                        flash_protocol_read.append(" ");
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
                        //qDebug() << "Flash protocol name:" << flash_protocol_name.at(index) << "and family:" << flash_protocol_protocol_name.at(index);
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
                        //qDebug() << "Add new list";
                        configValues->flash_protocol_id.append(QString::number(id));//car_model.attribute("id","No id"));
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
                            //qDebug() << flash_protocol_data.tagName();
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
                                        configValues->flash_protocol_ecu.replace(index, flash_protocol_ecu.at(i));
                                        configValues->flash_protocol_mcu.replace(index, flash_protocol_mcu.at(i));
                                        configValues->flash_protocol_mode.replace(index, flash_protocol_mode.at(i));
                                        configValues->flash_protocol_checksum.replace(index, flash_protocol_checksum.at(i));
                                        configValues->flash_protocol_read.replace(index, flash_protocol_read.at(i));
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
                        //qDebug() << "Flash protocol ID:" << configValues->flash_protocol_id.at(index) << "make:" << configValues->flash_protocol_make.at(index) << "model:" << configValues->flash_protocol_model.at(index) << "flash method:" << configValues->flash_protocol_protocol_name.at(index);
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

    QString filename = configValues->logger_config_file;

    QFile file(filename);
    if(!file.open(QFile::ReadWrite | QFile::Text)) {
        QMessageBox::warning(this, tr("Logger file"), "Unable to open logger definition file for reading");
        return NULL;
    }
    xmlBOM.setContent(&file);

    QDomElement root = xmlBOM.documentElement();

    if (!modify)
    {
        logValues->dashboard_log_value_id.clear();
        logValues->lower_panel_log_value_id.clear();
        logValues->lower_panel_switch_id.clear();
    }

    bool ecu_id_found = false;
    int index = 0;

    if (root.tagName() == "logger")
    {
        QDomElement ecus = root.firstChild().toElement();
        while (!ecus.isNull())
        {
            if (ecus.tagName() == "ecus")
            {
                QDomElement ecu = ecus.firstChild().toElement();
                while (!ecu.isNull())
                {
                    if (ecu.tagName() == "ecu")
                    {
                        QString file_ecu_id = ecu.attribute("id","No id");

                        if (ecu_id == file_ecu_id)
                        {
                            ecu_id_found = true;
                            //qDebug() << "Found ECU ID" << file_ecu_id;
                            QDomElement protocol = ecu.firstChild().toElement();
                            while (!protocol.isNull())
                            {
                                if (protocol.tagName() == "protocol")
                                {
                                    //qDebug() << "Found protocol" << protocol.attribute("id","No id");
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
                                                                gauges.attribute("id", logValues->dashboard_log_value_id.at(index));
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
                                                        switches.attribute("id", logValues->lower_panel_switch_id.at(index));
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
                if (!ecu_id_found)
                {
                    file.resize(0);

                    //qDebug() << "ECU ID not found, initializing log parameters";
                    logValues->logging_values_protocol = logValues->log_value_protocol.at(0);
                    for (int i = 0; i < logValues->log_value_id.length(); i++)
                    {
                        if (logValues->log_value_enabled.at(i) == "1" && logValues->dashboard_log_value_id.length() < 15)
                            logValues->dashboard_log_value_id.append(logValues->log_value_id.at(i));
                    }
                    for (int i = 0; i < logValues->log_value_id.length(); i++)
                    {
                        if (logValues->log_value_enabled.at(i) == "1" && logValues->lower_panel_log_value_id.length() < 12)
                        logValues->lower_panel_log_value_id.append(logValues->log_value_id.at(i));
                    }
                    for (int i = 0; i < logValues->log_switch_id.length(); i++)
                    {
                        if (logValues->log_switch_enabled.at(i) == "1" && logValues->lower_panel_switch_id.length() < 20)
                        logValues->lower_panel_switch_id.append(logValues->log_switch_id.at(i));
                    }
                    //qDebug() << "Values initialized, creating xml data";
                    //save_logger_conf(logValues, ecu_id);
                    QDomElement ecu = xmlBOM.createElement("ecu");
                    ecu.setAttribute("id", ecu_id);
                    ecus.appendChild(ecu);
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
                    //qDebug() << "Saving log parameters";
                    QTextStream output(&file);
                    output << xmlBOM.toString();
                    file.close();
                }
            }
            ecus = ecus.nextSibling().toElement();
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

void *FileActions::save_logger_conf(FileActions::LogValuesStructure *logValues, QString ecu_id)
{
    ConfigValuesStructure *configValues = &ConfigValuesStruct;

    QFile file(configValues->logger_config_file);
    if (!file.open(QIODevice::ReadWrite)) {
        QMessageBox::warning(this, tr("Config file"), "Unable to open config file for writing");
        return 0;
    }
    QXmlStreamReader reader;
    reader.setDevice(&file);

    QXmlStreamWriter stream(&file);
    //file.resize(0);
    stream.setAutoFormatting(true);
    stream.writeStartDocument();
    stream.writeStartElement("logger");
    stream.writeStartElement("ecus");
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

FileActions::LogValuesStructure *FileActions::read_logger_definition_file()
{
    LogValuesStructure *logValues = &LogValuesStruct;
    ConfigValuesStructure *configValues = &ConfigValuesStruct;

    //The QDomDocument class represents an XML document.
    QDomDocument xmlBOM;

    QString filename = configValues->romraider_logger_definition_file;
    //qDebug() << "Logger filename =" << filename;
    QFile file(filename);
    if(!file.open(QFile::ReadOnly | QFile::Text)) {
        QMessageBox::warning(this, tr("Logger file"), "Unable to open logger definition file for reading");
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
        //qDebug() << "Logger start element";
        QDomElement protocols = root.firstChild().toElement();

        while (!protocols.isNull())
        {
            //qDebug() << "Protocols start element";
            if (protocols.tagName() == "protocols")
            {
                QDomElement protocol = protocols.firstChild().toElement();
                while(!protocol.isNull())
                {
                    if (protocol.tagName() == "protocol")
                    {
                        QString log_value_protocol = protocol.attribute("id","No protocol id");
                        //qDebug() << "Protocol =" << protocol.attribute("id","No id");
                        QDomElement transports = protocol.firstChild().toElement();
                        while(!transports.isNull())
                        {
                            if (transports.tagName() == "transports")
                            {
                                //qDebug() << "Transports for protocol" << protocol.attribute("id","No id");
                                QDomElement transport = transports.firstChild().toElement();
                                while(!transport.isNull())
                                {
                                    if (transport.tagName() == "transport")
                                    {
                                        //qDebug() << "Transport =" << transport.attribute("id","No id") << transport.attribute("name","No name") << transport.attribute("desc","No description");
                                    }
                                    QDomElement module = transport.firstChild().toElement();
                                    while(!module.isNull())
                                    {
                                        if (module.tagName() == "module")
                                        {
                                            //qDebug() << "Module =" << module.attribute("id","No id") << module.attribute("address","No address") << module.attribute("desc","No description") << module.attribute("tester","No tester");
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
                                        //qDebug() << "Parameter =" << parameter.attribute("id","No id") << parameter.attribute("name","No name") << parameter.attribute("desc","No description");
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
                                                //qDebug() << "Address =" << param_options.text();
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
                                                        //qDebug() << "Conversion =" << conversion.attribute("units","No units") << conversion.attribute("expr","No expr") << conversion.attribute("format","No format") << conversion.attribute("gauge_min","0") << conversion.attribute("gauge_max","0") << conversion.attribute("gauge_step","0");
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
                                        //qDebug() << "Switch =" << paramswitch.attribute("id","No id") << paramswitch.attribute("name","No name") << paramswitch.attribute("desc","No description");
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
                                        //qDebug() << "DT code =" << dtcode.attribute("id","No id") << dtcode.attribute("name","No name") << dtcode.attribute("desc","No description");
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
                                        //qDebug() << "ECU param =" << ecuparam.attribute("id","No id") << ecuparam.attribute("name","No name") << ecuparam.attribute("desc","No description");
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
    QFile file(configValues->config_base_directory + "/menu.cfg");
    if (!file.open(QIODevice::ReadOnly ))
    {
        // Error while loading file
        QMessageBox::warning(this, tr("Ecu menu file"), "Unable to open ecu menu file");
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
                    //qDebug() << "Menu: " << menuName;

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
                                        //qDebug() << menuItemShortcut;
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
                                //qDebug() << menuItemShortcut;
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
    //qDebug() << "Constructed byte:" << ecuid_byte;

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

    //qDebug() << "Parse EcuFlash def files" << ecuCalDef->RomId;

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

        //qDebug() << "EcuFlash cal id:" << i << configValues->ecuflash_def_cal_id.at(i) << "->" << cal_id_ascii << "->" << cal_id_hex;

        if (cal_id_ascii_confirmed)
        {
            qDebug() << "EcuFlash cal id" << cal_id_ascii << "found";
            ecuCalDef->RomId = cal_id_ascii;
            break;
        }
        if (cal_id_hex_confirmed)
        {
            qDebug() << "EcuFlash cal id" << cal_id_hex << "found";
            ecuCalDef->RomId = cal_id_hex;
            break;
        }
    }
    //qDebug() << "Parse EcuFlash def files no ID found..." << ecuCalDef->RomId;

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

    //qDebug() << "Parse RomRaider def files" << ecuCalDef->RomId;

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

        //qDebug() << "RomRaider cal id:" << i << configValues->romraider_def_cal_id.at(i) << "->" << cal_id_ascii << "->" << cal_id_hex;

        if (cal_id_ascii_confirmed)
        {
            qDebug() << "RomRaider cal id" << cal_id_ascii << "found";
            ecuCalDef->RomId = cal_id_ascii;
            break;
        }
        if (cal_id_hex_confirmed)
        {
            qDebug() << "RomRaider cal id" << cal_id_hex << "found";
            ecuCalDef->RomId = cal_id_hex;
            break;
        }
    }
    //qDebug() << "Parse RomRaider def files no ID found..." << ecuCalDef->RomId;

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
        //qDebug() << "ECU id family/address list:" << cal_id_family_list.at(i) << "-" << ecu_id_addr_list.at(i) << "-" << cal_id_addr_list.at(i);
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
        if (filename == "") filename = "default.bin";

        QFile file(filename);
        QFileInfo fileInfo(file.fileName());
        file_name_str = fileInfo.fileName();
    }

    def_map_index = 0;

    ecuCalDef->use_ecuflash_definition = false;
    ecuCalDef->use_romraider_definition = false;

    if (configValues->primary_definition_base == "ecuflash" && configValues->ecuflash_definition_files_directory.length())
    {
        if (configValues->use_ecuflash_definitions == "enabled")
        {
            parse_ecuid_ecuflash_def_files(ecuCalDef, id_is_ascii);
            if (ecuCalDef->RomId != "")
            {
                //qDebug() << "Parse EcuFlash def files (primary)" << ecuCalDef->RomId;
                read_ecuflash_ecu_def(ecuCalDef, ecuCalDef->RomId);
                parse_ecuflash_def_scalings(ecuCalDef);
            }
        }
        if (!ecuCalDef->use_ecuflash_definition && configValues->use_romraider_definitions == "enabled")
        {
            parse_ecuid_romraider_def_files(ecuCalDef, id_is_ascii);
            if (ecuCalDef->RomId != "")
            {
                //qDebug() << "Parse RomRaider def files (secondary)" << ecuCalDef->RomId;
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
                //qDebug() << "Parse RomRaider def files (primary)" << ecuCalDef->RomId;
                read_romraider_ecu_def(ecuCalDef, ecuCalDef->RomId);
            }
        }
        if(!ecuCalDef->use_romraider_definition && configValues->use_ecuflash_definitions == "enabled")
        {
            parse_ecuid_ecuflash_def_files(ecuCalDef, id_is_ascii);
            if (ecuCalDef->RomId !="")
            {
                //qDebug() << "Parse EcuFlash def files (secondary)" << ecuCalDef->RomId;
                read_ecuflash_ecu_def(ecuCalDef, ecuCalDef->RomId);
                parse_ecuflash_def_scalings(ecuCalDef);
            }
        }
    }

    if (!ecuCalDef->use_romraider_definition && !ecuCalDef->use_ecuflash_definition)
    {
        QMessageBox::warning(this, tr("Calibration file"), "Unable to find definition for selected ROM file!");
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
        ecuCalDef->RomInfo.replace(FlashMethod, configValues->flash_protocol_selected_protocol_name);
        ecuCalDef->RomInfo.replace(FileSize, QString::number(ecuCalDef->FullRomData.length() / 1024) + "kb");
        ecuCalDef->RomInfo.replace(DefFile, " ");
    }

    if (configValues->flash_protocol_selected_checksum == "yes")
        ecuCalDef->RomInfo.replace(ChecksumModule, configValues->flash_protocol_selected_protocol_name);
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
        file_name_str = ecuCalDef->RomId;

    ecuCalDef->OemEcuFile = true;
    ecuCalDef->FileName = file_name_str;
    ecuCalDef->FullFileName = filename;
    ecuCalDef->FileSize = QString::number(ecuCalDef->FullRomData.length());
    //ecuCalDef->RomId = selected_id;
    //qDebug() << "File size =" << ecuCalDef->FileSize;

    QByteArray padding;// = QByteArray("\x00", 0x8000);
    padding.clear();
    if (ecuCalDef->RomInfo.at(FlashMethod) == "wrx02" && ecuCalDef->FileSize.toUInt() < 190 * 1024)
    {
        for (int i = 0; i < 0x8000; i++)
            ecuCalDef->FullRomData.insert(0x20000, (uint8_t)0x00);
    }
    //qDebug() << "QByteArray size =" << ecuCalDef->FullRomData.length();

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
        //qDebug() << "Start parsing map" << ecuCalDef->NameList.at(i);
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
            //qDebug() << "Map" << ecuCalDef->NameList.at(i) << "is bloblist";
            storagesize = ecuCalDef->SelectionsValueList.at(i).split(",").at(0).length() / 2;
            uint8_t dataByte = 0;
            uint32_t byteAddress = ecuCalDef->AddressList.at(i).toUInt(&bStatus,16);
            for (int k = 0; k < storagesize; k++)
            {
                dataByte = (uint8_t)ecuCalDef->FullRomData.at(byteAddress + k);
                mapData.append(QString("%1").arg(dataByte,2,16,QLatin1Char('0')));
            }
            ecuCalDef->MapData.replace(i, mapData);
        }
        else
        {
            //qDebug() << "Map" << ecuCalDef->NameList.at(i) << "is normal map";

            for (unsigned j = 0; j < ecuCalDef->XSizeList.at(i).toUInt() * ecuCalDef->YSizeList.at(i).toUInt(); j++)
            {
                uint32_t dataByte = 0;
                uint32_t startPos = ecuCalDef->StartPosList.at(i).toUInt(&bStatus,16);
                uint32_t interval = ecuCalDef->IntervalList.at(i).toUInt(&bStatus,16);
                uint32_t byteAddress = ecuCalDef->AddressList.at(i).toUInt(&bStatus,16) + (j * storagesize * interval + (startPos - 1) * storagesize);

                //qDebug() << "Map value address:" << hex << byteAddress;
                if (ecuCalDef->RomInfo.at(FlashMethod) == "wrx02" && (uint32_t)ecuCalDef->FullRomData.length() < byteAddress)
                    byteAddress -= 0x8000;
                for (int k = 0; k < storagesize; k++)
                {
                    //qDebug() << "Check endian";
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
                    //qDebug() << "dataByte:" << dataByte << mapDataValue.twoByteValue[0] << mapDataValue.twoByteValue[1];
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
                    //    qDebug() << "MapData value" << value;
                }
                mapData.append(QString::number(value, 'g', float_precision) + ",");
            }
            ecuCalDef->MapData.replace(i, mapData);

            if (ecuCalDef->XSizeList.at(i).toUInt() > 1)
            {
                //qDebug() << "Map" << ecuCalDef->NameList.at(i) << "x scale";
                if (ecuCalDef->XScaleTypeList.at(i) == "Static Y Axis" || ecuCalDef->XScaleTypeList.at(i) == "Static X Axis")
                {
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

                        //qDebug() << "X Scale value address:" << hex << byteAddress;
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
            }
            else
                ecuCalDef->XScaleData.replace(i, " ");
            if (ecuCalDef->YSizeList.at(i).toUInt() > 1)
            {
                //qDebug() << "Map" << ecuCalDef->NameList.at(i) << "y scale";
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

                    //qDebug() << "Y Scale value address:" << ecuCalDef->NameList.at(i) << ecuCalDef->YScaleNameList.at(i) << byteAddress;
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
            }
            else
                ecuCalDef->YScaleData.replace(i, " ");
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
        //qDebug() << "Unable to open file for writing";
        QMessageBox::warning(this, tr("Ecu calibration file"), "Unable to open file for writing");
        return NULL;
    }

    file.write(ecuCalDef->FullRomData);
    file.close();

    ecuCalDef->FullFileName = filename;
    ecuCalDef->FileName = file_name_str;

    return 0;
}

FileActions::EcuCalDefStructure *FileActions::apply_subaru_cal_changes_to_rom_data(FileActions::EcuCalDefStructure *ecuCalDef)
{
    ConfigValuesStructure *configValues = &ConfigValuesStruct;

    int storagesize = 0;
    QString mapData;
    bool bStatus = false;

    union mapData{
        uint8_t oneByteValue[4];
        uint16_t twoByteValue[2];
        uint32_t fourByteValue;
        float floatValue;
    } mapDataValue;

    for (int i = 0; i < ecuCalDef->NameList.length(); i++)
    {
        if (ecuCalDef->StorageTypeList.at(i) == "uint8")
            storagesize = 1;
        if (ecuCalDef->StorageTypeList.at(i) == "uint16")
            storagesize = 2;
        if (ecuCalDef->StorageTypeList.at(i) == "uint24")
            storagesize = 3;
        if (ecuCalDef->StorageTypeList.at(i) == "uint32" || ecuCalDef->StorageTypeList.at(i) == "float")
            storagesize = 4;
        mapData.clear();
        QStringList mapDataList = ecuCalDef->MapData.at(i).split(",");
        if (ecuCalDef->StorageTypeList.at(i) == "bloblist")
        {
            storagesize = ecuCalDef->SelectionsValueList.at(i).split(",").at(0).length() / 2;
            uint8_t dataByte = 0;
            uint32_t byteAddress = ecuCalDef->AddressList.at(i).toUInt(&bStatus,16);
            for (int k = 0; k < storagesize; k++)
            {
                dataByte = ecuCalDef->MapData.at(i).mid(0, 2).toUInt(&bStatus, 16);
                ecuCalDef->FullRomData[byteAddress] = dataByte;
            }
        }
        else
        {
            for (unsigned j = 0; j < ecuCalDef->XSizeList.at(i).toUInt() * ecuCalDef->YSizeList.at(i).toUInt(); j++)
            {
                uint32_t dataByte = 0;
                uint32_t startPos = ecuCalDef->StartPosList.at(i).toUInt(&bStatus,16);
                uint32_t interval = ecuCalDef->IntervalList.at(i).toUInt(&bStatus,16);
                uint32_t byteAddress = ecuCalDef->AddressList.at(i).toUInt(&bStatus,16) + (j * storagesize * interval + (startPos - 1) * storagesize);
                if (ecuCalDef->RomInfo.at(FlashMethod) == "wrx02" && (uint32_t)ecuCalDef->FullRomData.length() < byteAddress)
                    byteAddress -= 0x8000;

                if (ecuCalDef->TypeList.at(i) != "Switch")
                {
                    if (ecuCalDef->StorageTypeList.at(i) == "float"){
                        mapDataValue.floatValue = calculate_value_from_expression(parse_stringlist_from_expression_string(ecuCalDef->ToByteList.at(i), mapDataList.at(j)));
                    }
                    else
                    {
                        mapDataValue.floatValue = calculate_value_from_expression(parse_stringlist_from_expression_string(ecuCalDef->ToByteList.at(i), mapDataList.at(j)));
                        mapDataValue.fourByteValue = (uint32_t)(qRound(mapDataValue.floatValue));
                    }

                    if (mapDataValue.floatValue == 0)
                        mapDataValue.floatValue = 0.0f;

                    for (int k = 0; k < storagesize; k++)
                    {
                        if (ecuCalDef->EndianList.at(i) == "little")
                        {
                            ecuCalDef->FullRomData[byteAddress + k] = (uint8_t)(mapDataValue.oneByteValue[storagesize - 1 - k]);
                        }
                        else
                        {
                            ecuCalDef->FullRomData[byteAddress + k] = (uint8_t)(mapDataValue.oneByteValue[storagesize - 1 - k]);
                        }
                    }
                }
            }

            if (ecuCalDef->XSizeList.at(i).toUInt() > 1 && ecuCalDef->XScaleTypeList.at(i) != "Static Y Axis" && ecuCalDef->XScaleTypeList.at(i) != "Static X Axis" && ecuCalDef->XScaleNameList.at(i) != " ")
            {
                if (ecuCalDef->XScaleStorageTypeList.at(i) == "uint8")
                    storagesize = 1;
                if (ecuCalDef->XScaleStorageTypeList.at(i) == "uint16")
                    storagesize = 2;
                if (ecuCalDef->XScaleStorageTypeList.at(i) == "uint24")
                    storagesize = 3;
                if (ecuCalDef->XScaleStorageTypeList.at(i) == "uint32" || ecuCalDef->XScaleStorageTypeList.at(i) == "float")
                    storagesize = 4;
                mapData.clear();
                QStringList mapDataList = ecuCalDef->XScaleData.at(i).split(",");
                for (unsigned j = 0; j < ecuCalDef->XSizeList.at(i).toUInt(); j++)
                {
                    uint32_t dataByte = 0;
                    uint32_t startPos = ecuCalDef->XScaleStartPosList.at(i).toUInt(&bStatus,16);
                    uint32_t interval = ecuCalDef->XScaleIntervalList.at(i).toUInt(&bStatus,16);
                    uint32_t byteAddress = ecuCalDef->XScaleAddressList.at(i).toUInt(&bStatus,16) + (j * storagesize * interval + (startPos - 1) * storagesize);
                    if (ecuCalDef->RomInfo.at(FlashMethod) == "wrx02" && (uint32_t)ecuCalDef->FullRomData.length() < byteAddress)
                        byteAddress -= 0x8000;

                    if (ecuCalDef->XScaleTypeList.at(i) != "Switch")
                    {
                        if (ecuCalDef->XScaleStorageTypeList.at(i) == "float"){
                            mapDataValue.floatValue = calculate_value_from_expression(parse_stringlist_from_expression_string(ecuCalDef->XScaleToByteList.at(i), mapDataList.at(j)));
                        }
                        else
                        {
                            mapDataValue.floatValue = calculate_value_from_expression(parse_stringlist_from_expression_string(ecuCalDef->XScaleToByteList.at(i), mapDataList.at(j)));
                            mapDataValue.fourByteValue = (uint32_t)(qRound(mapDataValue.floatValue));
                        }

                        if (mapDataValue.floatValue == 0)
                            mapDataValue.floatValue = 0.0f;

                        for (int k = 0; k < storagesize; k++)
                        {
                            if (ecuCalDef->XScaleEndianList.at(i) == "little")
                            {
                                ecuCalDef->FullRomData[byteAddress + k] = (uint8_t)(mapDataValue.oneByteValue[storagesize - 1 - k]);
                            }
                            else
                            {
                                ecuCalDef->FullRomData[byteAddress + k] = (uint8_t)(mapDataValue.oneByteValue[storagesize - 1 - k]);
                            }
                        }
                    }
                }
            }
            if (ecuCalDef->YSizeList.at(i).toUInt() > 1 && ecuCalDef->YScaleTypeList.at(i) != "Static Y Axis" && ecuCalDef->XScaleTypeList.at(i) != "Static X Axis" && ecuCalDef->YScaleNameList.at(i) != " ")
            {
                if (ecuCalDef->YScaleStorageTypeList.at(i) == "uint8")
                    storagesize = 1;
                if (ecuCalDef->YScaleStorageTypeList.at(i) == "uint16")
                    storagesize = 2;
                if (ecuCalDef->YScaleStorageTypeList.at(i) == "uint24")
                    storagesize = 3;
                if (ecuCalDef->YScaleStorageTypeList.at(i) == "uint32" || ecuCalDef->YScaleStorageTypeList.at(i) == "float")
                    storagesize = 4;
                mapData.clear();
                QStringList mapDataList = ecuCalDef->YScaleData.at(i).split(",");
                for (unsigned j = 0; j < ecuCalDef->YSizeList.at(i).toUInt(); j++)
                {
                    uint32_t dataByte = 0;
                    uint32_t startPos = ecuCalDef->YScaleStartPosList.at(i).toUInt(&bStatus,16);
                    uint32_t interval = ecuCalDef->YScaleIntervalList.at(i).toUInt(&bStatus,16);
                    uint32_t byteAddress = ecuCalDef->YScaleAddressList.at(i).toUInt(&bStatus,16) + (j * storagesize * interval + (startPos - 1) * storagesize);
                    if (ecuCalDef->RomInfo.at(FlashMethod) == "wrx02" && (uint32_t)ecuCalDef->FullRomData.length() < byteAddress)
                        byteAddress -= 0x8000;

                    if (ecuCalDef->YScaleTypeList.at(i) != "Switch")
                    {
                        if (ecuCalDef->YScaleStorageTypeList.at(i) == "float"){
                            mapDataValue.floatValue = calculate_value_from_expression(parse_stringlist_from_expression_string(ecuCalDef->YScaleToByteList.at(i), mapDataList.at(j)));
                        }
                        else
                        {
                            mapDataValue.floatValue = calculate_value_from_expression(parse_stringlist_from_expression_string(ecuCalDef->YScaleToByteList.at(i), mapDataList.at(j)));
                            mapDataValue.fourByteValue = (uint32_t)(qRound(mapDataValue.floatValue));
                        }

                        if (mapDataValue.floatValue == 0)
                            mapDataValue.floatValue = 0.0f;

                        for (int k = 0; k < storagesize; k++)
                        {
                            if (ecuCalDef->YScaleEndianList.at(i) == "little")
                            {
                                ecuCalDef->FullRomData[byteAddress + k] = (uint8_t)(mapDataValue.oneByteValue[storagesize - 1 - k]);
                            }
                            else
                            {
                                ecuCalDef->FullRomData[byteAddress + k] = (uint8_t)(mapDataValue.oneByteValue[storagesize - 1 - k]);
                            }
                        }
                    }
                }
            }
        }
    }

    ecuCalDef = checksum_correction(ecuCalDef);

    return ecuCalDef;
}

FileActions::EcuCalDefStructure *FileActions::checksum_correction(FileActions::EcuCalDefStructure *ecuCalDef)
{
    ConfigValuesStructure *configValues = &ConfigValuesStruct;

    bool chksumModuleAvailable = false;

    QString flashMethod = ecuCalDef->RomInfo[FlashMethod];

    qDebug() << "Protocol:" << configValues->flash_protocol_selected_protocol_name;
    qDebug() << "Make:" << configValues->flash_protocol_selected_make;
    qDebug() << "Checksum:" << configValues->flash_protocol_selected_checksum;

    if (configValues->flash_protocol_selected_checksum == "yes")
    {
        if (configValues->flash_protocol_selected_make == "Subaru")
        {
            qDebug() << "ROM memory model is" << ecuCalDef->RomInfo[MemModel];

            /*
            * Denso ECU
            */
            if (flashMethod.startsWith("sub_ecu_sh7055"))
            {
                chksumModuleAvailable = true;
                ChecksumEcuSubaruDensoSH705x *checksumEcuSubaruDensoSH705x = new ChecksumEcuSubaruDensoSH705x();
                ecuCalDef->FullRomData = checksumEcuSubaruDensoSH705x->calculate_checksum(ecuCalDef->FullRomData, 0x07FB80, 17 * 12);
            }
            else if (flashMethod.startsWith("sub_ecu_sh7058"))
            {
                chksumModuleAvailable = true;
                ChecksumEcuSubaruDensoSH705x *checksumEcuSubaruDensoSH705x = new ChecksumEcuSubaruDensoSH705x();
                ecuCalDef->FullRomData = checksumEcuSubaruDensoSH705x->calculate_checksum(ecuCalDef->FullRomData, 0x0FFB80, 17 * 12);
            }
            /*
            * Denso TCU
            */
            else if (flashMethod.startsWith("sub_tcu_denso_sh7055_can"))
            {
                chksumModuleAvailable = true;
                ChecksumTcuSubaruDensoSH7055 *checksumTcuSubaruDensoSH7055 = new ChecksumTcuSubaruDensoSH7055();
                ecuCalDef->FullRomData = checksumTcuSubaruDensoSH7055->calculate_checksum(ecuCalDef->FullRomData);
            }
            else if (flashMethod.startsWith("sub_tcu_denso_sh7058_can"))
            {
                chksumModuleAvailable = true;
                ChecksumEcuSubaruDensoSH705x *checksumEcuSubaruDensoSH705x = new ChecksumEcuSubaruDensoSH705x();
                ecuCalDef->FullRomData = checksumEcuSubaruDensoSH705x->calculate_checksum(ecuCalDef->FullRomData, 0x0FFB80, 17 * 12);
            }
            /*
            * Hitachi ECU
            */
            else if (flashMethod.startsWith("sub_ecu_hitachi_m32r_"))
            {
                chksumModuleAvailable = true;
                ChecksumEcuSubaruHitachiM32r *checksumEcuSubaruHitachiM32r = new ChecksumEcuSubaruHitachiM32r();
                ecuCalDef->FullRomData = checksumEcuSubaruHitachiM32r->calculate_checksum(ecuCalDef->FullRomData);
            }
            /*
            * Hitachi TCU
            */
            else if (flashMethod.startsWith("sub_tcu_hitachi_m32r_can"))
            {
                chksumModuleAvailable = true;
                ChecksumTcuSubaruHitachiM32rCan *checksumTcuSubaruHitachiM32rCan = new ChecksumTcuSubaruHitachiM32rCan();
                ecuCalDef->FullRomData = checksumTcuSubaruHitachiM32rCan->calculate_checksum(ecuCalDef->FullRomData);
            }
            /*
            * Mitsu TCU
            */
            else if (flashMethod.startsWith("sub_tcu_cvt_mitsu_mh8104_can"))
            {
                chksumModuleAvailable = true;
                ChecksumTcuMitsuMH8104Can *checksumTcuMitsuMH8104Can = new ChecksumTcuMitsuMH8104Can();
                ecuCalDef->FullRomData = checksumTcuMitsuMH8104Can->calculate_checksum(ecuCalDef->FullRomData);
            }
            else
                chksumModuleAvailable = false;
        }
    }
    if (!chksumModuleAvailable && configValues->flash_protocol_selected_checksum != "no")
    {
        QMessageBox *msgBox = new QMessageBox();
        msgBox->setIcon(QMessageBox::Warning);
        msgBox->setWindowTitle("File - Checksum Warning");
        //msgBox->setDetailedText("File - Checksum Warning");
        msgBox->setText("WARNING! There is no checksum module for this ROM!\
                            Be aware that if this ROM need checksum correction it must be done with another software!");
        QPushButton *cancelButton = msgBox->addButton(QMessageBox::Cancel);
        QPushButton *okButton = msgBox->addButton(QMessageBox::Ok);
        msgBox->exec();

        if (msgBox->clickedButton() == cancelButton)
        {
            qDebug() << "Write canceled!";
            return NULL;
        }
    }

    return ecuCalDef;
}

FileActions::EcuCalDefStructure *FileActions::checksum_module_subarudbw_denso32bit(FileActions::EcuCalDefStructure *ecuCalDef, uint32_t checksum_area_start, uint32_t checksum_area_length)
{
    QByteArray checksum_array;

    uint32_t checksum_area_end = checksum_area_start + checksum_area_length;
    uint32_t checksum_dword_addr_lo = 0;
    uint32_t checksum_dword_addr_hi = 0;
    uint32_t checksum = 0;
    uint32_t checksum_temp = 0;
    uint32_t checksum_diff = 0;
    uint32_t checksum_check = 0;
    uint8_t checksum_block = 0;

    bool checksum_ok = true;


    for (uint32_t i = checksum_area_start; i < checksum_area_end; i+=12)
    {
        checksum = 0;
        checksum_temp = 0;
        checksum_dword_addr_lo = 0;
        checksum_dword_addr_hi = 0;
        checksum_diff = 0;

        for (int j = 0; j < 4; j++)
        {
            checksum_dword_addr_lo = (checksum_dword_addr_lo << 8) + (uint8_t)(ecuCalDef->FullRomData[i + j]);
            checksum_dword_addr_hi = (checksum_dword_addr_hi << 8) + (uint8_t)(ecuCalDef->FullRomData[i + 4 + j]);
            checksum_diff = (checksum_diff << 8) + (uint8_t)(ecuCalDef->FullRomData[i + 8 + j]);
        }
        if (i == checksum_area_start && checksum_dword_addr_lo == 0 && checksum_dword_addr_hi == 0 && checksum_diff == 0x5aa5a55a)
        {
            QMessageBox::information(this, tr("32-bit checksum"), "ROM has all checksums disabled");
            return 0;
        }

        if (checksum_dword_addr_lo == 0 && checksum_dword_addr_hi == 0 && checksum_diff == 0x5aa5a55a)
        {
            //QMessageBox::information(this, tr("32-bit checksum"), "Checksums disabled");
        }
        if (checksum_dword_addr_lo != 0 && checksum_dword_addr_hi != 0 && checksum_diff != 0x5aa5a55a)
        {
            for (uint32_t j = checksum_dword_addr_lo; j < checksum_dword_addr_hi; j+=4)
            {
                for (int k = 0; k < 4; k++)
                {
                    checksum_temp = (checksum_temp << 8) + (uint8_t)(ecuCalDef->FullRomData[j + k]);
                }
                checksum += checksum_temp;
            }
        }
        checksum_check = 0x5aa5a55a - checksum;

        if (checksum_diff == checksum_check)
        {
            //qDebug() << "Checksum block " + QString::number(checksum_block) + " OK";
        }
        else
        {
            //qDebug() << "Checksum block " + QString::number(checksum_block) + " NOK";
            checksum_ok = false;
        }

        checksum_array.append(checksum_dword_addr_lo >> 24);
        checksum_array.append(checksum_dword_addr_lo >> 16);
        checksum_array.append(checksum_dword_addr_lo >> 8);
        checksum_array.append(checksum_dword_addr_lo);
        checksum_array.append(checksum_dword_addr_hi >> 24);
        checksum_array.append(checksum_dword_addr_hi >> 16);
        checksum_array.append(checksum_dword_addr_hi >> 8);
        checksum_array.append(checksum_dword_addr_hi);
        checksum_array.append(checksum_check >> 24);
        checksum_array.append(checksum_check >> 16);
        checksum_array.append(checksum_check >> 8);
        checksum_array.append(checksum_check);

        checksum_block++;
    }

    if (!checksum_ok)
    {
        ecuCalDef->FullRomData.replace(checksum_area_start, checksum_area_length, checksum_array);
        QMessageBox::information(this, tr("32-bit checksum"), "Checksums corrected");
        //qDebug() << "Checksums corrected";
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

    //qDebug() << "Parse expression stringlist" << expression << expression.length();

    while (i < expression.length())
    {
        QString number;
        //qDebug() << "Expression stringlist index:" << i << expression.at(i);

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

    //qDebug() << "Parse expression stringlist end";

    return numbers;
}

double FileActions::calculate_value_from_expression(QStringList expression)
{
    double value = 0;
    uint8_t index = 0;
    bool expression_ok;

    //qDebug() << "Calculate value from expression";
    if (expression.length() == 1)
    {
        QString valueString = expression.at(0);
        if (valueString.startsWith("--"))
            valueString.remove(0, 2);
        value = valueString.toDouble();
    }

    //qDebug() << "Calculate value from expression";

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
