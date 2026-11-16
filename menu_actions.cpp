#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "serial_port_actions.h"

#include <algorithm>
#include <cmath>
#include <cstring>

void MainWindow::menu_action_triggered(QString action)
{
    // FILE MENU
    if (action == "new"){
        qDebug() << action;}
    if (action == "open_calibration")
        open_calibration_file(NULL);
    if (action == "save_calibration")
        save_calibration_file();
    if (action == "save_calibration_as")
        save_calibration_file_as();
    if (action == "close_calibration")
        close_calibration();
    if (action == "quit")
        close_app();

    // EDIT MENU
    if (action == "undo")
        qDebug() << action;
    if (action == "redo")
        qDebug() << action;
    if (action == "copy")
        copy_value();
    if (action == "paste")
        paste_value();
    if (action == "winols_csv_to_romraider_xml")
        winols_csv_to_romraider_xml();
    if (action == "settings")
        show_preferences_window();

    // TUNE MENU
    if (action == "fine_inc" || action == "fine_dec" || action == "coarse_inc" || action == "coarse_dec")
        inc_dec_value(action);
    if (action == "set_value")
        set_value();
    if (action == "interpolate_horizontal" || action == "interpolate_vertical" || action == "interpolate_bidirectional")
        interpolate_value(action);
    if (action == "toggle_realtime")
        toggle_realtime();
    if (action == "log_to_file")
        toggle_log_to_file();

    // ECU MENU
    if (action == "connect_to_ecu")
        connect_to_ecu();
    if (action == "disconnect_from_ecu")
        disconnect_from_ecu();
    if (action == "read_rom_from_ecu")
        start_ecu_operations("read");
    if (action == "test_write_rom_to_ecu")
        start_ecu_operations("test_write");
    if (action == "write_rom_to_ecu")
        start_ecu_operations("write");

    // VIEW MENU
    if (action == "setlogviews")
        change_gauge_values();

    // TESTING MENU
    if (action == "dtc_window")
        show_dtc_window();
    if (action == "hex_editor")
        show_hex_editor();

    if (action == "haltech_ic7")
        toggle_haltech_ic7_display();
    if (action == "simulate_obd")
        toggle_simulate_obd();
    if (action == "can_listener")
        toggle_can_listener();
    if (action == "biu_communication")
        show_subaru_biu_window();
    if (action == "get_key")
        show_subaru_get_key_window();
    if (action == "terminal")
        show_terminal_window();

    // HELP MENU
    if (action == "about")
        QMessageBox::information(this, tr("FastECU"),       "FastECU is open source tuning software for Subaru ECUs,\n"
                                                            "TCUs and also modifying BIU and ECUs of other car makes.\n"
                                                            "\n"
                                                            "This is beta test version for read and write ROMs via\n"
                                                            "K-Line and CAN connection with Open Port 2.0 or generic\n"
                                                            "OBD2 cable. Software is tested in Win7/Win10 32/64bit\n"
                                                            "and Linux amd64 and aarch64 platforms.\n"
                                                            "\n"
                                                            "There WILL be bugs and things that don't work. Be patient\n"
                                                            "with new versions relesed.\n"
                                                            "\n"
                                                            "All liability lies with the user. We are not responsible any\n"
                                                            "harm, laws broken or bricked ECUs that can follow for using\n"
                                                            "this software.\n"
                                                            "\n"
                                                            "\n"
                                                            "Huge thanks to following:\n"
                                                            "\n"
                                                            "fenugrec - author of nisprog software\n"
                                                            "rimwall - modifier of nisprog kernels for Subaru use\n"
                                                            "SergArb - testing and software development\n"
                                                            "alesv - testing and software development\n"
                                                            "jimihimisimi - testing and software development\n"
                                                            "\n"
                                                            "...and to all of you who had support software development by\n"
                                                            "donating! All, even the smallest amount of donates are welcome!\n"
                                 );
}

/*
 * Resolve a selection in the active map window (table widget coordinates) to the values
 * it shows: map data, X axis or Y axis. Uses the same layout as set_maptablewidget_items():
 * with YSize > 1 column 0 holds the Y axis, with XSize > 1 (or a static axis) row 0 holds
 * the X axis. The selection is clipped to the part its top-left cell is in.
 * t.rom_number and t.map_number must be set. Returns false if nothing editable is selected.
 */
bool MainWindow::get_map_edit_target(int table_row, int table_col, int table_last_row, int table_last_col, MapEditTarget &t)
{
    FileActions::EcuCalDefStructure *def = ecuCalDef[t.rom_number];
    int m = t.map_number;
    bool address_ok = false;
    int x_size = def->XSizeList.at(m).toInt();
    int y_size = def->YSizeList.at(m).toInt();
    bool x_static = def->XScaleTypeList.at(m) == "Static Y Axis" || def->XScaleTypeList.at(m) == "Static X Axis";
    int col_offset = (y_size > 1) ? 1 : 0;
    int row_offset = (x_size > 1 || x_static) ? 1 : 0;
    QString start_pos, interval;

    if (table_col == 0 && y_size > 1)
    {
        if (def->YScaleTypeList.at(m).startsWith("Static"))
            return false;
        t.part = MapEditTarget::YScale;
        t.cells = def->YScaleData.at(m).split(",");
        t.stride = 1;
        t.max_rows = y_size;
        t.max_cols = 1;
        t.first_row = table_row - 1;
        t.last_row = table_last_row - 1;
        t.first_col = 0;
        t.last_col = 0;
        t.storagetype = def->YScaleStorageTypeList.at(m);
        t.endian = def->YScaleEndianList.at(m);
        t.to_byte = def->YScaleToByteList.at(m);
        t.from_byte = def->YScaleFromByteList.at(m);
        t.min_value = def->YScaleMinValueList.at(m);
        t.max_value = def->YScaleMaxValueList.at(m);
        t.coarse_inc = def->YScaleCoarseIncList.at(m).toFloat();
        t.fine_inc = def->YScaleFineIncList.at(m).toFloat();
        t.address = def->YScaleAddressList.at(m).toUInt(&address_ok, 16);
        start_pos = def->YScaleStartPosList.at(m);
        interval = def->YScaleIntervalList.at(m);
    }
    else if (table_row == 0 && x_size > 1)
    {
        if (x_static)
            return false;
        t.part = MapEditTarget::XScale;
        t.cells = def->XScaleData.at(m).split(",");
        t.stride = x_size;
        t.max_rows = 1;
        t.max_cols = x_size;
        t.first_row = 0;
        t.last_row = 0;
        t.first_col = table_col - col_offset;
        t.last_col = table_last_col - col_offset;
        t.storagetype = def->XScaleStorageTypeList.at(m);
        t.endian = def->XScaleEndianList.at(m);
        t.to_byte = def->XScaleToByteList.at(m);
        t.from_byte = def->XScaleFromByteList.at(m);
        t.min_value = def->XScaleMinValueList.at(m);
        t.max_value = def->XScaleMaxValueList.at(m);
        t.coarse_inc = def->XScaleCoarseIncList.at(m).toFloat();
        t.fine_inc = def->XScaleFineIncList.at(m).toFloat();
        t.address = def->XScaleAddressList.at(m).toUInt(&address_ok, 16);
        start_pos = def->XScaleStartPosList.at(m);
        interval = def->XScaleIntervalList.at(m);
    }
    else
    {
        t.part = MapEditTarget::MapData;
        t.cells = def->MapData.at(m).split(",");
        t.stride = x_size;
        t.max_rows = y_size;
        t.max_cols = x_size;
        t.first_row = table_row - row_offset;
        t.last_row = table_last_row - row_offset;
        t.first_col = table_col - col_offset;
        t.last_col = table_last_col - col_offset;
        t.storagetype = def->StorageTypeList.at(m);
        t.endian = def->EndianList.at(m);
        t.to_byte = def->ToByteList.at(m);
        t.from_byte = def->FromByteList.at(m);
        t.min_value = def->MinValueList.at(m);
        t.max_value = def->MaxValueList.at(m);
        t.coarse_inc = def->CoarseIncList.at(m).toFloat();
        t.fine_inc = def->FineIncList.at(m).toFloat();
        t.address = def->AddressList.at(m).toUInt(&address_ok, 16);
        start_pos = def->StartPosList.at(m);
        interval = def->IntervalList.at(m);
    }

    static const QStringList editable_types = { "uint8", "uint16", "uint24", "uint32", "int8", "int16", "int24", "int32", "float" };
    if (!address_ok || !editable_types.contains(t.storagetype))
        return false;

    bool ok = false;
    t.start_pos = start_pos.toUInt(&ok, 16);
    if (!ok || t.start_pos == 0)
        t.start_pos = 1;
    t.interval = interval.toUInt(&ok, 16);
    if (!ok || t.interval == 0)
        t.interval = 1;

    t.first_row = qMax(0, t.first_row);
    t.first_col = qMax(0, t.first_col);
    t.last_row = qMin(t.max_rows - 1, t.last_row);
    t.last_col = qMin(t.max_cols - 1, t.last_col);
    if (t.first_row > t.last_row || t.first_col > t.last_col)
        return false;
    if (t.cells.count() < (t.max_rows - 1) * t.stride + t.max_cols)
        return false;

    return true;
}

void MainWindow::store_map_edit_target(const MapEditTarget &t)
{
    if (t.part == MapEditTarget::YScale)
        ecuCalDef[t.rom_number]->YScaleData.replace(t.map_number, t.cells.join(","));
    else if (t.part == MapEditTarget::XScale)
        ecuCalDef[t.rom_number]->XScaleData.replace(t.map_number, t.cells.join(","));
    else
        ecuCalDef[t.rom_number]->MapData.replace(t.map_number, t.cells.join(","));
}

/*
 * Convert a value in display units to the raw ROM value (as text) with the table's
 * to_byte expression. Integer storage types are rounded and limited to the range of
 * the storage type; *clamped tells if the value had to be limited.
 */
QString MainWindow::map_value_to_rom_value(const MapEditTarget &t, double map_value, bool *clamped)
{
    double raw = fileActions->calculate_value_from_expression(fileActions->parse_stringlist_from_expression_string(t.to_byte, QString::number(map_value, 'g', 10)));
    if (clamped)
        *clamped = false;

    if (t.storagetype == "float")
        return QString::number((float)raw, 'g', 9);

    int bits = 8;
    if (t.storagetype.endsWith("16"))
        bits = 16;
    else if (t.storagetype.endsWith("24"))
        bits = 24;
    else if (t.storagetype.endsWith("32"))
        bits = 32;

    double lo = 0;
    double hi = std::pow(2.0, bits) - 1;
    if (t.storagetype.startsWith("int"))
    {
        lo = -std::pow(2.0, bits - 1);
        hi = std::pow(2.0, bits - 1) - 1;
    }

    double value = std::round(raw);
    if (std::isnan(value))
        value = 0;
    if (value < lo || value > hi)
    {
        value = qBound(lo, value, hi);
        if (clamped)
            *clamped = true;
    }
    return QString::number((qint64)value);
}

static bool map_value_limit(const QString &min_value, const QString &max_value, double &value)
{
    bool limited = false;
    if (!min_value.trimmed().isEmpty() && value < min_value.toDouble()) {
        value = min_value.toDouble();
        limited = true;
    }
    if (!max_value.trimmed().isEmpty() && value > max_value.toDouble()) {
        value = max_value.toDouble();
        limited = true;
    }
    return limited;
}

void MainWindow::inc_dec_value(QString action)
{
    QMdiSubWindow* w = ui->mdiArea->activeSubWindow();
    if (!w)
        return;
    QTableWidget* mapTableWidget = w->findChild<QTableWidget*>(w->objectName());
    if (!mapTableWidget || mapTableWidget->selectedRanges().isEmpty())
        return;

    QStringList mapWindowString = w->objectName().split(",");
    QTableWidgetSelectionRange range = mapTableWidget->selectedRanges().first();
    MapEditTarget t;
    t.rom_number = mapWindowString.at(0).toInt();
    t.map_number = mapWindowString.at(1).toInt();
    if (!get_map_edit_target(range.topRow(), range.leftColumn(), range.bottomRow(), range.rightColumn(), t))
        return;

    double step = action.startsWith("coarse") ? t.coarse_inc : t.fine_inc;
    if (step == 0)
    {
        QMessageBox::warning(this, tr("Set value"), "Fine / Coarse inc value not set or set to zero in definition file!");
        return;
    }
    if (action.endsWith("_dec"))
        step = -step;

    for (int j = t.first_row; j <= t.last_row; j++)
    {
        for (int i = t.first_col; i <= t.last_col; i++)
        {
            int index = j * t.stride + i;
            double map_item_value = t.cells.at(index).toDouble();
            QString rom_data_value = get_rom_data_value(t.rom_number, t.address, index, t.storagetype, t.endian, t.interval, t.start_pos);
            QString new_rom_data_value = rom_data_value;

            // Step until the stored value changes: a step smaller than the ROM resolution
            // would otherwise round back to the same raw value. Stop at the limits.
            for (int n = 0; n < 1000; n++)
            {
                map_item_value += step;
                bool limited = map_value_limit(t.min_value, t.max_value, map_item_value);
                bool clamped = false;
                new_rom_data_value = map_value_to_rom_value(t, map_item_value, &clamped);
                if (new_rom_data_value.toDouble() != rom_data_value.toDouble() || limited || clamped)
                    break;
            }

            set_rom_data_value(t.rom_number, t.address, index, t.storagetype, t.endian, new_rom_data_value, t.interval, t.start_pos);
            double new_value = fileActions->calculate_value_from_expression(fileActions->parse_stringlist_from_expression_string(t.from_byte, new_rom_data_value));
            t.cells.replace(index, QString::number(new_value, 'g', 10));
        }
    }
    store_map_edit_target(t);
    set_maptablewidget_items();
}

void MainWindow::set_value()
{
    QMdiSubWindow* w = ui->mdiArea->activeSubWindow();
    if (!w)
        return;
    QTableWidget* mapTableWidget = w->findChild<QTableWidget*>(w->objectName());
    if (!mapTableWidget || mapTableWidget->selectedRanges().isEmpty())
        return;

    QStringList mapWindowString = w->objectName().split(",");
    QTableWidgetSelectionRange range = mapTableWidget->selectedRanges().first();
    MapEditTarget t;
    t.rom_number = mapWindowString.at(0).toInt();
    t.map_number = mapWindowString.at(1).toInt();
    if (!get_map_edit_target(range.topRow(), range.leftColumn(), range.bottomRow(), range.rightColumn(), t))
        return;

    bool ok = false;
    QString text = QInputDialog::getText(this, tr("Set value"),
                                         tr("Set value: (ie: x20 | *20 | +20 | -20 | /20 | 20)"),
                                         QLineEdit::Normal, "", &ok).trimmed();
    text.replace(",", ".");
    if (!ok || text.isEmpty())
        return;

    // First character + - x * / is the operation, otherwise the value is set as is
    QChar op = '=';
    QString number = text;
    if (QString("+-xX*/").contains(text.at(0)))
    {
        op = text.at(0).toLower();
        if (op == '*')
            op = 'x';
        number = text.mid(1).trimmed();
    }
    double operand = number.toDouble(&ok);
    if (!ok)
    {
        QMessageBox::warning(this, tr("Set value"), "Invalid value: " + text);
        return;
    }
    if (op == '/' && operand == 0)
    {
        QMessageBox::warning(this, tr("Set value"), "Cannot divide by zero!");
        return;
    }

    for (int j = t.first_row; j <= t.last_row; j++)
    {
        for (int i = t.first_col; i <= t.last_col; i++)
        {
            int index = j * t.stride + i;
            double map_item_value = t.cells.at(index).toDouble();

            if (op == '+')
                map_item_value += operand;
            else if (op == '-')
                map_item_value -= operand;
            else if (op == 'x')
                map_item_value *= operand;
            else if (op == '/')
                map_item_value /= operand;
            else
                map_item_value = operand;

            map_value_limit(t.min_value, t.max_value, map_item_value);
            QString rom_data_value = map_value_to_rom_value(t, map_item_value);
            set_rom_data_value(t.rom_number, t.address, index, t.storagetype, t.endian, rom_data_value, t.interval, t.start_pos);
            double new_value = fileActions->calculate_value_from_expression(fileActions->parse_stringlist_from_expression_string(t.from_byte, rom_data_value));
            t.cells.replace(index, QString::number(new_value, 'g', 10));
        }
    }
    store_map_edit_target(t);
    set_maptablewidget_items();
}

void MainWindow::interpolate_value(QString action)
{
    QMdiSubWindow* w = ui->mdiArea->activeSubWindow();
    if (!w)
        return;
    QTableWidget* mapTableWidget = w->findChild<QTableWidget*>(w->objectName());
    if (!mapTableWidget || mapTableWidget->selectedRanges().isEmpty())
        return;

    QStringList mapWindowString = w->objectName().split(",");
    QTableWidgetSelectionRange range = mapTableWidget->selectedRanges().first();
    MapEditTarget t;
    t.rom_number = mapWindowString.at(0).toInt();
    t.map_number = mapWindowString.at(1).toInt();
    if (!get_map_edit_target(range.topRow(), range.leftColumn(), range.bottomRow(), range.rightColumn(), t))
        return;

    int interpolateColCount = t.last_col - t.first_col + 1;
    int interpolateRowCount = t.last_row - t.first_row + 1;
    QVector<QVector<double>> cellValue(interpolateColCount, QVector<double>(interpolateRowCount));

    for (int j = 0; j < interpolateRowCount; j++)
        for (int i = 0; i < interpolateColCount; i++)
            cellValue[i][j] = t.cells.at((t.first_row + j) * t.stride + t.first_col + i).toDouble();

    double topLeftValue = cellValue[0][0];
    double topRightValue = cellValue[interpolateColCount - 1][0];
    double bottomLeftValue = cellValue[0][interpolateRowCount - 1];
    double bottomRightValue = cellValue[interpolateColCount - 1][interpolateRowCount - 1];

    double leftRowAdder = 0;
    double rightRowAdder = 0;
    if (interpolateRowCount > 1)
    {
        leftRowAdder = (bottomLeftValue - topLeftValue) / (interpolateRowCount - 1);
        rightRowAdder = (bottomRightValue - topRightValue) / (interpolateRowCount - 1);
    }

    if (action == "interpolate_horizontal" && interpolateColCount > 1)
    {
        for (int j = 0; j < interpolateRowCount; j++)
        {
            double colAdder = (cellValue[interpolateColCount - 1][j] - cellValue[0][j]) / (interpolateColCount - 1);
            for (int i = 0; i < interpolateColCount; i++)
                cellValue[i][j] = cellValue[0][j] + i * colAdder;
        }
    }
    if (action == "interpolate_vertical" && interpolateRowCount > 1)
    {
        for (int i = 0; i < interpolateColCount; i++)
        {
            double rowAdder = (cellValue[i][interpolateRowCount - 1] - cellValue[i][0]) / (interpolateRowCount - 1);
            for (int j = 0; j < interpolateRowCount; j++)
                cellValue[i][j] = cellValue[i][0] + j * rowAdder;
        }
    }
    if (action == "interpolate_bidirectional")
    {
        for (int j = 0; j < interpolateRowCount; j++)
        {
            cellValue[0][j] = topLeftValue + j * leftRowAdder;
            cellValue[interpolateColCount - 1][j] = topRightValue + j * rightRowAdder;
        }
        if (interpolateColCount > 1)
        {
            for (int j = 0; j < interpolateRowCount; j++)
            {
                double colAdder = (cellValue[interpolateColCount - 1][j] - cellValue[0][j]) / (interpolateColCount - 1);
                for (int i = 0; i < interpolateColCount; i++)
                    cellValue[i][j] = cellValue[0][j] + i * colAdder;
            }
        }
    }

    for (int j = 0; j < interpolateRowCount; j++)
    {
        for (int i = 0; i < interpolateColCount; i++)
        {
            int index = (t.first_row + j) * t.stride + t.first_col + i;
            QString rom_data_value = map_value_to_rom_value(t, cellValue[i][j]);
            set_rom_data_value(t.rom_number, t.address, index, t.storagetype, t.endian, rom_data_value, t.interval, t.start_pos);
            double new_value = fileActions->calculate_value_from_expression(fileActions->parse_stringlist_from_expression_string(t.from_byte, rom_data_value));
            t.cells.replace(index, QString::number(new_value, 'g', 10));
        }
    }
    store_map_edit_target(t);
    set_maptablewidget_items();
}

void MainWindow::copy_value()
{
    QMdiSubWindow* w = ui->mdiArea->activeSubWindow();
    if (!w)
        return;
    QTableWidget* mapTableWidget = w->findChild<QTableWidget*>(w->objectName());
    if (!mapTableWidget)
        return;

    // selectedIndexes() is not in any guaranteed order: sort to rows, then columns
    QModelIndexList cells = mapTableWidget->selectionModel()->selectedIndexes();
    std::sort(cells.begin(), cells.end(), [](const QModelIndex &a, const QModelIndex &b) {
        return a.row() != b.row() ? a.row() < b.row() : a.column() < b.column();
    });

    QString text;
    int currentRow = -1;
    for (const QModelIndex &cell : cells)
    {
        if (currentRow != -1)
            text += (cell.row() != currentRow) ? '\n' : '\t';
        currentRow = cell.row();
        text += cell.data().toString();
    }
    QApplication::clipboard()->setText(text);
}

void MainWindow::paste_value()
{
    QMdiSubWindow* w = ui->mdiArea->activeSubWindow();
    if (!w)
        return;
    QTableWidget* mapTableWidget = w->findChild<QTableWidget*>(w->objectName());
    if (!mapTableWidget || mapTableWidget->selectedRanges().isEmpty())
        return;

    // Clipboard from spreadsheets uses \r\n and ends with a line feed
    QString pasteString = QApplication::clipboard()->text();
    pasteString.replace("\r\n", "\n");
    while (pasteString.endsWith('\n'))
        pasteString.chop(1);
    if (pasteString.isEmpty())
        return;

    QList<QStringList> rows;
    int numColumns = 0;
    for (const QString &row : pasteString.split('\n'))
    {
        rows.append(row.split('\t'));
        numColumns = qMax(numColumns, (int)rows.last().count());
    }

    QStringList mapWindowString = w->objectName().split(",");
    QTableWidgetSelectionRange range = mapTableWidget->selectedRanges().first();
    MapEditTarget t;
    t.rom_number = mapWindowString.at(0).toInt();
    t.map_number = mapWindowString.at(1).toInt();
    if (!get_map_edit_target(range.topRow(), range.leftColumn(), range.topRow() + rows.count() - 1, range.leftColumn() + numColumns - 1, t))
        return;

    for (int j = 0; j < rows.count() && t.first_row + j <= t.last_row; j++)
    {
        for (int i = 0; i < rows.at(j).count() && t.first_col + i <= t.last_col; i++)
        {
            bool ok = false;
            double map_item_value = QString(rows.at(j).at(i)).trimmed().replace(",", ".").toDouble(&ok);
            if (!ok)
                continue;

            int index = (t.first_row + j) * t.stride + t.first_col + i;
            map_value_limit(t.min_value, t.max_value, map_item_value);
            QString rom_data_value = map_value_to_rom_value(t, map_item_value);
            set_rom_data_value(t.rom_number, t.address, index, t.storagetype, t.endian, rom_data_value, t.interval, t.start_pos);
            double new_value = fileActions->calculate_value_from_expression(fileActions->parse_stringlist_from_expression_string(t.from_byte, rom_data_value));
            t.cells.replace(index, QString::number(new_value, 'g', 10));
        }
    }
    store_map_edit_target(t);
    set_maptablewidget_items();
}

int MainWindow::connect_to_ecu()
{
    ecuid.clear();
    ecu_init_complete = false;
    set_status_bar_label(false, false, "");
    serial->reset_connection();

    qDebug() << "Opening interface, please wait...";
    open_serial_port();
    if (serial->is_serial_port_open())
    {
        serial_port_list->setDisabled(true);
        refresh_serial_port_list->setDisabled(true);
        qDebug() << "Initialising ECU, please wait...";
        int loopcount = 0;
        while (!ecu_init_complete && loopcount < 5)
        {
            ecu_init();
            delay(500);
            loopcount++;
        }
        if (!ecu_init_complete)
            disconnect_from_ecu();
    }
    else
    {
        QMessageBox::warning(this, tr("Serial port"), "Could not open interface!");
        return STATUS_ERROR;
    }
    return STATUS_SUCCESS;
}

void MainWindow::disconnect_from_ecu()
{
    qDebug() << "Disconnecting...";
    ecuid.clear();
    ecu_init_complete = false;
    set_status_bar_label(false, false, "");
    serial->reset_connection();

    serial->set_serial_port_baudrate("4800");
    serial->set_serial_port_parity(QSerialPort::NoParity);

    serial_port_list->setEnabled(true);
    refresh_serial_port_list->setEnabled(true);
}

void MainWindow::ecu_definition_manager()
{
    QDialog *definitions_manager_dialog = new QDialog;
    definitions_manager_dialog->setObjectName("ecu_definition_manager_dialog");
    definitions_manager_dialog->setFixedWidth(640);
    definitions_manager_dialog->setFixedHeight(240);
    definitions_manager_dialog->setWindowModality(Qt::ApplicationModal);
    definitions_manager_dialog->resize(800, 600);
    definitions_manager_dialog->setWindowTitle("ECU definition manager");
    definitions_manager_dialog->setAttribute(Qt::WA_DeleteOnClose);

    QVBoxLayout *definitions_manager_layout = new QVBoxLayout;
    definitions_manager_dialog->setLayout(definitions_manager_layout);

    QListWidget *definition_files = new QListWidget;
    definition_files->setObjectName("ecu_definition_files_list");
    definition_files->setSelectionMode(QAbstractItemView::ExtendedSelection);
    for (int i = 0; i < configValues->romraider_definition_files.length(); i++)
    {
        new QListWidgetItem(configValues->romraider_definition_files.at(i), definition_files);
    }
    definitions_manager_layout->addWidget(definition_files);

    QWidget *definitions_manager_widget = new QWidget;
    QHBoxLayout *definitions_manager_buttons = new QHBoxLayout;
    definitions_manager_layout->addWidget(definitions_manager_widget);
    definitions_manager_widget->setLayout(definitions_manager_buttons);

    QPushButton *add_new_file = new QPushButton("Add new file");
    QPushButton *remove_file = new QPushButton("Remove file");
    QPushButton *close = new QPushButton("Close");
    QSpacerItem *spacer = new QSpacerItem(20, 20, QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);

    definitions_manager_buttons->addWidget(add_new_file);
    definitions_manager_buttons->addWidget(remove_file);
    definitions_manager_buttons->addSpacerItem(spacer);
    definitions_manager_buttons->addWidget(close);

    connect(add_new_file, SIGNAL(clicked()), this, SLOT(add_new_ecu_definition_file()));
    connect(remove_file, SIGNAL(clicked()), this, SLOT(remove_ecu_definition_file()));
    connect(close, SIGNAL(clicked()), definitions_manager_dialog, SLOT(close()));
    definitions_manager_dialog->exec();

}

void MainWindow::logger_definition_manager()
{

}

void MainWindow::set_realtime_state(bool state)
{
    QList<QMenu*> menus = ui->menubar->findChildren<QMenu*>();
    foreach (QMenu *menu, menus) {
        foreach (QAction *action, menu->actions()) {
            if (action->isSeparator()) {

            } else if (action->menu()) {

            } else {
                if (action->text() == "Logging")
                {
                    action->setChecked(state);
                }
            }
        }
    }
}

void MainWindow::toggle_realtime()
{
    QAction *logger;
    QList<QMenu*> menus = ui->menubar->findChildren<QMenu*>();
    foreach (QMenu *menu, menus) {
        foreach (QAction *action, menu->actions()) {
            if (action->isSeparator()) {

            } else if (action->menu()) {

            } else {
                if (action->text() == "Logging")
                {
                    logger = action;
                    logging_state = logger->isChecked();
                }
            }
        }
    }

    if (logging_state)
    {
        qDebug() << "Start datalog";
        if (!ecu_init_complete)
        {
            if (connect_to_ecu())
            {
                QMessageBox::information(this, tr("ECU connection"), "Unable to connect to ECU");
                logger->setChecked(false);
                return;
            }
        }
        logging_counter = 0;
        logging_state = true;
        log_ssm_values();
        logparams_poll_timer->start();
    }
    else
    {
        qDebug() << "Stop datalog";
        if (datalog_file_open){
            datalog_file_open = false;
            datalog_file.close();
        }

        logging_state = false;
        log_params_request_started = false;
        log_ssm_values();
        delay(200);
        logparams_poll_timer->stop();

        //disconnect_from_ecu();
    }
}

void MainWindow::toggle_log_to_file()
{
    QList<QMenu*> menus = ui->menubar->findChildren<QMenu*>();
    foreach (QMenu *menu, menus) {
        foreach (QAction *action, menu->actions()) {
            if (action->isSeparator()) {

            } else if (action->menu()) {

            } else {
                if (action->text() == "Log to file")
                    write_datalog_to_file = action->isChecked();
            }
        }
    }

    if (!write_datalog_to_file){
        if (datalog_file_open){
            datalog_file_open = false;
            datalog_file.close();
        }
    }
}

void MainWindow::toggle_haltech_ic7_display()
{
    QList<QMenu*> menus = ui->menubar->findChildren<QMenu*>();
    foreach (QMenu *menu, menus) {
        foreach (QAction *action, menu->actions()) {
            if (action->isSeparator()) {

            } else if (action->menu()) {

            } else {
                if (action->text() == "Haltech IC-7")
                    haltech_ic7_display_on = action->isChecked();
            }
        }
    }
    if (haltech_ic7_display_on)
        test_haltech_ic7_display();
}

void MainWindow::toggle_simulate_obd()
{
    QList<QMenu*> menus = ui->menubar->findChildren<QMenu*>();
    foreach (QMenu *menu, menus) {
        foreach (QAction *action, menu->actions()) {
            if (action->isSeparator()) {

            } else if (action->menu()) {

            } else {
                if (action->text() == "Simulate OBD")
                    simulate_obd_on = action->isChecked();
            }
        }
    }
    if (simulate_obd_on)
        simulate_obd();
}

void MainWindow::toggle_can_listener()
{
    QList<QMenu*> menus = ui->menubar->findChildren<QMenu*>();
    foreach (QMenu *menu, menus) {
        foreach (QAction *action, menu->actions()) {
            if (action->isSeparator()) {

            } else if (action->menu()) {

            } else {
                if (action->text() == "CAN listener")
                    can_listener_on = action->isChecked();
            }
        }
    }
    if (can_listener_on)
        can_listener();
}

void MainWindow::show_dtc_window()
{
    serial->reset_connection();
    ecuid.clear();
    ecu_init_complete = false;

    QStringList spl;
    spl.append(serial_ports.at(serial_port_list->currentIndex()));
    serial->set_serial_port_list(spl);

    emit LOG_D("Starting DTC operations", true, true);

    DtcOperations dtcOperations(serial, this);
    QObject::connect(&dtcOperations, &DtcOperations::LOG_E, syslogger, &SystemLogger::log_messages);
    QObject::connect(&dtcOperations, &DtcOperations::LOG_W, syslogger, &SystemLogger::log_messages);
    QObject::connect(&dtcOperations, &DtcOperations::LOG_I, syslogger, &SystemLogger::log_messages);
    QObject::connect(&dtcOperations, &DtcOperations::LOG_D, syslogger, &SystemLogger::log_messages);

    dtcOperations.exec();
    //dtcOperations->run();

    emit LOG_D("DTC operations stopped", true, true);
}

void MainWindow::show_hex_editor()
{
    emit LOG_D("Show hex editor", true, true);

    int rom_number = 0;

    QTreeWidgetItem *selectedItem = NULL;
    int item_count = ui->calibrationFilesTreeWidget->selectedItems().count();
    if (item_count)
    {
        selectedItem = ui->calibrationFilesTreeWidget->selectedItems().at(0);
        rom_number = ui->calibrationFilesTreeWidget->indexOfTopLevelItem(selectedItem);

        //HexEdit *hexEdit = new HexEdit(ecuCalDef[rom_number], this);
        new HexEdit(ecuCalDef[rom_number], this);
    }

}

void MainWindow::show_preferences_window()
{
    Settings settings(configValues);
    settings.exec();
    //fileActions->save_config_file();

}

void MainWindow::show_subaru_biu_window()
{
    logging_poll_timer->stop();

    serial->reset_connection();
    ecuid.clear();
    ecu_init_complete = false;
    serial->set_add_iso14230_header(false);
    serial->set_is_iso14230_connection(true);
    open_serial_port();
    serial->change_port_speed("10400");
    //serial->change_port_speed("4800");

    BiuOperationsSubaru biuOperationsSubaru(serial, this);
    QObject::connect(&biuOperationsSubaru, &BiuOperationsSubaru::LOG_E, syslogger, &SystemLogger::log_messages);
    QObject::connect(&biuOperationsSubaru, &BiuOperationsSubaru::LOG_W, syslogger, &SystemLogger::log_messages);
    QObject::connect(&biuOperationsSubaru, &BiuOperationsSubaru::LOG_I, syslogger, &SystemLogger::log_messages);
    QObject::connect(&biuOperationsSubaru, &BiuOperationsSubaru::LOG_D, syslogger, &SystemLogger::log_messages);

    biuOperationsSubaru.exec();

    emit LOG_D("BIU stopped", true, true);

    serial->set_add_iso14230_header(false);
}

void MainWindow::show_terminal_window()
{
    QStringList serial_port;
    serial_port.append(serial_ports.at(serial_port_list->currentIndex()));
    serial->set_serial_port_list(serial_port);
    DataTerminal hexCommander(serial, this);
    QObject::connect(&hexCommander, &DataTerminal::LOG_E, syslogger, &SystemLogger::log_messages);
    QObject::connect(&hexCommander, &DataTerminal::LOG_W, syslogger, &SystemLogger::log_messages);
    QObject::connect(&hexCommander, &DataTerminal::LOG_I, syslogger, &SystemLogger::log_messages);
    QObject::connect(&hexCommander, &DataTerminal::LOG_D, syslogger, &SystemLogger::log_messages);

    hexCommander.exec();
}

void MainWindow::show_subaru_get_key_window()
{

    GetKeyOperationsSubaru getKeyOperationsSubaru (this);
    getKeyOperationsSubaru.exec();
}

void MainWindow::winols_csv_to_romraider_xml()
{
    DefinitionFileConvert definitionFileMaker;
    definitionFileMaker.exec();
}

void MainWindow::set_maptablewidget_items()
{
    int mapRomNumber = 0;
    int mapNumber = 0;
    QString mapName = "";

    QMdiSubWindow* w = ui->mdiArea->activeSubWindow();
    if (w)
    {
        QStringList mapWindowString = w->objectName().split(",");
        mapRomNumber = mapWindowString.at(0).toInt();
        mapNumber = mapWindowString.at(1).toInt();
        mapName = mapWindowString.at(2);

        QTableWidget* mapTableWidget = w->findChild<QTableWidget*>(w->objectName());
        if (mapTableWidget)
        {
            int xSize = ecuCalDef[mapRomNumber]->XSizeList.at(mapNumber).toInt();
            int ySize = ecuCalDef[mapRomNumber]->YSizeList.at(mapNumber).toInt();
            int mapSize = xSize * ySize;

            int xSizeOffset = 0;
            int ySizeOffset = 0;

            if (ecuCalDef[mapRomNumber]->YSizeList.at(mapNumber).toInt() > 1)
                xSizeOffset = 1;
            if (ecuCalDef[mapRomNumber]->XSizeList.at(mapNumber).toInt() > 1 || ecuCalDef[mapRomNumber]->XScaleTypeList.at(mapNumber) == "Static Y Axis" || ecuCalDef[mapRomNumber]->XScaleTypeList.at(mapNumber) == "Static X Axis")
                ySizeOffset = 1;

            QFont cellFont = mapTableWidget->font();
            cellFont.setPointSize(cellFontSize);
            cellFont.setFamily("Franklin Gothic");

            if (xSize > 1)
            {
                QStringList xScaleCellText = ecuCalDef[mapRomNumber]->XScaleData.at(mapNumber).split(",");
                for (int i = 0; i < xSize; i++)
                {
                    QTableWidgetItem *cellItem;
                    if (ySize > 1)
                        cellItem = mapTableWidget->item(0, i + 1);
                    else
                        cellItem = mapTableWidget->item(0, i);

                    cellItem->setTextAlignment(Qt::AlignCenter);
                    cellItem->setFont(cellFont);

                    QString xScaleCellDataText;

                    if (xScaleCellText.at(i) == " ") {
                        xScaleCellText.insert(i, QString::number(i));
                        xScaleCellDataText = xScaleCellText.at(i);
                    }
                    else if (ecuCalDef[mapRomNumber]->XScaleTypeList.at(mapNumber) == "Static Y Axis" || ecuCalDef[mapRomNumber]->XScaleTypeList.at(mapNumber) == "Static X Axis")
                        xScaleCellDataText = xScaleCellText.at(i);
                    else
                        xScaleCellDataText = QString::number(xScaleCellText.at(i).toFloat(), 'f', get_mapvalue_decimal_count(ecuCalDef[mapRomNumber]->XScaleFormatList.at(mapNumber)));

                    if (i < xScaleCellText.count())
                        cellItem->setText(xScaleCellDataText);
                }
            }
            if (ySize > 1)
            {
                QStringList yScaleCellText = ecuCalDef[mapRomNumber]->YScaleData.at(mapNumber).split(",");
                for (int i = 0; i < ySize; i++)
                {
                    QTableWidgetItem *cellItem;
                    cellItem = mapTableWidget->item(i + 1, 0);

                    cellItem->setTextAlignment(Qt::AlignCenter);
                    cellItem->setFont(cellFont);
                    if (i < yScaleCellText.count())
                        cellItem->setText(QString::number(yScaleCellText.at(i).toFloat(), 'f', get_mapvalue_decimal_count(ecuCalDef[mapRomNumber]->YScaleFormatList.at(mapNumber))));
                }
            }
            QStringList mapDataCellText = ecuCalDef[mapRomNumber]->MapData.at(mapNumber).split(",");
            for (int i = 0; i < mapSize; i++)
            {
                int yPos = 0;
                int xPos = 0;
                if (ecuCalDef[mapRomNumber]->XSizeList.at(mapNumber).toUInt() > 1)
                    yPos = i / xSize + ySizeOffset;
                else
                    yPos = i / xSize;
                if (ecuCalDef[mapRomNumber]->YSizeList.at(mapNumber).toUInt() > 1)
                    xPos = i - (yPos - ySizeOffset) * xSize + xSizeOffset;
                else
                    xPos = i - (yPos - ySizeOffset) * xSize;

                //qDebug() << "X pos:" << xPos << "Y pos:" << yPos;
                QTableWidgetItem *cellItem;// = new QTableWidgetItem;
                cellItem = mapTableWidget->item(yPos, xPos);

                cellItem->setTextAlignment(Qt::AlignCenter);
                cellItem->setFont(cellFont);
                int mapItemColor = get_map_cell_colors(ecuCalDef[mapRomNumber], mapDataCellText.at(i).toFloat(), mapNumber);
                int mapItemColorRed = (mapItemColor >> 16) & 0xff;
                int mapItemColorGreen = (mapItemColor >> 8) & 0xff;
                int mapItemColorBlue = mapItemColor & 0xff;
                cellItem->setBackground(QBrush(QColor(mapItemColorRed , mapItemColorGreen, mapItemColorBlue, 255)));
                //if (ecuCalDef[mapRomNumber]->TypeList.at(mapNumber) == "1D")
                    cellItem->setForeground(Qt::black);
                //else
                //    cellItem->setForeground(Qt::white);

                if (i < mapDataCellText.count())
                    cellItem->setText(QString::number(mapDataCellText.at(i).toFloat(), 'f', get_mapvalue_decimal_count(ecuCalDef[mapRomNumber]->FormatList.at(mapNumber))));
            }
        }

        //fileActions->checksum_correction(ecuCalDef[mapRomNumber]);

    }
}

static int rom_storage_size(const QString &storagetype)
{
    if (storagetype == "uint16" || storagetype == "int16")
        return 2;
    if (storagetype == "uint24" || storagetype == "int24")
        return 3;
    if (storagetype == "uint32" || storagetype == "int32" || storagetype == "float")
        return 4;
    return 1;
}

/*
 * Read one raw value of a table from the ROM, as text (integer, or float for float storage).
 * Same addressing and byte order as the table parsing in FileActions: big endian unless
 * endian is "little", floats are always big endian.
 */
QString MainWindow::get_rom_data_value(uint8_t rom_number, uint32_t data_address, uint16_t value_index, QString storagetype, QString endian, uint32_t interval, uint32_t start_pos)
{
    int storagesize = rom_storage_size(storagetype);
    uint32_t byte_address = data_address + (value_index * interval + (start_pos - 1)) * storagesize;
    if (ecuCalDef[rom_number]->RomInfo.at(FlashMethod) == "wrx02" && (uint32_t)ecuCalDef[rom_number]->FullRomData.length() <= byte_address)
        byte_address -= 0x8000;
    if (byte_address + storagesize > (uint32_t)ecuCalDef[rom_number]->FullRomData.length())
    {
        emit LOG_E("get_rom_data_value: address 0x" + QString::number(byte_address, 16) + " is outside the ROM", true, true);
        return QString();
    }

    uint32_t raw = 0;
    for (int k = 0; k < storagesize; k++)
    {
        uint32_t data_byte = (uint8_t)ecuCalDef[rom_number]->FullRomData.at(byte_address + k);
        if (endian == "little" && storagetype != "float")
            raw |= data_byte << (8 * k);
        else
            raw = (raw << 8) | data_byte;
    }

    if (storagetype == "float")
    {
        float value;
        std::memcpy(&value, &raw, sizeof(value));
        return QString::number(value, 'g', 9);
    }
    if (storagetype.startsWith("int"))
    {
        qint64 value = raw;
        if (raw & (1u << (8 * storagesize - 1)))
            value -= (qint64)1 << (8 * storagesize);
        return QString::number(value);
    }
    return QString::number(raw);
}

/*
 * Write one raw value (text: integer, or float for float storage) of a table to the ROM.
 * Byte order and addressing as in get_rom_data_value().
 */
void MainWindow::set_rom_data_value(uint8_t rom_number, uint32_t data_address, uint16_t value_index, QString storagetype, QString endian, QString rom_value, uint32_t interval, uint32_t start_pos)
{
    int storagesize = rom_storage_size(storagetype);
    uint32_t byte_address = data_address + (value_index * interval + (start_pos - 1)) * storagesize;
    if (ecuCalDef[rom_number]->RomInfo.at(FlashMethod) == "wrx02" && (uint32_t)ecuCalDef[rom_number]->FullRomData.length() <= byte_address)
        byte_address -= 0x8000;
    if (byte_address + storagesize > (uint32_t)ecuCalDef[rom_number]->FullRomData.length())
    {
        emit LOG_E("set_rom_data_value: address 0x" + QString::number(byte_address, 16) + " is outside the ROM", true, true);
        return;
    }

    uint32_t raw = 0;
    if (storagetype == "float")
    {
        float value = rom_value.toFloat();
        std::memcpy(&raw, &value, sizeof(raw));
    }
    else
        raw = (uint32_t)qRound64(rom_value.toDouble());

    emit LOG_D("set_rom_data_value: 0x" + QString::number(raw, 16) + " | 0x" + QString::number(value_index, 16), true, true);

    for (int k = 0; k < storagesize; k++)
    {
        if (endian == "little" && storagetype != "float")
            ecuCalDef[rom_number]->FullRomData[byte_address + k] = (char)((raw >> (8 * k)) & 0xff);
        else
            ecuCalDef[rom_number]->FullRomData[byte_address + k] = (char)((raw >> (8 * (storagesize - 1 - k))) & 0xff);
    }
}

int MainWindow::get_mapvalue_decimal_count(QString valueFormat)
{
    if (valueFormat.contains("."))
        return valueFormat.split(".").at(1).count(QLatin1Char('0'));
    else
        return 0;//valueFormat.count(QLatin1Char('0'));
}

int MainWindow::get_map_cell_colors(FileActions::EcuCalDefStructure *ecuCalDef, float mapDataValue, int mapIndex)
{
    int mapCellColors;
    float mapMinValue = 0;
    float mapMaxValue = 0;
    float scale_start = (210.0/360.0);

    mapMinValue = ecuCalDef->MapCellColorMin.at(mapIndex).toFloat();
    mapMaxValue = ecuCalDef->MapCellColorMax.at(mapIndex).toFloat();

    QColor color;
    float color_scale = (1 - (mapDataValue - mapMinValue) / (mapMaxValue - mapMinValue)) * scale_start;
    float color_value = scale_start - color_scale;
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    double r = 0;
    double g = 0;
    double b = 0;
#else
    float r = 0;
    float g = 0;
    float b = 0;
#endif

    if (color_value < 0)
        color_value = 0;

    color.setHsvF(color_value, 0.85, 0.85);
    color.getRgbF(&r, &g, &b);
    mapCellColors = ((int)(r*255)<<16) + ((int)(g*255)<<8) + b*255;

    //qDebug() << "Map min:" << mapMinValue << "Map max:" << mapMaxValue << "color scale:" << color_scale << "scale start:" << scale_start;

    return mapCellColors;
}

int MainWindow::test_haltech_ic7_display()
{
    QByteArray output;
    QByteArray received;

    uint16_t temp_base = 2300;

    uint16_t RPM = 0;
    uint16_t MAP = 0;
    uint16_t TPS = 0;

    uint16_t IDC = 0;
    uint16_t IGN = 0;

    uint16_t SPD = 0;
    uint16_t GEAR = 0;

    uint16_t BATT = 0;

    uint16_t CLT = 0;
    uint16_t IAT = 0;
    uint16_t FLT = 0;
    uint16_t OLT = 0;

    int i = 0;

    //serial_poll_timer->stop();
    //ssm_init_poll_timer->stop();
    logging_poll_timer->stop();

    serial->set_is_iso15765_connection(true);
    //serial->set_is_can_connection(true);
    serial->set_is_29_bit_id(false);
    serial->set_can_speed("1000000");

    serial->reset_connection();
    ecuid.clear();
    ecu_init_complete = false;
    open_serial_port();

    qDebug() << "Send data to Haltech IC-7 display";

    while (haltech_ic7_display_on)
    {
        i = 0;
        while (i < 101)
        {
            RPM = i * 60;
            MAP = i * 10;
            TPS = i * 10;

            IDC = i * 10;
            IGN = i * 5;

            SPD = i * 10;
            GEAR = i / 20 + 1;
            BATT = i / 2 + 100;

            CLT = temp_base + i * 10;
            IAT = temp_base + i * 10;
            FLT = temp_base + i * 10;
            OLT = temp_base + i * 10;

            qDebug() << "MAP:" << MAP << "TPS:" << TPS;

            output.clear();
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x03);
            output.append((uint8_t)0x60);
            //output.append((uint8_t)0x00 & 0xFF);
            //output.append((uint8_t)0x00 & 0xFF);
            output.append((uint8_t)(RPM >> 8) & 0xFF);
            output.append((uint8_t)RPM & 0xFF);
            output.append((uint8_t)(MAP >> 8) & 0xFF);
            output.append((uint8_t)MAP & 0xFF);
            output.append((uint8_t)(TPS >> 8) & 0xFF);
            output.append((uint8_t)TPS & 0xFF);

            received = serial->write_serial_data_echo_check(output);

            output.clear();
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x03);
            output.append((uint8_t)0xE0);
            output.append((uint8_t)(CLT >> 8) & 0xFF);
            output.append((uint8_t)CLT & 0xFF);
            output.append((uint8_t)(IAT >> 8) & 0xFF);
            output.append((uint8_t)IAT & 0xFF);
            output.append((uint8_t)(FLT >> 8) & 0xFF);
            output.append((uint8_t)FLT & 0xFF);
            output.append((uint8_t)(OLT >> 8) & 0xFF);
            output.append((uint8_t)OLT & 0xFF);

            received = serial->write_serial_data_echo_check(output);

            output.clear();
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x03);
            output.append((uint8_t)0x62);
            output.append((uint8_t)(IDC >> 8) & 0xFF);
            output.append((uint8_t)IDC & 0xFF);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x00);
            output.append((uint8_t)(IGN >> 8) & 0xFF);
            output.append((uint8_t)IGN & 0xFF);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x00);

            received = serial->write_serial_data_echo_check(output);

            output.clear();
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x03);
            output.append((uint8_t)0x70);
            output.append((uint8_t)(SPD >> 8) & 0xFF);
            output.append((uint8_t)SPD & 0xFF);
            output.append((uint8_t)(GEAR >> 8) & 0xFF);
            output.append((uint8_t)GEAR & 0xFF);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x00);

            received = serial->write_serial_data_echo_check(output);

            output.clear();
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x03);
            output.append((uint8_t)0x72);
            output.append((uint8_t)(BATT >> 8) & 0xFF);
            output.append((uint8_t)BATT & 0xFF);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x00);

            received = serial->write_serial_data_echo_check(output);

            i++;

            delay(20);
        }
    }

    /*
    output.append((uint8_t)0x03);
    output.append((uint8_t)0x60);
    output.append((uint8_t)0x03);
    output.append((uint8_t)0x61);

    output.append((uint8_t)0x03);
    output.append((uint8_t)0x68);
    output.append((uint8_t)0x03);
    output.append((uint8_t)0x69);

    output.append((uint8_t)0x03);
    output.append((uint8_t)0x73);
    output.append((uint8_t)0x03);
    output.append((uint8_t)0x74);
    output.append((uint8_t)0x03);
    output.append((uint8_t)0x75);
    output.append((uint8_t)0x03);
    output.append((uint8_t)0xE0);
    output.append((uint8_t)0x03);
    output.append((uint8_t)0xE2);
*/
    serial->set_can_speed("500000");
    //serial_poll_timer->start();
    //ssm_init_poll_timer->start();

    return 0;
}

int MainWindow::simulate_obd()
{
    QByteArray output;
    QByteArray received;

    QByteArray sid3e = { "\x81\xf1\x12\x7e" };

    uint8_t sid_4d_ff_b4[] = { 0x4c, 0x00, 0xb4 };




    qDebug() << "Simulating OBD communications";

    //serial_poll_timer->stop();
    //ssm_init_poll_timer->stop();
    logging_poll_timer->stop();

    serial->reset_connection();
    ecuid.clear();
    ecu_init_complete = false;

    serial->set_add_iso14230_header(false);
    //serial->set_add_iso14230_header(true);
    open_serial_port();
    //serial->change_port_speed("10400");
    serial->change_port_speed("9600");

    while (simulate_obd_on)
    {
        received.clear();
        output.clear();
        received = serial->read_serial_data(100);
        if (received != "")
        {
            qDebug() << "Received:" << parse_message_to_hex(received);
            // sid_4d_ff_b4
            if ((uint8_t)received.at(0) == 0x4d || (uint8_t)received.at(1) == 0xff || (uint8_t)received.at(2) == 0xb4)
                for (uint8_t i = 0; i < sizeof(sid_4d_ff_b4); i++) output.append((uint8_t)sid_4d_ff_b4[i]);
/*
            if ((uint8_t)received.at(3) == 0x3e || (uint8_t)received.at(4) == 0x3e)
                output = sid3e;

            if ((uint8_t)received.at(3) == 0x81 || (uint8_t)received.at(4) == 0x81)
                for (uint8_t i = 0; i < sizeof(sid_81); i++) output.append((uint8_t)sid_81[i]);
            if ((uint8_t)received.at(3) == 0x82 || (uint8_t)received.at(4) == 0x82)
                for (uint8_t i = 0; i < sizeof(sid_82); i++) output.append((uint8_t)sid_82[i]);
            if (((uint8_t)received.at(3) == 0x83 && (uint8_t)received.at(4) == 0x00) || ((uint8_t)received.at(4) == 0x83 && (uint8_t)received.at(5) == 0x00))
                for (uint8_t i = 0; i < sizeof(sid_83_00); i++) output.append((uint8_t)sid_83_00[i]);
            if (((uint8_t)received.at(3) == 0x83 && (uint8_t)received.at(4) == 0x02) || ((uint8_t)received.at(4) == 0x83 && (uint8_t)received.at(5) == 0x02))
                for (uint8_t i = 0; i < sizeof(sid_83_02); i++) output.append((uint8_t)sid_83_02[i]);
            if (((uint8_t)received.at(3) == 0x83 && (uint8_t)received.at(4) == 0x03) || ((uint8_t)received.at(4) == 0x83 && (uint8_t)received.at(5) == 0x03))
                for (uint8_t i = 0; i < sizeof(sid_83_03); i++) output.append((uint8_t)sid_83_03[i]);

            if (((uint8_t)received.at(3) == 0x27 && (uint8_t)received.at(4) == 0x01) || ((uint8_t)received.at(4) == 0x27 && (uint8_t)received.at(5) == 0x01))
                for (uint8_t i = 0; i < sizeof(sid_27_01); i++) output.append((uint8_t)sid_27_01[i]);
            if (((uint8_t)received.at(3) == 0x27 && (uint8_t)received.at(4) == 0x02) || ((uint8_t)received.at(4) == 0x27 && (uint8_t)received.at(5) == 0x02))
                for (uint8_t i = 0; i < sizeof(sid_27_02); i++) output.append((uint8_t)sid_27_02[i]);
            if (((uint8_t)received.at(3) == 0x27 && (uint8_t)received.at(4) == 0x10) || ((uint8_t)received.at(4) == 0x27 && (uint8_t)received.at(5) == 0x10))
                for (uint8_t i = 0; i < sizeof(sid_27_10); i++) output.append((uint8_t)sid_27_10[i]);
            if (((uint8_t)received.at(3) == 0x27 && (uint8_t)received.at(4) == 0x11) || ((uint8_t)received.at(4) == 0x27 && (uint8_t)received.at(5) == 0x11))
                for (uint8_t i = 0; i < sizeof(sid_27_11); i++) output.append((uint8_t)sid_27_11[i]);

            if ((uint8_t)received.at(1) == 0x1a && (uint8_t)received.at(2) == 0x90)
                for (uint8_t i = 0; i < sizeof(sid_1a_90); i++) output.append((uint8_t)sid_1a_90[i]);

            if ((uint8_t)received.at(3) == 0x21 && (uint8_t)received.at(4) == 0x09)
                for (uint8_t i = 0; i < sizeof(sid_21_09); i++) output.append((uint8_t)sid_21_09[i]);
*/
            if (output != "")
            {
                qDebug() << "Send msg:" << parse_message_to_hex(output);
                received = serial->write_serial_data_echo_check(output);
            }
        }
    }

    // 81 f1 12 7e
    // 81 f1 12 c2
    // 83 f1 12 c1 ef 8f
    // 80 f1 12 07 c3 02 00 28 00 14 00
    // 80 f1 12 07 c3 00 00 ef 00 78 00






    return 0;
}
