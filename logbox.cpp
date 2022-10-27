#include "logbox.h"
//#include "ui_preferences.h"

LogBox::LogBox(QWidget *parent) : QWidget(parent)
{

}

QGroupBox *LogBox::drawLogBoxes(QString type, uint8_t index, uint8_t logBoxCount, QString title, QString unit, QString value)
{
    QGroupBox *gb = NULL;

    if (type == "switch")
        gb = drawLogSwitchBox(index, logBoxCount, title, unit, value);
    if (type == "log")
        gb = drawLogValueBox(index, logBoxCount, title, unit, value);

    return gb;
}

QGroupBox *LogBox::drawLogSwitchBox(uint8_t index, uint8_t switchBoxCount, QString title, QString unit, QString value)
{
    QScreen *screen = QGuiApplication::primaryScreen();
    QRect size = screen->geometry();

    int minWidth = size.width() / switchBoxCount / 2;
    int maxWidth = size.width() / switchBoxCount;

    QGroupBox *switchGroupBox = new QGroupBox();
    switchGroupBox->setContextMenuPolicy(Qt::CustomContextMenu);
    switchGroupBox->setObjectName("switchGroupBox" + QString::number(index));
    switchGroupBox->setMinimumWidth(minWidth);
    switchGroupBox->setMaximumWidth(maxWidth);
    switchGroupBox->setMinimumHeight(25);
    switchGroupBox->setMaximumHeight(25);
    switchGroupBox->setStyleSheet("QGroupBox{font: bold;border:1px solid gray;border-radius:5px;margin-top: 0px;padding:0px 0px 0px 0px;} QGroupBox::title{subcontrol-origin: margin;left: 7px;padding:0px 3px 0px 3px;}");

    QString labelText = title;

    QLabel *switchBoxLabel = new QLabel();
    //switchBoxLabel->setFixedWidth(100);
    switchBoxLabel->setAlignment(Qt::AlignCenter);
    switchBoxLabel->setAlignment(Qt::AlignLeft);
    switchBoxLabel->setText(labelText);
    //int labelFontSize = size.width() / 180;
    int labelFontSize = size.width() / 200;
    QFont f("Arial", labelFontSize, QFont::Bold);
    QFontMetrics fm(f);
    switchBoxLabel->setFont(f);

    QVBoxLayout *switchBoxLayout = new QVBoxLayout();
    switchBoxLayout->setContentsMargins(0, 0, 0, 0);
    switchBoxLayout->setAlignment(Qt::AlignCenter);
    switchBoxLayout->addWidget(switchBoxLabel);
    switchGroupBox->setLayout(switchBoxLayout);

    return switchGroupBox;
}

QGroupBox *LogBox::drawLogValueBox(uint8_t index, uint8_t logBoxCount, QString title, QString unit, QString value)
{
    QScreen *screen = QGuiApplication::primaryScreen();
    QRect size = screen->geometry();

    QGroupBox *logGroupBox = new QGroupBox(title);
    logGroupBox->setContextMenuPolicy(Qt::CustomContextMenu);
    logGroupBox->setObjectName("valueGroupBox" + QString::number(index));
    int minWidth = size.width() / (logBoxCount) / 2;
    int maxWidth = size.width() / (logBoxCount);
    int groupBoxFontSize = size.width() / 170;
    QFont t("Arial",groupBoxFontSize);
    logGroupBox->setStyleSheet("QGroupBox{font: bold;border:1px solid black;border-radius:5px;margin-left: 1px; margin-right: 1px; margin-top: "+QString::number(groupBoxFontSize)+"px;} QGroupBox::title{subcontrol-origin: margin;left: 7px;padding:0px 3px 0px 3px;}");
    logGroupBox->setMinimumWidth(minWidth);
    logGroupBox->setMaximumWidth(maxWidth);
    logGroupBox->setFont(t);

    //QString labelText = QString::number(value.toFloat(), 'f', 2);
    QString labelText = "0";
    labelText.append(" <font size=1px color=grey>");
    labelText.append(unit);
    labelText.append("</font>");

    QLabel *logBoxLabel = new QLabel();
    logBoxLabel->setObjectName("log_label" + QString::number(index));
    //logBoxLabel->setFixedWidth(100);
    logBoxLabel->setAlignment(Qt::AlignRight);
    logBoxLabel->setText(labelText);
    int labelFontSize = size.width() / 90;
    QFont f("Arial",labelFontSize);
    //QFontMetrics fm(f);
    logBoxLabel->setFont(f);

    QVBoxLayout *logBoxLayout = new QVBoxLayout();
    logBoxLayout->setAlignment(Qt::AlignRight);
    logBoxLayout->addWidget(logBoxLabel);
    logGroupBox->setLayout(logBoxLayout);

    return logGroupBox;
}

void LogBox::updateLogBox()
{

}

void LogBox::updateSwitchBox()
{

}

