#include "file_actions.h"

QString FileActions::convert_value_format(QString value_format)
{
    QStringList format_text = value_format.split(".");
    QString decimal_count;
    QString decimals = "0";
    if (format_text.length() > 1)
    {
        if (format_text.at(1).contains("f"))
            decimal_count = format_text.at(1).split("f").at(0);
    }
    if (decimal_count.toInt() > 0)
        decimals.append(".");

    for (int m = 0; m < decimal_count.toInt(); m++)
        decimals.append("0");

    return decimals;
}

FileActions::ConfigValuesStructure *FileActions::create_ecuflash_def_id_list(ConfigValuesStructure *configValues)
{
    QString rombase;
    QString xmlid;

    QString filename;
    QString dir = configValues->ecuflash_definition_files_directory;
    QDirIterator it(dir, QStringList() << "*.xml", QDir::Files, QDirIterator::Subdirectories);

    int file_count = 0;

    qDebug() << "Checking EcuFlash definition files ids";

    while (it.hasNext())
    {
        while (it.filePath() == "")
            it.next();
        filename = it.filePath();

        QFile file(filename);
        if (!file.open(QIODevice::ReadOnly ))
        {
            qDebug() << "Unable to open ecuflash definition file " + filename + " for reading";
            QMessageBox::warning(this, tr("Ecu definition file"), "Unable to open ecuflash definition file " + filename + " for reading");
            //return NULL;
        }

        // Set data into the QDomDocument before processing
        QDomDocument xmlBOM;
        xmlBOM.setContent(&file);
        file.close();

        QDomElement root = xmlBOM.documentElement();

        if (root.tagName() == "rom")
        while (!root.isNull())
        {
            //qDebug() << "Checking <rom> section" << root.tagName();
            if (root.tagName() == "rom")
            {
                //qDebug() << "Found <rom> section";
                QDomElement child = root.firstChild().toElement();
                while (!child.isNull())
                {
                    if (child.tagName() == "romid")
                    {
                        //qDebug() << "Found ROM ID section";
                        QDomElement sub_child = child.firstChild().toElement();
                        while (!sub_child.tagName().isNull())
                        {

                            if (sub_child.tagName() == "xmlid")
                            {
                                configValues->ecuflash_def_ecu_id.append(sub_child.text());
                                configValues->ecuflash_def_filename.append(filename);

                            }
                            sub_child = sub_child.nextSibling().toElement();
                        }
                    }
                    child = child.nextSibling().toElement();
                }
            }
            root = root.nextSibling().toElement();
        }
        it.next();
        file_count++;
    }
    qDebug() << "Iterated through" << file_count << "ecuflash definition files";

    return configValues;
}

FileActions::EcuCalDefStructure *FileActions::read_ecuflash_ecu_base_def(EcuCalDefStructure *ecuCalDef)
{
    ConfigValuesStructure *configValues = &ConfigValuesStruct;

    bool OemEcuDefBaseFileFound = false;

    QString rombase;
    QString xmlid;
    QString ecuflash_ecu_id;

    // Load xml file as raw data
    int file_count = 0;
    QString filename;

    int file_index = 0;
    while (configValues->ecuflash_def_ecu_id.at(file_index) != ecuCalDef->RomInfo[RomBase])
        file_index++;

    filename = configValues->ecuflash_def_filename.at(file_index);
    qDebug() << "EcuFlash base def filename" << filename << "with base name" << ecuCalDef->RomInfo[RomBase] << "at index" << file_index;

    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly ))
    {
        ecuCalDef = NULL;
        qDebug() << "Unable to open OEM ecu base definitions file " + filename + " for reading";
        QMessageBox::warning(this, tr("Ecu definitions file"), "Unable to open OEM ecu base definitions file " + filename + " for reading");
        return NULL;
    }

    // Set data into the QDomDocument before processing
    QDomDocument xmlBOM;
    xmlBOM.setContent(&file);
    file.close();

    QDomElement root = xmlBOM.documentElement();

    if (root.tagName() == "rom")
    while (!root.isNull())
    {
        qDebug() << "Checking <rom> section" << root.tagName();
        if (root.tagName() == "rom")
        {
            //qDebug() << "BASE" << ecuCalDef->RomInfo[RomBase];
            qDebug() << "Found <rom> section";
            rombase = root.attribute("base","No base");
            if (rombase == "No base")
            {
                //qDebug() << "Rom base is" << rombase;
                QDomElement child = root.firstChild().toElement();
                while (!child.isNull())
                {
                    if (child.tagName() == "romid")
                    {
                        //qDebug() << "Found ROM ID section";
                        QDomElement sub_child = child.firstChild().toElement();
                        while (!sub_child.tagName().isNull())
                        {

                            if (sub_child.tagName() == "xmlid")
                                xmlid = sub_child.text();

                            //qDebug() << "xmlid" << xmlid;

                            //if (xmlid == ecuCalDef->RomInfo[RomBase])
                                //continue;

                            sub_child = sub_child.nextSibling().toElement();
                        }
                    }
                    if (xmlid == ecuCalDef->RomInfo[RomBase])
                    {
                        //qDebug() << "Reading BASE" << ecuCalDef->RomInfo[RomBase];

                        OemEcuDefBaseFileFound = true;
                        //qDebug() << "Found ROM BASE section" << xmlid << "for ECUID" << ecuCalDef->RomInfo[XmlId];
                        if (child.tagName() == "scaling")
                        {
                            ecuCalDef->ScalingNameList.append(child.attribute("name"," "));
                            ecuCalDef->ScalingUnitsList.append(child.attribute("units"," "));
                            ecuCalDef->ScalingFromByteList.append(child.attribute("toexpr"," "));
                            ecuCalDef->ScalingToByteList.append(child.attribute("frexpr"," "));
                            ecuCalDef->ScalingFormatList.append(child.attribute("format"," "));
                            ecuCalDef->ScalingMinValueList.append(child.attribute("min"," "));
                            ecuCalDef->ScalingMaxValueList.append(child.attribute("max"," "));
                            ecuCalDef->ScalingIncList.append(child.attribute("inc"," "));
                            ecuCalDef->ScalingStorageTypeList.append(child.attribute("storagetype"," "));
                            ecuCalDef->ScalingEndianList.append(child.attribute("endian"," "));
                        }

                        // Check if the child tag name is table
                        else if (child.tagName() == "table")
                        {
                            //qDebug() << "Check tables";
                            for (int i = 0; i < ecuCalDef->NameList.length(); i++)
                            {
                                if (child.attribute("name","No name") == ecuCalDef->NameList.at(i))
                                {
                                    QString type = child.attribute("type"," ");
                                    if (type == "1D")
                                        type = "2D";
                                    ecuCalDef->TypeList.replace(i, type);
                                    ecuCalDef->CategoryList.replace(i, child.attribute("category"," "));
                                    ecuCalDef->MapScalingNameList.replace(i, child.attribute("scaling"," "));
                                    for (int k = 0; k < ecuCalDef->ScalingNameList.length(); k++)
                                    {
                                        if (ecuCalDef->ScalingNameList.at(k) == ecuCalDef->MapScalingNameList.at(i))
                                        {
                                            ecuCalDef->StorageTypeList.replace(i, ecuCalDef->ScalingStorageTypeList.at(k));
                                            ecuCalDef->UnitsList.replace(i, ecuCalDef->ScalingUnitsList.at(k));
                                            ecuCalDef->FineIncList.replace(i, ecuCalDef->ScalingIncList.at(k));
                                            ecuCalDef->CoarseIncList.replace(i, ecuCalDef->ScalingIncList.at(k));
                                            ecuCalDef->MinValueList.replace(i, ecuCalDef->ScalingMinValueList.at(k));
                                            ecuCalDef->MaxValueList.replace(i, ecuCalDef->ScalingMaxValueList.at(k));
                                            ecuCalDef->EndianList.replace(i, ecuCalDef->ScalingEndianList.at(k));
                                            ecuCalDef->FromByteList.replace(i, ecuCalDef->ScalingFromByteList.at(k));
                                            ecuCalDef->ToByteList.replace(i, ecuCalDef->ScalingToByteList.at(k));
                                            ecuCalDef->FormatList.replace(i, convert_value_format(ecuCalDef->ScalingFormatList.at(k)));
                                        }
                                    }
                                    qDebug() << ecuCalDef->NameList.at(i) << "/" << child.attribute("name"," ") << ecuCalDef->AddressList.at(i) << ecuCalDef->FormatList.at(i);

                                    // Get the first child of the Table
                                    QDomElement sub_child = child.firstChild().toElement();
                                    QString TableSelections;
                                    QString TableSelectionsSorted;
                                    QString TableDescription;
                                    QString TableStates;

                                    // Read each child of the Table node
                                    while (!sub_child.isNull())
                                    {
                                        if (sub_child.tagName() == "table")
                                        {
                                            QString ScaleType = sub_child.attribute("type"," ");
                                            if (ScaleType == "X Axis")
                                            {
                                                if (ecuCalDef->XSizeList.at(i) == "" || ecuCalDef->XSizeList.at(i) == " ")
                                                    ecuCalDef->XSizeList.replace(i, sub_child.attribute("elements", "1"));
                                                ecuCalDef->XScaleNameList.replace(i, sub_child.attribute("name"," "));
                                                ecuCalDef->XScaleTypeList.replace(i, ScaleType);
                                                ecuCalDef->XScaleScalingNameList.replace(i, sub_child.attribute("scaling"," "));
                                                ecuCalDef->XScaleLogParamList.replace(i, sub_child.attribute("logparam"," "));

                                                for (int k = 0; k < ecuCalDef->ScalingNameList.length(); k++)
                                                {
                                                    if (ecuCalDef->ScalingNameList.at(k) == ecuCalDef->XScaleScalingNameList.at(i))
                                                    {
                                                        ecuCalDef->XScaleStorageTypeList.replace(i, ecuCalDef->ScalingStorageTypeList.at(k));
                                                        ecuCalDef->XScaleUnitsList.replace(i, ecuCalDef->ScalingUnitsList.at(k));
                                                        ecuCalDef->XScaleFineIncList.replace(i, ecuCalDef->ScalingIncList.at(k));
                                                        ecuCalDef->XScaleCoarseIncList.replace(i, ecuCalDef->ScalingIncList.at(k));
                                                        ecuCalDef->XScaleMinValueList.replace(i, ecuCalDef->ScalingMinValueList.at(k));
                                                        ecuCalDef->XScaleMaxValueList.replace(i, ecuCalDef->ScalingMaxValueList.at(k));
                                                        ecuCalDef->XScaleEndianList.replace(i, ecuCalDef->ScalingEndianList.at(k));
                                                        ecuCalDef->XScaleFromByteList.replace(i, ecuCalDef->ScalingFromByteList.at(k));
                                                        ecuCalDef->XScaleToByteList.replace(i, ecuCalDef->ScalingToByteList.at(k));
                                                        ecuCalDef->XScaleFormatList.replace(i, convert_value_format(ecuCalDef->ScalingFormatList.at(k)));
                                                     }
                                                }
                                                qDebug() << "X" << ecuCalDef->NameList.at(i) << ecuCalDef->XSizeList.at(i) << ecuCalDef->XScaleAddressList.at(i) << ecuCalDef->XScaleFormatList.at(i);
                                            }
                                            else if (ScaleType == "Y Axis" && ecuCalDef->TypeList.at(i) == "3D")
                                            {
                                                if (ecuCalDef->YSizeList.at(i) == "" || ecuCalDef->YSizeList.at(i) == " ")
                                                    ecuCalDef->YSizeList.replace(i, sub_child.attribute("elements", "1"));
                                                ecuCalDef->YScaleNameList.replace(i, sub_child.attribute("name"," "));
                                                ecuCalDef->YScaleTypeList.replace(i, ScaleType);
                                                ecuCalDef->YScaleScalingNameList.replace(i, sub_child.attribute("scaling"," "));
                                                ecuCalDef->YScaleLogParamList.replace(i, sub_child.attribute("logparam"," "));
                                                //ecuCalDef->XScaleStaticDataList.append(" ");
                                                qDebug() << "Y" << ecuCalDef->NameList.at(i) << ecuCalDef->YSizeList.at(i) << ecuCalDef->YScaleAddressList.at(i);

                                                for (int k = 0; k < ecuCalDef->ScalingNameList.length(); k++)
                                                {
                                                    if (ecuCalDef->ScalingNameList.at(k) == ecuCalDef->YScaleScalingNameList.at(i))
                                                    {
                                                        ecuCalDef->YScaleStorageTypeList.replace(i, ecuCalDef->ScalingStorageTypeList.at(k));
                                                        ecuCalDef->YScaleUnitsList.replace(i, ecuCalDef->ScalingUnitsList.at(k));
                                                        ecuCalDef->YScaleFineIncList.replace(i, ecuCalDef->ScalingIncList.at(k));
                                                        ecuCalDef->YScaleCoarseIncList.replace(i, ecuCalDef->ScalingIncList.at(k));
                                                        ecuCalDef->YScaleMinValueList.replace(i, ecuCalDef->ScalingMinValueList.at(k));
                                                        ecuCalDef->YScaleMaxValueList.replace(i, ecuCalDef->ScalingMaxValueList.at(k));
                                                        ecuCalDef->YScaleEndianList.replace(i, ecuCalDef->ScalingEndianList.at(k));
                                                        ecuCalDef->YScaleFromByteList.replace(i, ecuCalDef->ScalingFromByteList.at(k));
                                                        ecuCalDef->YScaleToByteList.replace(i, ecuCalDef->ScalingToByteList.at(k));
                                                        ecuCalDef->YScaleFormatList.replace(i, convert_value_format(ecuCalDef->ScalingFormatList.at(k)));
                                                     }
                                                }
                                                qDebug() << "Y" << ecuCalDef->NameList.at(i) << ecuCalDef->XSizeList.at(i) << ecuCalDef->YScaleAddressList.at(i) << ecuCalDef->YScaleFormatList.at(i);
                                            }
                                            else if (ScaleType == "Static Y Axis" || (ScaleType == "Y Axis" && ecuCalDef->TypeList.at(i) == "2D"))
                                            {
                                                ecuCalDef->XSizeList.replace(i, sub_child.attribute("elements", "1"));
                                                ecuCalDef->XScaleNameList.replace(i, sub_child.attribute("name"," "));
                                                ecuCalDef->XScaleTypeList.replace(i, ScaleType);
                                                ecuCalDef->XScaleScalingNameList.replace(i, sub_child.attribute("scaling"," "));
                                                ecuCalDef->XScaleLogParamList.replace(i, sub_child.attribute("logparam"," "));
                                                qDebug() << "(static) Y 2D" << ecuCalDef->NameList.at(i) << ecuCalDef->XScaleAddressList.at(i) << ecuCalDef->XSizeList.at(i);

                                                for (int k = 0; k < ecuCalDef->ScalingNameList.length(); k++)
                                                {
                                                    if (ecuCalDef->ScalingNameList.at(k) == ecuCalDef->XScaleScalingNameList.at(i))
                                                    {
                                                        ecuCalDef->XScaleStorageTypeList.replace(i, ecuCalDef->ScalingStorageTypeList.at(k));
                                                        ecuCalDef->XScaleUnitsList.replace(i, ecuCalDef->ScalingUnitsList.at(k));
                                                        ecuCalDef->XScaleFormatList.replace(i, ecuCalDef->ScalingFormatList.at(k));
                                                        ecuCalDef->XScaleFineIncList.replace(i, ecuCalDef->ScalingIncList.at(k));
                                                        ecuCalDef->XScaleCoarseIncList.replace(i, ecuCalDef->ScalingIncList.at(k));
                                                        ecuCalDef->XScaleMinValueList.replace(i, ecuCalDef->ScalingMinValueList.at(k));
                                                        ecuCalDef->XScaleMaxValueList.replace(i, ecuCalDef->ScalingMaxValueList.at(k));
                                                        ecuCalDef->XScaleEndianList.replace(i, ecuCalDef->ScalingEndianList.at(k));
                                                        ecuCalDef->XScaleFromByteList.replace(i, ecuCalDef->ScalingFromByteList.at(k));
                                                        ecuCalDef->XScaleToByteList.replace(i, ecuCalDef->ScalingToByteList.at(k));
                                                        QString value_format;
                                                        value_format = ecuCalDef->ScalingFormatList.at(k);
                                                        ecuCalDef->XScaleFormatList.replace(i, convert_value_format(value_format));

                                                        ecuCalDef->XScaleStaticDataList.replace(i, " ");
                                                    }
                                                }

                                                QDomElement sub_child_data = sub_child.firstChild().toElement();
                                                QString StaticYScaleData;
                                                while (!sub_child_data.isNull())
                                                {
                                                    if (sub_child_data.tagName() == "data"){
                                                        StaticYScaleData.append(sub_child_data.text());
                                                        StaticYScaleData.append(",");
                                                    }
                                                    sub_child_data = sub_child_data.nextSibling().toElement();
                                                }
                                                //ecuCalDef->XSizeList.replace(i, ecuCalDef->YSizeList[i]);
                                                ecuCalDef->YSizeList[i] = "1";
                                                //ecuCalDef->XScaleAddressList.replace(i, ecuCalDef->YScaleAddressList[i]);
                                                ecuCalDef->YScaleAddressList[i] = " ";

                                                ecuCalDef->XScaleStaticDataList.replace(i, StaticYScaleData);
                                            }
                                        }
                                        else if (sub_child.tagName() == "description"){
                                            TableDescription = sub_child.text();
                                            if (!TableDescription.isEmpty())
                                                ecuCalDef->DescriptionList.replace(i, "\n\n" + TableDescription);
                                            else
                                                ecuCalDef->DescriptionList.replace(i, " ");
                                        }
                                        else if (sub_child.tagName() == "state"){
                                            TableStates.append(sub_child.attribute("name"," ") + ",");
                                            TableStates.append(sub_child.attribute("data"," ") + ",");

                                        }
                                        else
                                        {
                                            /*
                                            ecuCalDef->UnitsList.replace(i, sub_child.attribute("units"," "));
                                            ecuCalDef->FormatList.replace(i, sub_child.attribute("format"," "));
                                            ecuCalDef->FineIncList.replace(i, sub_child.attribute("fineincrement"," "));
                                            ecuCalDef->CoarseIncList.replace(i, sub_child.attribute("coarseincrement"," "));

                                            ecuCalDef->FromByteList.replace(i, sub_child.attribute("expression","x"));
                                            ecuCalDef->ToByteList.replace(i, sub_child.attribute("to_byte","x"));
                                            */
                                        }
                                        // Next child
                                        sub_child = sub_child.nextSibling().toElement();
                                    }
                                    if (!TableSelections.isEmpty())
                                    {
                                        ecuCalDef->SelectionsList.replace(i, TableSelections);
                                        ecuCalDef->SelectionsListSorted.replace(i, TableSelectionsSorted);
                                    }
                                    ecuCalDef->StateList.replace(i, TableStates);

                                }
                            }
                        }
                    }
                    child = child.nextSibling().toElement();
                }
            }
        }
        root = root.nextSibling().toElement();
    }

    if (!OemEcuDefBaseFileFound)
        return NULL;

    //qDebug() << "ECU rom base file read";
    return ecuCalDef;
}

FileActions::EcuCalDefStructure *FileActions::read_ecuflash_ecu_def(EcuCalDefStructure *ecuCalDef, QString ecuId)
{
    ConfigValuesStructure *configValues = &ConfigValuesStruct;

    bool inherits_another_def = false;
    bool needs_base_def = false;
    bool ecuid_def_found = false;
    bool contains_x_scale = false;
    bool contains_y_scale = false;

    QString inherits_ecu_id;
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
    qDebug() << dateTimeString << "Start looking ECU ID from EcuFlash def files";

    QString filename;

    int file_index = 0;
    while (configValues->ecuflash_def_ecu_id.at(file_index) != ecuId)
        file_index++;

    filename = configValues->ecuflash_def_filename.at(file_index);
    qDebug() << "EcuFlash ecu def filename" << filename<< "with ecu id" << ecuId << "at index" << file_index;

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

    QFile file(filename);

    if (!file.open(QIODevice::ReadOnly ))
    {
        ecuCalDef = NULL;
        //qDebug() << "Unable to open ECU definition file " + filename + " for reading");
        QMessageBox::warning(this, tr("Ecu definitions file"), "Unable to open ECU definition file " + filename + " for reading");
        return NULL;
    }

    QDomDocument xmlBOM;
    xmlBOM.setContent(&file);
    file.close();

    QDomElement root = xmlBOM.documentElement();

    if (root.tagName() == "rom")
    {
        qDebug() << "<rom> TAG";
        QDomElement child = root.firstChild().toElement();
        while (!child.isNull())
        {
            if (child.tagName() == "include")
            {
                qDebug() << "<include> TAG";
                QString rom_base_text = child.text();
                if (rom_base_text != "")
                {
                    rombase = rom_base_text;
                }
                if (rombase.contains("BASE"))
                {
                    qDebug() << "BASE DEF" << rombase;
                    ecuCalDef->RomInfo.replace(RomBase, rombase);
                    ecuCalDef->RomBase = rombase;
                    needs_base_def = true;
                    inherits_ecu_id = "";
                    inherits_another_def = false;
                }
                else
                {
                    qDebug() << "INHERITS" << rombase;
                    needs_base_def = false;
                    inherits_ecu_id = rombase;
                    inherits_another_def = true;
                }
                qDebug() << "Is NOT BASE def" << inherits_another_def << rombase;
            }
            else if (child.tagName() == "romid")
            {
                qDebug() << "<romid> TAG";
                QDomElement sub_child = child.firstChild().toElement();
                while (!sub_child.isNull())
                {
                    if (sub_child.tagName() == "xmlid")
                        xmlid = sub_child.text();
                    else if (sub_child.tagName() == "internalidaddress")
                        internalidaddress = sub_child.text();
                    else if (sub_child.tagName() == "internalidstring")
                        internalidstring = sub_child.text();
                    else if (sub_child.tagName() == "ecuid")
                        ecuid = sub_child.text();
                    else if (sub_child.tagName() == "year")
                        year = sub_child.text();
                    else if (sub_child.tagName() == "market")
                        market = sub_child.text();
                    else if (sub_child.tagName() == "make")
                        make = sub_child.text();
                    else if (sub_child.tagName() == "model")
                        model = sub_child.text();
                    else if (sub_child.tagName() == "submodel")
                        submodel = sub_child.text();
                    else if (sub_child.tagName() == "transmission")
                        transmission = sub_child.text();
                    else if (sub_child.tagName() == "memmodel")
                        memmodel = sub_child.text();
                    else if (sub_child.tagName() == "checksummodule")
                        checksummodule = sub_child.text();
                    else if (sub_child.tagName() == "flashmethod")
                        flashmethod = sub_child.text();
                    else if (sub_child.tagName() == "filesize")
                        filesize = sub_child.text();

                    sub_child = sub_child.nextSibling().toElement();
                }
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

                    //qDebug() << "inherits_another_def" << inherits_another_def << rombase;
                    //if (inherits_another_def)
                        //read_ecuflash_ecu_def(ecuCalDef, rombase);

                }
            }
            else if (child.tagName() == "scaling")
            {
                ecuCalDef->ScalingNameList.append(child.attribute("name"," "));
                ecuCalDef->ScalingUnitsList.append(child.attribute("units"," "));
                ecuCalDef->ScalingFromByteList.append(child.attribute("toexpr"," "));
                ecuCalDef->ScalingToByteList.append(child.attribute("frexpr"," "));
                ecuCalDef->ScalingFormatList.append(child.attribute("format"," "));
                ecuCalDef->ScalingMinValueList.append(child.attribute("min"," "));
                ecuCalDef->ScalingMaxValueList.append(child.attribute("max"," "));
                ecuCalDef->ScalingIncList.append(child.attribute("inc"," "));
                ecuCalDef->ScalingStorageTypeList.append(child.attribute("storagetype"," "));
                ecuCalDef->ScalingEndianList.append(child.attribute("endian"," "));
            }
            else if (child.tagName() == "table" && xmlid == ecuId)
            {
                qDebug() << "ECU ID" << ecuId << "found";
                ecuid_def_found = true;

                add_ecuflash_def_list_item(ecuCalDef);

                QString type = child.attribute("type"," ");
                if (type == "1D")
                    type = "2D";
                if (ecuCalDef->NameList.at(def_map_index) == " ")
                    ecuCalDef->NameList.replace(def_map_index, child.attribute("name", " "));
                if (ecuCalDef->AddressList.at(def_map_index) == " ")
                    ecuCalDef->AddressList.replace(def_map_index, child.attribute("address", ""));
                if (ecuCalDef->TypeList.at(def_map_index) == " ")
                    ecuCalDef->TypeList.replace(def_map_index, type);
                if (ecuCalDef->CategoryList.at(def_map_index) == " ")
                    ecuCalDef->CategoryList.replace(def_map_index, child.attribute("category"," "));
                if (ecuCalDef->MapScalingNameList.at(def_map_index) == " ")
                    ecuCalDef->MapScalingNameList.replace(def_map_index, child.attribute("scaling"," "));
                for (int k = 0; k < ecuCalDef->ScalingNameList.length(); k++)
                {
                    if (ecuCalDef->ScalingNameList.at(k) == ecuCalDef->MapScalingNameList.at(def_map_index))
                    {
                        ecuCalDef->StorageTypeList.replace(def_map_index, ecuCalDef->ScalingStorageTypeList.at(k));
                        ecuCalDef->UnitsList.replace(def_map_index, ecuCalDef->ScalingUnitsList.at(k));
                        ecuCalDef->FineIncList.replace(def_map_index, ecuCalDef->ScalingIncList.at(k));
                        ecuCalDef->CoarseIncList.replace(def_map_index, ecuCalDef->ScalingIncList.at(k));
                        ecuCalDef->MinValueList.replace(def_map_index, ecuCalDef->ScalingMinValueList.at(k));
                        ecuCalDef->MaxValueList.replace(def_map_index, ecuCalDef->ScalingMaxValueList.at(k));
                        ecuCalDef->EndianList.replace(def_map_index, ecuCalDef->ScalingEndianList.at(k));
                        ecuCalDef->FromByteList.replace(def_map_index, ecuCalDef->ScalingFromByteList.at(k));
                        ecuCalDef->ToByteList.replace(def_map_index, ecuCalDef->ScalingToByteList.at(k));
                        ecuCalDef->FormatList.replace(def_map_index, convert_value_format(ecuCalDef->ScalingFormatList.at(k)));
                    }
                }
                qDebug() << ecuCalDef->NameList.at(def_map_index) << "/" << child.attribute("name"," ") << ecuCalDef->AddressList.at(def_map_index) << ecuCalDef->FormatList.at(def_map_index);

                QDomElement sub_child = child.firstChild().toElement();
                int i = 0;
                while (!sub_child.isNull())
                {
                    if (sub_child.tagName() == "table")
                    {
                        QString ScaleType = sub_child.attribute("type"," ");
                        if (ScaleType == "X Axis")
                        {
                            if (ecuCalDef->XSizeList.at(def_map_index) == " ")
                                ecuCalDef->XSizeList.replace(def_map_index, sub_child.attribute("elements", " "));
                            if (ecuCalDef->XScaleNameList.at(def_map_index) == " ")
                                ecuCalDef->XScaleNameList.replace(def_map_index, sub_child.attribute("name"," "));
                            if (ecuCalDef->XScaleAddressList.at(def_map_index) == " ")
                                ecuCalDef->XScaleAddressList.replace(def_map_index, sub_child.attribute("address", " "));
                            if (ecuCalDef->XScaleTypeList.at(def_map_index) == " ")
                                ecuCalDef->XScaleTypeList.replace(def_map_index, ScaleType);
                            if (ecuCalDef->XScaleScalingNameList.at(def_map_index) == " ")
                                ecuCalDef->XScaleScalingNameList.replace(def_map_index, sub_child.attribute("scaling"," "));

                            for (int k = 0; k < ecuCalDef->ScalingNameList.length(); k++)
                            {
                                if (ecuCalDef->ScalingNameList.at(k) == ecuCalDef->XScaleScalingNameList.at(def_map_index))
                                {
                                    ecuCalDef->XScaleStorageTypeList.replace(def_map_index, ecuCalDef->ScalingStorageTypeList.at(k));
                                    ecuCalDef->XScaleUnitsList.replace(def_map_index, ecuCalDef->ScalingUnitsList.at(k));
                                    ecuCalDef->XScaleFineIncList.replace(def_map_index, ecuCalDef->ScalingIncList.at(k));
                                    ecuCalDef->XScaleCoarseIncList.replace(def_map_index, ecuCalDef->ScalingIncList.at(k));
                                    ecuCalDef->XScaleMinValueList.replace(def_map_index, ecuCalDef->ScalingMinValueList.at(k));
                                    ecuCalDef->XScaleMaxValueList.replace(def_map_index, ecuCalDef->ScalingMaxValueList.at(k));
                                    ecuCalDef->XScaleEndianList.replace(def_map_index, ecuCalDef->ScalingEndianList.at(k));
                                    ecuCalDef->XScaleFromByteList.replace(def_map_index, ecuCalDef->ScalingFromByteList.at(k));
                                    ecuCalDef->XScaleToByteList.replace(def_map_index, ecuCalDef->ScalingToByteList.at(k));
                                    ecuCalDef->XScaleFormatList.replace(def_map_index, convert_value_format(ecuCalDef->ScalingFormatList.at(k)));
                                 }
                            }
                            qDebug() << "X" << ecuCalDef->XScaleNameList.at(def_map_index) << ecuCalDef->TypeList.at(def_map_index) << ecuCalDef->XSizeList.at(def_map_index) << ecuCalDef->XScaleAddressList.at(def_map_index) << ecuCalDef->XScaleFormatList.at(def_map_index);
                        }
                        else if (ScaleType == "Y Axis" && ecuCalDef->TypeList.at(def_map_index) == "3D")
                        {
                            if (ecuCalDef->YSizeList.at(def_map_index) == " ")
                                ecuCalDef->YSizeList.replace(def_map_index, sub_child.attribute("elements", " "));
                            if (ecuCalDef->YScaleNameList.at(def_map_index) == " ")
                                ecuCalDef->YScaleNameList.replace(def_map_index, sub_child.attribute("name"," "));
                            if (ecuCalDef->YScaleAddressList.at(def_map_index) == " ")
                                ecuCalDef->YScaleAddressList.replace(def_map_index, sub_child.attribute("address", " "));
                            if (ecuCalDef->YScaleTypeList.at(def_map_index) == " ")
                                ecuCalDef->YScaleTypeList.replace(def_map_index, ScaleType);
                            if (ecuCalDef->YScaleScalingNameList.at(def_map_index) == " ")
                                ecuCalDef->YScaleScalingNameList.replace(def_map_index, sub_child.attribute("scaling"," "));

                            for (int k = 0; k < ecuCalDef->ScalingNameList.length(); k++)
                            {
                                if (ecuCalDef->ScalingNameList.at(k) == ecuCalDef->YScaleScalingNameList.at(def_map_index))
                                {
                                    ecuCalDef->YScaleStorageTypeList.replace(def_map_index, ecuCalDef->ScalingStorageTypeList.at(k));
                                    ecuCalDef->YScaleUnitsList.replace(def_map_index, ecuCalDef->ScalingUnitsList.at(k));
                                    ecuCalDef->YScaleFineIncList.replace(def_map_index, ecuCalDef->ScalingIncList.at(k));
                                    ecuCalDef->YScaleCoarseIncList.replace(def_map_index, ecuCalDef->ScalingIncList.at(k));
                                    ecuCalDef->YScaleMinValueList.replace(def_map_index, ecuCalDef->ScalingMinValueList.at(k));
                                    ecuCalDef->YScaleMaxValueList.replace(def_map_index, ecuCalDef->ScalingMaxValueList.at(k));
                                    ecuCalDef->YScaleEndianList.replace(def_map_index, ecuCalDef->ScalingEndianList.at(k));
                                    ecuCalDef->YScaleFromByteList.replace(def_map_index, ecuCalDef->ScalingFromByteList.at(k));
                                    ecuCalDef->YScaleToByteList.replace(def_map_index, ecuCalDef->ScalingToByteList.at(k));
                                    ecuCalDef->YScaleFormatList.replace(def_map_index, convert_value_format(ecuCalDef->ScalingFormatList.at(k)));
                                 }
                            }
                            qDebug() << "Y" << ecuCalDef->YScaleNameList.at(def_map_index) << ecuCalDef->TypeList.at(def_map_index) << ecuCalDef->YSizeList.at(def_map_index) << ecuCalDef->YScaleAddressList.at(def_map_index) << ecuCalDef->YScaleFormatList.at(def_map_index);
                        }
                        else if (ScaleType == "Static Y Axis" || (ScaleType == "Y Axis" && ecuCalDef->TypeList.at(def_map_index) == "2D"))
                        {
                            if (ecuCalDef->XSizeList.at(def_map_index) == " ")
                                ecuCalDef->XSizeList.replace(def_map_index, sub_child.attribute("elements", " "));
                            if (ecuCalDef->XScaleNameList.at(def_map_index) == " ")
                                ecuCalDef->XScaleNameList.replace(def_map_index, sub_child.attribute("name"," "));
                            if (ecuCalDef->XScaleAddressList.at(def_map_index) == " ")
                                ecuCalDef->XScaleAddressList.replace(def_map_index, sub_child.attribute("address", " "));
                            if (ecuCalDef->XScaleTypeList.at(def_map_index) == " ")
                                ecuCalDef->XScaleTypeList.replace(def_map_index, ScaleType);
                            if (ecuCalDef->XScaleScalingNameList.at(def_map_index) == " ")
                                ecuCalDef->XScaleScalingNameList.replace(def_map_index, sub_child.attribute("scaling"," "));

                            for (int k = 0; k < ecuCalDef->ScalingNameList.length(); k++)
                            {
                                if (ecuCalDef->ScalingNameList.at(k) == ecuCalDef->XScaleScalingNameList.at(i))
                                {
                                    ecuCalDef->XScaleStorageTypeList.replace(def_map_index, ecuCalDef->ScalingStorageTypeList.at(k));
                                    ecuCalDef->XScaleUnitsList.replace(def_map_index, ecuCalDef->ScalingUnitsList.at(k));
                                    ecuCalDef->XScaleFormatList.replace(def_map_index, ecuCalDef->ScalingFormatList.at(k));
                                    ecuCalDef->XScaleFineIncList.replace(def_map_index, ecuCalDef->ScalingIncList.at(k));
                                    ecuCalDef->XScaleCoarseIncList.replace(def_map_index, ecuCalDef->ScalingIncList.at(k));
                                    ecuCalDef->XScaleMinValueList.replace(def_map_index, ecuCalDef->ScalingMinValueList.at(k));
                                    ecuCalDef->XScaleMaxValueList.replace(def_map_index, ecuCalDef->ScalingMaxValueList.at(k));
                                    ecuCalDef->XScaleEndianList.replace(def_map_index, ecuCalDef->ScalingEndianList.at(k));
                                    ecuCalDef->XScaleFromByteList.replace(def_map_index, ecuCalDef->ScalingFromByteList.at(k));
                                    ecuCalDef->XScaleToByteList.replace(def_map_index, ecuCalDef->ScalingToByteList.at(k));
                                    ecuCalDef->XScaleFormatList.replace(def_map_index, convert_value_format(ecuCalDef->ScalingFormatList.at(k)));

                                    ecuCalDef->XScaleStaticDataList.replace(def_map_index, " ");
                                }
                            }
                            if (ScaleType == "Static Y Axis")
                            {
                                QDomElement sub_child_data = sub_child.firstChild().toElement();
                                QString StaticYScaleData;
                                while (!sub_child_data.isNull())
                                {
                                    if (sub_child_data.tagName() == "data"){
                                        StaticYScaleData.append(sub_child_data.text());
                                        StaticYScaleData.append(",");
                                    }
                                    sub_child_data = sub_child_data.nextSibling().toElement();
                                }
                                ecuCalDef->XScaleStaticDataList.replace(def_map_index, StaticYScaleData);
                            }

                            qDebug() << "(static) Y 2D" << ecuCalDef->NameList.at(i) << ecuCalDef->XScaleTypeList.at(def_map_index) << ecuCalDef->XSizeList.at(i) << ecuCalDef->XScaleAddressList.at(i);
                        }
                        else if (sub_child.attribute("name"," ") != " ")
                        {
                            if (i == 0)
                            {
                                ecuCalDef->XScaleAddressList.replace(def_map_index, sub_child.attribute("address", " "));
                                ecuCalDef->XSizeList.replace(def_map_index, sub_child.attribute("elements", " "));
                                qDebug() << "Table:" << ecuCalDef->NameList.at(def_map_index) << "X Axis address:" << ecuCalDef->XScaleAddressList.at(def_map_index);
                            }
                            if (i == 1)
                            {
                                ecuCalDef->YScaleAddressList.replace(def_map_index, sub_child.attribute("address", " "));
                                ecuCalDef->YSizeList.replace(def_map_index, sub_child.attribute("elements", " "));
                                qDebug() << "Table:" << ecuCalDef->NameList.at(def_map_index) << "Y Axis address:" << ecuCalDef->YScaleAddressList.at(def_map_index);
                            }
                            i++;
                        }
                    }
                    sub_child = sub_child.nextSibling().toElement();
                }
                if (ecuCalDef->YSizeList[def_map_index] == " ")
                {
                    ecuCalDef->YSizeList[def_map_index] = "1";
                    ecuCalDef->YScaleAddressList[def_map_index] = " ";
                }
                def_map_index++;
            }
            child = child.nextSibling().toElement();
        }
        if (inherits_another_def)
        {
            qDebug() << "inherits_another_def" << inherits_another_def << rombase;
            read_ecuflash_ecu_def(ecuCalDef, rombase);
            inherits_another_def = false;
        }
        if (needs_base_def && ecuid_def_found && ecuCalDef->RomInfo.at(RomBase) != "")
        {
            qDebug() << "Check inherits_another_def" << inherits_another_def;
            qDebug() << "ECU ID" << ecuCalDef->RomInfo.at(EcuId) << "found and def read, move to ECU BASE def" << ecuCalDef->RomBase;
            read_ecuflash_ecu_base_def(ecuCalDef);
        }
        ecuid_def_found = false;
    }
    ecuCalDef->IdList.append(ecuCalDef->RomInfo.at(EcuId));

    dateTime = dateTime.currentDateTime();
    dateTimeString = dateTime.toString("[yyyy-MM-dd hh':'mm':'ss'.'zzz']");

    ecuCalDef->use_ecuflash_definition = true;

    return ecuCalDef;
}

FileActions::EcuCalDefStructure *FileActions::add_ecuflash_def_list_item(EcuCalDefStructure *ecuCalDef)
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
    ecuCalDef->SelectionsList.append(" ");
    ecuCalDef->SelectionsListSorted.append(" ");
    ecuCalDef->DescriptionList.append(" ");
    ecuCalDef->StateList.append(" ");
    ecuCalDef->MapScalingNameList.append(" ");
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
    ecuCalDef->XScaleScalingNameList.append(" ");
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
    ecuCalDef->YScaleScalingNameList.append(" ");
    ecuCalDef->YScaleData.append(" ");

    ecuCalDef->StorageTypeList.append(" ");
    ecuCalDef->EndianList.append(" ");
    ecuCalDef->LogParamList.append(" ");
    ecuCalDef->FromByteList.append(" ");
    ecuCalDef->ToByteList.append(" ");

    return ecuCalDef;
}
