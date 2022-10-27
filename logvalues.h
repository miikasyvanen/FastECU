#ifndef LOGVALUES_H
#define LOGVALUES_H

#include <QMainWindow>
#include <QDebug>
#include <QWidget>
#include <QComboBox>

#include <file_actions.h>

QT_BEGIN_NAMESPACE
namespace Ui
{
    class LogValues;
}
QT_END_NAMESPACE

class LogValues : public QWidget
{
    Q_OBJECT

public:
    explicit LogValues(FileActions::LogValuesStructure *logValues, int tabIndex, QString protocol, QWidget *parent = nullptr);
    ~LogValues();

private:
    FileActions::LogValuesStructure *logValues;

    QString protocol;

    struct log_value {
        uint16_t address;
        uint8_t len;
        QString expr;
    };

    QStringList ssm_log_values = {
        "Engine Speed",
        "Manifold Absolute Pressure",
        "Air/Fuel Learning #2",
        "Air/Fuel Correction #2",
        "Air/Fuel Learning #1",
        "Air/Fuel Correction #1",
        "Coolant Temperature",
        "Engine Load",

        "Front O2 Sensor #2",
        "Rear O2 Sensor",
        "Front O2 Sensor #1",
        "Throttle Opening Angle",
        "Mass Air Flow",
        "Intake Air Temperature",
        "Ignition Timing",
        "Vehicle Speed",

        "Battery Voltage",
        "Air Flow Sensor Voltage",
        "Throttle Sensor Voltage",
        "Differential Pressure Sensor Voltage",
        "Fuel Injection #1 Pulse Width",
        "Fuel Injection #2 Pulse Width",
        "Knock Correction",
        "Atmospheric Pressure",

        "Manifold Relative Pressure",
        "Pressure Differential Sensor",
        "Fuel Tank Pressure",
        "CO Adjustment",
        "Learned Ignition Timing",
        "Accelerator Opening Angle",
        "Fuel Temperature",
        "Front O2 Heater #1",

        "Rear O2 Heater Current",
        "Front O2 Heater #2",
        "Fuel Level",
        "",
        "Primary Wastegate Duty Cycle",
        "Secondary Wastegate Duty Cycle",
        "CPC Valve Duty Ratio",
        "Tumble Valve Position Sensor Right",

        "",
        "",
        "",
        "",
        "",
        "",
        "",
        "",

        "",
        "",
        "",
        "",
        "",
        "",
        "",
        "",
    };

    Ui::LogValues *ui;

private slots:
    void change_log_gauge_value(int);
    void change_log_digital_value(int);
    void change_log_switch_value(int);

};

#endif // LOGVALUES_H
