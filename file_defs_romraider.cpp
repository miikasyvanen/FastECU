#include "file_actions.h"

FileActions::ConfigValuesStructure *FileActions::create_romraider_def_id_list(ConfigValuesStructure *configValues)
{
    QString filename;

    int file_count = 0;
    int cal_id_count = 0;

    bool cal_id_found = false;
    bool cal_id_addr_found = false;
    bool ecu_id_found = false;

    for (int i = 0; i < configValues->romraider_definition_files.length(); i++)
    {
        filename = configValues->romraider_definition_files.at(i);

        //qDebug() << "Reading RomRaider ID's from file:" << filename;

        QFile file(filename);
        if (!file.open(QIODevice::ReadOnly ))
        {
            QMessageBox::warning(this, tr("Ecu definition file"), "Unable to open ecuflash definition file " + filename + " for reading");
        }

        QDomDocument xmlBOM;
        xmlBOM.setContent(&file);
        file.close();

        QDomElement root = xmlBOM.documentElement();

        QDomNodeList items = root.childNodes();
        for(int x=0; x< items.count(); x++)
        {
            if (items.at(x).isComment())
                continue;

            QDomElement child = items.at(x).toElement();

            if (child.tagName() == "rom")
            {
                QDomElement sub_child = child.firstChild().toElement();
                while (!sub_child.tagName().isNull())
                {
                    if (sub_child.tagName() == "romid")
                    {
                        QDomElement id_child = sub_child.firstChild().toElement();
                        while (!id_child.tagName().isNull())
                        {
                            if (id_child.tagName() == "xmlid")
                            {
                                //qDebug() << cal_id_count << "cal tag:" << id_child.text();
                                cal_id_found = true;
                                configValues->romraider_def_cal_id.append(id_child.text());
                                configValues->romraider_def_filename.append(filename);
                                cal_id_count++;
                            }
                            if (id_child.tagName() == "internalidaddress")
                            {
                                //qDebug() << "cal addr:" << id_child.text();
                                cal_id_addr_found = true;
                                configValues->romraider_def_cal_id_addr.append(id_child.text());
                            }
                            if (id_child.tagName() == "ecuid")
                            {
                                //qDebug() << "ecuid:" << id_child.text();
                                ecu_id_found = true;
                                configValues->romraider_def_ecu_id.append(id_child.text());
                            }
                            id_child = id_child.nextSibling().toElement();
                        }
                        if (!cal_id_found)
                            configValues->romraider_def_cal_id.append("");
                        if (!cal_id_addr_found)
                            configValues->romraider_def_cal_id_addr.append("");
                        if (!ecu_id_found)
                            configValues->romraider_def_ecu_id.append("");

                        cal_id_found = false;
                        cal_id_addr_found = false;
                        ecu_id_found = false;
                    }
                    sub_child = sub_child.nextSibling().toElement();
                }
            }
        }
        file_count++;
    }
    qDebug() << file_count << "RomRaider definition files found";
    qDebug() << cal_id_count << "RomRaider ecu id's found";

    return configValues;
}

FileActions::EcuCalDefStructure *FileActions::read_romraider_ecu_base_def(EcuCalDefStructure *ecuCalDef)
{
    ConfigValuesStructure *configValues = &ConfigValuesStruct;

    bool OemEcuDefBaseFileFound = false;

    QString rombase;
    QString xmlid;
    QString description;

    QDomDocument xmlBOM;

    QString filename = ecuCalDef->DefinitionFileName;
    //QString filename = configValues->romraider_definition_files.at(0);

    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly ))
    {
        ecuCalDef = NULL;
        //qDebug() << "Unable to open OEM ecu base definitions file " + filename + " for reading";
        QMessageBox::warning(this, tr("Ecu definitions file"), "Unable to open OEM ecu base definitions file " + filename + " for reading");
        return NULL;
    }

    xmlBOM.setContent(&file);
    file.close();

    QDomElement root = xmlBOM.documentElement();

    QString Type = root.tagName();
    QString Name = root.attribute("name"," ");
    QDomElement TagType = root.firstChild().toElement();

    uint16_t index = 0;

    QDomNodeList items = root.childNodes();
    for(int x=0; x< items.count(); x++)
    {
        if (items.at(x).isComment())
            continue;

        QDomElement TagType = items.at(x).toElement();
        if (TagType.tagName() == "rom")
        {
            //qDebug() << "Reading base...";
            rombase = TagType.attribute("base","No base");
            if (rombase == "No base")
            {
                QDomElement RomId = TagType.firstChild().toElement();
                if (RomId.tagName() == "romid")
                {
                    QDomElement RomInfo = RomId.firstChild().toElement();
                    while (!RomInfo.tagName().isNull())
                    {

                        if (RomInfo.tagName() == "xmlid")
                            xmlid = RomInfo.text();

                        RomInfo = RomInfo.nextSibling().toElement();
                    }
                }
                if (xmlid == ecuCalDef->RomInfo[RomBase])
                {
                    //qDebug() << "Found base...";

                    OemEcuDefBaseFileFound = true;
                    QDomElement Table = TagType.firstChild().toElement();
                    while(!Table.isNull())
                    {
                        if (Table.tagName() == "table")
                        {
                            for (int i = 0; i < ecuCalDef->NameList.length(); i++)
                            {
                                //qDebug() << "Checking table:" << ecuCalDef->NameList.at(i);
                                if (Table.attribute("name","No name") == ecuCalDef->NameList.at(i))
                                {
                                    //qDebug() << "Found table:" << ecuCalDef->NameList.at(i);
                                    QString type = Table.attribute("type"," ");
                                    QString storage_type = Table.attribute("storagetype"," ");
                                    if (type == "Switch")
                                    {
                                        type = "Selectable";
                                        storage_type = "bloblist";
                                    }
                                    ecuCalDef->TypeList.replace(i, type);
                                    ecuCalDef->CategoryList.replace(i, Table.attribute("category"," "));

                                    if (ecuCalDef->XSizeList.at(i) == "" || ecuCalDef->XSizeList.at(i) == " ")
                                        ecuCalDef->XSizeList.replace(i, Table.attribute("sizex", "1"));
                                    if (ecuCalDef->YSizeList.at(i) == "" || ecuCalDef->YSizeList.at(i) == " ")
                                        ecuCalDef->YSizeList.replace(i, Table.attribute("sizey", "1"));
                                    ecuCalDef->StartPosList.replace(i, Table.attribute("startpos", "1"));
                                    ecuCalDef->IntervalList.replace(i, Table.attribute("interval", "1"));
                                    ecuCalDef->MinValueList.replace(i, Table.attribute("minvalue","0"));
                                    ecuCalDef->MaxValueList.replace(i, Table.attribute("maxvalue","0"));

                                    ecuCalDef->StorageTypeList.replace(i, storage_type);
                                    ecuCalDef->EndianList.replace(i, Table.attribute("endian"," "));
                                    ecuCalDef->LogParamList.replace(i, Table.attribute("logparam"," "));

                                    //qDebug() << "DEF BASE:" << ecuCalDef->XSizeList.at(i) << ecuCalDef->YSizeList.at(i);
                                    ecuCalDef->SyncedWithEcu = true;

                                    QDomElement TableChild = Table.firstChild().toElement();

                                    QString selection_name;
                                    QString selection_value;

                                    while (!TableChild.isNull())
                                    {
                                        if (TableChild.tagName() == "scaling")
                                        {
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
                                            if (ScaleType == "X Axis")
                                            {
                                                ecuCalDef->XScaleNameList.replace(i, TableChild.attribute("name"," "));
                                                ecuCalDef->XScaleTypeList.replace(i, ScaleType);
                                                ecuCalDef->XScaleStorageTypeList.replace(i, TableChild.attribute("storagetype"," "));
                                                ecuCalDef->XScaleEndianList.replace(i, TableChild.attribute("endian"," "));
                                                ecuCalDef->XScaleLogParamList.replace(i, TableChild.attribute("logparam"," "));
                                                ecuCalDef->XScaleStartPosList.replace(i, TableChild.attribute("startpos","1"));
                                                ecuCalDef->XScaleIntervalList.replace(i, TableChild.attribute("interval","1"));

                                                QDomElement SubChild = TableChild.firstChild().toElement();
                                                while (!SubChild.isNull())
                                                {
                                                    if (SubChild.tagName() == "scaling")
                                                    {
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
                                            else if (ScaleType == "Y Axis" && ecuCalDef->TypeList.at(i) == "3D")
                                            {
                                                ecuCalDef->YScaleNameList.replace(i, TableChild.attribute("name"," "));
                                                ecuCalDef->YScaleTypeList.replace(i, ScaleType);
                                                ecuCalDef->YScaleStorageTypeList.replace(i, TableChild.attribute("storagetype"," "));
                                                ecuCalDef->YScaleEndianList.replace(i, TableChild.attribute("endian"," "));
                                                ecuCalDef->YScaleLogParamList.replace(i, TableChild.attribute("logparam"," "));
                                                ecuCalDef->YScaleStartPosList.replace(i, TableChild.attribute("startpos","1"));
                                                ecuCalDef->YScaleIntervalList.replace(i, TableChild.attribute("interval","1"));
                                                ecuCalDef->XScaleStaticDataList.append(" ");

                                                QDomElement SubChild = TableChild.firstChild().toElement();
                                                while (!SubChild.isNull())
                                                {
                                                    if (SubChild.tagName() == "scaling")
                                                    {
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
                                            else if (ScaleType == "Static Y Axis" || ScaleType == "Static X Axis" || (ScaleType == "Y Axis" && ecuCalDef->TypeList.at(i) == "2D"))
                                            {
                                                if (ScaleType == "Static Y Axis")
                                                    ScaleType = "Static X Axis";
                                                ecuCalDef->XScaleNameList.replace(i, TableChild.attribute("name"," "));
                                                ecuCalDef->XScaleTypeList.replace(i, ScaleType);
                                                ecuCalDef->XScaleStorageTypeList.replace(i, TableChild.attribute("storagetype"," "));
                                                ecuCalDef->XScaleEndianList.replace(i, TableChild.attribute("endian"," "));
                                                ecuCalDef->XScaleLogParamList.replace(i, TableChild.attribute("logparam"," "));
                                                ecuCalDef->XScaleStartPosList.replace(i, TableChild.attribute("startpos","1"));
                                                ecuCalDef->XScaleIntervalList.replace(i, TableChild.attribute("interval","1"));

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
                                        else if (TableChild.tagName() == "state")
                                        {
                                            selection_name.append(TableChild.attribute("name"," ") + ",");
                                            selection_value.append(TableChild.attribute("data"," ") + ",");
                                            selection_value.remove(' ');
                                        }
                                        else if (TableChild.tagName() == "description")
                                        {
                                            description = TableChild.text();
                                            if (!description.isEmpty())
                                                ecuCalDef->DescriptionList.replace(i, "\n\n" + description);
                                            else
                                                ecuCalDef->DescriptionList.replace(i, " ");
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
                                        TableChild = TableChild.nextSibling().toElement();
                                    }
                                    ecuCalDef->SelectionsNameList.replace(i, selection_name);
                                    ecuCalDef->SelectionsValueList.replace(i, selection_value);
                                }
                            }
                        }
                        Table = Table.nextSibling().toElement();
                    }
                }
            }
        }
        TagType = TagType.nextSibling().toElement();
    }

    if (!OemEcuDefBaseFileFound)
        return NULL;

    return ecuCalDef;
}

FileActions::EcuCalDefStructure *FileActions::read_romraider_ecu_def(EcuCalDefStructure *ecuCalDef, QString cal_id)
{
    ConfigValuesStructure *configValues = &ConfigValuesStruct;

    bool cal_id_file_found = false;
    bool inherits_another_def = false;
    bool ecuid_def_found = false;

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

    QString filename;

    // Check if any ECU definition file is selected
    if (!configValues->romraider_definition_files.length() && !configValues->ecuflash_definition_files_directory.length()) {
        QMessageBox::warning(this, tr("Ecu definition file"), "No RomRaider definition file(s), use definition manager at 'Edit' menu to choose file(s)");
        ecuCalDef = NULL;
        return NULL;
    }

    int file_index = 0;

    if (!configValues->romraider_def_cal_id.length())
        return NULL;

    for (int index = 0; index < configValues->romraider_def_cal_id.length(); index++)
    {
        if (configValues->romraider_def_cal_id.at(index) == cal_id)
        {
            //qDebug() << "RomRaider ID found:" << configValues->romraider_def_cal_id.at(index) << cal_id;
            //qDebug() << "RomRaider file name:" << configValues->romraider_def_filename.at(index);
            cal_id_file_found = true;
            file_index = index;
            continue;
        }
    }

    if (!cal_id_file_found)
        return ecuCalDef;

    ecuCalDef->use_romraider_definition = true;

    filename = configValues->romraider_def_filename.at(file_index);

    while (ecuCalDef->RomInfo.length() < RomInfoStrings.length())
        ecuCalDef->RomInfo.append(" ");

    qDebug() << "Open file:" << filename;
    QFile file(filename);

    if (!file.open(QIODevice::ReadOnly ))
    {
        QMessageBox::warning(this, tr("Ecu definitions file"), "Unable to open ECU definition file " + filename + " for reading");
        ecuCalDef = NULL;
        return NULL;
    }

    ecuCalDef->DefinitionFileName = filename;
    QDomDocument xmlBOM;
    xmlBOM.setContent(&file);
    file.close();

    uint16_t index = 0;

    QDomElement root = xmlBOM.documentElement();

    QString Type = root.tagName();
    QString Name = root.attribute("name"," ");

    QDomNodeList items = root.childNodes();
    qDebug() << "Child nodes:" << items.count();
    for(int x = 0; x < items.count(); x++)
    {
        if (items.at(x).isComment())
            continue;

        QDomElement roms_child = items.at(x).toElement();
        if (roms_child.tagName() == "rom")
        {
            if (roms_child.attribute("base", "") != "")
                rombase = roms_child.attribute("base", "");
            if (rombase.contains("BASE"))
                inherits_another_def = false;
            else
                inherits_another_def = true;

            QDomElement rom_child = roms_child.firstChild().toElement();
            while (!rom_child.isNull())
            {
                if (rom_child.tagName() == "romid")
                {
                    QDomElement rom_id_child = rom_child.firstChild().toElement();
                    while (!rom_id_child.isNull())
                    {
                        if (rom_id_child.tagName() == "xmlid")
                            xmlid = rom_id_child.text();
                        else if (rom_id_child.tagName() == "internalidaddress")
                            internalidaddress = rom_id_child.text();
                        else if (rom_id_child.tagName() == "internalidstring")
                            internalidstring = rom_id_child.text();
                        else if (rom_id_child.tagName() == "ecuid")
                            ecuid = rom_id_child.text();
                        else if (rom_id_child.tagName() == "year")
                            year = rom_id_child.text();
                        else if (rom_id_child.tagName() == "market")
                            market = rom_id_child.text();
                        else if (rom_id_child.tagName() == "make")
                            make = rom_id_child.text();
                        else if (rom_id_child.tagName() == "model")
                            model = rom_id_child.text();
                        else if (rom_id_child.tagName() == "submodel")
                            submodel = rom_id_child.text();
                        else if (rom_id_child.tagName() == "transmission")
                            transmission = rom_id_child.text();
                        else if (rom_id_child.tagName() == "memmodel")
                            memmodel = rom_id_child.text();
                        else if (rom_id_child.tagName() == "checksummodule")
                            checksummodule = rom_id_child.text();
                        else if (rom_id_child.tagName() == "flashmethod")
                            //flashmethod = rom_id_child.text();
                            flashmethod = configValues->flash_protocol_selected_family;
                        else if (rom_id_child.tagName() == "filesize")
                            filesize = rom_id_child.text();

                        rom_id_child = rom_id_child.nextSibling().toElement();
                    }
                    if (xmlid == cal_id)
                    {
                        qDebug() << "XML ID:" << xmlid << cal_id;
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

                        if (inherits_another_def)
                            read_romraider_ecu_def(ecuCalDef, rombase);

                        if (!inherits_another_def)
                        {
                            ecuCalDef->RomInfo.replace(RomBase, rombase);
                            ecuCalDef->RomBase = rombase;
                        }
                    }
                }
                else if (rom_child.tagName() == "table" && xmlid == cal_id)
                {
                    ecuid_def_found = true;
                    ecuCalDef->use_romraider_definition = true;

                    add_romraider_def_list_item(ecuCalDef);

                    ecuCalDef->NameList.replace(index, rom_child.attribute("name", " "));
                    ecuCalDef->AddressList.replace(index, rom_child.attribute("storageaddress", " "));
                    ecuCalDef->XSizeList.replace(index, rom_child.attribute("sizex", " "));
                    ecuCalDef->YSizeList.replace(index, rom_child.attribute("sizey", " "));

                    QDomElement rom_sub_child = rom_child.firstChild().toElement();

                    while (!rom_sub_child.isNull())
                    {
                        if (rom_sub_child.tagName() == "table")
                        {
                            if (rom_sub_child.tagName() == "table")
                            {
                                if (rom_sub_child.attribute("type", "") == "X Axis")
                                    ecuCalDef->XScaleAddressList.replace(index, rom_sub_child.attribute("storageaddress", " "));
                                if (rom_sub_child.attribute("type", "") == "Y Axis")
                                    ecuCalDef->YScaleAddressList.replace(index, rom_sub_child.attribute("storageaddress", " "));
                                if (rom_sub_child.attribute("type", "") == "Static Y Axis")
                                {

                                }
                            }
                        }
                        rom_sub_child = rom_sub_child.nextSibling().toElement();
                    }
                    index++;
                }
                rom_child = rom_child.nextSibling().toElement();
            }
            if (!inherits_another_def && ecuid_def_found)
            {
                read_romraider_ecu_base_def(ecuCalDef);
            }
            if (ecuid_def_found)
                ecuCalDef->IdList.append(ecuCalDef->RomInfo.at(EcuId));
            ecuid_def_found = false;
        }
        roms_child = roms_child.nextSibling().toElement();
    }

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
    ecuCalDef->StartPosList.append(" ");
    ecuCalDef->IntervalList.append(" ");
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
    ecuCalDef->MapScalingNameList.append(" ");
    ecuCalDef->MapData.append(" ");

    ecuCalDef->XScaleTypeList.append(" ");
    ecuCalDef->XScaleNameList.append(" ");
    ecuCalDef->XScaleAddressList.append(" ");
    ecuCalDef->XScaleStartPosList.append(" ");
    ecuCalDef->XScaleIntervalList.append(" ");
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
    ecuCalDef->XScaleScalingNameList.append(" ");
    ecuCalDef->XScaleData.append(" ");

    ecuCalDef->YScaleTypeList.append(" ");
    ecuCalDef->YScaleNameList.append(" ");
    ecuCalDef->YScaleAddressList.append(" ");
    ecuCalDef->YScaleStartPosList.append(" ");
    ecuCalDef->YScaleIntervalList.append(" ");
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
    ecuCalDef->YScaleScalingNameList.append(" ");
    ecuCalDef->YScaleData.append(" ");

    ecuCalDef->StorageTypeList.append(" ");
    ecuCalDef->EndianList.append(" ");
    ecuCalDef->LogParamList.append(" ");
    ecuCalDef->FromByteList.append(" ");
    ecuCalDef->ToByteList.append(" ");

    return ecuCalDef;
}

