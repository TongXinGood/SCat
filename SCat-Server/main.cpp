#include <QCoreApplication>
#include <QSqlError>
#include <QSqlDatabase>
#include <QDebug>
#include "Server/include/Server.h"
#include "Database/include/Database.h"

int main(int argc, char* argv[])
{
    QCoreApplication a(argc, argv);

    Database db;
    if (!db.connect("127.0.0.1", 3306, "scat", "root", "root"))
        return -1;

    Server server(&db);
    if (!server.start(8888))
        return -1;

    return a.exec();
}