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
    QString filename;
    QString dir = configValues->ecuflash_definition_files_directory;
    int file_count = 0;
    int cal_id_count = 0;

    bool cal_id_found = false;
    bool cal_id_addr_found = false;
    bool ecu_id_found = false;

    if (QDir(configValues->ecuflash_definition_files_directory).exists())
    {
        QDirIterator it(dir, QStringList() << "*.xml", QDir::Files, QDirIterator::Subdirectories);

        while (it.hasNext())
        {
            filename = it.next();

            QFile file(filename);
            if (!file.open(QIODevice::ReadOnly ))
            {
                QMessageBox::warning(this, tr("Ecu definition file"), "Unable to open ecuflash definition file " + filename + " for reading");
            }

            QDomDocument xmlBOM;
            xmlBOM.setContent(&file);
            file.close();

            QDomElement root = xmlBOM.documentElement();

            int i = 0;
            while (root.tagName() != "rom" && i < 5)
            {
                root = root.firstChild().toElement();
                i++;
            }

            //if (root.tagName() == "rom")
            while (!root.isNull())
            {
                if (root.tagName() == "rom")
                {
                    QDomElement child = root.firstChild().toElement();
                    while (!child.isNull())
                    {
                        if (child.tagName() == "romid")
                        {
                            QDomElement sub_child = child.firstChild().toElement();
                            while (!sub_child.tagName().isNull())
                            {
                                if (sub_child.tagName() == "xmlid")
                                {
                                    //qDebug() << "cal tag:" << sub_child.text();
                                    //qDebug() << "Filename" << filename << "with tag:" << sub_child.text();
                                    cal_id_found = true;
                                    configValues->ecuflash_def_cal_id.append(sub_child.text());
                                    configValues->ecuflash_def_filename.append(filename);
                                    cal_id_count++;
                                }
                                if (sub_child.tagName() == "internalidaddress")
                                {
                                    //qDebug() << "cal addr:" << sub_child.text();
                                    cal_id_addr_found = true;
                                    configValues->ecuflash_def_cal_id_addr.append(sub_child.text());
                                }
                                if (sub_child.tagName() == "ecuid")
                                {
                                    //qDebug() << "ecuid:" << sub_child.text();
                                    ecu_id_found = true;
                                    configValues->ecuflash_def_ecu_id.append(sub_child.text());
                                }
                                sub_child = sub_child.nextSibling().toElement();
                            }
                            if (!cal_id_found)
                            {
                                configValues->ecuflash_def_cal_id.append("");
                                configValues->ecuflash_def_filename.append("");
                            }
                            if (!cal_id_addr_found)
                                configValues->ecuflash_def_cal_id_addr.append("");
                            if (!ecu_id_found)
                                configValues->ecuflash_def_ecu_id.append("");

                            cal_id_found = false;
                            cal_id_addr_found = false;
                            ecu_id_found = false;
                        }
                        child = child.nextSibling().toElement();
                    }
                }
                root = root.nextSibling().toElement();
            }
            file_count++;
        }
    }
    qDebug() << file_count << "EcuFlash definition files found";
    qDebug() << cal_id_count << "EcuFlash ecu id's found";

    return configValues;
}
/*
FileActions::EcuCalDefStructure *FileActions::read_ecuflash_ecu_base_def(EcuCalDefStructure *ecuCalDef)
{
    ConfigValuesStructure *configValues = &ConfigValuesStruct;

    bool OemEcuDefBaseFileFound = false;

    QString rombase;
    QString xmlid;

    QString filename;

    int scaling_index = 0;

    int file_index = 0;
    while (configValues->ecuflash_def_cal_id.at(file_index) != ecuCalDef->RomInfo[RomBase])
        file_index++;

    filename = configValues->ecuflash_def_filename.at(file_index);

    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly ))
    {
        QMessageBox::warning(this, tr("Ecu definitions file"), "Unable to open OEM ecu base definitions file " + filename + " for reading");
        ecuCalDef = NULL;
        return NULL;
    }

    QDomDocument xmlBOM;
    xmlBOM.setContent(&file);
    file.close();

    QDomElement root = xmlBOM.documentElement();

    int i = 0;
    while (root.tagName() != "rom" && i < 5)
    {
        root = root.firstChild().toElement();
        i++;
    }
    while (!root.isNull())
    {
        if (root.tagName() == "rom")
        {
            rombase = root.attribute("base","No base");
            if (rombase == "No base")
            {
                QDomElement child = root.firstChild().toElement();
                while (!child.isNull())
                {
                    if (child.tagName() == "romid")
                    {
                        QDomElement sub_child = child.firstChild().toElement();
                        while (!sub_child.tagName().isNull())
                        {

                            if (sub_child.tagName() == "xmlid")
                                xmlid = sub_child.text();

                            sub_child = sub_child.nextSibling().toElement();
                        }
                    }
                    if (xmlid == ecuCalDef->RomInfo[RomBase])
                    {

                        OemEcuDefBaseFileFound = true;
                        if (child.tagName() == "scaling")
                        {
                            ecuCalDef->ScalingNameList.append(child.attribute("name"," "));
                            ecuCalDef->ScalingUnitsList.append(child.attribute("units"," "));
                            ecuCalDef->ScalingFromByteList.append(child.attribute("toexpr"," "));
                            ecuCalDef->ScalingToByteList.append(child.attribute("frexpr"," "));
                            ecuCalDef->ScalingFormatList.append(child.attribute("format"," "));
                            ecuCalDef->ScalingMinValueList.append(child.attribute("min"," "));
                            ecuCalDef->ScalingMaxValueList.append(child.attribute("max"," "));
                            ecuCalDef->ScalingCoarseIncList.append(child.attribute("inc"," "));
                            ecuCalDef->ScalingFineIncList.append(QString::number(ecuCalDef->ScalingCoarseIncList.at(scaling_index).toFloat() / 10.0f));
                            ecuCalDef->ScalingStorageTypeList.append(child.attribute("storagetype"," "));
                            ecuCalDef->ScalingEndianList.append(child.attribute("endian"," "));
                            QString selection_name;
                            QString selection_value;
                            if (child.attribute("storagetype"," ") == "bloblist")
                            {
                                QDomElement sub_child = child.firstChild().toElement();
                                while (!sub_child.isNull())
                                {
                                    if (sub_child.tagName() == "data"){
                                        selection_name.append(sub_child.attribute("name"," ") + ",");
                                        selection_value.append(sub_child.attribute("value"," ") + ",");
                                    }
                                    sub_child = sub_child.nextSibling().toElement();
                                }
                            }
                            if (selection_name == NULL)
                                selection_name.append(" ");
                            if (selection_value == NULL)
                                selection_value.append(" ");
                            ecuCalDef->ScalingSelectionsNameList.append(selection_name);
                            ecuCalDef->ScalingSelectionsValueList.append(selection_value);
                            scaling_index++;
                        }

                        else if (child.tagName() == "table")
                        {
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
                                            if (ecuCalDef->ScalingStorageTypeList.at(k) == "bloblist")
                                                ecuCalDef->TypeList.replace(i, "Selectable");
                                            ecuCalDef->StorageTypeList.replace(i, ecuCalDef->ScalingStorageTypeList.at(k));
                                            ecuCalDef->UnitsList.replace(i, ecuCalDef->ScalingUnitsList.at(k));
                                            ecuCalDef->FineIncList.replace(i, ecuCalDef->ScalingFineIncList.at(k));
                                            ecuCalDef->CoarseIncList.replace(i, ecuCalDef->ScalingCoarseIncList.at(k));
                                            ecuCalDef->MinValueList.replace(i, ecuCalDef->ScalingMinValueList.at(k));
                                            ecuCalDef->MaxValueList.replace(i, ecuCalDef->ScalingMaxValueList.at(k));
                                            ecuCalDef->EndianList.replace(i, ecuCalDef->ScalingEndianList.at(k));
                                            ecuCalDef->FromByteList.replace(i, ecuCalDef->ScalingFromByteList.at(k));
                                            ecuCalDef->ToByteList.replace(i, ecuCalDef->ScalingToByteList.at(k));
                                            ecuCalDef->FormatList.replace(i, convert_value_format(ecuCalDef->ScalingFormatList.at(k)));
                                        }
                                    }

                                    QDomElement sub_child = child.firstChild().toElement();
                                    QString TableDescription;
                                    QString TableStates;

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
                                                        ecuCalDef->XScaleFineIncList.replace(i, ecuCalDef->ScalingFineIncList.at(k));
                                                        ecuCalDef->XScaleCoarseIncList.replace(i, ecuCalDef->ScalingCoarseIncList.at(k));
                                                        ecuCalDef->XScaleMinValueList.replace(i, ecuCalDef->ScalingMinValueList.at(k));
                                                        ecuCalDef->XScaleMaxValueList.replace(i, ecuCalDef->ScalingMaxValueList.at(k));
                                                        ecuCalDef->XScaleEndianList.replace(i, ecuCalDef->ScalingEndianList.at(k));
                                                        ecuCalDef->XScaleFromByteList.replace(i, ecuCalDef->ScalingFromByteList.at(k));
                                                        ecuCalDef->XScaleToByteList.replace(i, ecuCalDef->ScalingToByteList.at(k));
                                                        ecuCalDef->XScaleFormatList.replace(i, convert_value_format(ecuCalDef->ScalingFormatList.at(k)));
                                                     }
                                                }
                                            }
                                            else if (ScaleType == "Y Axis" && ecuCalDef->TypeList.at(i) == "3D")
                                            {
                                                if (ecuCalDef->YSizeList.at(i) == "" || ecuCalDef->YSizeList.at(i) == " ")
                                                    ecuCalDef->YSizeList.replace(i, sub_child.attribute("elements", "1"));
                                                ecuCalDef->YScaleNameList.replace(i, sub_child.attribute("name"," "));
                                                ecuCalDef->YScaleTypeList.replace(i, ScaleType);
                                                ecuCalDef->YScaleScalingNameList.replace(i, sub_child.attribute("scaling"," "));
                                                ecuCalDef->YScaleLogParamList.replace(i, sub_child.attribute("logparam"," "));

                                                for (int k = 0; k < ecuCalDef->ScalingNameList.length(); k++)
                                                {
                                                    if (ecuCalDef->ScalingNameList.at(k) == ecuCalDef->YScaleScalingNameList.at(i))
                                                    {
                                                        ecuCalDef->YScaleStorageTypeList.replace(i, ecuCalDef->ScalingStorageTypeList.at(k));
                                                        ecuCalDef->YScaleUnitsList.replace(i, ecuCalDef->ScalingUnitsList.at(k));
                                                        ecuCalDef->YScaleFineIncList.replace(i, ecuCalDef->ScalingFineIncList.at(k));
                                                        ecuCalDef->YScaleCoarseIncList.replace(i, ecuCalDef->ScalingCoarseIncList.at(k));
                                                        ecuCalDef->YScaleMinValueList.replace(i, ecuCalDef->ScalingMinValueList.at(k));
                                                        ecuCalDef->YScaleMaxValueList.replace(i, ecuCalDef->ScalingMaxValueList.at(k));
                                                        ecuCalDef->YScaleEndianList.replace(i, ecuCalDef->ScalingEndianList.at(k));
                                                        ecuCalDef->YScaleFromByteList.replace(i, ecuCalDef->ScalingFromByteList.at(k));
                                                        ecuCalDef->YScaleToByteList.replace(i, ecuCalDef->ScalingToByteList.at(k));
                                                        ecuCalDef->YScaleFormatList.replace(i, convert_value_format(ecuCalDef->ScalingFormatList.at(k)));
                                                     }
                                                }
                                            }
                                            else if (ScaleType == "Static Y Axis" || (ScaleType == "Y Axis" && ecuCalDef->TypeList.at(i) == "2D"))
                                            {
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
                                                        ecuCalDef->XScaleFormatList.replace(i, ecuCalDef->ScalingFormatList.at(k));
                                                        ecuCalDef->XScaleFineIncList.replace(i, ecuCalDef->ScalingCoarseIncList.at(k));
                                                        ecuCalDef->XScaleCoarseIncList.replace(i, ecuCalDef->ScalingCoarseIncList.at(k));
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
                                                ecuCalDef->YSizeList[i] = "1";
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
                                        }
                                        sub_child = sub_child.nextSibling().toElement();
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

    return ecuCalDef;
}
*/
FileActions::EcuCalDefStructure *FileActions::read_ecuflash_ecu_def(EcuCalDefStructure *ecuCalDef, QString cal_id)
{
    ConfigValuesStructure *configValues = &ConfigValuesStruct;

    //bool ecuid_def_found = false;
    //bool inherits_another_def = false;
    bool cal_id_file_found = false;
    bool base_defined = false;
    bool map_defined = false;

    QString inherits_cal_id;
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

    int scaling_index = 0;

    int file_index = 0;
    if (!configValues->ecuflash_def_cal_id.length())
        return NULL;

    for (int index = 0; index < configValues->ecuflash_def_cal_id.length(); index++)
    {
        if (configValues->ecuflash_def_cal_id.at(index) == cal_id)
        {
            //qDebug() << "ID found:" << configValues->ecuflash_def_cal_id.at(index) << cal_id;
            //qDebug() << "File name:" << configValues->ecuflash_def_filename.at(index);
            cal_id_file_found = true;
            file_index = index;
            continue;
        }
    }

    //qDebug() << "File index:" << file_index;
    //qDebug() << "File name:" << configValues->ecuflash_def_filename.at(file_index);

    if (!cal_id_file_found)
        return ecuCalDef;

    ecuCalDef->use_ecuflash_definition = true;

    filename = configValues->ecuflash_def_filename.at(file_index);

    while (ecuCalDef->RomInfo.length() < RomInfoStrings.length())
        ecuCalDef->RomInfo.append(" ");

    QFile file(filename);

    if (!file.open(QIODevice::ReadOnly ))
    {
        QMessageBox::warning(this, tr("Ecu definitions file"), "Unable to open ECU definition file " + filename + " for reading");
        ecuCalDef = NULL;
        return NULL;
    }

    QDomDocument xmlBOM;
    xmlBOM.setContent(&file);
    file.close();

    QDomElement root = xmlBOM.documentElement();

    if (cal_id.contains("BASE"))
    {
        def_map_index = 0;
        base_defined = true;
    }
    else
        base_defined = false;

    qDebug() << "***";
    qDebug() << "*** ROM" << cal_id;
    qDebug() << "***";

    QDomNodeList rom_childs = xmlBOM.elementsByTagName("rom");
    QDomElement rom_child = rom_childs.at(0).toElement().firstChild().toElement();
    while (!rom_child.isNull())
    {
        if (rom_child.tagName() == "romid")
        {
            QDomElement rom_id_child = rom_child.firstChild().toElement();
            while (!rom_id_child.isNull())
            {
                //qDebug() << "ROM ID TAG childs:" << rom_id_child.tagName() << rom_id_child.text();
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
                    flashmethod = rom_id_child.text();
                else if (rom_id_child.tagName() == "filesize")
                    filesize = rom_id_child.text();

                rom_id_child = rom_id_child.nextSibling().toElement();
            }
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
        }
        else if (rom_child.tagName() == "include")
        {
            inherits_cal_id = rom_child.text();
            //qDebug() << "INCLUDE TAG childs:" << cal_id << "-> inherits" << inherits_cal_id;
            if (inherits_cal_id.contains("BASE"))
                ecuCalDef->RomInfo.replace(RomBase, inherits_cal_id);
        }
        else if (rom_child.tagName() == "scaling")
        {
            ecuCalDef->ScalingNameList.append(rom_child.attribute("name"," "));
            ecuCalDef->ScalingUnitsList.append(rom_child.attribute("units"," "));
            ecuCalDef->ScalingFromByteList.append(rom_child.attribute("toexpr"," "));
            ecuCalDef->ScalingToByteList.append(rom_child.attribute("frexpr"," "));
            ecuCalDef->ScalingFormatList.append(rom_child.attribute("format"," "));
            ecuCalDef->ScalingMinValueList.append(rom_child.attribute("min"," "));
            ecuCalDef->ScalingMaxValueList.append(rom_child.attribute("max"," "));
            ecuCalDef->ScalingCoarseIncList.append(rom_child.attribute("inc"," "));
            ecuCalDef->ScalingFineIncList.append(QString::number(ecuCalDef->ScalingCoarseIncList.at(scaling_index).toFloat() / 10.0f));
            ecuCalDef->ScalingStorageTypeList.append(rom_child.attribute("storagetype"," "));
            ecuCalDef->ScalingEndianList.append(rom_child.attribute("endian"," "));
            QString selection_name;
            QString selection_value;
            if (rom_child.attribute("storagetype"," ") == "bloblist")
            {
                QDomElement sub_child = rom_child.firstChild().toElement();
                while (!sub_child.isNull())
                {
                    if (sub_child.tagName() == "data"){
                        selection_name.append(sub_child.attribute("name"," ") + ",");
                        selection_value.append(sub_child.attribute("value"," ") + ",");
                    }
                    sub_child = sub_child.nextSibling().toElement();
                }
            }
            if (selection_name == NULL)
                selection_name.append(" ");
            if (selection_value == NULL)
                selection_value.append(" ");
            ecuCalDef->ScalingSelectionsNameList.append(selection_name);
            ecuCalDef->ScalingSelectionsValueList.append(selection_value);
            scaling_index++;
        }
        else if (rom_child.tagName() == "table")
        {
            map_defined = false;

            //qDebug() << "TABLE TAG childs:" << rom_child.tagName() << rom_child.attribute("name", "n/a");
            QString map_name = rom_child.attribute("name"," ");
            for (int i = 0; i < ecuCalDef->NameList.length(); i++)
            {
                if (ecuCalDef->NameList.at(i) == map_name)
                {
                    if (base_defined)
                        def_map_index = i;
                    map_defined = true;
                }
            }

            if (!map_defined && !base_defined)
            {
                add_ecuflash_def_list_item(ecuCalDef);
            }
            if ((!map_defined && !base_defined) || (map_defined && base_defined))
            {
                QString type = rom_child.attribute("type"," ");
                if (type == "1D" || type == "X Axis" || type == "Y Axis")
                    type = "2D";
                if (ecuCalDef->NameList.at(def_map_index) == " ")
                    ecuCalDef->NameList.replace(def_map_index, rom_child.attribute("name", " "));
                if (ecuCalDef->AddressList.at(def_map_index) == " ")
                    ecuCalDef->AddressList.replace(def_map_index, rom_child.attribute("address", ""));
                if (ecuCalDef->TypeList.at(def_map_index) == " ")
                    ecuCalDef->TypeList.replace(def_map_index, type);
                if (ecuCalDef->CategoryList.at(def_map_index) == " ")
                    ecuCalDef->CategoryList.replace(def_map_index, rom_child.attribute("category"," "));
                if (ecuCalDef->MapScalingNameList.at(def_map_index) == " ")
                    ecuCalDef->MapScalingNameList.replace(def_map_index, rom_child.attribute("scaling"," "));

                QDomElement rom_scale_child = rom_child.firstChild().toElement();
                int i = 0;
                while (!rom_scale_child.isNull())
                {
                    if (rom_scale_child.tagName() == "scaling")
                    {
                        //qDebug() << "Table scaling";
                        if (ecuCalDef->StorageTypeList.at(def_map_index) == " ")
                            ecuCalDef->StorageTypeList.replace(def_map_index, rom_scale_child.attribute("storagetype", " "));
                        if (ecuCalDef->UnitsList.at(def_map_index) == " ")
                            ecuCalDef->UnitsList.replace(def_map_index, rom_scale_child.attribute("units", " "));
                        if (ecuCalDef->CoarseIncList.at(def_map_index) == " ")
                            ecuCalDef->CoarseIncList.replace(def_map_index, rom_scale_child.attribute("inc", " "));
                        if (ecuCalDef->FineIncList.at(def_map_index) == " ")
                            //ecuCalDef->FineIncList.replace(def_map_index, rom_scale_child.attribute("inc", " "));
                            ecuCalDef->FineIncList.replace(def_map_index, QString::number(ecuCalDef->CoarseIncList.at(def_map_index).toFloat() / 10.0f));
                        if (ecuCalDef->MinValueList.at(def_map_index) == " ")
                            ecuCalDef->MinValueList.replace(def_map_index, rom_scale_child.attribute("min", " "));
                        if (ecuCalDef->MaxValueList.at(def_map_index) == " ")
                            ecuCalDef->MaxValueList.replace(def_map_index, rom_scale_child.attribute("max", " "));
                        if (ecuCalDef->EndianList.at(def_map_index) == " ")
                            ecuCalDef->EndianList.replace(def_map_index, rom_scale_child.attribute("endian", " "));
                        if (ecuCalDef->FromByteList.at(def_map_index) == " ")
                            ecuCalDef->FromByteList.replace(def_map_index, rom_scale_child.attribute("toexpr", " "));
                        if (ecuCalDef->ToByteList.at(def_map_index) == " ")
                            ecuCalDef->ToByteList.replace(def_map_index, rom_scale_child.attribute("frexpr", " "));
                        if (ecuCalDef->FormatList.at(def_map_index) == " ")
                            ecuCalDef->FormatList.replace(def_map_index, convert_value_format(rom_scale_child.attribute("format", " ")));

                        QString selection_name;
                        QString selection_value;
                        if (rom_scale_child.attribute("storagetype"," ") == "bloblist")
                        {
                            ecuCalDef->TypeList.replace(def_map_index, "Selectable");
                            QDomElement rom_scale_sub_child = rom_scale_child.firstChild().toElement();
                            while (!rom_scale_sub_child.isNull())
                            {
                                if (rom_scale_sub_child.tagName() == "data"){
                                    selection_name.append(rom_scale_sub_child.attribute("name"," ") + ",");
                                    selection_value.append(rom_scale_sub_child.attribute("value"," ") + ",");
                                }
                                rom_scale_sub_child = rom_scale_sub_child.nextSibling().toElement();
                            }
                        }
                        if (selection_name == NULL)
                            selection_name.append(" ");
                        if (selection_value == NULL)
                            selection_value.append(" ");
                        if (ecuCalDef->SelectionsNameList.at(def_map_index) == " ")
                            ecuCalDef->SelectionsNameList.replace(def_map_index, selection_name);
                        if (ecuCalDef->SelectionsValueList.at(def_map_index) == " ")
                            ecuCalDef->SelectionsValueList.replace(def_map_index, selection_value);
                    }
                    else if (rom_scale_child.tagName() == "table")
                    {
                        //qDebug() << "Scale scaling";
                        QString ScaleType = rom_scale_child.attribute("type"," ");
                        if (ScaleType == "X Axis")
                        {
                            if (ecuCalDef->XSizeList.at(def_map_index) == "" || ecuCalDef->XSizeList.at(def_map_index) == " ")
                                ecuCalDef->XSizeList.replace(def_map_index, rom_scale_child.attribute("elements", " "));
                            if (ecuCalDef->XScaleNameList.at(def_map_index) == " ")
                                ecuCalDef->XScaleNameList.replace(def_map_index, rom_scale_child.attribute("name"," "));
                            if (ecuCalDef->XScaleAddressList.at(def_map_index) == " ")
                                ecuCalDef->XScaleAddressList.replace(def_map_index, rom_scale_child.attribute("address", " "));
                            if (ecuCalDef->XScaleTypeList.at(def_map_index) == " ")
                                ecuCalDef->XScaleTypeList.replace(def_map_index, ScaleType);
                            if (ecuCalDef->XScaleScalingNameList.at(def_map_index) == " ")
                                ecuCalDef->XScaleScalingNameList.replace(def_map_index, rom_scale_child.attribute("scaling"," "));

                            if (ecuCalDef->NameList.at(def_map_index) == "Primary Open Loop Fueling (Failsafe)")
                                qDebug() << "X:" << def_map_index << cal_id << ecuCalDef->NameList.at(def_map_index) << ecuCalDef->XSizeList.at(def_map_index) << rom_scale_child.attribute("elements", " ");

                            QDomElement rom_scale_sub_child = rom_scale_child.firstChild().toElement();
                            if (rom_scale_sub_child.tagName() == "scaling")
                            {
                                //qDebug() << "Table scaling";
                                if (ecuCalDef->XScaleStorageTypeList.at(def_map_index) == " ")
                                    ecuCalDef->XScaleStorageTypeList.replace(def_map_index, rom_scale_sub_child.attribute("storagetype", " "));
                                if (ecuCalDef->XScaleUnitsList.at(def_map_index) == " ")
                                    ecuCalDef->XScaleUnitsList.replace(def_map_index, rom_scale_sub_child.attribute("units", " "));
                                if (ecuCalDef->XScaleCoarseIncList.at(def_map_index) == " ")
                                    ecuCalDef->XScaleCoarseIncList.replace(def_map_index, rom_scale_sub_child.attribute("inc", " "));
                                if (ecuCalDef->XScaleFineIncList.at(def_map_index) == " ")
                                    ecuCalDef->XScaleFineIncList.replace(def_map_index, QString::number(ecuCalDef->CoarseIncList.at(def_map_index).toFloat() / 10.0f));
                                if (ecuCalDef->XScaleMinValueList.at(def_map_index) == " ")
                                    ecuCalDef->XScaleMinValueList.replace(def_map_index, rom_scale_sub_child.attribute("min", " "));
                                if (ecuCalDef->XScaleMaxValueList.at(def_map_index) == " ")
                                    ecuCalDef->XScaleMaxValueList.replace(def_map_index, rom_scale_sub_child.attribute("max", " "));
                                if (ecuCalDef->XScaleEndianList.at(def_map_index) == " ")
                                    ecuCalDef->XScaleEndianList.replace(def_map_index, rom_scale_sub_child.attribute("endian", " "));
                                if (ecuCalDef->XScaleFromByteList.at(def_map_index) == " ")
                                    ecuCalDef->XScaleFromByteList.replace(def_map_index, rom_scale_sub_child.attribute("toexpr", " "));
                                if (ecuCalDef->XScaleToByteList.at(def_map_index) == " ")
                                    ecuCalDef->XScaleToByteList.replace(def_map_index, rom_scale_sub_child.attribute("frexpr", " "));
                                if (ecuCalDef->XScaleFormatList.at(def_map_index) == " ")
                                    ecuCalDef->XScaleFormatList.replace(def_map_index, convert_value_format(rom_scale_sub_child.attribute("format", " ")));
                            }
                        }
                        else if (ScaleType == "Y Axis" && ecuCalDef->TypeList.at(def_map_index) == "3D")
                        {
                            if (ecuCalDef->YSizeList.at(def_map_index) == " " || ecuCalDef->YSizeList.at(def_map_index) == "")
                                ecuCalDef->YSizeList.replace(def_map_index, rom_scale_child.attribute("elements", " "));
                            if (ecuCalDef->YScaleNameList.at(def_map_index) == " ")
                                ecuCalDef->YScaleNameList.replace(def_map_index, rom_scale_child.attribute("name"," "));
                            if (ecuCalDef->YScaleAddressList.at(def_map_index) == " ")
                                ecuCalDef->YScaleAddressList.replace(def_map_index, rom_scale_child.attribute("address", " "));
                            if (ecuCalDef->YScaleTypeList.at(def_map_index) == " ")
                                ecuCalDef->YScaleTypeList.replace(def_map_index, ScaleType);
                            if (ecuCalDef->YScaleScalingNameList.at(def_map_index) == " ")
                                ecuCalDef->YScaleScalingNameList.replace(def_map_index, rom_scale_child.attribute("scaling"," "));

                            if (ecuCalDef->NameList.at(def_map_index) == "Primary Open Loop Fueling (Failsafe)")
                                qDebug() << "Y:" << def_map_index << cal_id << ecuCalDef->NameList.at(def_map_index) << ecuCalDef->YSizeList.at(def_map_index) << rom_scale_child.attribute("elements", " ");;

                            QDomElement rom_scale_sub_child = rom_scale_child.firstChild().toElement();
                            if (rom_scale_sub_child.tagName() == "scaling")
                            {
                                //qDebug() << "Table scaling";
                                if (ecuCalDef->YScaleStorageTypeList.at(def_map_index) == " ")
                                    ecuCalDef->YScaleStorageTypeList.replace(def_map_index, rom_scale_sub_child.attribute("storagetype", " "));
                                if (ecuCalDef->YScaleUnitsList.at(def_map_index) == " ")
                                    ecuCalDef->YScaleUnitsList.replace(def_map_index, rom_scale_sub_child.attribute("units", " "));
                                if (ecuCalDef->YScaleCoarseIncList.at(def_map_index) == " ")
                                    ecuCalDef->YScaleCoarseIncList.replace(def_map_index, rom_scale_sub_child.attribute("inc", " "));
                                if (ecuCalDef->YScaleFineIncList.at(def_map_index) == " ")
                                    ecuCalDef->YScaleFineIncList.replace(def_map_index, QString::number(ecuCalDef->CoarseIncList.at(def_map_index).toFloat() / 10.0f));
                                if (ecuCalDef->YScaleMinValueList.at(def_map_index) == " ")
                                    ecuCalDef->YScaleMinValueList.replace(def_map_index, rom_scale_sub_child.attribute("min", " "));
                                if (ecuCalDef->YScaleMaxValueList.at(def_map_index) == " ")
                                    ecuCalDef->YScaleMaxValueList.replace(def_map_index, rom_scale_sub_child.attribute("max", " "));
                                if (ecuCalDef->YScaleEndianList.at(def_map_index) == " ")
                                    ecuCalDef->YScaleEndianList.replace(def_map_index, rom_scale_sub_child.attribute("endian", " "));
                                if (ecuCalDef->YScaleFromByteList.at(def_map_index) == " ")
                                    ecuCalDef->YScaleFromByteList.replace(def_map_index, rom_scale_sub_child.attribute("toexpr", " "));
                                if (ecuCalDef->YScaleToByteList.at(def_map_index) == " ")
                                    ecuCalDef->YScaleToByteList.replace(def_map_index, rom_scale_sub_child.attribute("frexpr", " "));
                                if (ecuCalDef->YScaleFormatList.at(def_map_index) == " ")
                                    ecuCalDef->YScaleFormatList.replace(def_map_index, convert_value_format(rom_scale_sub_child.attribute("format", " ")));
                            }
                        }
                        else if (ScaleType == "Static Y Axis" || (ScaleType == "Y Axis" && ecuCalDef->TypeList.at(def_map_index) == "2D"))
                        {
                            if (ecuCalDef->XSizeList.at(def_map_index) == " " || ecuCalDef->XSizeList.at(def_map_index) == "")
                                ecuCalDef->XSizeList.replace(def_map_index, rom_scale_child.attribute("elements", " "));
                            if (ecuCalDef->XScaleNameList.at(def_map_index) == " ")
                                ecuCalDef->XScaleNameList.replace(def_map_index, rom_scale_child.attribute("name"," "));
                            if (ecuCalDef->XScaleAddressList.at(def_map_index) == " ")
                                ecuCalDef->XScaleAddressList.replace(def_map_index, rom_scale_child.attribute("address", " "));
                            if (ecuCalDef->XScaleTypeList.at(def_map_index) == " ")
                                ecuCalDef->XScaleTypeList.replace(def_map_index, ScaleType);
                            if (ecuCalDef->XScaleScalingNameList.at(def_map_index) == " ")
                                ecuCalDef->XScaleScalingNameList.replace(def_map_index, rom_scale_child.attribute("scaling"," "));

                            QDomElement rom_scale_sub_child = rom_scale_child.firstChild().toElement();
                            if (rom_scale_sub_child.tagName() == "scaling")
                            {
                                //qDebug() << "Table scaling";
                                if (ecuCalDef->XScaleStorageTypeList.at(def_map_index) == " ")
                                    ecuCalDef->XScaleStorageTypeList.replace(def_map_index, rom_scale_sub_child.attribute("storagetype", " "));
                                if (ecuCalDef->XScaleUnitsList.at(def_map_index) == " ")
                                    ecuCalDef->XScaleUnitsList.replace(def_map_index, rom_scale_sub_child.attribute("units", " "));
                                if (ecuCalDef->XScaleCoarseIncList.at(def_map_index) == " ")
                                    ecuCalDef->XScaleCoarseIncList.replace(def_map_index, rom_scale_sub_child.attribute("inc", " "));
                                if (ecuCalDef->XScaleFineIncList.at(def_map_index) == " ")
                                    ecuCalDef->XScaleFineIncList.replace(def_map_index, QString::number(ecuCalDef->CoarseIncList.at(def_map_index).toFloat() / 10.0f));
                                if (ecuCalDef->XScaleMinValueList.at(def_map_index) == " ")
                                    ecuCalDef->XScaleMinValueList.replace(def_map_index, rom_scale_sub_child.attribute("min", " "));
                                if (ecuCalDef->XScaleMaxValueList.at(def_map_index) == " ")
                                    ecuCalDef->XScaleMaxValueList.replace(def_map_index, rom_scale_sub_child.attribute("max", " "));
                                if (ecuCalDef->XScaleEndianList.at(def_map_index) == " ")
                                    ecuCalDef->XScaleEndianList.replace(def_map_index, rom_scale_sub_child.attribute("endian", " "));
                                if (ecuCalDef->XScaleFromByteList.at(def_map_index) == " ")
                                    ecuCalDef->XScaleFromByteList.replace(def_map_index, rom_scale_sub_child.attribute("toexpr", " "));
                                if (ecuCalDef->XScaleToByteList.at(def_map_index) == " ")
                                    ecuCalDef->XScaleToByteList.replace(def_map_index, rom_scale_sub_child.attribute("frexpr", " "));
                                if (ecuCalDef->XScaleFormatList.at(def_map_index) == " ")
                                    ecuCalDef->XScaleFormatList.replace(def_map_index, convert_value_format(rom_scale_sub_child.attribute("format", " ")));
                            }
                            if (ScaleType == "Static Y Axis")
                            {
                                QDomElement rom_scale_child_data = rom_scale_child.firstChild().toElement();
                                QString StaticYScaleData;
                                while (!rom_scale_child_data.isNull())
                                {
                                    if (rom_scale_child_data.tagName() == "data"){
                                        StaticYScaleData.append(rom_scale_child_data.text());
                                        StaticYScaleData.append(",");
                                    }
                                    rom_scale_child_data = rom_scale_child_data.nextSibling().toElement();
                                }
                                ecuCalDef->XScaleStaticDataList.replace(def_map_index, StaticYScaleData);
                            }
                        }
                        else if (rom_scale_child.attribute("name"," ") != " ")
                        {
                            if (i == 0)
                            {
                                ecuCalDef->XScaleNameList.replace(def_map_index, rom_scale_child.attribute("name"," "));
                                ecuCalDef->XScaleAddressList.replace(def_map_index, rom_scale_child.attribute("address", " "));
                                ecuCalDef->XSizeList.replace(def_map_index, rom_scale_child.attribute("elements", " "));
                            }
                            if (i == 1)
                            {
                                ecuCalDef->YScaleNameList.replace(def_map_index, rom_scale_child.attribute("name"," "));
                                ecuCalDef->YScaleAddressList.replace(def_map_index, rom_scale_child.attribute("address", " "));
                                ecuCalDef->YSizeList.replace(def_map_index, rom_scale_child.attribute("elements", " "));
                            }
                            i++;
                            if (ecuCalDef->NameList.at(def_map_index) == "Primary Open Loop Fueling (Failsafe)")
                                qDebug() << i << ":" << def_map_index << cal_id << ecuCalDef->NameList.at(def_map_index) << ecuCalDef->XSizeList.at(def_map_index) << ecuCalDef->YSizeList.at(def_map_index);
                        }
                    }
                    rom_scale_child = rom_scale_child.nextSibling().toElement();
                }

                def_map_index++;
            }
        }

        rom_child = rom_child.nextSibling().toElement();
    }

    qDebug() << "CAL ID" << cal_id << "read";
    //qDebug() << "Scale types" << ecuCalDef->ScaleTypeList[0];
    read_ecuflash_ecu_def(ecuCalDef, inherits_cal_id);
    //ecuCalDef->use_ecuflash_definition = true;

    if (base_defined)
    {
        for (int def_map_index = 0; def_map_index < ecuCalDef->NameList.length(); def_map_index++)
        {
            if (ecuCalDef->YSizeList[def_map_index] == " ")
            {
                ecuCalDef->YSizeList.replace(def_map_index, "1");
                ecuCalDef->YScaleAddressList.replace(def_map_index, " ");
            }
            if (ecuCalDef->XSizeList[def_map_index] == " ")
            {
                ecuCalDef->XSizeList.replace(def_map_index, "1");
                ecuCalDef->XScaleAddressList.replace(def_map_index, " ");
                ecuCalDef->XScaleTypeList.replace(def_map_index, " ");
            }
        }
    }

    return ecuCalDef;
}

FileActions::EcuCalDefStructure *FileActions::parse_ecuflash_def_scalings(EcuCalDefStructure *ecuCalDef)
{
    for (def_map_index = 0; def_map_index < ecuCalDef->NameList.length(); def_map_index++)
    {
        for (int k = 0; k < ecuCalDef->ScalingNameList.length(); k++)
        {
            if (ecuCalDef->ScalingNameList.at(k) == ecuCalDef->MapScalingNameList.at(def_map_index))
            {
                if (ecuCalDef->ScalingStorageTypeList.at(k) == "bloblist")
                {
                    ecuCalDef->TypeList.replace(def_map_index, "Selectable");
                    ecuCalDef->SelectionsNameList.replace(def_map_index, ecuCalDef->ScalingSelectionsNameList.at(k));
                    ecuCalDef->SelectionsValueList.replace(def_map_index, ecuCalDef->ScalingSelectionsValueList.at(k));
                }
                ecuCalDef->StorageTypeList.replace(def_map_index, ecuCalDef->ScalingStorageTypeList.at(k));
                ecuCalDef->UnitsList.replace(def_map_index, ecuCalDef->ScalingUnitsList.at(k));
                ecuCalDef->FineIncList.replace(def_map_index, ecuCalDef->ScalingFineIncList.at(k));
                ecuCalDef->CoarseIncList.replace(def_map_index, ecuCalDef->ScalingCoarseIncList.at(k));
                ecuCalDef->MinValueList.replace(def_map_index, ecuCalDef->ScalingMinValueList.at(k));
                ecuCalDef->MaxValueList.replace(def_map_index, ecuCalDef->ScalingMaxValueList.at(k));
                ecuCalDef->EndianList.replace(def_map_index, ecuCalDef->ScalingEndianList.at(k));
                ecuCalDef->FromByteList.replace(def_map_index, ecuCalDef->ScalingFromByteList.at(k));
                ecuCalDef->ToByteList.replace(def_map_index, ecuCalDef->ScalingToByteList.at(k));
                ecuCalDef->FormatList.replace(def_map_index, convert_value_format(ecuCalDef->ScalingFormatList.at(k)));
            }
            if (ecuCalDef->ScalingNameList.at(k) == ecuCalDef->XScaleScalingNameList.at(def_map_index))
            {
                ecuCalDef->XScaleStorageTypeList.replace(def_map_index, ecuCalDef->ScalingStorageTypeList.at(k));
                ecuCalDef->XScaleUnitsList.replace(def_map_index, ecuCalDef->ScalingUnitsList.at(k));
                ecuCalDef->XScaleFineIncList.replace(def_map_index, ecuCalDef->ScalingFineIncList.at(k));
                ecuCalDef->XScaleCoarseIncList.replace(def_map_index, ecuCalDef->ScalingCoarseIncList.at(k));
                ecuCalDef->XScaleMinValueList.replace(def_map_index, ecuCalDef->ScalingMinValueList.at(k));
                ecuCalDef->XScaleMaxValueList.replace(def_map_index, ecuCalDef->ScalingMaxValueList.at(k));
                ecuCalDef->XScaleEndianList.replace(def_map_index, ecuCalDef->ScalingEndianList.at(k));
                ecuCalDef->XScaleFromByteList.replace(def_map_index, ecuCalDef->ScalingFromByteList.at(k));
                ecuCalDef->XScaleToByteList.replace(def_map_index, ecuCalDef->ScalingToByteList.at(k));
                ecuCalDef->XScaleFormatList.replace(def_map_index, convert_value_format(ecuCalDef->ScalingFormatList.at(k)));
            }
            if (ecuCalDef->ScalingNameList.at(k) == ecuCalDef->YScaleScalingNameList.at(def_map_index))
            {
                ecuCalDef->YScaleStorageTypeList.replace(def_map_index, ecuCalDef->ScalingStorageTypeList.at(k));
                ecuCalDef->YScaleUnitsList.replace(def_map_index, ecuCalDef->ScalingUnitsList.at(k));
                ecuCalDef->YScaleFineIncList.replace(def_map_index, ecuCalDef->ScalingFineIncList.at(k));
                ecuCalDef->YScaleCoarseIncList.replace(def_map_index, ecuCalDef->ScalingCoarseIncList.at(k));
                ecuCalDef->YScaleMinValueList.replace(def_map_index, ecuCalDef->ScalingMinValueList.at(k));
                ecuCalDef->YScaleMaxValueList.replace(def_map_index, ecuCalDef->ScalingMaxValueList.at(k));
                ecuCalDef->YScaleEndianList.replace(def_map_index, ecuCalDef->ScalingEndianList.at(k));
                ecuCalDef->YScaleFromByteList.replace(def_map_index, ecuCalDef->ScalingFromByteList.at(k));
                ecuCalDef->YScaleToByteList.replace(def_map_index, ecuCalDef->ScalingToByteList.at(k));
                ecuCalDef->YScaleFormatList.replace(def_map_index, convert_value_format(ecuCalDef->ScalingFormatList.at(k)));
            }
        }
    }
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
    ecuCalDef->SelectionsNameList.append(" ");
    ecuCalDef->SelectionsValueList.append(" ");
    ecuCalDef->DescriptionList.append(" ");
    ecuCalDef->StateList.append(" ");
    ecuCalDef->MapScalingNameList.append(" ");
    ecuCalDef->MapData.append(" ");

    ecuCalDef->ScaleTypeList[0].append(" ");
    ecuCalDef->ScaleNameList[0].append(" ");
    ecuCalDef->ScaleAddressList[0].append(" ");
    ecuCalDef->ScaleMinValueList[0].append(" ");
    ecuCalDef->ScaleMaxValueList[0].append(" ");
    ecuCalDef->ScaleUnitsList[0].append(" ");
    ecuCalDef->ScaleFormatList[0].append(" ");
    ecuCalDef->ScaleFineIncList[0].append(" ");
    ecuCalDef->ScaleCoarseIncList[0].append(" ");
    ecuCalDef->ScaleStorageTypeList[0].append(" ");
    ecuCalDef->ScaleEndianList[0].append(" ");
    ecuCalDef->ScaleLogParamList[0].append(" ");
    ecuCalDef->ScaleFromByteList[0].append(" ");
    ecuCalDef->ScaleToByteList[0].append(" ");
    ecuCalDef->ScaleStaticDataList[0].append(" ");
    ecuCalDef->ScaleScalingNameList[0].append(" ");
    ecuCalDef->ScaleData[0].append(" ");

    ecuCalDef->ScaleTypeList[1].append(" ");
    ecuCalDef->ScaleNameList[1].append(" ");
    ecuCalDef->ScaleAddressList[1].append(" ");
    ecuCalDef->ScaleMinValueList[1].append(" ");
    ecuCalDef->ScaleMaxValueList[1].append(" ");
    ecuCalDef->ScaleUnitsList[1].append(" ");
    ecuCalDef->ScaleFormatList[1].append(" ");
    ecuCalDef->ScaleFineIncList[1].append(" ");
    ecuCalDef->ScaleCoarseIncList[1].append(" ");
    ecuCalDef->ScaleStorageTypeList[1].append(" ");
    ecuCalDef->ScaleEndianList[1].append(" ");
    ecuCalDef->ScaleLogParamList[1].append(" ");
    ecuCalDef->ScaleFromByteList[1].append(" ");
    ecuCalDef->ScaleToByteList[1].append(" ");
    ecuCalDef->ScaleStaticDataList[1].append(" ");
    ecuCalDef->ScaleScalingNameList[1].append(" ");
    ecuCalDef->ScaleData[1].append(" ");

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
