#include "hexedit.h"

HexEdit::HexEdit(FileActions::EcuCalDefStructure *ecuCalDef, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::HexEdit)
    , ecuCalDef(ecuCalDef)
{
    ui->setupUi(this);

    this->setWindowTitle("FastECU HexView");
    int addPixels = 6;

    QFontMetrics fm = ui->addrColumnHeader->fontMetrics();
    QString text = ui->addrColumnHeader->text();
    int chars = text.length();
    int w = fm.boundingRect(text).width() + chars * addPixels;
    // int w = fm.width(text);
    ui->addrColumnHeader->setFixedWidth(w);
    ui->addrColumnTextEdit->setFixedWidth(w);

    fm = ui->dataColumnHeader->fontMetrics();
    text = ui->dataColumnHeader->text();
    w = fm.boundingRect(text).width() + chars * addPixels;
    ui->dataColumnHeader->setFixedWidth(w);
    ui->dataColumnTextEdit->setFixedWidth(w);

    fm = ui->chartColumnHeader->fontMetrics();
    text = ui->chartColumnHeader->text();
    w = fm.boundingRect(text).width() + chars * addPixels;
    ui->chartColumnHeader->setFixedWidth(w);
    ui->charColumnTextEdit->setFixedWidth(w);

    //connect(ui->scrollArea->verticalScrollBar(), SIGNAL(sliderMoved(int)), ui->dataColumnTextEdit->verticalScrollBar(), SLOT(setValue(int)));
    //connect(ui->scrollArea->verticalScrollBar(), SIGNAL(sliderMoved(int)), ui->chartColumnTextEdit->verticalScrollBar(), SLOT(setValue(int)));
    //connect(ui->scrollArea->verticalScrollBar(), SIGNAL(sliderMoved(int)), ui->addrColumnTextEdit->verticalScrollBar(), SLOT(setValue(int)));

    ui->addrColumnTextEdit->verticalScrollBar()->setSingleStep(32);
    ui->dataColumnTextEdit->verticalScrollBar()->setSingleStep(32);
    ui->charColumnTextEdit->verticalScrollBar()->setSingleStep(32);
/*
    connect(ui->addrColumnTextEdit->verticalScrollBar(), SIGNAL(sliderMoved(int)), ui->dataColumnTextEdit->verticalScrollBar(), SLOT(setValue(int)));
    connect(ui->addrColumnTextEdit->verticalScrollBar(), SIGNAL(sliderMoved(int)), ui->chartColumnTextEdit->verticalScrollBar(), SLOT(setValue(int)));
    connect(ui->dataColumnTextEdit->verticalScrollBar(), SIGNAL(sliderMoved(int)), ui->addrColumnTextEdit->verticalScrollBar(), SLOT(setValue(int)));
    connect(ui->dataColumnTextEdit->verticalScrollBar(), SIGNAL(sliderMoved(int)), ui->chartColumnTextEdit->verticalScrollBar(), SLOT(setValue(int)));
    connect(ui->chartColumnTextEdit->verticalScrollBar(), SIGNAL(sliderMoved(int)), ui->addrColumnTextEdit->verticalScrollBar(), SLOT(setValue(int)));
    connect(ui->chartColumnTextEdit->verticalScrollBar(), SIGNAL(sliderMoved(int)), ui->dataColumnTextEdit->verticalScrollBar(), SLOT(setValue(int)));
*/
    connect(ui->addrColumnTextEdit->verticalScrollBar(), SIGNAL(valueChanged(int)), ui->dataColumnTextEdit->verticalScrollBar(), SLOT(setValue(int)));
    connect(ui->addrColumnTextEdit->verticalScrollBar(), SIGNAL(valueChanged(int)), ui->charColumnTextEdit->verticalScrollBar(), SLOT(setValue(int)));
    connect(ui->addrColumnTextEdit->verticalScrollBar(), SIGNAL(valueChanged(int)), ui->barColumnTextEdit->verticalScrollBar(), SLOT(setValue(int)));

    connect(ui->dataColumnTextEdit->verticalScrollBar(), SIGNAL(valueChanged(int)), ui->addrColumnTextEdit->verticalScrollBar(), SLOT(setValue(int)));
    connect(ui->dataColumnTextEdit->verticalScrollBar(), SIGNAL(valueChanged(int)), ui->charColumnTextEdit->verticalScrollBar(), SLOT(setValue(int)));
    connect(ui->dataColumnTextEdit->verticalScrollBar(), SIGNAL(valueChanged(int)), ui->barColumnTextEdit->verticalScrollBar(), SLOT(setValue(int)));

    connect(ui->charColumnTextEdit->verticalScrollBar(), SIGNAL(valueChanged(int)), ui->addrColumnTextEdit->verticalScrollBar(), SLOT(setValue(int)));
    connect(ui->charColumnTextEdit->verticalScrollBar(), SIGNAL(valueChanged(int)), ui->dataColumnTextEdit->verticalScrollBar(), SLOT(setValue(int)));
    connect(ui->charColumnTextEdit->verticalScrollBar(), SIGNAL(valueChanged(int)), ui->barColumnTextEdit->verticalScrollBar(), SLOT(setValue(int)));

    connect(ui->barColumnTextEdit->verticalScrollBar(), SIGNAL(valueChanged(int)), ui->addrColumnTextEdit->verticalScrollBar(), SLOT(setValue(int)));
    connect(ui->barColumnTextEdit->verticalScrollBar(), SIGNAL(valueChanged(int)), ui->dataColumnTextEdit->verticalScrollBar(), SLOT(setValue(int)));
    connect(ui->barColumnTextEdit->verticalScrollBar(), SIGNAL(valueChanged(int)), ui->charColumnTextEdit->verticalScrollBar(), SLOT(setValue(int)));

    connect(ui->dataColumnTextEdit, SIGNAL(cursorPositionChanged()), this, SLOT(getCursorPosition()));

    this->show();
}

HexEdit::~HexEdit()
{
    delete ui;
}

static auto const bars = QStringLiteral (
    "▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁▁"
    "▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂▂"
    "▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃▃"
    "▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄"
    "▅▅▅▅▅▅▅▅▅▅▅▅▅▅▅▅▅▅▅▅▅▅▅▅▅▅▅▅▅▅▅▅"
    "▆▆▆▆▆▆▆▆▆▆▆▆▆▆▆▆▆▆▆▆▆▆▆▆▆▆▆▆▆▆▆▆"
    "▇▇▇▇▇▇▇▇▇▇▇▇▇▇▇▇▇▇▇▇▇▇▇▇▇▇▇▇▇▇▇▇"
    "████████████████████████████████");

static auto const CP437 = QStringLiteral(
    " ☺☻♥♦♣♠•◘○◙♂♀♪♫☼▶◀↕‼¶§▬↨↑↓→←∟↔▲▼"
    "␣!\"#$%&'()*+,-./0123456789:;<=>?"
    "@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_"
    "`abcdefghijklmnopqrstuvwxyz{|}~ "
    "ÇüéâäàåçêëèïîìÄÅÉæÆôöòûùÿÖÜ¢£¥₧ƒ"
    "áíóúñÑªº¿⌐¬½¼¡«»░▒▓│┤╡╢╖╕╣║╗╝╜╛┐"
    "└┴┬├─┼╞╟╚╔╩╦╠═╬╧╨╤╥╙╘╒╓╫╪┘┌█▄▌▐▀"
    "αßΓπΣσµτΦΘΩδ∞φε∩≡±≥≤⌠⌡÷≈°∙·√ⁿ²■ ");

static inline QChar decode_chars(char ch) { return CP437[uchar(ch)]; }
static inline QChar decode_bars(char ch) { return bars[uchar(ch)]; }

void HexEdit::getCursorPosition()
{
    cursorCurrX = ui->dataColumnTextEdit->textCursor().columnNumber();
    if (cursorCurrX)
        cursorCurrY = ui->dataColumnTextEdit->textCursor().position() / 48;

    // Check if empty char to skip
    if ((cursorCurrX + 1) % 3 == 0 && cursorCurrX > cursorPrevX)
    {
        // Empty char, moving right, skip 1 to right
        ui->dataColumnTextEdit->moveCursor(QTextCursor::Right, QTextCursor::MoveAnchor);
    }
    else if ((cursorCurrX + 1) % 3 == 0 && cursorCurrX < cursorPrevX)
    {
        // Empty char, moving left, skip 1 to left
        ui->dataColumnTextEdit->moveCursor(QTextCursor::Left, QTextCursor::MoveAnchor);
    }

    cursorPrevX = cursorCurrX;
    cursorPrevY = cursorCurrY;
}

void HexEdit::run()
{
    mcu_type_string = ecuCalDef->McuType;
    mcu_type_index = 0;

    while (flashdevices[mcu_type_index].name != 0)
    {
        if (flashdevices[mcu_type_index].name == mcu_type_string)
            break;
        mcu_type_index++;
    }
    QString mcu_name = flashdevices[mcu_type_index].name;
    //emit LOG_D("MCU type: " + mcu_name + " " + mcu_type_string + " and index: " + QString::number(mcu_type_index), true, true);

    uint8_t column_data_size = 0x10; // 16 bytes / column
    uint32_t addr_row_count = ecuCalDef->FullRomData.length() / column_data_size; // Row count
    uint32_t start_addr = flashdevices[mcu_type_index].fblocks[0].start;
    uint32_t current_addr = start_addr;
    QString addr_column_text;
    QString data_column_text;
    QString chart_column_text;
    QString bar_column_text;
/*
    QColor color(QWidget::palette().color(QWidget::backgroundRole()));
    QString style = QString("background-color: rgb(%1,%2,%3); border: 0px ;").arg(color.red()).arg(color.green()).arg(color.blue());
    setStyleSheet(style);
    ui->dataColumnTextEdit->insertPlainText(style);
*/
    for (uint32_t i = 0; i < addr_row_count; i++)
    {
        addr_column_text.append(QString("%1").arg(current_addr,6,16,QLatin1Char('0')).toUpper());
        addr_column_text.append("h\n");

        for (int j = 0; j < column_data_size; j++)
            data_column_text.append(QString("%1 ").arg((uint8_t)ecuCalDef->FullRomData.at(current_addr+j),2,16,QLatin1Char('0')).toUpper());
        data_column_text.append("\n");

        for (int j = 0; j < column_data_size; j++)
            chart_column_text.append(decode_chars((uint8_t)ecuCalDef->FullRomData.at(current_addr+j)));
        chart_column_text.append("\n");

        for (int j = 0; j < column_data_size; j++)
            bar_column_text.append((uint8_t)ecuCalDef->FullRomData.at(current_addr+j) / 2 + 0x30);
        bar_column_text.append("\n");

        current_addr+=column_data_size;

    }

    QString msg;
    QFont f("fastecu_bars_128", 10);
    ui->barColumnTextEdit->setFont(f);
    //for (int i = 0x30; i < 0x100; i++)
    //    msg.append(QString(i));
    //ui->charColumnTextEdit->setText(msg);

    ui->addrColumnTextEdit->insertPlainText(addr_column_text);
    ui->dataColumnTextEdit->insertPlainText(data_column_text);
    ui->charColumnTextEdit->insertPlainText(chart_column_text);
    ui->barColumnTextEdit->insertPlainText(bar_column_text);

    ui->dataColumnTextEdit->moveCursor(QTextCursor::Start);
/*
    QTextEdit::ExtraSelection h;
    h.cursor = ui->dataColumnTextEdit->textCursor();
    h.format.setProperty(QTextFormat::FullWidthSelection, true);

    QList<QTextEdit::ExtraSelection> extras;
    while (!ui->dataColumnTextEdit->textCursor().atEnd())
    {
        ui->dataColumnTextEdit->moveCursor(QTextCursor::EndOfLine);
        ui->dataColumnTextEdit->moveCursor(QTextCursor::StartOfLine);

        h.format.setBackground(Qt::lightGray);

        extras << h;
        ui->dataColumnTextEdit->setExtraSelections(extras);

        ui->dataColumnTextEdit->moveCursor(QTextCursor::Down);
        ui->dataColumnTextEdit->moveCursor(QTextCursor::Down);
    }
*/

    //QSize size = ui->addrColumnTextEdit->document()->size().toSize();
    //ui->addrColumnTextEdit->setFixedHeight( size.height() + 3 );

    qDebug() << "Columns set";

/*    ui->addrColumnTextEdit->setFixedSize(ui->addrColumnTextEdit->sizeHintForColumn(0) +
                            ui->addrColumnTextEdit->frameWidth() * 2, ui->addrColumnTextEdit->sizeHintForRow(0) *
                            ui->addrColumnTextEdit->count() + 2 * ui->addrColumnTextEdit->frameWidth());*/
    /*
    ui->addrColumnTextEdit->setFixedWidth(ui->addrColumnTextEdit->document()->idealWidth() * 2 +
                                        ui->addrColumnTextEdit->contentsMargins().left() +
                                        ui->addrColumnTextEdit->contentsMargins().right());
    ui->dataColumnTextEdit->setFixedWidth(ui->dataColumnTextEdit->document()->idealWidth() * 2 +
                            ui->dataColumnTextEdit->contentsMargins().left() +
                            ui->dataColumnTextEdit->contentsMargins().right());*/
}

