#ifndef BIU_OPS_SUBARU_DATA_H
#define BIU_OPS_SUBARU_DATA_H

#include <QWidget>
#include <QLabel>

QT_BEGIN_NAMESPACE
namespace Ui
{
    class BiuOpsSubaruDataWindow;
}
QT_END_NAMESPACE

class BiuOpsSubaruData : public QWidget
{
    Q_OBJECT

public:
    explicit BiuOpsSubaruData(QStringList *data_result, QWidget *parent = nullptr);
    ~BiuOpsSubaruData();

    void update_data_results(QStringList *data_result);

private:
    QStringList *data_result;
    Ui::BiuOpsSubaruDataWindow *ui;
};

#endif // BIU_OPS_SUBARU_DATA_H
