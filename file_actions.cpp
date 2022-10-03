#include "file_actions.h"

FileActions::FileActions()
{

}

FileActions::ConfigValuesStructure *FileActions::checkConfigDir(){
    ConfigValuesStructure *configValues = &ConfigValuesStruct;

    //qDebug() << "Checking if dir" << configValues->baseDir << "exists";
    if (!QDir(configValues->base_directory).exists()){
        //qDebug() << "Create FastECU base directory";
        QDir().mkdir(configValues->base_directory);
    }

    //qDebug() << "Checking if dir" << configValues->calibrationsDir << "exists";
    if (!QDir(configValues->calibration_files_base_directory).exists()){
        //qDebug() << "Create calibrations directory";
        QDir().mkdir(configValues->calibration_files_base_directory);
        QDir dir("calibrations/");
        //qDebug() << "Copying files to directory";
        foreach (const QFileInfo& entry, dir.entryInfoList((QStringList() << "*.*", QDir::Files))){
            //qDebug() << entry.fileName();
            QFile().copy("calibrations/" + entry.fileName(), configValues->calibration_files_base_directory + "/" + entry.fileName());
        }
    }
    //qDebug() << "Checking if dir" << configValues->configDir << "exists";
    if (!QDir(configValues->config_base_directory).exists()){
        //qDebug() << "Create config directory";
        QDir().mkdir(configValues->config_base_directory);
        QDir dir("config/");
        //qDebug() << "Copying files to directory";
        foreach (const QFileInfo& entry, dir.entryInfoList((QStringList() << "*.*", QDir::Files))){
            //qDebug() << entry.fileName();
            QFile().copy("config/" + entry.fileName(), configValues->config_base_directory + "/" + entry.fileName());
        }
    }
    //qDebug() << "Checking if dir" << configValues->definitionsDir << "exists";
    if (!QDir(configValues->definition_files_base_directory).exists()){
        //qDebug() << "Create definitions directory";
        QDir().mkdir(configValues->definition_files_base_directory);
        QDir dir("definitions/");
        //qDebug() << "Copying files to directory";
        foreach (const QFileInfo& entry, dir.entryInfoList((QStringList() << "*.*", QDir::Files))){
            //qDebug() << entry.fileName();
            QFile().copy("definitions/" + entry.fileName(), configValues->definition_files_base_directory + "/" + entry.fileName());
        }
    }
    //qDebug() << "Checking if dir" << configValues->kernelsDir << "exists";
    if (!QDir(configValues->kernel_files_base_directory).exists()){
        //qDebug() << "Create kernels directory";
        QDir().mkdir(configValues->kernel_files_base_directory);
        QDir dir("kernels/");
        //qDebug() << "Copying files to directory";
        foreach (const QFileInfo& entry, dir.entryInfoList((QStringList() << "*.*", QDir::Files))){
            //qDebug() << entry.fileName();
            QFile().copy("kernels/" + entry.fileName(), configValues->kernel_files_base_directory + "/" + entry.fileName());
        }
    }
    //qDebug() << "Checking if dir" << configValues->logsDir << "exists";
    if (!QDir(configValues->log_files_base_directory).exists()){
        //qDebug() << "Create logs directory";
        QDir().mkdir(configValues->log_files_base_directory);
    }
    //qDebug() << "Directories checked";

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
                    //qDebug() << "Software settings";
                    while (reader.readNextStartElement())
                    {
                        if (reader.name() == "setting")
                        {
                            if (reader.attributes().value("name") == "serial_port"){
                                //qDebug() << "Serial port";
                                while(reader.readNextStartElement())
                                {
                                    configValues->serial_port = reader.attributes().value("data").toString();
                                    QString s = reader.readElementText();
                                }
                            }
                            if (reader.attributes().value("name") == "flash_method"){
                                //qDebug() << "Flash method";
                                while(reader.readNextStartElement())
                                {
                                    configValues->flash_method = reader.attributes().value("data").toString();
                                    QString s = reader.readElementText();
                                }
                            }
                            if (reader.attributes().value("name") == "calibration_files"){
                                //qDebug() << "Calibration files";
                                while(reader.readNextStartElement())
                                {
                                    //qDebug() << "Cal files:" << reader.name() << reader.attributes().value("data");
                                    configValues->calibration_files.append(reader.attributes().value("data").toString());
                                    QString s = reader.readElementText();
                                }
                            }
                            if (reader.attributes().value("name") == "calibration_files_directory"){
                                //qDebug() << "Calibration files directory";
                                while(reader.readNextStartElement())
                                {
                                    //qDebug() << "Calibration files directory:" << reader.name() << reader.attributes().value("data");
                                    configValues->calibration_files_directory = reader.attributes().value("data").toString();
                                    QString s = reader.readElementText();
                                }

                            }
                            if (reader.attributes().value("name") == "ecu_definition_files"){
                                //qDebug() << "ECU definition files";
                                while(reader.readNextStartElement())
                                {
                                    //qDebug() << "ECU definition files:" << reader.name() << reader.attributes().value("data");
                                    configValues->ecu_definition_files.append(reader.attributes().value("data").toString());
                                    QString s = reader.readElementText();
                                }
                            }
                            if (reader.attributes().value("name") == "logger_definition_file"){
                                //qDebug() << "Logger definition files";
                                while(reader.readNextStartElement())
                                {
                                    //qDebug() << "Logger definition files:" << reader.name() << reader.attributes().value("data");
                                    configValues->logger_definition_file = reader.attributes().value("data").toString();
                                    QString s = reader.readElementText();
                                }
                            }
                            if (reader.attributes().value("name") == "kernel_files_directory"){
                                //qDebug() << "Kernel files directory";
                                while(reader.readNextStartElement())
                                {
                                    //qDebug() << "Kernel files directory:" << reader.name() << reader.attributes().value("data");
                                    configValues->kernel_files_directory = reader.attributes().value("data").toString();
                                    QString s = reader.readElementText();
                                }

                            }
                            if (reader.attributes().value("name") == "logfiles_directory"){
                                //qDebug() << "Logfiles directory";
                                while(reader.readNextStartElement())
                                {
                                    //qDebug() << "Logfiles directory:" << reader.name() << reader.attributes().value("data");
                                    configValues->log_files_base_directory = reader.attributes().value("data").toString();
                                    QString s = reader.readElementText();
                                }

                            }
                        }
                    }
                }
            }
        }
    }

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
                                        //qDebug() << "Parameter =" << parameter.attribute("id","No id") << parameter.attribute("name","No name") << parameter.attribute("desc","No description");
                                        logValues->log_value_name.append(parameter.attribute("name","No name"));
                                        QDomElement param_options = parameter.firstChild().toElement();
                                        while (!param_options.isNull())
                                        {
                                            if (param_options.tagName() == "address")
                                            {
                                                //qDebug() << "Address =" << param_options.text();
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
                                        logValues->log_value.append("0.00");
                                        if (log_value_index < 12)
                                            logValues->lower_panel_log_value_id.append(QString::number(log_value_index));
                                        else
                                            logValues->lower_panel_log_value_id.append("-1");
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

/****************************************************************
 *
 *  OEM ECU Fileactions area
 *
 *
 *
 *
 *
 *
 ****************************************************************/
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

                                    if (ecuCalDef->XSizeList.at(i) == " ")
                                        ecuCalDef->XSizeList.replace(i, Table.attribute("sizex", "1"));
                                    if (ecuCalDef->YSizeList.at(i) == " ")
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
                                    bool ContainsTableSelections = false;
                                    bool ContainsTableDescription = false;
                                    bool ContainsTableXScale = false;
                                    bool ContainsTableYScale = false;
                                    bool ContainsTableStaticYScale = false;

                                    // Read each child of the Table node
                                    while (!TableChild.isNull())
                                    {
                                        if (TableChild.tagName() == "scaling"){
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
                                                ecuCalDef->XScaleNameList.append(TableChild.attribute("name"," "));
                                                ecuCalDef->XScaleTypeList.append(ScaleType);
                                                //ecuCalDef->XScaleAddressList.append(TableChild.attribute("storageaddress"," "));
                                                ecuCalDef->XScaleStorageTypeList.append(TableChild.attribute("storagetype"," "));
                                                ecuCalDef->XScaleEndianList.append(TableChild.attribute("endian"," "));
                                                ecuCalDef->XScaleLogParamList.append(TableChild.attribute("logparam"," "));

                                                //qDebug() << TableChild.attribute("name"," ");

                                                ContainsTableXScale = true;
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
                                                ecuCalDef->YScaleNameList.append(TableChild.attribute("name"," "));
                                                ecuCalDef->YScaleTypeList.append(ScaleType);
                                                //ecuCalDef->YScaleAddressList.append(TableChild.attribute("storageaddress"," "));
                                                ecuCalDef->YScaleStorageTypeList.append(TableChild.attribute("storagetype"," "));
                                                ecuCalDef->YScaleEndianList.append(TableChild.attribute("endian"," "));
                                                ecuCalDef->YScaleLogParamList.append(TableChild.attribute("logparam"," "));
                                                ecuCalDef->XScaleStaticDataList.append(" ");

                                                //qDebug() << TableChild.attribute("name"," ");

                                                ContainsTableYScale = true;
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

                                                //qDebug() << TableChild.attribute("name"," ");

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
                                    ContainsTableSelections = false;
                                    if (!ContainsTableDescription)
                                        ecuCalDef->DescriptionList.append(" ");
                                    ContainsTableDescription = false;
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

    //EcuCalDefStructure *ecuCalDef = new EcuCalDefStructure;
    ConfigValuesStructure *configValues = &ConfigValuesStruct;

    //The QDomDocument class represents an XML document.
    QDomDocument xmlBOM;
    // Load xml file as raw data
    QString filename = configValues->ecu_definition_files.at(0);
    QFile file(filename);

    if (!file.open(QIODevice::ReadOnly ))
    {
        ecuCalDef = NULL;
        qDebug() << "Unable to open OEM ecu definition file for reading";
        //QMessageBox::warning(this, tr("Ecu definitions file"), "Unable to open OEM ecu definition file for reading");
        return NULL;
    }

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

    ecuCalDef->IdList.append(ecuCalDef->RomInfo.at(EcuId));

    readEcuBaseDef(ecuCalDef);

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
            if (ecuCalDef->RomInfo.at(FlashMethod) == "wrx02" && ecuCalDef->FileSize < (170 * 1024) && byteAddress > 0x1FFFF)
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
                    if (ecuCalDef->RomInfo.at(FlashMethod) == "wrx02" && ecuCalDef->FileSize < (170 * 1024) && byteAddress > 0x1FFFF)
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
                    mapData.append(QString::number(value, 'g', float_precision) + ",");
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
                if (ecuCalDef->RomInfo.at(FlashMethod) == "wrx02" && ecuCalDef->FileSize < (170 * 1024) && byteAddress > 0x1FFFF)
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
                mapData.append(QString::number(value, 'g', float_precision) + ",");
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
    //qDebug() << "Saving file" << filename;

    //ConfigValuesStructure *configValues = &ConfigValuesStruct;

    bool bStatus = false;

    //qDebug() << "Checking file name";

    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly ))
    {
        qDebug() << "Unable to open file for writing";
        QMessageBox::warning(this, tr("Ecu calibration file"), "Unable to open file for writing");
        return NULL;
    }

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
            if (ecuCalDef->RomInfo.at(FlashMethod) == "wrx02" && ecuCalDef->FileSize < (170 * 1024) && byteAddress > 0x1FFFF)
                byteAddress -= 0x8000;

            if (ecuCalDef->TypeList.at(i) != "Switch")
            {
                if (ecuCalDef->StorageTypeList.at(i) == "float"){
                    //QStringList expression = parse_stringlist_from_expression_string(ecuCalDef->ToByteList.at(i), mapDataList.at(j));
                    //if (ecuCalDef->NameList.at(i) == "Extended Feedback Correction High RPM Compensation")
                        //qDebug() << expression;
                    mapDataValue.floatValue = calculate_value_from_expression(parse_stringlist_from_expression_string(ecuCalDef->ToByteList.at(i), mapDataList.at(j)));
                }
                else{
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
                if (ecuCalDef->RomInfo.at(FlashMethod) == "wrx02" && ecuCalDef->FileSize < (170 * 1024) && byteAddress > 0x1FFFF)
                    byteAddress -= 0x8000;

                if (ecuCalDef->XScaleTypeList.at(i) != "Switch")
                {
                    if (ecuCalDef->XScaleStorageTypeList.at(i) == "float"){
                        mapDataValue.floatValue = calculate_value_from_expression(parse_stringlist_from_expression_string(ecuCalDef->XScaleToByteList.at(i), mapDataList.at(j)));
                    }
                    else{
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
                if (ecuCalDef->RomInfo.at(FlashMethod) == "wrx02" && ecuCalDef->FileSize < (170 * 1024) && byteAddress > 0x1FFFF)
                    byteAddress -= 0x8000;

                if (ecuCalDef->YScaleTypeList.at(i) != "Switch")
                {
                    if (ecuCalDef->YScaleStorageTypeList.at(i) == "float"){
                        mapDataValue.floatValue = calculate_value_from_expression(parse_stringlist_from_expression_string(ecuCalDef->YScaleToByteList.at(i), mapDataList.at(j)));
                    }
                    else{
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

    file.write(ecuCalDef->FullRomData);
    file.close();

    return 0;
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
            //qDebug() << Qt::hex << (uint8_t)(filedata[i + j]) << Qt::hex << (uint8_t)(filedata[i + 4 + j]) << Qt::hex << (uint8_t)(filedata[i + 8 + j]);
        }
        //qDebug() << "Checksum dword addr lo:" << Qt::hex << checksum_dword_addr_lo;
        //qDebug() << "Checksum dword addr hi:" << Qt::hex << checksum_dword_addr_hi;
        //qDebug() << "Checksum read diff:" << Qt::hex << checksum_diff;
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
        //qDebug() << "Checksum calc diff:" << Qt::hex << checksum_check;
        //qDebug() << "Checksum:" << Qt::hex << checksum;

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
        ecuCalDef->FullRomData.replace(checksum_area_start, checksum_area_length, checksum_array);

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
