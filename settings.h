#ifndef SETTINGS_H
#define SETTINGS_H

#include <QCheckBox>
#include <QDebug>
#include <QMainWindow>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QListWidgetItem>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QWidget>

#include <file_actions.h>

QT_BEGIN_NAMESPACE
namespace Ui
{
    class Settings;
}
QT_END_NAMESPACE

class Settings : public QDialog
{
    Q_OBJECT

public:
    explicit Settings(FileActions::ConfigValuesStructure *configValues, QWidget *parent = nullptr);
    ~Settings();

private slots:

private:
    void closeEvent(QCloseEvent *bar);

    Ui::Settings *ui;

    FileActions::ConfigValuesStructure *configValues;
    FileActions *fileActions;

    QLineEdit *ecuflash_def_dir_lineedit;
    QLineEdit *romraider_logger_file_lineedit;
    QLineEdit *ecu_cal_dir_lineedit;
    QLineEdit *log_files_dir_lineedit;

    QSpinBox *toolbar_iconsize_spinbox;

    QListWidget *romraider_definition_files_list;

    QVBoxLayout *create_files_config_page(FileActions::ConfigValuesStructure *configValues);
    QVBoxLayout *create_ui_config_page(FileActions::ConfigValuesStructure *configValues);
    void create_list_icons();
    void change_page(QListWidgetItem *current, QListWidgetItem *previous);
    void set_ecuflash_def_dir();
    void set_romraider_logger_file();
    void set_ecu_cal_dir();
    void set_log_files_dir();
    void add_definition_files();
    void remove_definition_files();

private slots:
    void ecuflash_defs_enabled_checkbox(int state);
    void romraider_defs_enabled_checkbox(int state);
    void romraider_as_primary_def_base_checkbox(int state);
    void toolbar_iconsize_value_changed(int value);
    int save_config_file();

};

#endif // SETTINGS_H
