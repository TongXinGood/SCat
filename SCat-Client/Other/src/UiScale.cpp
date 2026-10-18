#include "../include/UiScale.h"
#include <QSettings>
#include <QByteArray>

namespace UiScale
{
    static const char* kScaleKey = "appearance/scale";

    // apply() 跑的时候还没有 QApplication，也还没设组织名和程序名，
    // QSettings 得自己写上。这两个名字必须跟 main.cpp 里设的一样，不然读不到同一份设置
    static const char* kOrgName = "SCat";
    static const char* kAppName = "SCat-Client";

    QList<int> options()
    {
        return { 100, 90, 80, 75 };
    }

    // 存的值不在可选档位里（手动改坏了注册表之类），就当 100
    static int validate(int value)
    {
        return options().contains(value) ? value : 100;
    }

    void apply()
    {
        QSettings settings(kOrgName, kAppName);
        int value = validate(settings.value(kScaleKey, 100).toInt());

        // 100% 也要明确写上 1：点"立即重启"时新进程会继承旧进程的环境变量，
        // 不写的话从 75% 改回 100%，新进程拿到的还是旧的 0.75
        qputenv("QT_SCALE_FACTOR", QByteArray::number(value / 100.0));
    }

    int percent()
    {
        QSettings settings(kOrgName, kAppName);
        return validate(settings.value(kScaleKey, 100).toInt());
    }

    void setPercent(int value)
    {
        QSettings settings(kOrgName, kAppName);
        settings.setValue(kScaleKey, validate(value));
    }
}