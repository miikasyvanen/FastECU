#ifndef BIUOPSSUBARUINPUT2_H
#define BIUOPSSUBARUINPUT2_H

#include <QWidget>
#include <QLabel>
#include <QButtonGroup>
#include <QRadioButton>
#include <QPushButton>

QT_BEGIN_NAMESPACE
namespace Ui
{
    class BiuOpsSubaruInput2Window;
}
QT_END_NAMESPACE


class BiuOpsSubaruInput2 : public QWidget
{
    Q_OBJECT

public:
    explicit BiuOpsSubaruInput2(QStringList *biu_option_names, QByteArray *biu_option_result, QWidget *parent = nullptr);
    ~BiuOpsSubaruInput2();

private:
    QByteArray *biu_option_result;
    QStringList *biu_option_names;
    Ui::BiuOpsSubaruInput2Window *ui;

private slots:
    void prepare_biu_setting2();

signals:
    void send_biu_setting2(QByteArray output);
};

#endif // BIUOPSSUBARUINPUT2_H
