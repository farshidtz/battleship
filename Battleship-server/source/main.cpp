#include "Server.h"
#include <QCoreApplication>
#include <iostream>
using namespace std;

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    ChatterBoxServer *server = new ChatterBoxServer();
    int port;
    qDebug() << "Enter the port number: ";
    cin >> port;
    bool success = server->listen(QHostAddress::Any, port);
    if(!success)
    {
        qFatal("Could not listen on port 4200.");
    }

    qDebug() << "Ready";

    return app.exec();
}
