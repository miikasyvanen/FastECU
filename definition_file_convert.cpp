#include "definition_file_convert.h"
#include <ui_definitionfileconvertwindow.h>

DefinitionFileConvert::DefinitionFileConvert(QWidget *parent)
    : QWidget(parent),
      ui(new Ui::DefinitionFileConvertWindow)
{
    ui->setupUi(this);
    this->setParent(parent);
    this->setAttribute(Qt::WA_QuitOnClose, true);

    convert_mappack_csv_file();

    this->close();
}

DefinitionFileConvert::~DefinitionFileConvert()
{

}

int DefinitionFileConvert::convert_mappack_csv_file()
{
    QString filename;
    QStringList titles;
    QStringList line_data;
    QStringList maps;
    QString line;
    QString format;
    QString column_data;
    QStringList map_names;

    int sizex = 0;
    int sizey = 0;

    QFileDialog openDialog;
    openDialog.setDefaultSuffix("*.csv");
    filename = QFileDialog::getOpenFileName(this, tr("Select MapPack CSV file"), NULL, tr("CSV file (*.csv)"));

    if (filename.isEmpty()){
        QMessageBox::information(this, tr("MapPack CSV file"), "No file selected");
        return STATUS_GENERAL_ERROR;
    }

    QFile source_file(filename);
    if (!source_file.open(QIODevice::ReadOnly ))
    {
        QMessageBox::warning(this, tr("MapPack CSV file"), "Unable to open MapPack CSV file for reading");
        return STATUS_FILE_OPEN_ERROR;
    }


    openDialog.setDefaultSuffix("*.xml");
    filename = QFileDialog::getSaveFileName(this, tr("Select RomRaider definition file"), NULL, tr("XML file (*.xml)"));

    if (filename.isEmpty()){
        QMessageBox::information(this, tr("RomRaider XML file"), "No file selected");
        return STATUS_GENERAL_ERROR;
    }

    if(!filename.endsWith(QString(".xml")))
         filename.append(QString(".xml"));

    QFile destination_file(filename);
    if (!destination_file.open(QIODevice::ReadWrite ))
    {
        QMessageBox::warning(this, tr("RomRaider XML file"), "Unable to open RomRaider XML file for writing");
        return STATUS_FILE_OPEN_ERROR;
    }

    QXmlStreamWriter stream(&destination_file);
    destination_file.resize(0);
    stream.setAutoFormatting(true);
    stream.writeStartDocument();
    stream.writeStartElement("roms");
        stream.writeStartElement("rom");
            stream.writeStartElement("romid");
                //stream.writeStartElement("xmlid");
                stream.writeTextElement("xmlid", "BASE");
                //stream.writeEndElement();
                stream.writeStartElement("internalidaddress");
                stream.writeEndElement();
                stream.writeStartElement("internalidstring");
                stream.writeEndElement();
                stream.writeStartElement("ecuid");
                stream.writeEndElement();
                stream.writeStartElement("year");
                stream.writeEndElement();
                stream.writeStartElement("market");
                stream.writeEndElement();
                stream.writeStartElement("make");
                stream.writeEndElement();
                stream.writeStartElement("model");
                stream.writeEndElement();
                stream.writeStartElement("submodel");
                stream.writeEndElement();
                stream.writeStartElement("transmission");
                stream.writeEndElement();
                stream.writeStartElement("memmodel");
                stream.writeEndElement();
                stream.writeStartElement("flashmethod");
                stream.writeEndElement();
                stream.writeStartElement("filesize");
                stream.writeEndElement();
                stream.writeStartElement("checksummodule");
                stream.writeEndElement();
            stream.writeEndElement();

            /*
             * Create BASE definition part
             */
            line = source_file.readLine();
            titles = line.split(";");

            bool is_comment = false;
            int index = 0;
            int line_index = 0;
            /*
            while (!source_file.atEnd())
            {
                line = source_file.readLine();
                line_data = line.split(";");
                map_names.append(line_data.at(0));
            }
            source_file.seek(0);
            line = source_file.readLine();
            */
            while (!source_file.atEnd())
            {
                line_index++;
                line_data.clear();
                line = source_file.readLine();
                //line_data = line.split(";");
                column_data.clear();
                for (int i = 0; i < line.length(); i++)
                {
                    if (line.at(i) == "\"" && index == 0) {
                        is_comment = true;
                        continue;
                    }
                    if (line.at(i) == "\"" && is_comment && index != 0) {
                        is_comment = false;
                        continue;
                    }

                    index++;

                    if (line.at(i) != ";" || is_comment)
                        column_data.append(line.at(i));
                    else {
                        line_data.append(column_data);
                        column_data.clear();
                        index = 0;
                        is_comment = false;
                    }

                }
/*
                for (int i = 0; i < line_data.length(); i++)
                {
                    if (line_data.at(i).startsWith("\""))
                    {
                        is_comment = true;

                        QString data_string = line_data.at(i);
                        data_string.remove(0, 1);
                        while (is_comment)
                        {
                            if (line_data.at(i).endsWith("\""))
                                break;
                            data_string.append(line_data.at(i + 1));
                            if (line_data.at(i + 1).endsWith("\""))
                                is_comment = false;
                            line_data.removeAt(i + 1);
                        }
                        data_string.remove(data_string.length() - 1, 1);
                        line_data.replace(i, data_string);
                    }
                }
                */
                // Check map size
                for (int i = 0; i < titles.count(); i++)
                {
                    if (titles.at(i) == "Columns")
                        sizex = line_data.at(i).toInt();
                    if (titles.at(i) == "Rows")
                        sizey = line_data.at(i).toInt();
                }
                // Add map table tag
                stream.writeStartElement("table");
                if ((sizex == 1 && sizey == 1) || (sizex == 1 && sizey > 1) || (sizex > 1 && sizey == 1))
                    stream.writeAttribute("type", "2D");
                else
                    stream.writeAttribute("type", "3D");
                for (int i = 0; i < titles.count(); i++)
                {
                    if (titles.at(i) == "Name")
                    {
                        stream.writeAttribute("name", line_data.at(i));
                    }
                    if (titles.at(i) == "FolderName")
                        stream.writeAttribute("category", line_data.at(i));
                    if (titles.at(i) == "DataOrg") {
                        if (line_data.at(i) == "eByte")
                        {
                            stream.writeAttribute("storagetype", "uint8");
                            stream.writeAttribute("endian", "big");
                        }
                        else if (line_data.at(i) == "eHiLo")
                        {
                            stream.writeAttribute("storagetype", "uint16");
                            stream.writeAttribute("endian", "big");
                        }
                        else if (line_data.at(i) == "eHiLoHiLo")
                        {
                            stream.writeAttribute("storagetype", "uint32");
                            stream.writeAttribute("endian", "big");
                        }
                        else
                        {
                            stream.writeAttribute("storagetype", "uint8");
                            stream.writeAttribute("endian", "big");
                        }
                    }
                    if (titles.at(i) == "Columns")
                        stream.writeAttribute("sizex", line_data.at(i));
                    if (titles.at(i) == "Rows")
                        stream.writeAttribute("sizey", line_data.at(i));

                }

                // Add map scaling tag
                stream.writeStartElement("scaling");
                for (int i = 0; i < titles.count(); i++)
                {
                    if (titles.at(i) == "Fieldvalues.Name")
                        stream.writeAttribute("units", line_data.at(i) + " (" + line_data.at(i + 1) + ")");
                    //Fieldvalues.Unit;
                    //Fieldvalues.Offset;
                }
                for (int i = 0; i < titles.count(); i++)
                {
                    if (titles.at(i) == "Fieldvalues.Factor")
                    {
                        QString data_string = line_data.at(i);
                        data_string.replace(",", ".");
                        stream.writeAttribute("expression", "x*" + data_string);
                        stream.writeAttribute("to_byte", "x/" + data_string);
                    }
                }
                for (int i = 0; i < titles.count(); i++)
                {
                    if (titles.at(i) == "Precision")
                    {
                        format = "#0.";
                        for (int m = 0; m < line_data.at(i).toInt(); m++)
                            format.append("0");
                        if (format.endsWith("."))
                            format = "#";
                        stream.writeAttribute("format", format);
                    }
                }
                stream.writeAttribute("fineincrement", "1");
                stream.writeAttribute("coarseincrement", "1");

                stream.writeEndElement();

                // Add X Axis table tag
                if (sizex > 1)
                {
                    stream.writeStartElement("table");
                    stream.writeAttribute("type", "X Axis");
                    for (int i = 0; i < titles.count(); i++)
                    {
                        if (titles.at(i) == "AxisX.Name")
                            stream.writeAttribute("name", line_data.at(i));
                        if (titles.at(i) == "AxisX.DataOrg") {
                            if (line_data.at(i) == "eByte")
                            {
                                stream.writeAttribute("storagetype", "uint8");
                                stream.writeAttribute("endian", "big");
                            }
                            else if (line_data.at(i) == "eHiLo")
                            {
                                stream.writeAttribute("storagetype", "uint16");
                                stream.writeAttribute("endian", "big");
                            }
                            else if (line_data.at(i) == "eHiLoHiLo")
                            {
                                stream.writeAttribute("storagetype", "uint32");
                                stream.writeAttribute("endian", "big");
                            }
                            else
                            {
                                stream.writeAttribute("storagetype", "uint8");
                                stream.writeAttribute("endian", "big");
                            }
                        }

                    }

                    // Add X Axis scaling tag
                    stream.writeStartElement("scaling");
                    for (int i = 0; i < titles.count(); i++)
                    {
                        if (titles.at(i) == "AxisX.Unit")
                            stream.writeAttribute("units", line_data.at(i));

                    }
                    for (int i = 0; i < titles.count(); i++)
                    {
                        if (titles.at(i) == "AxisX.Factor")
                        {
                            QString data_string = line_data.at(i);
                            data_string.replace(",", ".");
                            stream.writeAttribute("expression", "x*" + data_string);
                            stream.writeAttribute("to_byte", "x/" + data_string);
                        }
                    }
                    for (int i = 0; i < titles.count(); i++)
                    {
                        if (titles.at(i) == "AxisX.Precision")
                        {
                            format = "#0.";
                            for (int m = 0; m < line_data.at(i).toInt(); m++)
                                format.append("0");
                            if (format.endsWith("."))
                                format = "#";
                            stream.writeAttribute("format", format);
                        }
                    }
                    stream.writeAttribute("fineincrement", "1");
                    stream.writeAttribute("coarseincrement", "1");

                    stream.writeEndElement();

                    stream.writeEndElement();
                }
                // Add Y Axis table tag
                if (sizey > 1)
                {
                    stream.writeStartElement("table");
                    stream.writeAttribute("type", "Y Axis");
                    for (int i = 0; i < titles.count(); i++)
                    {
                        if (titles.at(i) == "AxisY.Name")
                            stream.writeAttribute("name", line_data.at(i));
                        if (titles.at(i) == "AxisY.DataOrg") {
                            if (line_data.at(i) == "eByte")
                            {
                                stream.writeAttribute("storagetype", "uint8");
                                stream.writeAttribute("endian", "big");
                            }
                            else if (line_data.at(i) == "eHiLo")
                            {
                                stream.writeAttribute("storagetype", "uint16");
                                stream.writeAttribute("endian", "big");
                            }
                            else if (line_data.at(i) == "eHiLoHiLo")
                            {
                                stream.writeAttribute("storagetype", "uint32");
                                stream.writeAttribute("endian", "big");
                            }
                            else
                            {
                                stream.writeAttribute("storagetype", "uint8");
                                stream.writeAttribute("endian", "big");
                            }
                        }

                    }

                    // Add Y Axis scaling tag
                    stream.writeStartElement("scaling");
                    for (int i = 0; i < titles.count(); i++)
                    {
                        if (titles.at(i) == "AxisY.Unit")
                            stream.writeAttribute("units", line_data.at(i));

                    }
                    for (int i = 0; i < titles.count(); i++)
                    {
                        if (titles.at(i) == "AxisY.Factor")
                        {
                            QString data_string = line_data.at(i);
                            data_string.replace(",", ".");
                            stream.writeAttribute("expression", "x*" + data_string);
                            stream.writeAttribute("to_byte", "x/" + data_string);
                        }
                    }
                    for (int i = 0; i < titles.count(); i++)
                    {
                        if (titles.at(i) == "AxisY.Precision")
                        {
                            format = "#0.";
                            for (int m = 0; m < line_data.at(i).toInt(); m++)
                                format.append("0");
                            if (format.endsWith("."))
                                format = "#";
                            stream.writeAttribute("format", format);
                        }
                    }
                    stream.writeAttribute("fineincrement", "1");
                    stream.writeAttribute("coarseincrement", "1");

                    stream.writeEndElement();

                    stream.writeEndElement();
                }
                if (sizex == 1 && sizey == 1)
                {
                    stream.writeStartElement("table");
                    stream.writeAttribute("type", "Static Y Axis");
                    stream.writeAttribute("name", " ");
                    stream.writeAttribute("sizey", "1");
                    stream.writeTextElement("data", "#");
                    stream.writeEndElement();
                }
                for (int i = 0; i < titles.count(); i++)
                {
                    if (titles.at(i) == "Comment")
                        stream.writeTextElement("description", line_data.at(i));
                }
                stream.writeEndElement();
            }

        stream.writeEndElement();

        QFileInfo fileInfo(source_file.fileName());
        QString ecuid(fileInfo.fileName().split(".").at(0));
        ecuid.remove(0, 4);

        stream.writeStartElement("rom");
            stream.writeAttribute("base", "BASE");
            stream.writeStartElement("romid");
                //stream.writeStartElement("xmlid");
                stream.writeTextElement("xmlid", ecuid);
                qDebug() << ecuid;
                //stream.writeEndElement();
                //stream.writeStartElement("internalidaddress");
                stream.writeTextElement("internalidaddress", "0x50");
                //stream.writeEndElement();
                //stream.writeStartElement("internalidstring");
                stream.writeTextElement("internalidstring", "1037369411P321/C51");
                //stream.writeEndElement();
                //stream.writeStartElement("ecuid");
                stream.writeTextElement("ecuid", ecuid);
                //stream.writeEndElement();
                stream.writeStartElement("year");
                stream.writeEndElement();
                stream.writeStartElement("market");
                stream.writeEndElement();
                stream.writeStartElement("make");
                stream.writeEndElement();
                stream.writeStartElement("model");
                stream.writeEndElement();
                stream.writeStartElement("submodel");
                stream.writeEndElement();
                stream.writeStartElement("transmission");
                stream.writeEndElement();
                stream.writeStartElement("memmodel");
                stream.writeEndElement();
                stream.writeStartElement("flashmethod");
                stream.writeEndElement();
                stream.writeStartElement("filesize");
                stream.writeEndElement();
                stream.writeStartElement("checksummodule");
                stream.writeEndElement();
            stream.writeEndElement();

            /*
             * Create definition part
             */
            source_file.seek(0);
            line = source_file.readLine();
            titles = line.split(";");
            line_index = 0;
            while (!source_file.atEnd())
            {
                line_index++;
                line_data.clear();
                line = source_file.readLine();
                //line_data = line.split(";");
                column_data.clear();
                for (int i = 0; i < line.length(); i++)
                {
                    if (line.at(i) == "\"" && index == 0) {
                        is_comment = true;
                        continue;
                    }
                    if (line.at(i) == "\"" && is_comment && index != 0) {
                        is_comment = false;
                        continue;
                    }

                    index++;

                    if (line.at(i) != ";" || is_comment)
                        column_data.append(line.at(i));
                    else {
                        line_data.append(column_data);
                        column_data.clear();
                        index = 0;
                        is_comment = false;
                    }

                }
                /*
                for (int i = 0; i < line_data.length(); i++)
                {
                    if (line_data.at(i).startsWith("\""))
                    {
                        is_comment = true;

                        QString data_string = line_data.at(i);
                        while (is_comment)
                        {
                            data_string.append(line_data.at(i + 1));
                            if (line_data.at(i + 1).endsWith("\""))
                                is_comment = false;
                            line_data.removeAt(i + 1);
                        }
                        line_data.replace(i, data_string);
                    }
                }
                */
                stream.writeStartElement("table");
                for (int i = 0; i < titles.count(); i++)
                {
                    if (titles.at(i) == "Name")
                        stream.writeAttribute("name", line_data.at(i));
                    if (titles.at(i) == "Fieldvalues.StartAddr")
                    {
                        QString storageaddress = line_data.at(i);
                        stream.writeAttribute("storageaddress", "0x" + storageaddress.remove("$"));
                    }
                }

                for (int i = 0; i < titles.count(); i++)
                {
                    if (titles.at(i) == "AxisX.DataHeader" && line_data.at(i).toInt() > 0)
                    {
                        stream.writeStartElement("table");
                        for (int i = 0; i < titles.count(); i++)
                        {
                            if (titles.at(i) == "AxisX.Name")
                                stream.writeAttribute("type", "X Axis");
                            if (titles.at(i) == "AxisX.DataAddr")
                            {
                                QString storageaddress = line_data.at(i);
                                stream.writeAttribute("storageaddress", "0x" + storageaddress.remove("$"));
                            }
                        }
                        stream.writeEndElement();
                    }
                }

                for (int i = 0; i < titles.count(); i++)
                {
                    if (titles.at(i) == "AxisY.DataHeader" && line_data.at(i).toInt() > 0)
                    {
                        stream.writeStartElement("table");
                        for (int i = 0; i < titles.count(); i++)
                        {
                            if (titles.at(i) == "AxisY.Name")
                                stream.writeAttribute("type", "Y Axis");
                            if (titles.at(i) == "AxisY.DataAddr")
                            {
                                QString storageaddress = line_data.at(i);
                                stream.writeAttribute("storageaddress", "0x" + storageaddress.remove("$"));
                            }
                        }
                        stream.writeEndElement();
                    }
                }

                stream.writeEndElement();
            }

        stream.writeEndElement();
    stream.writeEndElement();



    source_file.close();
    destination_file.close();

    return STATUS_SUCCESS;
}
