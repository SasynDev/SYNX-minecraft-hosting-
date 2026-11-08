#include <QApplication>
#include "main_window.hpp"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("SYNX HOSTING ADMIN");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("SYNX HOSTING");

    synx::admin::MainWindow window;
    window.show();

    return app.exec();
}

