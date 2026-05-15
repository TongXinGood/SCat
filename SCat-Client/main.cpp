#include <QApplication>
#include <QVBoxLayout>
#include <QLabel>
#include "NoFrame.h"
#include "Login/include/LoginWindow.h"
#include "Register/include/RegisterWindow.h"
#include "Login/include/Login.h"
#include"Chat/include/ChatWindow.h"
#include "Friend/include/FriendList.h"
#include "Scat/include/ScatWindow.h"


int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    LoginWindow log1;
    log1.show();

    

    return app.exec();
}