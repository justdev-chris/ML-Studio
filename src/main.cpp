#include <QApplication>
#include "ui/mainwindow/MainWindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    app.setApplicationName("MeowyLoops Studio");
    app.setOrganizationName("MeowyLoops");

    MainWindow window;
    window.show();

    return app.exec();
}