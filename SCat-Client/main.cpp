#include <QApplication>
#include <QVBoxLayout>
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

int main(int argc, char* argv[])
{
    QApplication a(argc, argv);

    AppController app;
    app.start();

    return a.exec();
}