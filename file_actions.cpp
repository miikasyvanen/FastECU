#include "file_actions.h"

FileActions::FileActions()
{

}

FileActions::ConfigValuesStructure *FileActions::check_config_dir(){
    ConfigValuesStructure *configValues = &ConfigValuesStruct;

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
        QDir dir("config/");
        foreach (const QFileInfo& entry, dir.entryInfoList((QStringList() << "*.*", QDir::Files))){
            QFile().copy("config/" + entry.fileName(), configValues->config_base_directory + "/" + entry.fileName());
        }
        save_config_file();
    }
    if (!QDir(configValues->definition_files_base_directory).exists()){
        QDir().mkdir(configValues->definition_files_base_directory);
        QDir dir("definitions/");
        foreach (const QFileInfo& entry, dir.entryInfoList((QStringList() << "*.*", QDir::Files))){
            QFile().copy("definitions/" + entry.fileName(), configValues->definition_files_base_directory + "/" + entry.fileName());
        }
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


FileActions::ConfigValuesStructure *FileActions::read_config_file()
{
    ConfigValuesStructure *configValues = &ConfigValuesStruct;

    QDomDocument xmlBOM;
    //QFile file(configValues->config_base_directory + "/fastecu.cfg");
    QFile file(configValues->config_file);
    if (!file.open(QIODevice::ReadOnly ))
    {
        QMessageBox::warning(this, tr("Config file"), "Unable to open application config file for reading");
        return configValues;
    }

    QXmlStreamReader reader;
    reader.setDevice(&file);

    if (reader.readNextStartElement())
    {
        if (reader.name() == "ecu" && reader.attributes().value("name") == "FastECU")
        {
            if (reader.readNextStartElement())
            {
                if (reader.name() == "software_settings")
                {
                    while (reader.readNextStartElement())
                    {
                        if (reader.name() == "setting" && reader.attributes().value("name") == "serial_port")
                        {
                            while(reader.readNextStartElement())
                            {
                                if (reader.name() == "value")
                                {
                                    configValues->serial_port = reader.attributes().value("data").toString();
                                    reader.skipCurrentElement();
                                }
                                else
                                    reader.skipCurrentElement();
                            }
                        }
                        else if (reader.name() == "setting" && reader.attributes().value("name") == "flash_method")
                        {
                            qDebug() << "Flash method";
                            while(reader.readNextStartElement())
                            {
                                if (reader.name() == "value")
                                {
                                    configValues->flash_method = reader.attributes().value("data").toString();
                                    reader.skipCurrentElement();
                                }
                                else
                                    reader.skipCurrentElement();
                            }
                        }
                        else if (reader.name() == "setting" && reader.attributes().value("name") == "flash_protocol")
                        {
                            qDebug() << "Flash protocol";
                            while(reader.readNextStartElement())
                            {
                                if (reader.name() == "value")
                                {
                                    configValues->flash_protocol = reader.attributes().value("data").toString();
                                    reader.skipCurrentElement();
                                }
                                else
                                    reader.skipCurrentElement();
                            }
                        }
                        else if (reader.name() == "setting" && reader.attributes().value("name") == "log_protocol")
                        {
                            qDebug() << "Log protocol";
                            while(reader.readNextStartElement())
                            {
                                if (reader.name() == "value")
                                {
                                    configValues->log_protocol = reader.attributes().value("data").toString();
                                    reader.skipCurrentElement();
                                }
                                else
                                    reader.skipCurrentElement();
                            }
                        }
                        else if (reader.name() == "setting" && reader.attributes().value("name") == "definition_types")
                        {
                            while(reader.readNextStartElement())
                            {
                                if (reader.name() == "value")
                                {
                                    configValues->definition_types.append(reader.attributes().value("data").toString());
                                    reader.skipCurrentElement();
                                }
                                else
                                    reader.skipCurrentElement();
                            }
                        }
                        else if (reader.name() == "setting" && reader.attributes().value("name") == "calibration_files")
                        {
                            while(reader.readNextStartElement())
                            {
                                if (reader.name() == "value")
                                {
                                    configValues->calibration_files.append(reader.attributes().value("data").toString());
                                    reader.skipCurrentElement();
                                }
                                else
                                    reader.skipCurrentElement();
                            }
                        }
                        else if (reader.name() == "setting" && reader.attributes().value("name") == "calibration_files_directory")
                        {
                            while(reader.readNextStartElement())
                            {
                                if (reader.name() == "value")
                                {
                                    configValues->calibration_files_directory = reader.attributes().value("data").toString();
                                    reader.skipCurrentElement();
                                }
                                else
                                    reader.skipCurrentElement();
                            }

                        }
                        else if (reader.name() == "setting" && reader.attributes().value("name") == "romraider_definition_files")
                        {
                            while(reader.readNextStartElement())
                            {
                                if (reader.name() == "value")
                                {
                                    configValues->romraider_definition_files.append(reader.attributes().value("data").toString());
                                    reader.skipCurrentElement();
                                }
                                else
                                    reader.skipCurrentElement();
                            }
                        }
                        else if (reader.name() == "setting" && reader.attributes().value("name") == "ecuflash_definition_files_directory")
                        {
                            while(reader.readNextStartElement())
                            {
                                if (reader.name() == "value")
                                {
                                    configValues->ecuflash_definition_files_directory = reader.attributes().value("data").toString();
                                    reader.skipCurrentElement();
                                }
                                else
                                    reader.skipCurrentElement();
                            }
                        }
                        else if (reader.name() == "setting" && reader.attributes().value("name") == "logger_definition_file")
                        {
                            while(reader.readNextStartElement())
                            {
                                if (reader.name() == "value")
                                {
                                    configValues->logger_definition_file = reader.attributes().value("data").toString();
                                    reader.skipCurrentElement();
                                }
                                else
                                    reader.skipCurrentElement();
                            }
                            qDebug() << "Logger def file:" << configValues->logger_definition_file;
                        }
                        else if (reader.name() == "setting" && reader.attributes().value("name") == "kernel_files_directory")
                        {
                            while(reader.readNextStartElement())
                            {
                                if (reader.name() == "value")
                                {
                                    configValues->kernel_files_directory = reader.attributes().value("data").toString();
                                    reader.skipCurrentElement();
                                }
                                else
                                    reader.skipCurrentElement();
                            }
                        }
                        else if (reader.name() == "setting" && reader.attributes().value("name") == "logfiles_directory")
                        {
                            while(reader.readNextStartElement())
                            {
                                if (reader.name() == "value")
                                {
                                    configValues->log_files_base_directory = reader.attributes().value("data").toString();
                                    reader.skipCurrentElement();
                                }
                                else
                                    reader.skipCurrentElement();
                            }

                        }
                        else
                            reader.skipCurrentElement();
                    }
                }
            }
        }
    }
    file.close();

    save_config_file();

    return configValues;
}

FileActions::ConfigValuesStructure *FileActions::save_config_file(){
    ConfigValuesStructure *configValues = &ConfigValuesStruct;

    //QFile file(configValues->config_base_directory + "/fastecu.cfg");
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
    stream.writeAttribute("name", "serial_port");
    stream.writeStartElement("value");
    stream.writeAttribute("data", configValues->serial_port);
    stream.writeEndElement();
    stream.writeEndElement();

    stream.writeStartElement("setting");
    stream.writeAttribute("name", "flash_method");
    stream.writeStartElement("value");
    stream.writeAttribute("data", configValues->flash_method);
    stream.writeEndElement();
    stream.writeEndElement();

    stream.writeStartElement("setting");
    stream.writeAttribute("name", "flash_protocol");
    stream.writeStartElement("value");
    stream.writeAttribute("data", configValues->flash_protocol);
    stream.writeEndElement();
    stream.writeEndElement();

    stream.writeStartElement("setting");
    stream.writeAttribute("name", "log_protocol");
    stream.writeStartElement("value");
    stream.writeAttribute("data", configValues->log_protocol);
    stream.writeEndElement();
    stream.writeEndElement();

    stream.writeStartElement("setting");
    stream.writeAttribute("name", "definition_types");
    for (int i = 0; i < configValues->definition_types.length(); i++)
    {
        stream.writeStartElement("value");
        stream.writeAttribute("data", configValues->definition_types.at(i));
        stream.writeEndElement();
    }
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
    stream.writeAttribute("name", "romraider_definition_files");
    for (int i = 0; i < configValues->romraider_definition_files.length(); i++)
    {
        stream.writeStartElement("value");
        stream.writeAttribute("data", configValues->romraider_definition_files.at(i));
        stream.writeEndElement();
    }
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
    stream.writeAttribute("data", configValues->logger_definition_file);
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
                            qDebug() << "Found ECU ID" << file_ecu_id;
                            QDomElement protocol = ecu.firstChild().toElement();
                            while (!protocol.isNull())
                            {
                                if (protocol.tagName() == "protocol")
                                {
                                    qDebug() << "Found protocol" << protocol.attribute("id","No id");
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

                    qDebug() << "ECU ID not found, initializing log parameters";
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
                    qDebug() << "Values initialized, creating xml data";
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
                    qDebug() << "Saving log parameters";
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
}

FileActions::LogValuesStructure *FileActions::read_logger_definition_file()
{
    LogValuesStructure *logValues = &LogValuesStruct;
    ConfigValuesStructure *configValues = &ConfigValuesStruct;

    //The QDomDocument class represents an XML document.
    QDomDocument xmlBOM;

    QString filename = configValues->logger_definition_file;
    qDebug() << "Logger filename =" << filename;
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
//                    qDebug() << "Menu: " << menuName;

                    QDomElement Child = Component.firstChild().toElement();
                    while(!Child.isNull())
                    {
                        // Check if the child tag name is table
                        if (Child.tagName() == "menuitem")
                        {

                            QString menuItemName = Child.attribute("name","No name");
                            if (menuItemName == "Separator"){
                                mainWindowMenu->addSeparator();
                            }
                            else{
                                QAction* action = new QAction(menuItemName, this);
                                QString menuItemActionName = Child.attribute("id","No id");;
                                QString menuItemCheckable = Child.attribute("checkable","No checkable");;
                                QString menuItemShortcut = Child.attribute("shortcut","No shortcut");;
                                QString menuItemToolbar = Child.attribute("toolbar","No toolbar");;
                                QString menuItemIcon = Child.attribute("icon","No icon");;
                                QString menuItemTooltip = Child.attribute("tooltip","No tooltip");;

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
                        Child = Child.nextSibling().toElement();
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

FileActions::EcuCalDefStructure *FileActions::open_subaru_rom_file(FileActions::EcuCalDefStructure *ecuCalDef, QString filename)
{
    ConfigValuesStructure *configValues = &ConfigValuesStruct;

    QString file_name_str;
    QString ecuId;
    bool ecuIdConfirmed = true;
    bool bStatus = false;
    uint16_t ecuIdAddr[] = { 0x0200, 0x2000 };

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
        QFile file(filename);
        QFileInfo fileInfo(file.fileName());
        file_name_str = fileInfo.fileName();
    }

    ecuIdConfirmed = true;
    //for (unsigned j = 0; j < 2; j++)
    for (unsigned j = 0; j < sizeof(ecuIdAddr) / sizeof(ecuIdAddr[0]); j++)
    {
        qDebug() << "ECU ID addr 0x" + QString("%1").arg(ecuIdAddr[j],4,16,QLatin1Char('0')).toUtf8();
        ecuIdConfirmed = true;
        ecuId.clear();
        for (int i = ecuIdAddr[j]; i < ecuIdAddr[j] + 8; i++)
        {
            if ((uint8_t)ecuCalDef->FullRomData.at(i) < 0x30 || (uint8_t)ecuCalDef->FullRomData.at(i) > 0x5A)
            {
                ecuIdConfirmed = false;
            }
            else
            {
                ecuId.append((uint8_t)ecuCalDef->FullRomData.at(i));
            }
        }
        if (ecuIdConfirmed)
            break;
    }

    qDebug() << "ECU ID:" << ecuId;

    if (!ecuIdConfirmed)
    {
        QMessageBox::warning(this, tr("Calibration file"), "Unable to find ID from selected ROM file, file might be from unsupported model!");
        return NULL;
    }
    if (!file_name_str.length())
        file_name_str = ecuId;// + ".bin";

    configValues->definition_types[0] = "ecuflash";
    //configValues->definition_types[0] = "romraider";

    def_map_index = 0;
    for (int i = 0; i < configValues->definition_types.length(); i++)
    {
        if (configValues->definition_types.at(i) == "ecuflash")// && !ecuCalDef->use_romraider_definition)
        {
            read_ecuflash_ecu_def(ecuCalDef, ecuId);
            parse_ecuflash_def_scalings(ecuCalDef);
        }
        if (configValues->definition_types.at(i) == "romraider")// && !ecuCalDef->use_ecuflash_definition)
            read_romraider_ecu_def(ecuCalDef, ecuId);
    }
    /*
    if (!ecuCalDef->use_romraider_definition && !ecuCalDef->use_ecuflash_definition)
        read_romraider_ecu_def(ecuCalDef, ecuId); // Fallback
        */
    if (!ecuCalDef->use_romraider_definition && !ecuCalDef->use_ecuflash_definition)
    {
        QMessageBox::warning(this, tr("Calibration file"), "Unable to find definition for selected ROM file with ECU ID: " + ecuId);
        ecuCalDef = NULL;
        return NULL;
    }

    //read_ecu_definition_file(ecuCalDef, ecuId);

    if (ecuCalDef == NULL)
    {
        QMessageBox::warning(this, tr("Calibration file"), "Unable to find definition for selected ROM file with ECU ID: " + ecuId);
        ecuCalDef = NULL;
        return NULL;
    }

    ecuCalDef->OemEcuFile = true;
    ecuCalDef->FileName = file_name_str;
    ecuCalDef->FullFileName = filename;
    ecuCalDef->FileSize = QString::number(ecuCalDef->FullRomData.length());
    ecuCalDef->RomId = ecuId;
    qDebug() << "File size =" << ecuCalDef->FileSize;

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
        //qDebug() << "Map number " + QString::number(i) + " and name " + ecuCalDef->NameList.at(i);
        if (ecuCalDef->StorageTypeList.at(i) == "uint8")
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
            storagesize = ecuCalDef->SelectionsValueList.at(i).split(",").at(0).length() / 2;
            uint8_t dataByte = 0;
            uint32_t byteAddress = ecuCalDef->AddressList.at(i).toUInt(&bStatus,16);
            for (int k = 0; k < storagesize; k++)
            {
                dataByte = (uint8_t)ecuCalDef->FullRomData.at(byteAddress + k);
                //qDebug() << hex << k << "/" << storagesize << dataByte;
                qDebug() << k << "/" << storagesize << QString("%1").arg(dataByte,2,16,QLatin1Char('0'));
                mapData.append(QString("%1").arg(dataByte,2,16,QLatin1Char('0')));
            }
            ecuCalDef->MapData.replace(i, mapData);

        }
        else
        {
            for (unsigned j = 0; j < ecuCalDef->XSizeList.at(i).toUInt() * ecuCalDef->YSizeList.at(i).toUInt(); j++)
            {
                uint32_t dataByte = 0;
                uint32_t byteAddress = ecuCalDef->AddressList.at(i).toUInt(&bStatus,16) + (j * storagesize);
                if (ecuCalDef->RomInfo.at(FlashMethod) == "wrx02" && ecuCalDef->FileSize.toUInt() < byteAddress)
                    byteAddress -= 0x8000;
                for (int k = 0; k < storagesize; k++)
                {
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

                double value = 0;
                if (ecuCalDef->TypeList.at(i) != "Switch")
                {
                    if (ecuCalDef->StorageTypeList.at(i) == "float"){
                        value = calculate_value_from_expression(parse_stringlist_from_expression_string(ecuCalDef->FromByteList.at(i), QString::number(mapDataValue.floatValue, 'g', float_precision)));
                    }
                    else{
                        value = calculate_value_from_expression(parse_stringlist_from_expression_string(ecuCalDef->FromByteList.at(i), QString::number(dataByte)));
                    }
                }
                mapData.append(QString::number(value, 'g', float_precision) + ",");
            }
            ecuCalDef->MapData.replace(i, mapData);
        }
        if (ecuCalDef->XSizeList.at(i).toUInt() > 1)
        {
            if (ecuCalDef->XScaleTypeList.at(i) == "Static Y Axis")
            {
                ecuCalDef->XScaleData.replace(i, ecuCalDef->XScaleStaticDataList.at(i));
            }
            else if (ecuCalDef->XScaleTypeList.at(i) == "X Axis" || (ecuCalDef->XScaleTypeList.at(i) == "Y Axis" && ecuCalDef->TypeList.at(i) == "2D"))
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
                for (unsigned j = 0; j < ecuCalDef->XSizeList.at(i).toUInt(); j++)
                {
                    uint32_t dataByte = 0;
                    uint32_t byteAddress = ecuCalDef->XScaleAddressList.at(i).toUInt(&bStatus,16) + (j * storagesize);
                    if (ecuCalDef->RomInfo.at(FlashMethod) == "wrx02" && ecuCalDef->FileSize.toUInt() < byteAddress)
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
                    if (ecuCalDef->XScaleTypeList.at(i) != "Switch")
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
            if (ecuCalDef->YScaleStorageTypeList.at(i) == "uint8")
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
                uint32_t byteAddress = ecuCalDef->YScaleAddressList.at(i).toUInt(&bStatus,16) + (j * storagesize);
                if (ecuCalDef->RomInfo.at(FlashMethod) == "wrx02" && ecuCalDef->FileSize.toUInt() < byteAddress)
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
                if (ecuCalDef->YScaleTypeList.at(i) != "Switch")
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

    if (ecuCalDef == NULL)
    {
        QMessageBox::warning(this, tr("Calibration file"), "Unable to find definition for selected calibration file with ECU ID: " + ecuId);
        return NULL;
    }

    if (ecuCalDef->RomInfo[MemModel] == "SH7055")
        checksum_module_subarudbw(ecuCalDef, 0x07FB80, 17 * 12);
    if (ecuCalDef->RomInfo[MemModel] == "SH7058")
        checksum_module_subarudbw(ecuCalDef, 0x0FFB80, 17 * 12);

    return ecuCalDef;

}

FileActions::EcuCalDefStructure *FileActions::save_subaru_rom_file(FileActions::EcuCalDefStructure *ecuCalDef, QString filename)
{
    QFile file(filename);
    QFileInfo fileInfo(file.fileName());
    QString file_name_str = fileInfo.fileName();

    if (!file.open(QIODevice::WriteOnly ))
    {
        qDebug() << "Unable to open file for writing";
        QMessageBox::warning(this, tr("Ecu calibration file"), "Unable to open file for writing");
        return NULL;
    }

    ecuCalDef = apply_subaru_cal_changes_to_rom_data(ecuCalDef);
    file.write(ecuCalDef->FullRomData);
    file.close();

    ecuCalDef->FullFileName = filename;
    ecuCalDef->FileName = file_name_str;

    return 0;
}

FileActions::EcuCalDefStructure *FileActions::apply_subaru_cal_changes_to_rom_data(FileActions::EcuCalDefStructure *ecuCalDef)
{
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
                qDebug() << QString("%1").arg(dataByte,2,16,QLatin1Char('0'));
                ecuCalDef->FullRomData[byteAddress] = dataByte;
            }
        }
        else
        {
            for (unsigned j = 0; j < ecuCalDef->XSizeList.at(i).toUInt() * ecuCalDef->YSizeList.at(i).toUInt(); j++)
            {
                uint32_t dataByte = 0;
                uint32_t byteAddress = ecuCalDef->AddressList.at(i).toUInt(&bStatus,16) + (j * storagesize);
                if (ecuCalDef->RomInfo.at(FlashMethod) == "wrx02" && ecuCalDef->FileSize.toUInt() < byteAddress)
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
        }
        if (ecuCalDef->XSizeList.at(i).toUInt() > 1 && ecuCalDef->XScaleTypeList.at(i) != "Static Y Axis" && ecuCalDef->XScaleNameList.at(i) != " ")
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
                uint32_t byteAddress = ecuCalDef->XScaleAddressList.at(i).toUInt(&bStatus,16) + (j * storagesize);
                if (ecuCalDef->RomInfo.at(FlashMethod) == "wrx02" && ecuCalDef->FileSize.toUInt() < byteAddress)
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
        if (ecuCalDef->YSizeList.at(i).toUInt() > 1 && ecuCalDef->YScaleTypeList.at(i) != "Static Y Axis" && ecuCalDef->YScaleNameList.at(i) != " ")
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
                uint32_t byteAddress = ecuCalDef->YScaleAddressList.at(i).toUInt(&bStatus,16) + (j * storagesize);
                if (ecuCalDef->RomInfo.at(FlashMethod) == "wrx02" && ecuCalDef->FileSize.toUInt() < byteAddress)
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

    if (ecuCalDef->RomInfo[MemModel] == "SH7055")
        checksum_module_subarudbw(ecuCalDef, 0x07FB80, 17 * 12);
    if (ecuCalDef->RomInfo[MemModel] == "SH7058")
        checksum_module_subarudbw(ecuCalDef, 0x0FFB80, 17 * 12);

    return ecuCalDef;
}

QByteArray FileActions::checksum_module_subarudbw(FileActions::EcuCalDefStructure *ecuCalDef, uint32_t checksum_area_start, uint32_t checksum_area_length)
{
    //qDebug() << "Checking 32-bit checksum, filesize = " << ecuCalDef->FileSize << ecuCalDef->RomInfo[FileSize];
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
            //qDebug() << "Checksum block " + QString::number(checksum_block) + " DISABLED!";
            //checksum = 0;
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

    //qDebug() << "checksum area:" << checksum_area_length;
    //qDebug() << "checksum_array:" << checksum_array.length() << checksum_array;

    if (!checksum_ok)
    {
        ecuCalDef->FullRomData.replace(checksum_area_start, checksum_area_length, checksum_array);
        QMessageBox::information(this, tr("32-bit checksum"), "Checksums corrected");
    }

    return 0;
}

QStringList FileActions::parse_stringlist_from_expression_string(QString expression, QString x)
{
    QStringList numbers;
    QStringList operators;
    uint8_t output = 0;
    uint8_t stack = 0;
    bool isOperator = true;

    int i = 0;
    //qDebug() << expression;
    while (i < expression.length())
    {
        QString number;

        if (expression.at(i) == "x")
        {
            isOperator = false;
            numbers.append(x);
            output++;
        }
        else if ((isOperator && expression.at(i) == "-" && expression.at(i + 1) == "x") || (expression.at(i) == "-" && expression.at(i + 1) == "x" && i == 0))
        {
            isOperator = false;
            number.append(expression.at(i));
            number.append(x);
            numbers.append(number);
            i++;
            output+=2;

        }
        else if (expression.at(i).isNumber() || expression.at(i) == "." || (isOperator && expression.at(i) == "-"))
        {
            isOperator = false;
            number.append(expression.at(i));
            i++;
            while (expression.at(i).isNumber() || expression.at(i) == ".")
            {
                number.append(expression.at(i));
                i++;
            }
            i--;
            numbers.append(number);
            output++;
        }
        else if (expression.at(i) == "(")
        {
            isOperator = true;
            operators.append(expression.at(i));
            stack++;
        }
        else if (expression.at(i) == ")")
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
        else if (expression.at(i) == "*")
        {
            isOperator = true;
            operators.append(expression.at(i));
            stack++;
        }
        else if (expression.at(i) == "/")
        {
            isOperator = true;
            operators.append(expression.at(i));
            stack++;
        }
        else if (expression.at(i) == "+")
        {
            isOperator = true;
            if ((operators.length() > 0 && i > 0) && (operators.at(stack - 1) == "/" || operators.at(stack - 1) == "*"))
            {
                //qDebug() << "Check expression +: / or * found";
                numbers.append(operators.at(stack - 1));
                output++;
                operators.replace(stack - 1, expression.at(i));
            }
            else
            {
                //qDebug() << "Check expression +: / or * NOT found";
                operators.append(expression.at(i));
                stack++;
            }
        }
        else if (expression.at(i) == "-")
        {
            isOperator = true;
            if ((operators.length() > 0 && i > 0) && (operators.at(stack - 1) == "/" || operators.at(stack - 1) == "*"))
            {
                //qDebug() << "Check expression -: / or * found";
                numbers.append(operators.at(stack - 1));
                output++;
                operators.replace(stack - 1, expression.at(i));
            }
            else
            {
                //qDebug() << "Check expression -: / or * NOT found";
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

    //if (numbers.length() > 0 && numbers.at(0) == "14.7")
    //    qDebug() << numbers;
    //if (numbers.length() > 1 && numbers.at(1) == "14.7")
    //    qDebug() << numbers;

    return numbers;
}

double FileActions::calculate_value_from_expression(QStringList expression)
{
    double value = 0;
    uint8_t index = 0;
    bool expression_ok;

    if (expression.length() == 1)
    {
        QString valueString = expression.at(0);
        if (valueString.startsWith("--"))
            valueString.remove(0, 2);
        value = valueString.toDouble();
    }

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

    return value;
}
