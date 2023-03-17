#ifndef BIU_OPS_SUBARU_DTCS_H
#define BIU_OPS_SUBARU_DTCS_H

#include <QWidget>
#include <QLabel>

QT_BEGIN_NAMESPACE
namespace Ui
{
    class BiuOpsSubaruDtcsWindow;
}
QT_END_NAMESPACE

class BiuOpsSubaruDtcs : public QWidget
{
    Q_OBJECT

public:
    explicit BiuOpsSubaruDtcs(QStringList *dtc_result, QWidget *parent = nullptr);
    ~BiuOpsSubaruDtcs();

private:
    QStringList *dtc_result;
    Ui::BiuOpsSubaruDtcsWindow *ui;
};

#endif // BIU_OPS_SUBARU_DTCS_H
