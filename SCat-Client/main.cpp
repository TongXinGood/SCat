#include <QApplication>
#include <QVBoxLayout>
#include <QTranslator>
#include <QLibraryInfo>
#include <QLabel>
#include "NoFrame.h"
#include "Login/include/LoginWindow.h"
#include "Register/include/RegisterWindow.h"
#include "Register/include/Register.h"
#include "Login/include/Login.h"
#include"Chat/include/ChatWindow.h"
#include "Friend/include/FriendList.h"
#include "Scat/include/ScatWindow.h"
#include "NetWork/include/NetWorkManager.h"
#include "Other/include/AppController.h"
#include "Friend/include/AddFriendWindow.h"
static const char* kMenuStyle = R"(
    QMenu {
        background-color: #FFFFFF;
        border: 1px solid #ECE9F2;
        border-radius: 10px;
        padding: 6px;
    }
    QMenu::item {
        background-color: transparent;
        color: #1A1A1A;
        font-size: 13px;
        padding: 8px 32px 8px 16px;
        margin: 1px 2px;
        border-radius: 6px;
    }
    QMenu::item:selected {
        background-color: #F4F1FA;
        color: #20202E;
    }
    QMenu::item:disabled {
        color: #C0C0C0;
    }
    QMenu::separator {
        height: 1px;
        background-color: #F0F0F0;
        margin: 5px 8px;
    }
)";

int main(int argc, char* argv[])
{   
    
    QApplication a(argc, argv);

    QCoreApplication::setOrganizationName("SCat");
    QCoreApplication::setApplicationName("SCat-Client");
    a.setStyleSheet(kMenuStyle);
    QTranslator qtTranslator;
    if (qtTranslator.load(QStringLiteral("qtbase_zh_CN"),
        QLibraryInfo::path(QLibraryInfo::TranslationsPath))) {
        a.installTranslator(&qtTranslator);
    }
    else {
        qDebug() << "qtbase_zh_CN.qm not found, Qt built-in texts stay English";
    }
    AppController app;
    app.start();
    
    return a.exec();
   

}