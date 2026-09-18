#include <QCoreApplication>
#include <QSettings>
#include <QFile>
#include <QDebug>
#include "Server/include/Server.h"
#include "Database/include/Database.h"

int main(int argc, char* argv[])
{
    QCoreApplication a(argc, argv);

    // 数据库连接信息和监听端口全部放在 exe 旁边的 config.ini 里。
    // 部署到服务器时改这个文件就行，不用重新编译
    QString cfgPath = QCoreApplication::applicationDirPath() + "/config.ini";
    QSettings cfg(cfgPath, QSettings::IniFormat);

    QString dbHost = cfg.value("db/host", "127.0.0.1").toString();
    quint16 dbPort = static_cast<quint16>(cfg.value("db/port", 3306).toUInt());
    QString dbName = cfg.value("db/name", "scat").toString();
    QString dbUser = cfg.value("db/user", "root").toString();
    QString dbPwd = cfg.value("db/password", "root").toString();
    quint16 listenPort = static_cast<quint16>(cfg.value("server/port", 8888).toUInt());

    // 文件不存在就按默认值生成一份，管理员拿到手就知道有哪些项可以改
    if (!QFile::exists(cfgPath)) {
        cfg.setValue("db/host", dbHost);
        cfg.setValue("db/port", dbPort);
        cfg.setValue("db/name", dbName);
        cfg.setValue("db/user", dbUser);
        cfg.setValue("db/password", dbPwd);
        cfg.setValue("server/port", listenPort);
        cfg.sync();

        qDebug() << "config.ini created:" << cfgPath;
    }

    // 密码不打印出来，日志可能会被别人看到
    qDebug() << "database:" << dbUser << "@" << dbHost << ":" << dbPort << "/" << dbName;

    Database db;
    if (!db.connect(dbHost, dbPort, dbName, dbUser, dbPwd))
        return -1;

    Server server(&db);
    if (!server.start(listenPort))
        return -1;

    return a.exec();
}