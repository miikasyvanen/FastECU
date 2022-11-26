#include "file_actions.h"

FileActions::EcuCalDefStructure *FileActions::read_romraider_ecu_base_def(EcuCalDefStructure *ecuCalDef)
{
    ConfigValuesStructure *configValues = &ConfigValuesStruct;

    bool OemEcuDefBaseFileFound = false;

    QString rombase;
    QString xmlid;

    //The QDomDocument class represents an XML document.
    QDomDocument xmlBOM;
    // Load xml file as raw data
    //QString filename = configValues->definitionsDir + "/" + ecuCalDef->RomBase + ".xml";
    QString filename = configValues->romraider_definition_files.at(0);

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
            //qDebug() << "BASE" << ecuCalDef->RomInfo[RomBase];
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
                    qDebug() << "Reading BASE" << ecuCalDef->RomInfo[RomBase];

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
                                    ecuCalDef->TypeList.replace(i, Table.attribute("type"," "));
                                    ecuCalDef->CategoryList.replace(i, Table.attribute("category"," "));

                                    //qDebug() << ecuCalDef->NameList.at(i) << ecuCalDef->CategoryList.at(i);

                                    if (ecuCalDef->XSizeList.at(i) == "" || ecuCalDef->XSizeList.at(i) == " ")
                                        ecuCalDef->XSizeList.replace(i, Table.attribute("sizex", "1"));
                                    if (ecuCalDef->YSizeList.at(i) == "" || ecuCalDef->YSizeList.at(i) == " ")
                                        ecuCalDef->YSizeList.replace(i, Table.attribute("sizey", "1"));
                                    ecuCalDef->MinValueList.replace(i, Table.attribute("minvalue","0"));
                                    ecuCalDef->MaxValueList.replace(i, Table.attribute("maxvalue","0"));

                                    ecuCalDef->StorageTypeList.replace(i, Table.attribute("storagetype"," "));
                                    ecuCalDef->EndianList.replace(i, Table.attribute("endian"," "));
                                    ecuCalDef->LogParamList.replace(i, Table.attribute("logparam"," "));

                                    ecuCalDef->SyncedWithEcu = true;

                                    // Get the first child of the Table
                                    QDomElement TableChild = Table.firstChild().toElement();
                                    QString TableSelections;
                                    QString TableSelectionsSorted;
                                    QString TableDescription;
                                    QString TableStates;

                                    // Read each child of the Table node
                                    while (!TableChild.isNull())
                                    {
                                        if (TableChild.tagName() == "scaling"){
                                            ecuCalDef->UnitsList.replace(i, TableChild.attribute("units"," "));
                                            ecuCalDef->FormatList.replace(i, TableChild.attribute("format"," "));
                                            ecuCalDef->FineIncList.replace(i, TableChild.attribute("fineincrement"," "));
                                            ecuCalDef->CoarseIncList.replace(i, TableChild.attribute("coarseincrement"," "));

                                            ecuCalDef->FromByteList.replace(i, TableChild.attribute("expression","x"));
                                            ecuCalDef->ToByteList.replace(i, TableChild.attribute("to_byte","x"));

                                        }
                                        else if (TableChild.tagName() == "table")
                                        {
                                            QString ScaleType = TableChild.attribute("type"," ");
                                            if (ScaleType == "X Axis"){
                                                ecuCalDef->XScaleNameList.replace(i, TableChild.attribute("name"," "));
                                                ecuCalDef->XScaleTypeList.replace(i, ScaleType);
                                                ecuCalDef->XScaleStorageTypeList.replace(i, TableChild.attribute("storagetype"," "));
                                                ecuCalDef->XScaleEndianList.replace(i, TableChild.attribute("endian"," "));
                                                ecuCalDef->XScaleLogParamList.replace(i, TableChild.attribute("logparam"," "));

                                                QDomElement SubChild = TableChild.firstChild().toElement();
                                                while (!SubChild.isNull())
                                                {
                                                    if (SubChild.tagName() == "scaling"){
                                                        ecuCalDef->XScaleUnitsList.replace(i, SubChild.attribute("units"," "));
                                                        ecuCalDef->XScaleFormatList.replace(i, SubChild.attribute("format"," "));
                                                        ecuCalDef->XScaleFineIncList.replace(i, SubChild.attribute("fineincrement"," "));
                                                        ecuCalDef->XScaleCoarseIncList.replace(i, SubChild.attribute("coarseincrement"," "));

                                                        ecuCalDef->XScaleFromByteList.replace(i, SubChild.attribute("expression","x"));
                                                        ecuCalDef->XScaleToByteList.replace(i, SubChild.attribute("to_byte","x"));

                                                        SubChild = SubChild.nextSibling().toElement();
                                                    }
                                                }
                                            }
                                            else if (ScaleType == "Y Axis" && ecuCalDef->TypeList.at(i) == "3D"){
                                                ecuCalDef->YScaleNameList.replace(i, TableChild.attribute("name"," "));
                                                ecuCalDef->YScaleTypeList.replace(i, ScaleType);
                                                ecuCalDef->YScaleStorageTypeList.replace(i, TableChild.attribute("storagetype"," "));
                                                ecuCalDef->YScaleEndianList.replace(i, TableChild.attribute("endian"," "));
                                                ecuCalDef->YScaleLogParamList.replace(i, TableChild.attribute("logparam"," "));
                                                ecuCalDef->XScaleStaticDataList.append(" ");

                                                QDomElement SubChild = TableChild.firstChild().toElement();
                                                while (!SubChild.isNull())
                                                {
                                                    if (SubChild.tagName() == "scaling"){
                                                        ecuCalDef->YScaleUnitsList.replace(i, SubChild.attribute("units"," "));
                                                        ecuCalDef->YScaleFormatList.replace(i, SubChild.attribute("format"," "));
                                                        ecuCalDef->YScaleFineIncList.replace(i, SubChild.attribute("fineincrement"," "));
                                                        ecuCalDef->YScaleCoarseIncList.replace(i, SubChild.attribute("coarseincrement"," "));

                                                        ecuCalDef->YScaleFromByteList.replace(i, SubChild.attribute("expression","x"));
                                                        ecuCalDef->YScaleToByteList.replace(i, SubChild.attribute("to_byte","x"));

                                                        SubChild = SubChild.nextSibling().toElement();
                                                    }
                                                }
                                            }
                                            else if (ScaleType == "Static Y Axis" || (ScaleType == "Y Axis" && ecuCalDef->TypeList.at(i) == "2D"))
                                            {
                                                ecuCalDef->XScaleNameList.replace(i, TableChild.attribute("name"," "));
                                                ecuCalDef->XScaleTypeList.replace(i, ScaleType);
                                                ecuCalDef->XScaleStorageTypeList.replace(i, TableChild.attribute("storagetype"," "));
                                                ecuCalDef->XScaleEndianList.replace(i, TableChild.attribute("endian"," "));
                                                ecuCalDef->XScaleLogParamList.replace(i, TableChild.attribute("logparam"," "));

                                                QDomElement SubChild = TableChild.firstChild().toElement();
                                                QString StaticYScaleData;
                                                if (SubChild.tagName() == "scaling")
                                                {
                                                    ecuCalDef->XScaleUnitsList.replace(i, SubChild.attribute("units"," "));
                                                    ecuCalDef->XScaleFormatList.replace(i, SubChild.attribute("format"," "));
                                                    ecuCalDef->XScaleFineIncList.replace(i, SubChild.attribute("fineincrement"," "));
                                                    ecuCalDef->XScaleCoarseIncList.replace(i, SubChild.attribute("coarseincrement"," "));

                                                    ecuCalDef->XScaleFromByteList.replace(i, SubChild.attribute("expression","x"));
                                                    ecuCalDef->XScaleToByteList.replace(i, SubChild.attribute("to_byte","x"));
                                                    ecuCalDef->XScaleStaticDataList.replace(i, " ");

                                                }
                                                if (SubChild.tagName() == "data"){
                                                    while (!SubChild.isNull())
                                                    {
                                                        if (SubChild.tagName() == "data"){
                                                            StaticYScaleData.append(SubChild.text());
                                                            StaticYScaleData.append(",");
                                                        }
                                                        SubChild = SubChild.nextSibling().toElement();
                                                    }
                                                    ecuCalDef->XScaleUnitsList.replace(i, SubChild.attribute("units"," "));
                                                    ecuCalDef->XScaleFormatList.replace(i, SubChild.attribute("format"," "));
                                                    ecuCalDef->XScaleFineIncList.replace(i, SubChild.attribute("fineincrement"," "));
                                                    ecuCalDef->XScaleCoarseIncList.replace(i, SubChild.attribute("coarseincrement"," "));

                                                    ecuCalDef->XScaleFromByteList.replace(i, SubChild.attribute("expression"," "));
                                                    ecuCalDef->XScaleToByteList.replace(i, SubChild.attribute("to_byte"," "));
                                                    ecuCalDef->XScaleStaticDataList.replace(i, StaticYScaleData);
                                                }
                                                ecuCalDef->XSizeList.replace(i, ecuCalDef->YSizeList[i]);
                                                ecuCalDef->YSizeList[i] = "1";
                                                ecuCalDef->XScaleAddressList.replace(i, ecuCalDef->YScaleAddressList[i]);
                                                ecuCalDef->YScaleAddressList[i] = " ";
                                            }
                                        }
                                        else if (TableChild.tagName() == "description"){
                                            TableDescription = TableChild.text();
                                            if (!TableDescription.isEmpty())
                                                ecuCalDef->DescriptionList.replace(i, "\n\n" + TableDescription);
                                            else
                                                ecuCalDef->DescriptionList.replace(i, " ");
                                        }
                                        else if (TableChild.tagName() == "state"){
                                            TableStates.append(TableChild.attribute("name"," ") + ",");
                                            TableStates.append(TableChild.attribute("data"," ") + ",");

                                        }
                                        else
                                        {
                                            ecuCalDef->UnitsList.replace(i, TableChild.attribute("units"," "));
                                            ecuCalDef->FormatList.replace(i, TableChild.attribute("format"," "));
                                            ecuCalDef->FineIncList.replace(i, TableChild.attribute("fineincrement"," "));
                                            ecuCalDef->CoarseIncList.replace(i, TableChild.attribute("coarseincrement"," "));

                                            ecuCalDef->FromByteList.replace(i, TableChild.attribute("expression","x"));
                                            ecuCalDef->ToByteList.replace(i, TableChild.attribute("to_byte","x"));
                                        }
                                        // Next child
                                        TableChild = TableChild.nextSibling().toElement();
                                    }
                                    ecuCalDef->StateList.replace(i, TableStates);

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

FileActions::EcuCalDefStructure *FileActions::read_romraider_ecu_def(EcuCalDefStructure *ecuCalDef, QString ecuId)
{
    ConfigValuesStructure *configValues = &ConfigValuesStruct;

    bool inherits_another_def = false;
    bool ecuid_def_found = false;
    bool contains_x_scale = false;
    bool contains_y_scale = false;

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

    // Check if any ECU definition file is selected
    if (!configValues->romraider_definition_files.length() && !configValues->ecuflash_definition_files_directory.length()) {
        QMessageBox::warning(this, tr("Ecu definition file"), "No RomRaider definition file(s), use definition manager at 'Edit' menu to choose file(s)");
        ecuCalDef = NULL;
        return NULL;
    }

    while (ecuCalDef->RomInfo.length() < RomInfoStrings.length())
    {
        ecuCalDef->RomInfo.append(" ");
    }

    for (int i = 0; i < configValues->romraider_definition_files.length(); i++)
    {
        QString filename = configValues->romraider_definition_files.at(0);
        QFile file(filename);

        if (!file.open(QIODevice::ReadOnly ))
        {
            ecuCalDef = NULL;
            //qDebug() << "Unable to open ECU definition file " + filename + " for reading");
            QMessageBox::warning(this, tr("Ecu definitions file"), "Unable to open ECU definition file " + filename + " for reading");
            return NULL;
        }

        QXmlStreamReader reader;
        reader.setDevice(&file);

        int index = 0;

        if (reader.readNextStartElement())
        {
            if (reader.name() == "roms")
            {
                //qDebug() << "<roms> TAG";
                while (reader.readNextStartElement())
                {
                    if (reader.name() == "rom")
                    {
                        if (reader.attributes().value("base").toString() != "")
                            rombase = reader.attributes().value("base").toString();
                        if (rombase.contains("BASE"))
                            inherits_another_def = false;
                        else
                            inherits_another_def = true;
                        //qDebug() << "Is NOT BASE def" << inherits_another_def << rombase;

                        //qDebug() << "<rom> TAG";
                        while (reader.readNextStartElement())
                        {
                            if (reader.name() == "romid" && rombase != "")
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
                                /*
                                if (xmlid == ecuId)
                                    qDebug() << "ECU ID" << xmlid << "found";
                                if (inherits_another_def)
                                    qDebug() << xmlid << "inherits from definition" << rombase;
                                    */
                                if (xmlid == ecuId)
                                {
                                    if (ecuCalDef->RomInfo.at(XmlId) == " ")
                                    {
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
                                    }

                                    qDebug() << "inherits_another_def" << inherits_another_def << rombase;
                                    if (inherits_another_def)
                                        read_romraider_ecu_def(ecuCalDef, rombase);
                                        //read_romraider_ecu_definition_file(ecuCalDef, rombase);

                                    if (!inherits_another_def)
                                    {
                                        ecuCalDef->RomInfo.replace(RomBase, rombase);
                                        ecuCalDef->RomBase = rombase;
                                    }
                                }
                            }
                            else if (reader.name() == "table" && xmlid == ecuId)
                            {
                                //qDebug() << "ECU ID" << ecuId << "found";
                                ecuid_def_found = true;

                                //qDebug() << reader.attributes().value("name").toString() << reader.attributes().value("storageaddress").toString();

                                add_romraider_def_list_item(ecuCalDef);

                                ecuCalDef->NameList.replace(index, reader.attributes().value("name").toString());
                                ecuCalDef->AddressList.replace(index, reader.attributes().value("storageaddress").toString());
                                ecuCalDef->XSizeList.replace(index, reader.attributes().value("sizex").toString());
                                ecuCalDef->YSizeList.replace(index, reader.attributes().value("sizey").toString());

                                while (reader.readNextStartElement())
                                {
                                    if (reader.name() == "table")
                                    {
                                        //qDebug() << reader.attributes().value("type").toString() << reader.attributes().value("storageaddress").toString();
                                        if (reader.attributes().value("type").toString() == "X Axis")
                                        {
                                            //contains_x_scale = true;
                                            ecuCalDef->XScaleAddressList.replace(index, reader.attributes().value("storageaddress").toString());
                                            qDebug() << "Table:" << ecuCalDef->NameList.at(index) << "X Axis address:" << ecuCalDef->XScaleAddressList.at(index);
                                        }
                                        if (reader.attributes().value("type").toString() == "Y Axis")
                                        {
                                            //contains_y_scale = true;
                                            ecuCalDef->YScaleAddressList.replace(index, reader.attributes().value("storageaddress").toString());
                                            qDebug() << "Table:" << ecuCalDef->NameList.at(index) << "Y Axis address:" << ecuCalDef->YScaleAddressList.at(index);
                                        }
                                        if (reader.attributes().value("type").toString() == "Static Y Axis")
                                        {

                                        }
                                        //else
                                            reader.skipCurrentElement();
                                    }
                                    else
                                        reader.skipCurrentElement();
                                }
                                index++;
                            }
                            else
                                reader.skipCurrentElement();
                        }
                        if (!inherits_another_def && ecuid_def_found)
                        {
                            qDebug() << "Check inherits_another_def" << inherits_another_def;
                            qDebug() << "ECU ID" << ecuCalDef->RomInfo.at(EcuId) << "found and def read, move to ECU BASE def" << ecuCalDef->RomBase;
                            read_romraider_ecu_base_def(ecuCalDef);
                        }
                        ecuid_def_found = false;

                    }
                    else
                        reader.skipCurrentElement();
                }
            }
        }
        file.close();

        ecuCalDef->IdList.append(ecuCalDef->RomInfo.at(EcuId));
    }

    dateTime = dateTime.currentDateTime();
    dateTimeString = dateTime.toString("[yyyy-MM-dd hh':'mm':'ss'.'zzz']");
    qDebug() << dateTimeString << "Read ECU base def";

    ecuCalDef->use_romraider_definition = true;

    return ecuCalDef;
}

FileActions::EcuCalDefStructure *FileActions::add_romraider_def_list_item(EcuCalDefStructure *ecuCalDef)
{
    ecuCalDef->IdList.append(" ");
    ecuCalDef->TypeList.append(" ");
    ecuCalDef->NameList.append(" ");
    ecuCalDef->AddressList.append(" ");
    ecuCalDef->CategoryList.append(" ");
    ecuCalDef->CategoryExpandedList.append(" ");
    ecuCalDef->XSizeList.append(" ");
    ecuCalDef->YSizeList.append(" ");
    ecuCalDef->MinValueList.append(" ");
    ecuCalDef->MaxValueList.append(" ");
    ecuCalDef->UnitsList.append(" ");
    ecuCalDef->FormatList.append(" ");
    ecuCalDef->FineIncList.append(" ");
    ecuCalDef->CoarseIncList.append(" ");
    ecuCalDef->VisibleList.append(" ");
    ecuCalDef->SelectionsNameList.append(" ");
    ecuCalDef->SelectionsValueList.append(" ");
    ecuCalDef->DescriptionList.append(" ");
    ecuCalDef->StateList.append(" ");
    ecuCalDef->MapData.append(" ");

    ecuCalDef->XScaleTypeList.append(" ");
    ecuCalDef->XScaleNameList.append(" ");
    ecuCalDef->XScaleAddressList.append(" ");
    ecuCalDef->XScaleMinValueList.append(" ");
    ecuCalDef->XScaleMaxValueList.append(" ");
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
    ecuCalDef->XScaleData.append(" ");

    ecuCalDef->YScaleTypeList.append(" ");
    ecuCalDef->YScaleNameList.append(" ");
    ecuCalDef->YScaleAddressList.append(" ");
    ecuCalDef->YScaleMinValueList.append(" ");
    ecuCalDef->YScaleMaxValueList.append(" ");
    ecuCalDef->YScaleUnitsList.append(" ");
    ecuCalDef->YScaleFormatList.append(" ");
    ecuCalDef->YScaleFineIncList.append(" ");
    ecuCalDef->YScaleCoarseIncList.append(" ");
    ecuCalDef->YScaleStorageTypeList.append(" ");
    ecuCalDef->YScaleEndianList.append(" ");
    ecuCalDef->YScaleLogParamList.append(" ");
    ecuCalDef->YScaleFromByteList.append(" ");
    ecuCalDef->YScaleToByteList.append(" ");
    ecuCalDef->YScaleData.append(" ");

    ecuCalDef->StorageTypeList.append(" ");
    ecuCalDef->EndianList.append(" ");
    ecuCalDef->LogParamList.append(" ");
    ecuCalDef->FromByteList.append(" ");
    ecuCalDef->ToByteList.append(" ");

    return ecuCalDef;
}

