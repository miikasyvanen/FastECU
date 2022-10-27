#include "file_actions.h"

FileActions::FileActions()
{

}

FileActions::ConfigValuesStructure *FileActions::checkConfigDir(){
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
        saveConfigFile();
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


FileActions::ConfigValuesStructure *FileActions::readConfigFile()
{
    ConfigValuesStructure *configValues = &ConfigValuesStruct;

    QDomDocument xmlBOM;
    QFile file(configValues->config_base_directory + "/fastecu.cfg");
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
                        else if (reader.name() == "setting" && reader.attributes().value("name") == "ecu_definition_files")
                        {
                            while(reader.readNextStartElement())
                            {
                                if (reader.name() == "value")
                                {
                                    configValues->ecu_definition_files.append(reader.attributes().value("data").toString());
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

    return configValues;
}

FileActions::ConfigValuesStructure *FileActions::saveConfigFile(){
    ConfigValuesStructure *configValues = &ConfigValuesStruct;

    QFile file(configValues->config_base_directory + "/fastecu.cfg");
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
    stream.writeAttribute("name", "ecu_definition_files");
    for (int i = 0; i < configValues->ecu_definition_files.length(); i++)
    {
        stream.writeStartElement("value");
        stream.writeAttribute("data", configValues->ecu_definition_files.at(i));
        stream.writeEndElement();
    }
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

FileActions::LogValuesStructure *FileActions::read_logger_conf(FileActions::LogValuesStructure *logValues, QString ecu_id)
{
    //LogValuesStructure *logValues = &LogValuesStruct;
    ConfigValuesStructure *configValues = &ConfigValuesStruct;

    //The QDomDocument class represents an XML document.
    QDomDocument xmlBOM;

    QString filename = configValues->logger_config_file;
    //qDebug() << "Logger filename =" << filename;
    QFile file(filename);
    if(!file.open(QFile::ReadOnly | QFile::Text)) {
        QMessageBox::warning(this, tr("Logger file"), "Unable to open logger definition file for reading");
        return NULL;
    }
    xmlBOM.setContent(&file);
    file.close();

    // Extract the root markup
    QDomElement root = xmlBOM.documentElement();

    logValues->dashboard_log_value_id.clear();
    logValues->lower_panel_log_value_id.clear();
    logValues->lower_panel_switch_id.clear();

    if (root.tagName() == "logger")
    {
        //qDebug() << "Logger start element";
        QDomElement ecus = root.firstChild().toElement();
        while (!ecus.isNull())
        {
            if (ecus.tagName() == "ecus")
            {
                //qDebug() << "Found ecus tag";
                QDomElement ecu = ecus.firstChild().toElement();
                while (!ecu.isNull())
                {
                    if (ecu.tagName() == "ecu")
                    {
                        QString file_ecu_id = ecu.attribute("id","No id");

                        if (ecu_id == file_ecu_id)
                        {
                            //qDebug() << "Found ECU ID" << file_ecu_id;
                            QDomElement protocol = ecu.firstChild().toElement();
                            while (!protocol.isNull())
                            {
                                //qDebug() << "Protocols start element";
                                if (protocol.tagName() == "protocol")
                                {
                                    //qDebug() << "Found protocol" << protocol.attribute("id","No id");
                                    QDomElement parameters = protocol.firstChild().toElement();
                                    while(!parameters.isNull())
                                    {
                                        if (parameters.tagName() == "parameters")
                                        {
                                            //qDebug() << "Found parameters tag";
                                            QDomElement parameter_type = parameters.firstChild().toElement();
                                            while(!parameter_type.isNull())
                                            {
                                                if (parameter_type.tagName() == "gauges")
                                                {
                                                    //qDebug() << "Found gauges tag";
                                                    QDomElement gauges = parameter_type.firstChild().toElement();
                                                    while(!gauges.isNull())
                                                    {
                                                        if (gauges.tagName() == "parameter")
                                                        {
                                                            //qDebug() << "Found gauge parameter tag with ID" << gauges.attribute("id","No id");
                                                            logValues->dashboard_log_value_id.append(gauges.attribute("id","No id"));
                                                        }
                                                        gauges = gauges.nextSibling().toElement();
                                                    }
                                                }
                                                if (parameter_type.tagName() == "lower_panel")
                                                {
                                                    //qDebug() << "Found lower_panel tag";
                                                    QDomElement lower_panel = parameter_type.firstChild().toElement();
                                                    while(!lower_panel.isNull())
                                                    {
                                                        if (lower_panel.tagName() == "parameter")
                                                        {
                                                            //qDebug() << "Found lower_panel parameter tag with ID" << lower_panel.attribute("id","No id");
                                                            logValues->lower_panel_log_value_id.append(lower_panel.attribute("id","No id"));
                                                        }
                                                        lower_panel = lower_panel.nextSibling().toElement();
                                                    }
                                                }
                                                parameter_type = parameter_type.nextSibling().toElement();
                                            }
                                        }
                                        if (parameters.tagName() == "switches")
                                        {
                                            //qDebug() << "Found switches tag";
                                            QDomElement switches = parameters.firstChild().toElement();
                                            while(!switches.isNull())
                                            {
                                                if (switches.tagName() == "switch")
                                                {
                                                    //qDebug() << "Found switch tag with ID" << switches.attribute("id","No id");
                                                    logValues->lower_panel_switch_id.append(switches.attribute("id","No id"));
                                                }
                                                switches = switches.nextSibling().toElement();
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
            ecus = ecus.nextSibling().toElement();
        }
    }

    return logValues;
}

FileActions::LogValuesStructure *FileActions::save_logger_conf(FileActions::LogValuesStructure *logValues, QString ecu_id)
{

}

FileActions::LogValuesStructure *FileActions::readLoggerDefinitionFile()
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

QSignalMapper *FileActions::readMenuFile(QMenuBar *menubar, QToolBar *toolBar)
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

FileActions::EcuCalDefStructure *FileActions::readEcuBaseDef(EcuCalDefStructure *ecuCalDef)
{
    ConfigValuesStructure *configValues = &ConfigValuesStruct;

    bool OemEcuDefBaseFileFound = false;

    QString rombase;
    QString xmlid;

    //The QDomDocument class represents an XML document.
    QDomDocument xmlBOM;
    // Load xml file as raw data
    //QString filename = configValues->definitionsDir + "/" + ecuCalDef->RomBase + ".xml";
    QString filename = configValues->ecu_definition_files.at(0);

    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly ))
    {
        ecuCalDef = NULL;
        qDebug() << "Unable to open OEM ecu base definitions file " + filename + " for reading";
        QMessageBox::warning(this, tr("Ecu definitions file"), "Unable to open OEM ecu base definitions file " + filename + " for reading");
        return NULL;
    }

    // Set data into the QDomDocument before processing
    xmlBOM.setContent(&file);
    file.close();

    //qDebug() << "Reading XML BASE def file" << filename;

    // Extract the root markup
    QDomElement root = xmlBOM.documentElement();

    // Get root names and attributes
    QString Type = root.tagName();
    QString Name = root.attribute("name"," ");
    //qDebug() << Type;
    QDomElement TagType = root.firstChild().toElement();

    uint16_t index = 0;

    QDomNodeList items = root.childNodes();
    for(int x=0; x< items.count(); x++)
    {
        if (items.at(x).isComment())
            continue;

        QDomElement TagType = items.at(x).toElement();

        //while (!TagType.isNull())
        //{

        if (TagType.tagName() == "rom")
        {
            //qDebug() << ecuCalDef->RomInfo[RomBase];
            //qDebug() << "Found <rom> section";
            rombase = TagType.attribute("base","No base");
            if (rombase == "No base")
            {
                //qDebug() << "Rom base is" << rombase;
                QDomElement RomId = TagType.firstChild().toElement();
                if (RomId.tagName() == "romid")
                {
                    //qDebug() << "Found ROM ID section";
                    QDomElement RomInfo = RomId.firstChild().toElement();
                    while (!RomInfo.tagName().isNull())
                    {

                        if (RomInfo.tagName() == "xmlid")
                            xmlid = RomInfo.text();

                        //qDebug() << "xmlid" << xmlid;

                        //if (xmlid == ecuCalDef->RomInfo[RomBase])
                            //continue;

                        RomInfo = RomInfo.nextSibling().toElement();
                    }
                }
                if (xmlid == ecuCalDef->RomInfo[RomBase])
                {
                    OemEcuDefBaseFileFound = true;
                    //qDebug() << "Found ROM BASE section" << xmlid << "for ECUID" << ecuCalDef->RomInfo[XmlId];
                    QDomElement Table = TagType.firstChild().toElement();
                    //qDebug() << "Check if tables" << Table.tagName();
                    while(!Table.isNull())
                    {
                        //qDebug() << "Table.TagName" << Table.tagName();
                        // Check if the child tag name is table
                        if (Table.tagName() == "table")
                        {
                            //qDebug() << "Check tables";
                            for (int i = 0; i < ecuCalDef->NameList.length(); i++)
                            {
                                if (Table.attribute("name","No name") == ecuCalDef->NameList.at(i))
                                {
                                    //qDebug() << ecuCalDef->NameList.at(i) << "/" << Table.attribute("name"," ");
                                    //qDebug() << ecuCalDef->NameList.at(i);
                                    ecuCalDef->TypeList.append(Table.attribute("type"," "));
                                    ecuCalDef->CategoryList.append(Table.attribute("category"," "));

                                    if (ecuCalDef->XSizeList.at(i) == "" || ecuCalDef->XSizeList.at(i) == " ")
                                        ecuCalDef->XSizeList.replace(i, Table.attribute("sizex", "1"));
                                    if (ecuCalDef->YSizeList.at(i) == "" || ecuCalDef->YSizeList.at(i) == " ")
                                        ecuCalDef->YSizeList.replace(i, Table.attribute("sizey", "1"));
                                    ecuCalDef->MinValueList.append(Table.attribute("minvalue","0"));
                                    ecuCalDef->MaxValueList.append(Table.attribute("maxvalue","0"));

                                    ecuCalDef->StorageTypeList.append(Table.attribute("storagetype"," "));
                                    ecuCalDef->EndianList.append(Table.attribute("endian"," "));
                                    ecuCalDef->LogParamList.append(Table.attribute("logparam"," "));

                                    ecuCalDef->MapData.append(" ");
                                    ecuCalDef->XScaleData.append(" ");
                                    ecuCalDef->YScaleData.append(" ");
                                    ecuCalDef->CategoryExpandedList.append(" ");
                                    ecuCalDef->VisibleList.append(" ");
                                    ecuCalDef->SyncedWithEcu = true;

                                    // Get the first child of the Table
                                    QDomElement TableChild = Table.firstChild().toElement();
                                    QString TableSelections;
                                    QString TableSelectionsSorted;
                                    QString TableDescription;
                                    QString TableStates;
                                    bool ContainsTableScalings = false;
                                    bool ContainsTableSelections = false;
                                    bool ContainsTableDescription = false;
                                    bool ContainsTableXScale = false;
                                    bool ContainsTableYScale = false;
                                    bool ContainsTableStaticYScale = false;
                                    bool ContainsTableState = false;

                                    // Read each child of the Table node
                                    while (!TableChild.isNull())
                                    {
                                        if (TableChild.tagName() == "scaling"){
                                            ContainsTableScalings = true;
                                            ecuCalDef->UnitsList.append(TableChild.attribute("units"," "));
                                            ecuCalDef->FormatList.append(TableChild.attribute("format"," "));
                                            ecuCalDef->FineIncList.append(TableChild.attribute("fineincrement"," "));
                                            ecuCalDef->CoarseIncList.append(TableChild.attribute("coarseincrement"," "));

                                            ecuCalDef->FromByteList.append(TableChild.attribute("expression","x"));
                                            ecuCalDef->ToByteList.append(TableChild.attribute("to_byte","x"));
                                        }
                                        else if (TableChild.tagName() == "table")
                                        {
                                            QString ScaleType = TableChild.attribute("type"," ");
                                            if (ScaleType == "X Axis"){
                                                ContainsTableXScale = true;
                                                ecuCalDef->XScaleNameList.append(TableChild.attribute("name"," "));
                                                ecuCalDef->XScaleTypeList.append(ScaleType);
                                                //ecuCalDef->XScaleAddressList.append(TableChild.attribute("storageaddress"," "));
                                                ecuCalDef->XScaleStorageTypeList.append(TableChild.attribute("storagetype"," "));
                                                ecuCalDef->XScaleEndianList.append(TableChild.attribute("endian"," "));
                                                ecuCalDef->XScaleLogParamList.append(TableChild.attribute("logparam"," "));

                                                QDomElement SubChild = TableChild.firstChild().toElement();
                                                while (!SubChild.isNull())
                                                {
                                                    if (SubChild.tagName() == "scaling"){
                                                        ecuCalDef->XScaleUnitsList.append(SubChild.attribute("units"," "));
                                                        ecuCalDef->XScaleFormatList.append(SubChild.attribute("format"," "));
                                                        ecuCalDef->XScaleFineIncList.append(SubChild.attribute("fineincrement"," "));
                                                        ecuCalDef->XScaleCoarseIncList.append(SubChild.attribute("coarseincrement"," "));

                                                        ecuCalDef->XScaleFromByteList.append(SubChild.attribute("expression","x"));
                                                        ecuCalDef->XScaleToByteList.append(SubChild.attribute("to_byte","x"));

                                                        SubChild = SubChild.nextSibling().toElement();
                                                    }
                                                }
                                            }
                                            else if (ScaleType == "Y Axis" && ecuCalDef->TypeList.at(i) == "3D"){
                                                ContainsTableYScale = true;
                                                ecuCalDef->YScaleNameList.append(TableChild.attribute("name"," "));
                                                ecuCalDef->YScaleTypeList.append(ScaleType);
                                                ecuCalDef->YScaleStorageTypeList.append(TableChild.attribute("storagetype"," "));
                                                ecuCalDef->YScaleEndianList.append(TableChild.attribute("endian"," "));
                                                ecuCalDef->YScaleLogParamList.append(TableChild.attribute("logparam"," "));
                                                ecuCalDef->XScaleStaticDataList.append(" ");

                                                QDomElement SubChild = TableChild.firstChild().toElement();
                                                while (!SubChild.isNull())
                                                {
                                                    if (SubChild.tagName() == "scaling"){
                                                        ecuCalDef->YScaleUnitsList.append(SubChild.attribute("units"," "));
                                                        ecuCalDef->YScaleFormatList.append(SubChild.attribute("format"," "));
                                                        ecuCalDef->YScaleFineIncList.append(SubChild.attribute("fineincrement"," "));
                                                        ecuCalDef->YScaleCoarseIncList.append(SubChild.attribute("coarseincrement"," "));

                                                        ecuCalDef->YScaleFromByteList.append(SubChild.attribute("expression","x"));
                                                        ecuCalDef->YScaleToByteList.append(SubChild.attribute("to_byte","x"));

                                                        SubChild = SubChild.nextSibling().toElement();
                                                    }
                                                }
                                            }
                                            else if (ScaleType == "Static Y Axis" || (ScaleType == "Y Axis" && ecuCalDef->TypeList.at(i) == "2D")){
                                                ecuCalDef->XScaleNameList.append(TableChild.attribute("name"," "));
                                                ecuCalDef->XScaleTypeList.append(ScaleType);
                                                //ecuCalDef->XScaleAddressList.append(TableChild.attribute("storageaddress"," "));
                                                ecuCalDef->XScaleStorageTypeList.append(TableChild.attribute("storagetype"," "));
                                                ecuCalDef->XScaleEndianList.append(TableChild.attribute("endian"," "));
                                                ecuCalDef->XScaleLogParamList.append(TableChild.attribute("logparam"," "));

                                                QDomElement SubChild = TableChild.firstChild().toElement();
                                                QString StaticYScaleData;
                                                if (SubChild.tagName() == "scaling"){
                                                    ContainsTableXScale = true;
                                                    ecuCalDef->XScaleUnitsList.append(SubChild.attribute("units"," "));
                                                    ecuCalDef->XScaleFormatList.append(SubChild.attribute("format"," "));
                                                    ecuCalDef->XScaleFineIncList.append(SubChild.attribute("fineincrement"," "));
                                                    ecuCalDef->XScaleCoarseIncList.append(SubChild.attribute("coarseincrement"," "));

                                                    ecuCalDef->XScaleFromByteList.append(SubChild.attribute("expression","x"));
                                                    ecuCalDef->XScaleToByteList.append(SubChild.attribute("to_byte","x"));
                                                    ecuCalDef->XScaleStaticDataList.append(" ");

                                                }
                                                if (SubChild.tagName() == "data"){
                                                    while (!SubChild.isNull())
                                                    {
                                                        if (SubChild.tagName() == "data"){
                                                            ContainsTableXScale = true;
                                                            StaticYScaleData.append(SubChild.text());
                                                            StaticYScaleData.append(",");
                                                            //qDebug() << SubChild.text();
                                                        }
                                                        SubChild = SubChild.nextSibling().toElement();
                                                    }
                                                    ecuCalDef->XScaleUnitsList.append(SubChild.attribute("units"," "));
                                                    ecuCalDef->XScaleFormatList.append(SubChild.attribute("format"," "));
                                                    ecuCalDef->XScaleFineIncList.append(SubChild.attribute("fineincrement"," "));
                                                    ecuCalDef->XScaleCoarseIncList.append(SubChild.attribute("coarseincrement"," "));

                                                    ecuCalDef->XScaleFromByteList.append(SubChild.attribute("expression"," "));
                                                    ecuCalDef->XScaleToByteList.append(SubChild.attribute("to_byte"," "));
                                                    ecuCalDef->XScaleStaticDataList.append(StaticYScaleData);
                                                }
                                                ecuCalDef->XSizeList.replace(i, ecuCalDef->YSizeList[i]);
                                                ecuCalDef->YSizeList[i] = "1";
                                                ecuCalDef->XScaleAddressList.replace(i, ecuCalDef->YScaleAddressList[i]);
                                                ecuCalDef->YScaleAddressList[i] = " ";
                                            }
                                        }
                                        else if (TableChild.tagName() == "description"){
                                            ContainsTableDescription = true;
                                            TableDescription = TableChild.text();
                                            if (!TableDescription.isEmpty())
                                                ecuCalDef->DescriptionList.append("\n\n" + TableDescription);
                                            else
                                                ecuCalDef->DescriptionList.append(" ");
                                        }
                                        else if (TableChild.tagName() == "state"){
                                            ContainsTableState = true;
                                            TableStates.append(TableChild.attribute("name"," ") + ",");
                                            TableStates.append(TableChild.attribute("data"," ") + ",");

                                        }
                                        else
                                        {
                                            ecuCalDef->UnitsList.append(TableChild.attribute("units"," "));
                                            ecuCalDef->FormatList.append(TableChild.attribute("format"," "));
                                            ecuCalDef->FineIncList.append(TableChild.attribute("fineincrement"," "));
                                            ecuCalDef->CoarseIncList.append(TableChild.attribute("coarseincrement"," "));

                                            ecuCalDef->FromByteList.append(TableChild.attribute("expression","x"));
                                            ecuCalDef->ToByteList.append(TableChild.attribute("to_byte","x"));
                                        }
                                        // Next child
                                        TableChild = TableChild.nextSibling().toElement();
                                    }

                                    if (!ContainsTableScalings)
                                    {
                                        ecuCalDef->UnitsList.append(" ");
                                        ecuCalDef->FormatList.append(" ");
                                        ecuCalDef->FineIncList.append(" ");
                                        ecuCalDef->CoarseIncList.append(" ");

                                        ecuCalDef->FromByteList.append(" ");
                                        ecuCalDef->ToByteList.append(" ");
                                    }
                                    if (!ContainsTableSelections)
                                    {
                                        ecuCalDef->SelectionsList.append(" ");
                                        ecuCalDef->SelectionsListSorted.append(" ");
                                    }
                                    else
                                    {
                                        if (!TableSelections.isEmpty())
                                        {
                                            ecuCalDef->SelectionsList.append(TableSelections);
                                            ecuCalDef->SelectionsListSorted.append(TableSelectionsSorted);
                                        }
                                        else
                                        {
                                            ecuCalDef->SelectionsList.append(" ");
                                            ecuCalDef->SelectionsListSorted.append(" ");
                                        }
                                    }

                                    if (!ContainsTableDescription)
                                        ecuCalDef->DescriptionList.append(" ");

                                    if (!ContainsTableState)
                                        ecuCalDef->StateList.append(" ");
                                    else
                                        ecuCalDef->StateList.append(TableStates);

                                    if (!ContainsTableXScale){
                                        ecuCalDef->XScaleTypeList.append(" ");
                                        ecuCalDef->XScaleNameList.append(" ");
                                        ecuCalDef->XScaleAddressList.append(" ");
                                        ecuCalDef->XScaleUnitsList.append(" ");
                                        ecuCalDef->XScaleFormatList.append(" ");
                                        ecuCalDef->XScaleFineIncList.append(" ");
                                        ecuCalDef->XScaleCoarseIncList.append(" ");
                                        ecuCalDef->XScaleStorageTypeList.append(" ");
                                        ecuCalDef->XScaleEndianList.append(" ");
                                        ecuCalDef->XScaleLogParamList.append(" ");
                                        ecuCalDef->XScaleFromByteList.append(" ");
                                        ecuCalDef->XScaleToByteList.append(" ");
                                        ecuCalDef->XScaleStaticDataList.append(" ");
                                    }
                                    if (!ContainsTableYScale){
                                        ecuCalDef->YScaleTypeList.append(" ");
                                        ecuCalDef->YScaleNameList.append(" ");
                                        ecuCalDef->YScaleAddressList.append(" ");
                                        ecuCalDef->YScaleUnitsList.append(" ");
                                        ecuCalDef->YScaleFormatList.append(" ");
                                        ecuCalDef->YScaleFineIncList.append(" ");
                                        ecuCalDef->YScaleCoarseIncList.append(" ");
                                        ecuCalDef->YScaleStorageTypeList.append(" ");
                                        ecuCalDef->YScaleEndianList.append(" ");
                                        ecuCalDef->YScaleLogParamList.append(" ");
                                        ecuCalDef->YScaleFromByteList.append(" ");
                                        ecuCalDef->YScaleToByteList.append(" ");
                                    }

                                    ContainsTableScalings = false;
                                    ContainsTableSelections = false;
                                    ContainsTableDescription = false;
                                    ContainsTableState = false;
                                    ContainsTableXScale = false;
                                    ContainsTableYScale = false;
                                    ContainsTableStaticYScale = false;
                                }
                            }
                        }
                        // Next component
                        Table = Table.nextSibling().toElement();
                    }
                }
            }
        }
        TagType = TagType.nextSibling().toElement();
    }

    if (!OemEcuDefBaseFileFound)
        return NULL;

    //qDebug() << "ECU rom base file read";
    return ecuCalDef;
}

FileActions::EcuCalDefStructure *FileActions::readEcuDef(EcuCalDefStructure *ecuCalDef, QString ecuId)
{
    //qDebug() << "Read OEM ECU definition file";

    bool ContainsTableXScale = false;
    bool ContainsTableYScale = false;
    bool OemEcuDefFileFound = false;

    QString rombase;
    QString xmlid;
    QString internalidaddress;
    QString internalidstring;
    QString ecuid;
    QString year;
    QString market;
    QString make;
    QString model;
    QString submodel;
    QString transmission;
    QString memmodel;
    QString checksummodule;
    QString flashmethod;
    QString filesize;

    QDateTime dateTime = dateTime.currentDateTime();
    QString dateTimeString = dateTime.toString("[yyyy-MM-dd hh':'mm':'ss'.'zzz']");
    qDebug() << dateTimeString << "Start looking ECU ID";

    //EcuCalDefStructure *ecuCalDef = new EcuCalDefStructure;
    ConfigValuesStructure *configValues = &ConfigValuesStruct;

    //The QDomDocument class represents an XML document.
    QDomDocument xmlBOM;
    // Load xml file as raw data
    if (!configValues->ecu_definition_files.length()) {
        QMessageBox::warning(this, tr("Ecu definition file"), "No definition file(s), use definition manager at 'Edit' menu to choose file(s)");
        ecuCalDef = NULL;
        return NULL;
    }

    QString filename = configValues->ecu_definition_files.at(0);
    QFile file(filename);

    if (!file.open(QIODevice::ReadOnly ))
    {
        ecuCalDef = NULL;
        qDebug() << "Unable to open OEM ecu definition file for reading";
        //QMessageBox::warning(this, tr("Ecu definitions file"), "Unable to open OEM ecu definition file for reading");
        return NULL;
    }

    QXmlStreamReader reader;
    reader.setDevice(&file);
    int index = 0;

    while (ecuCalDef->RomInfo.length() < RomInfoStrings.length())
    {
        ecuCalDef->RomInfo.append(" ");
    }

    if (reader.readNextStartElement())
    {
        if (reader.name() == "roms")
        {
            //qDebug() << "<roms> TAG";
            while (reader.readNextStartElement())
            {
                if (reader.name() == "rom")
                {
                    rombase = reader.attributes().value("base").toString();
                    //qDebug() << "<rom> TAG";
                    while (reader.readNextStartElement())
                    {
                        if (xmlid == ecuId)
                        {
                            dateTime = dateTime.currentDateTime();
                            dateTimeString = dateTime.toString("[yyyy-MM-dd hh':'mm':'ss'.'zzz']");
                            //qDebug() << dateTimeString << "ECU ID" << xmlid << "found";

                            //qDebug() << "Correct ROM ID found =" << xmlid;
                            OemEcuDefFileFound = true;
                            ecuCalDef->RomInfo.replace(XmlId, xmlid);
                            ecuCalDef->RomInfo.replace(InternalIdAddress, internalidaddress);
                            ecuCalDef->RomInfo.replace(InternalIdString, internalidstring);
                            ecuCalDef->RomInfo.replace(EcuId, ecuid);
                            ecuCalDef->RomInfo.replace(Year, year);
                            ecuCalDef->RomInfo.replace(Market, market);
                            ecuCalDef->RomInfo.replace(Make, make);
                            ecuCalDef->RomInfo.replace(Model, model);
                            ecuCalDef->RomInfo.replace(SubModel, submodel);
                            ecuCalDef->RomInfo.replace(Transmission, transmission);
                            ecuCalDef->RomInfo.replace(MemModel, memmodel);
                            ecuCalDef->RomInfo.replace(ChecksumModule, checksummodule);
                            ecuCalDef->RomInfo.replace(FlashMethod, flashmethod);
                            ecuCalDef->RomInfo.replace(FileSize, filesize);
                            ecuCalDef->RomInfo.replace(RomBase, rombase);

                            ecuCalDef->RomBase = rombase;

                            xmlid = "";

                            //qDebug() << "Header ready " + ecuCalDef->RomInfo.at(MemModel);
                        }
                        if (reader.name() == "romid")
                        {
                            //qDebug() << "<romid> TAG";
                            while (reader.readNextStartElement())
                            {
                                if (reader.name() == "xmlid")
                                    xmlid = reader.readElementText();
                                else if (reader.name() == "internalidaddress")
                                    internalidaddress = reader.readElementText();
                                else if (reader.name() == "internalidstring")
                                    internalidstring = reader.readElementText();
                                else if (reader.name() == "ecuid")
                                    ecuid = reader.readElementText();
                                else if (reader.name() == "year")
                                    year = reader.readElementText();
                                else if (reader.name() == "market")
                                    market = reader.readElementText();
                                else if (reader.name() == "make")
                                    make = reader.readElementText();
                                else if (reader.name() == "model")
                                    model = reader.readElementText();
                                else if (reader.name() == "submodel")
                                    submodel = reader.readElementText();
                                else if (reader.name() == "transmission")
                                    transmission = reader.readElementText();
                                else if (reader.name() == "memmodel")
                                    memmodel = reader.readElementText();
                                else if (reader.name() == "checksummodule")
                                    checksummodule = reader.readElementText();
                                else if (reader.name() == "flashmethod")
                                    flashmethod = reader.readElementText();
                                else if (reader.name() == "filesize")
                                    filesize = reader.readElementText();
                                else
                                    reader.skipCurrentElement();

                            }
                            //reader.readNextStartElement();
                        }
                        else if (reader.name() == "table" && OemEcuDefFileFound)
                        {
                            //qDebug() << "Create body with table name '" + reader.attributes().value("name").toString() + "' and type " + reader.attributes().value("type").toString();

                            ecuCalDef->NameList.append(" ");
                            ecuCalDef->AddressList.append(" ");
                            ecuCalDef->XSizeList.append(" ");
                            ecuCalDef->YSizeList.append(" ");
                            //ecuCalDef->XScaleAddressList.append(" ");
                            //ecuCalDef->YScaleAddressList.append(" ");

                            //qDebug() << "Add values";
                            //qDebug() << Table.attribute("name"," ");
                            ecuCalDef->NameList.replace(index, reader.attributes().value("name").toString());
                            ecuCalDef->AddressList.replace(index, reader.attributes().value("storageaddress").toString());
                            ecuCalDef->XSizeList.replace(index, reader.attributes().value("sizex").toString());
                            ecuCalDef->YSizeList.replace(index, reader.attributes().value("sizey").toString());
                            index++;
                            while(reader.readNextStartElement())
                            {
                                if (reader.name() == "table")
                                {
                                    //qDebug() << "Body sub with table name '" + reader.attributes().value("name").toString() + "' and type " + reader.attributes().value("type").toString();
                                    if (reader.attributes().value("type").toString() == "X Axis")
                                    {
                                        ContainsTableXScale = true;
                                        //ecuCalDef->XScaleNameList.append(reader.attributes().value("name").toString());
                                        ecuCalDef->XScaleAddressList.append(reader.attributes().value("storageaddress").toString());
                                    }
                                    if (reader.attributes().value("type").toString() == "Y Axis")
                                    {
                                        ContainsTableYScale = true;
                                        //ecuCalDef->YScaleNameList.append(reader.attributes().value("name").toString());
                                        ecuCalDef->YScaleAddressList.append(reader.attributes().value("storageaddress").toString());
                                    }
                                    if (reader.attributes().value("type").toString() == "Static Y Axis")
                                    {
                                        //ContainsTableYScale = true;
                                        //ecuCalDef->YScaleNameList.append(reader.attributes().value("name").toString());
                                        //ecuCalDef->YScaleAddressList.append(reader.attributes().value("storageaddress").toString());
                                    }
                                }
                                //else
                                    //reader.skipCurrentElement();
                                QString s = reader.readElementText();
                            }
                            if (!ContainsTableXScale){
                                //ecuCalDef->XScaleNameList.append(" ");
                                ecuCalDef->XScaleAddressList.append(" ");
                            }
                            if (!ContainsTableYScale){
                                //ecuCalDef->YScaleNameList.append(" ");
                                ecuCalDef->YScaleAddressList.append(" ");
                            }
                            ContainsTableXScale = false;
                            ContainsTableYScale = false;
                        }
                        else
                            reader.skipCurrentElement();
                        //QString s = reader.readElementText();
                    }
                }
                else
                    reader.skipCurrentElement();
                //QString s = reader.readElementText();

                if (OemEcuDefFileFound)
                    break;
            }
            if (!OemEcuDefFileFound)
                return NULL;
        }
    }
    file.close();

/*
    //qDebug() << filename;
    // Set data into the QDomDocument before processing
    xmlBOM.setContent(&file);
    file.close();

    //qDebug() << "Reading XML def file " + filename;

    // Extract the root markup
    QDomElement root = xmlBOM.documentElement();

    // Get root names and attributes
    QString Type = root.tagName();
    QString Name = root.attribute("name"," ");

    while (ecuCalDef->RomInfo.length() < RomInfoStrings.length())
    {
        ecuCalDef->RomInfo.append(" ");
    }

    QDomNodeList items = root.childNodes();
    for(int x=0; x < items.count(); x++)
    {
        if (items.at(x).isComment())
            continue;

        QDomElement TagType = items.at(x).toElement();

        int index = 0;

        if (TagType.tagName() == "rom")
        {
            rombase = TagType.attribute("base","No base");
            if (rombase != "No base")
            {
                QDomElement RomId = TagType.firstChild().toElement();
                if (RomId.tagName() == "romid")
                {
                    QDomElement RomInfo = RomId.firstChild().toElement();
                    while (!RomInfo.tagName().isNull() && RomInfo.tagName() != "xmlid")
                    {
                        RomInfo = RomInfo.nextSibling().toElement();
                    }
                    if (RomInfo.text() == ecuId)
                    {
                        qDebug() << "Found ROM ID:" << RomInfo.text();
                        RomInfo = RomId.firstChild().toElement();
                        while (!RomInfo.tagName().isNull())
                        {
                            if (RomInfo.tagName() == "xmlid")
                                xmlid = RomInfo.text();
                            else if (RomInfo.tagName() == "internalidaddress")
                                internalidaddress = RomInfo.text();
                            else if (RomInfo.tagName() == "internalidstring")
                                internalidstring = RomInfo.text();
                            else if (RomInfo.tagName() == "ecuid")
                                ecuid = RomInfo.text();
                            else if (RomInfo.tagName() == "year")
                                year = RomInfo.text();
                            else if (RomInfo.tagName() == "market")
                                market = RomInfo.text();
                            else if (RomInfo.tagName() == "make")
                                make = RomInfo.text();
                            else if (RomInfo.tagName() == "model")
                                model = RomInfo.text();
                            else if (RomInfo.tagName() == "submodel")
                                submodel = RomInfo.text();
                            else if (RomInfo.tagName() == "transmission")
                                transmission = RomInfo.text();
                            else if (RomInfo.tagName() == "memmodel")
                                memmodel = RomInfo.text();
                            else if (RomInfo.tagName() == "checksummodule")
                                checksummodule = RomInfo.text();
                            else if (RomInfo.tagName() == "flashmethod")
                                flashmethod = RomInfo.text();
                            else if (RomInfo.tagName() == "filesize")
                                filesize = RomInfo.text();

                            RomInfo = RomInfo.nextSibling().toElement();
                        }
                    }
                }

                if (xmlid == ecuId)
                {
                    dateTime = dateTime.currentDateTime();
                    dateTimeString = dateTime.toString("[yyyy-MM-dd hh':'mm':'ss'.'zzz']");
                    qDebug() << dateTimeString << "ECU ID found";

                    //qDebug() << "Correct ROM ID found =" << xmlid;
                    OemEcuDefFileFound = true;
                    ecuCalDef->RomInfo.replace(XmlId, xmlid);
                    ecuCalDef->RomInfo.replace(InternalIdAddress, internalidaddress);
                    ecuCalDef->RomInfo.replace(InternalIdString, internalidstring);
                    ecuCalDef->RomInfo.replace(EcuId, ecuid);
                    ecuCalDef->RomInfo.replace(Year, year);
                    ecuCalDef->RomInfo.replace(Market, market);
                    ecuCalDef->RomInfo.replace(Make, make);
                    ecuCalDef->RomInfo.replace(Model, model);
                    ecuCalDef->RomInfo.replace(SubModel, submodel);
                    ecuCalDef->RomInfo.replace(Transmission, transmission);
                    ecuCalDef->RomInfo.replace(MemModel, memmodel);
                    ecuCalDef->RomInfo.replace(ChecksumModule, checksummodule);
                    ecuCalDef->RomInfo.replace(FlashMethod, flashmethod);
                    ecuCalDef->RomInfo.replace(FileSize, filesize);
                    ecuCalDef->RomInfo.replace(RomBase, rombase);

                    ecuCalDef->RomBase = rombase;

                    xmlid = "";
                    QDomElement Table = TagType.firstChild().toElement();
                    while(!Table.isNull())
                    {
                        // Check if the child tag name is table
                        if (Table.tagName() == "table")
                        {
                            //qDebug() << Table.attribute("name"," ");
                            ecuCalDef->NameList.append(Table.attribute("name"," "));
                            ecuCalDef->AddressList.append(Table.attribute("storageaddress"," "));
                            ecuCalDef->XSizeList.append(Table.attribute("sizex"," "));
                            ecuCalDef->YSizeList.append(Table.attribute("sizey"," "));
                            index++;
                            QDomElement TableChild = Table.firstChild().toElement();
                            while(!TableChild.isNull())
                            {

                                if (TableChild.tagName() == "table")
                                {
                                    if (TableChild.attribute("type","No type") == "X Axis")
                                    {
                                        ContainsTableXScale = true;
                                        //ecuCalDef->XScaleNameList.append(TableChild.attribute("name"," "));
                                        ecuCalDef->XScaleAddressList.append(TableChild.attribute("storageaddress"," "));
                                    }
                                    if (TableChild.attribute("type","No type") == "Y Axis")
                                    {
                                        ContainsTableYScale = true;
                                        //ecuCalDef->YScaleNameList.append(TableChild.attribute("name"," "));
                                        ecuCalDef->YScaleAddressList.append(TableChild.attribute("storageaddress"," "));
                                    }
                                    if (TableChild.attribute("type","No type") == "Static Y Axis")
                                    {
                                        //ContainsTableYScale = true;
                                        //ecuCalDef->YScaleNameList.append(TableChild.attribute("name"," "));
                                        //ecuCalDef->YScaleAddressList.append(TableChild.attribute("storageaddress"," "));
                                    }
                                }

                                TableChild = TableChild.nextSibling().toElement();
                            }
                            if (!ContainsTableXScale){
                                //ecuCalDef->XScaleNameList.append(" ");
                                ecuCalDef->XScaleAddressList.append(" ");
                            }
                            if (!ContainsTableYScale){
                                //ecuCalDef->YScaleNameList.append(" ");
                                ecuCalDef->YScaleAddressList.append(" ");
                            }
                            ContainsTableXScale = false;
                            ContainsTableYScale = false;
                        }
                        Table = Table.nextSibling().toElement();
                    }
                }
            }
        }
        //TagType = TagType.nextSibling().toElement();
        if (OemEcuDefFileFound)
            break;
    }
    if (!OemEcuDefFileFound)
        return NULL;
*/
    //qDebug() << ecuCalDef->NameList.length();
    //qDebug() << ecuCalDef->AddressList.length();
    //qDebug() << ecuCalDef->XSizeList.length();
    //qDebug() << ecuCalDef->YSizeList.length();
    //qDebug() << ecuCalDef->XScaleAddressList.length();
    //qDebug() << ecuCalDef->YScaleAddressList.length();

    ecuCalDef->IdList.append(ecuCalDef->RomInfo.at(EcuId));
    dateTime = dateTime.currentDateTime();
    dateTimeString = dateTime.toString("[yyyy-MM-dd hh':'mm':'ss'.'zzz']");
    qDebug() << dateTimeString << "Read ECU base def";

    readEcuBaseDef(ecuCalDef);

    dateTime = dateTime.currentDateTime();
    dateTimeString = dateTime.toString("[yyyy-MM-dd hh':'mm':'ss'.'zzz']");
    qDebug() << dateTimeString << "ECU base def read";

    return ecuCalDef;

}

FileActions::EcuCalDefStructure *FileActions::openRomFile(FileActions::EcuCalDefStructure *ecuCalDef, QString filename)
{
    //EcuCalDefStructure *ecuCalDef = new EcuCalDefStructure;
    ConfigValuesStructure *configValues = &ConfigValuesStruct;

    QByteArray file_data;
    QString file_name_str;
    QString ecuId;
    bool ecuIdConfirmed = true;
    bool bStatus = false;
    uint16_t ecuIdAddr[] = { 0x0200, 0x2000 };

    //qDebug() << "Read OEM ECU file";
    if (!ecuCalDef->FullRomData.length())
    {
        //qDebug() << "Read file";
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

        //qDebug() << filename;
        QFile file(filename);
        QFileInfo fileInfo(file.fileName());
        file_name_str = fileInfo.fileName();

        if (!file.open(QIODevice::ReadOnly ))
        {
            QMessageBox::warning(this, tr("Calibration file"), "Unable to open calibration file for reading");
            return NULL;
        }
        file_data = file.readAll();
    }
    else
    {
        //qDebug() << "ROM file from ECU";
        QFile file(filename);
        QFileInfo fileInfo(file.fileName());
        file_name_str = fileInfo.fileName();
        file_data = ecuCalDef->FullRomData;
    }

    ecuIdConfirmed = true;
    for (unsigned j = 0; j < 2; j++)//sizeof(ecuIdAddr) / sizeof(ecuIdAddr[0]); j++)
    {
        //qDebug() << "ECU ID addr 0x" + QString("%1").arg(ecuIdAddr[j],4,16,QLatin1Char('0')).toUtf8();
        ecuIdConfirmed = true;
        ecuId.clear();
        for (int i = ecuIdAddr[j]; i < ecuIdAddr[j] + 8; i++)
        {
            if ((uint8_t)file_data.at(i) < 0x30 || (uint8_t)file_data.at(i) > 0x5A)
            {
                ecuIdConfirmed = false;
            }
            else
            {
                ecuId.append((uint8_t)file_data.at(i));
            }
        }
        if (ecuIdConfirmed)
            break;
    }

    if (!ecuIdConfirmed)
    {
        QMessageBox::warning(this, tr("Calibration file"), "Unable to find ID from selected ROM file, file might be from unsupported model!");
        return NULL;
    }
    if (!file_name_str.length())
        file_name_str = ecuId;// + ".bin";

    //qDebug() << ecuId;
    ecuCalDef = readEcuDef(ecuCalDef, ecuId);
    //qDebug() << "Returned from readOemEcuDefFile";

    if (ecuCalDef == NULL)
    {
        QMessageBox::warning(this, tr("Calibration file"), "Unable to find definition for selected ROM file with ECU ID: " + ecuId);
        return NULL;
    }

    ecuCalDef->OemEcuFile = true;
    ecuCalDef->FileName = file_name_str;
    ecuCalDef->FullFileName = filename;
    ecuCalDef->FullRomData.append(file_data);
    //ecuCalDef->FileSize = file.size();
    ecuCalDef->FileSize = file_data.size();
    ecuCalDef->RomId = ecuId;

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
        for (unsigned j = 0; j < ecuCalDef->XSizeList.at(i).toUInt() * ecuCalDef->YSizeList.at(i).toUInt(); j++)
        {
            uint32_t dataByte = 0;
            uint32_t byteAddress = ecuCalDef->AddressList.at(i).toUInt(&bStatus,16) + (j * storagesize);
            if (ecuCalDef->RomInfo.at(FlashMethod) == "wrx02" && ecuCalDef->FileSize < (170 * 1024) && byteAddress > 0x27FFF)
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
            /*
            QString mapFormat = ecuCalDef->FormatList.at(i);
            int format = 0;
            if (mapFormat.contains("0"))
                format = mapFormat.count(QLatin1Char('0')) - 1;
*/
            //mapData.append(QString::number(value, 'f', format) + ",");
            mapData.append(QString::number(value, 'g', float_precision) + ",");
        }
        ecuCalDef->MapData.insert(i, mapData);
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
                    if (ecuCalDef->RomInfo.at(FlashMethod) == "wrx02" && ecuCalDef->FileSize < (170 * 1024) && byteAddress > 0x27FFF)
                        byteAddress -= 0x8000;
                    for (int k = 0; k < storagesize; k++)
                    {
                        if (ecuCalDef->XScaleEndianList.at(i) == "little" || ecuCalDef->StorageTypeList.at(i) == "float")
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
                    /*
                    QString mapFormat = ecuCalDef->XScaleFormatList.at(i);
                    int format = 0;
                    if (mapFormat.contains("0"))
                        format = mapFormat.count(QLatin1Char('0')) - 1;
*/
                    mapData.append(QString::number(value, 'g', float_precision) + ",");
                    //mapData.append(QString::number(value, 'f', format) + ",");
                }
                ecuCalDef->XScaleData.insert(i, mapData);
            }
        }
        else
            ecuCalDef->XScaleData.insert(i, " ");
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
                if (ecuCalDef->RomInfo.at(FlashMethod) == "wrx02" && ecuCalDef->FileSize < (170 * 1024) && byteAddress > 0x27FFF)
                    byteAddress -= 0x8000;
                for (int k = 0; k < storagesize; k++)
                {
                    if (ecuCalDef->YScaleEndianList.at(i) == "little" || ecuCalDef->StorageTypeList.at(i) == "float")
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
                /*
                QString mapFormat = ecuCalDef->YScaleFormatList.at(i);
                int format = 0;
                if (mapFormat.contains("0"))
                    format = mapFormat.count(QLatin1Char('0')) - 1;
*/
                mapData.append(QString::number(value, 'g', float_precision) + ",");
                //mapData.append(QString::number(value, 'f', format) + ",");
            }
            ecuCalDef->YScaleData.insert(i, mapData);
        }
        else
            ecuCalDef->YScaleData.insert(i, " ");

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

FileActions::EcuCalDefStructure *FileActions::saveRomFile(FileActions::EcuCalDefStructure *ecuCalDef, QString filename)
{
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly ))
    {
        qDebug() << "Unable to open file for writing";
        QMessageBox::warning(this, tr("Ecu calibration file"), "Unable to open file for writing");
        return NULL;
    }

    ecuCalDef = apply_cal_changes_to_rom_data(ecuCalDef);
    file.write(ecuCalDef->FullRomData);
    file.close();

    return 0;
}

FileActions::EcuCalDefStructure *FileActions::apply_cal_changes_to_rom_data(FileActions::EcuCalDefStructure *ecuCalDef)
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
        for (unsigned j = 0; j < ecuCalDef->XSizeList.at(i).toUInt() * ecuCalDef->YSizeList.at(i).toUInt(); j++)
        {
            uint32_t dataByte = 0;
            uint32_t byteAddress = ecuCalDef->AddressList.at(i).toUInt(&bStatus,16) + (j * storagesize);
            if (ecuCalDef->RomInfo.at(FlashMethod) == "wrx02" && ecuCalDef->FileSize < (170 * 1024) && byteAddress > 0x27FFF)
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
                if (ecuCalDef->NameList.at(i) == "Min Idle Speed High Electrical Load (AT)")
                    qDebug() << mapDataList.at(j) << mapDataValue.floatValue << mapDataValue.fourByteValue;
                if (ecuCalDef->NameList.at(i) == "Min Idle Speed High Electrical Load (MT)")
                    qDebug() << mapDataList.at(j) << mapDataValue.floatValue << mapDataValue.fourByteValue;

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
                if (ecuCalDef->RomInfo.at(FlashMethod) == "wrx02" && ecuCalDef->FileSize < (170 * 1024) && byteAddress > 0x27FFF)
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
                        //mapDataValue.fourByteValue = (qRound(mapDataValue.floatValue));
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
                if (ecuCalDef->RomInfo.at(FlashMethod) == "wrx02" && ecuCalDef->FileSize < (170 * 1024) && byteAddress > 0x27FFF)
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
                        //mapDataValue.fourByteValue = (qRound(mapDataValue.floatValue));
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
