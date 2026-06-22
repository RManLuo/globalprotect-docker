#include <QtGui/QCloseEvent>
#include <QtCore/QTimer>
#include <plog/Log.h>

#include "standardloginwindow.h"
#include "ui_standardloginwindow.h"
#include "gphelper.h"
#include "credentialprovider.h"

using namespace gpclient::helper;

StandardLoginWindow::StandardLoginWindow(const QString &portalAddress, const QString &labelUsername,
                                         const QString &labelPassword, const QString &authMessage) :
        QDialog(nullptr),
        ui(new Ui::StandardLoginWindow) {
    ui->setupUi(this);
    ui->portalAddress->setText(portalAddress);
    ui->username->setPlaceholderText(labelUsername);
    ui->password->setPlaceholderText(labelPassword);
    ui->authMessage->setText(authMessage);

    autocomplete();
    if (autocompleteFromOnePassword()) {
        QTimer::singleShot(0, this, &StandardLoginWindow::on_loginButton_clicked);
    }

    setWindowTitle("GlobalProtect Login");
    setFixedSize(width(), height());
    setModal(true);
}

void StandardLoginWindow::autocomplete() {
    QString username, password;
    auto allowPlainPassword = getenv("GPAGENT_ALLOW_PLAIN_PASSWORD");
    if (allowPlainPassword && atoi(allowPlainPassword)) {
        settings::get("username", username);
        settings::get("password", password);
    }
    else {
        settings::secureGet("username", username);
        settings::secureGet("password", password);
    }

    if (!username.isEmpty() && !password.isEmpty()) {
        ui->username->setText(username);
        ui->password->setText(password);
    }
}

bool StandardLoginWindow::autocompleteFromOnePassword()
{
    const QByteArray enabled = qgetenv("GPAGENT_AUTO_RECONNECT");
    if (enabled != "1" && enabled.toLower() != "true") {
        return false;
    }

    std::string error;
    const auto credentials = OnePasswordCredentialProvider().fetch(&error);
    if (!credentials.isValid()) {
        if (!error.empty()) {
            LOGW << "1Password credential autofill skipped: " << QString::fromStdString(error);
        }
        return false;
    }

    ui->username->setText(QString::fromStdString(credentials.username));
    ui->password->setText(QString::fromStdString(credentials.password));
    return true;
}

void StandardLoginWindow::setProcessing(bool isProcessing) {
    ui->username->setReadOnly(isProcessing);
    ui->password->setReadOnly(isProcessing);
    ui->loginButton->setDisabled(isProcessing);
}

void StandardLoginWindow::on_loginButton_clicked() {
    const QString username = ui->username->text().trimmed();
    const QString password = ui->password->text().trimmed();

    if (username.isEmpty() || password.isEmpty()) {
        return;
    }

    auto allowPlainPassword = getenv("GPAGENT_ALLOW_PLAIN_PASSWORD");
    if (allowPlainPassword && atoi(allowPlainPassword)) {
        settings::save("username", username);
        settings::save("password", password);
    }
    else {
        settings::secureSave("username", username);
        settings::secureSave("password", password);
    }

    emit performLogin(username, password);
}

void StandardLoginWindow::closeEvent(QCloseEvent *event) {
    event->accept();
    reject();
}
