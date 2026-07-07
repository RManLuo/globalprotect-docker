#include <QtNetwork/QNetworkReply>
#include <QtCore/QSysInfo>
#include <QtCore/QTimer>
#include <QtCore/QUrl>
#include <plog/Log.h>

#include "portalauthenticator.h"
#include "gphelper.h"
#include "standardloginwindow.h"
#include "samlloginwindow.h"
#include "samlloginautomation.h"
#include "loginparams.h"
#include "portalconfigcriteria.h"
#include "preloginresponse.h"
#include "portalconfigresponse.h"
#include "gpgateway.h"

using namespace gpclient::helper;

namespace
{
QByteArray formItem(const QString &key, const QString &value)
{
    QByteArray item;
    item.append(QUrl::toPercentEncoding(key));
    item.append('=');
    item.append(QUrl::toPercentEncoding(value));
    return item;
}

void addFormItem(QByteArray &form, const QString &key, const QString &value)
{
    if (!form.isEmpty()) {
        form.append('&');
    }
    form.append(formItem(key, value));
}
}

PortalAuthenticator::PortalAuthenticator(const QString& portal, const QString& clientos) : QObject()
  , portal(portal)
  , clientos(clientos)
  , preloginUrl("https://" + portal + "/global-protect/prelogin.esp?tmp=tmp&kerberos-support=yes&ipv6-support=yes&clientVer=4100")
  , configUrl("https://" + portal + "/global-protect/getconfig.esp")
  , cscConfigUrl("https://" + portal + "/global-protect/getconfig_csc.esp")
{
    if (!clientos.isEmpty()) {
        preloginUrl = preloginUrl + "&clientos=" + clientos;
    }
}

PortalAuthenticator::~PortalAuthenticator()
{
    delete standardLoginWindow;
}

void PortalAuthenticator::authenticate()
{
    attempts++;

    LOGI << QString("(%1/%2) attempts").arg(attempts).arg(MAX_ATTEMPTS) << ", preform portal prelogin at " << preloginUrl;

    QNetworkReply *reply = createRequest(preloginUrl);
    connect(reply, &QNetworkReply::finished, this, &PortalAuthenticator::onPreloginFinished);
}

void PortalAuthenticator::onPreloginFinished()
{
    auto *reply = qobject_cast<QNetworkReply*>(sender());

    if (reply->error()) {
        LOGE << QString("Error occurred while accessing %1, %2").arg(preloginUrl, reply->errorString());
        emit preloginFailed("Error occurred on the portal prelogin interface.");
        delete reply;
        return;
    }

    LOGI << "Portal prelogin succeeded.";

    preloginResponse = PreloginResponse::parse(reply->readAll());

    LOGI << "Finished parsing the prelogin response. The region field is: " << preloginResponse.region();

    if (preloginResponse.hasSamlAuthFields()) {
        // Do SAML authentication
        samlAuth();
    } else if (preloginResponse.hasNormalAuthFields()) {
        // Do normal username/password authentication
        tryAutoLogin();
    } else {
        LOGE << QString("Unknown prelogin response for %1 got %2").arg(preloginUrl).arg(QString::fromUtf8(preloginResponse.rawResponse()));
        emit preloginFailed("Unknown response for portal prelogin interface.");
    }

    delete reply;
}

void PortalAuthenticator::tryAutoLogin()
{
    {
        const QString username = settings::get("username").toString();
        const QString password = settings::get("password").toString();

        if (!username.isEmpty() && !password.isEmpty()) {
            LOGI << "Trying auto login using the saved credentials";
            isAutoLogin = true;
            fetchConfig(username, password);
            return;
        }
    }

    {
        QString username; settings::secureGet("username", username);
        QString password; settings::secureGet("password", password);
 
        if (!username.isEmpty() && !password.isEmpty()) {
            LOGI << "Trying auto login using the saved credentials";
            isAutoLogin = true;
            fetchConfig(username, password);
            return;
        }
    }

    normalAuth();
}

void PortalAuthenticator::normalAuth()
{
    LOGI << "Trying to launch the normal login window...";

    standardLoginWindow = new StandardLoginWindow {portal, preloginResponse.labelUsername(), preloginResponse.labelPassword(), preloginResponse.authMessage() };

    // Do login
    connect(standardLoginWindow, &StandardLoginWindow::performLogin, this, &PortalAuthenticator::onPerformNormalLogin);
    connect(standardLoginWindow, &StandardLoginWindow::rejected, this, &PortalAuthenticator::onLoginWindowRejected);
    connect(standardLoginWindow, &StandardLoginWindow::finished, this, &PortalAuthenticator::onLoginWindowFinished);

    standardLoginWindow->show();
}

void PortalAuthenticator::onPerformNormalLogin(const QString &username, const QString &password)
{
    standardLoginWindow->setProcessing(true);
    fetchConfig(username, password);
}

void PortalAuthenticator::onLoginWindowRejected()
{
    emitFail();
}

void PortalAuthenticator::onLoginWindowFinished()
{
    delete standardLoginWindow;
    standardLoginWindow = nullptr;
}

void PortalAuthenticator::samlAuth()
{
    LOGI << "Trying to perform SAML login with saml-method " << preloginResponse.samlMethod();

    auto *loginWindow = new SAMLLoginWindow;

    connect(loginWindow, &SAMLLoginWindow::success, [this, loginWindow](const QMap<QString, QString> samlResult) {
        this->onSAMLLoginSuccess(samlResult);
        loginWindow->deleteLater();
    });
    connect(loginWindow, &SAMLLoginWindow::fail, [this, loginWindow](const QString &code, const QString msg) {
        this->onSAMLLoginFail(code, msg);
        loginWindow->deleteLater();
    });
    connect(loginWindow, &SAMLLoginWindow::rejected, [this, loginWindow]() {
        this->onLoginWindowRejected();
        loginWindow->deleteLater();
    });

    loginWindow->login(preloginResponse.samlMethod(), preloginResponse.samlRequest(), preloginUrl);
}

void PortalAuthenticator::onSAMLLoginSuccess(const QMap<QString, QString> samlResult)
{
    if (samlResult.contains("preloginCookie")) {
        LOGI << "SAML login succeeded, got the prelogin-cookie";
    } else {
        LOGI << "SAML login succeeded, got the portal-userauthcookie";
    }

    fetchConfig(samlResult.value("username"), "", samlResult.value("preloginCookie"), samlResult.value("userAuthCookie"));
}

void PortalAuthenticator::onSAMLLoginFail(const QString &code, const QString &msg)
{
    if (code == "ERR004") {
        const int delaySeconds = SamlLoginAutomation::throttleSleepSeconds();
        LOGW << "Identity provider throttled portal SAML login; restarting from prelogin in "
             << delaySeconds << " seconds";
        QTimer::singleShot(delaySeconds * 1000, this, [this]() {
            authenticate();
        });
        return;
    }

    if (code == "ERR002" && attempts < MAX_ATTEMPTS) {
        LOGI << "Failed to authenticate, trying to re-authenticate...";
        authenticate();
    } else {
        emitFail(msg);
    }
}

void PortalAuthenticator::fetchConfig(QString username, QString password, QString preloginCookie, QString userAuthCookie)
{
    LoginParams loginParams { clientos };
    loginParams.setServer(portal);
    loginParams.setUser(username);
    loginParams.setPassword(password);
    loginParams.setPreloginCookie(preloginCookie);
    loginParams.setUserAuthCookie(userAuthCookie);

    // Save the username and password for future use.
    this->username = username;
    this->password = password;

    LOGI << "Fetching the portal config from " << configUrl;

    isFetchingCscConfig = false;
    auto *reply = createRequest(configUrl, loginParams.toUtf8());
    connect(reply, &QNetworkReply::finished, this, &PortalAuthenticator::onFetchConfigFinished);
}

void PortalAuthenticator::onFetchConfigFinished()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());

    if (reply->error()) {
        LOGE << QString("Failed to fetch the portal config from %1, %2").arg(configUrl).arg(reply->errorString());

        // Login failed, enable the fields of the normal login window
        if (standardLoginWindow) {
            standardLoginWindow->setProcessing(false);
            openMessageBox("Portal login failed.", "Please check your credentials and try again.");
        } else if (isAutoLogin) {
            isAutoLogin = false;
            normalAuth();
        } else {
            emit portalConfigFailed("Failed to fetch the portal config.");
        }
        delete reply;
        return;
    }

    LOGI << "Fetch the portal config succeeded.";
    const QByteArray responseXml = reply->readAll();

    if (!isFetchingCscConfig) {
        const QByteArray cscParams = cscRequestParams(responseXml);
        if (!cscParams.isEmpty()) {
            LOGI << "Portal config contains config selection criteria; fetching CSC portal config from "
                 << cscConfigUrl << " with " << cscParams.size() << " bytes of form data";
            isFetchingCscConfig = true;
            auto *cscReply = createRequest(cscConfigUrl, cscParams);
            connect(cscReply, &QNetworkReply::finished, this, &PortalAuthenticator::onFetchConfigFinished);
            delete reply;
            return;
        }
    }

    PortalConfigResponse response = PortalConfigResponse::parse(responseXml);

    // Add the username & password to the response object
    response.setUsername(username);
    response.setPassword(password);

    // Close the login window
    if (standardLoginWindow) {
        LOGI << "Closing the StandardLoginWindow...";

        standardLoginWindow->close();
    }

    emit success(response, preloginResponse.region());
    delete reply;
}

QByteArray PortalAuthenticator::cscRequestParams(const QByteArray &portalConfigXml) const
{
    const PortalConfigCriteria criteria = PortalConfigCriteria::parse(portalConfigXml);
    if (!criteria.requiresCscRequest()) {
        return QByteArray();
    }

    const QByteArray cscData = criteria.cscData();
    if (cscData.isEmpty()) {
        LOGW << "Portal config requires CSC data, but no certificate CSC data could be generated";
        return QByteArray();
    }

    auto osVersion = settings::get("os-version", "").toString();
    if (osVersion.isEmpty()) {
        osVersion = QSysInfo::prettyProductName();
    }

    QString hostId = QString::fromLocal8Bit(qgetenv("GPAGENT_HOST_ID"));
    if (hostId.isEmpty()) {
        hostId = QString::fromLocal8Bit(QSysInfo::machineUniqueId());
    }

    const QString serialNo = QString::fromLocal8Bit(qgetenv("GPAGENT_SERIALNO"));

    QByteArray params;
    addFormItem(params, "user", username);
    addFormItem(params, "clientVer", "4100");
    addFormItem(params, "clientos", clientos);
    addFormItem(params, "os-version", osVersion);
    addFormItem(params, "hostid", hostId);
    addFormItem(params, "serialno", serialNo);
    addFormItem(params, "portal-cc-auth-cookie", criteria.portalCscAuthCookie());
    addFormItem(params, "portal-csc-auth-cookie", criteria.portalCscAuthCookie());
    addFormItem(params, "config-digest", criteria.configDigest());
    addFormItem(params, "csc-digest", criteria.cscDigest());
    addFormItem(params, "csc-data", QString::fromUtf8(cscData));
    addFormItem(params, "swg-auth-token", "");
    addFormItem(params, "swg-nonce", "0");

    LOGI << "Prepared portal CSC request: auth cookie present="
         << (!criteria.portalCscAuthCookie().isEmpty())
         << ", config digest present=" << (!criteria.configDigest().isEmpty())
         << ", csc data bytes=" << cscData.size();

    return params;
}

void PortalAuthenticator::emitFail(const QString& msg)
{
    emit fail(msg);
}
