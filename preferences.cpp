#include "preferences.h"
#include "ui_preferences.h"

Preferences::Preferences(QWidget *parent)
    : QWidget(parent),
      ui(new Ui::Preferences)
{
    ui->setupUi(this);

}

Preferences::~Preferences()
{
    delete ui;
}
