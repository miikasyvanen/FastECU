#ifndef DEFINITIONFILEMAKER_H
#define DEFINITIONFILEMAKER_H

#include <QDebug>
#include <QFileDialog>
#include <QMessageBox>
#include <QWidget>
#include <QXmlStreamReader>

QT_BEGIN_NAMESPACE
namespace Ui
{
    class DefinitionFileConvertWindow;
}
QT_END_NAMESPACE

class DefinitionFileConvert : public QDialog
{
    Q_OBJECT
public:
    explicit DefinitionFileConvert(QWidget *parent = nullptr);
    ~DefinitionFileConvert();



private:
    enum ReturnValues {
        STATUS_SUCCESS,
        STATUS_GENERAL_ERROR,
        STATUS_FILE_OPEN_ERROR,
    };

    int convert_mappack_csv_file();

    Ui::DefinitionFileConvertWindow *ui;
};

#endif // DEFINITIONFILEMAKER_H
