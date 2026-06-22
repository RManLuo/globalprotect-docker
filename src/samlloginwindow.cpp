#include <QtWidgets/QVBoxLayout>
#include <QtCore/QTimer>
#include <QtWebEngineWidgets/QWebEngineProfile>
#include <QtWebEngineWidgets/QWebEngineView>
#include <QWebEngineCookieStore>
#include <plog/Log.h>

#include "samlloginwindow.h"
#include "samlloginautomation.h"

SAMLLoginWindow::SAMLLoginWindow(QWidget *parent)
    : QDialog(parent)
    , webView(new EnhancedWebView(this))
{
    setWindowTitle("GlobalProtect Login");
    setModal(true);
    resize(700, 550);

    QVBoxLayout *verticalLayout = new QVBoxLayout(this);
    webView->setUrl(QUrl("about:blank"));
    webView->setAttribute(Qt::WA_DeleteOnClose);
    verticalLayout->addWidget(webView);

    webView->initialize();
    connect(webView, &EnhancedWebView::responseReceived, this, &SAMLLoginWindow::onResponseReceived);
    connect(webView, &EnhancedWebView::loadFinished, this, &SAMLLoginWindow::onLoadFinished);

    // Show the login window automatically when exceeds the MAX_WAIT_TIME
    QTimer::singleShot(MAX_WAIT_TIME, this, [this]() {
        if (failed) {
            return;
        }
        LOGI << "MAX_WAIT_TIME exceeded, display the login window.";
        this->show();
        this->retryAutomatedLogin();
    });
}

void SAMLLoginWindow::closeEvent(QCloseEvent *event)
{
    event->accept();
    reject();
}

void SAMLLoginWindow::login(const QString samlMethod, const QString samlRequest, const QString preloginUrl)
{
    webView->page()->profile()->cookieStore()->deleteSessionCookies();

    if (samlMethod == "POST") {
        webView->setHtml(samlRequest, preloginUrl);
    } else if (samlMethod == "REDIRECT") {
        LOGI << "Redirect to " << samlRequest;
        webView->load(samlRequest);
    } else {
        LOGE << "Unknown saml-auth-method expected POST or REDIRECT, got " << samlMethod;
        failed = true;
        emit fail("ERR001", "Unknown saml-auth-method, got " + samlMethod);
    }
}

void SAMLLoginWindow::onResponseReceived(QJsonObject params)
{
    const auto type = params.value("type").toString();
    // Skip non-document response
    if (type != "Document") {
        return;
    }

    auto response = params.value("response").toObject();
    auto headers = response.value("headers").toObject();

    LOGI << "Trying to receive authentication cookie from " << response.value("url").toString();

    const auto username = headers.value("saml-username").toString();
    const auto preloginCookie = headers.value("prelogin-cookie").toString();
    const auto userAuthCookie = headers.value("portal-userauthcookie").toString();

    this->checkSamlResult(username, preloginCookie, userAuthCookie);
}

void SAMLLoginWindow::checkSamlResult(QString username, QString preloginCookie, QString userAuthCookie)
{
    LOGI << "Checking the authentication result...";

    if (!username.isEmpty()) {
        samlResult.insert("username", username);
    }

    if (!preloginCookie.isEmpty()) {
        samlResult.insert("preloginCookie", preloginCookie);
    }

    if (!userAuthCookie.isEmpty()) {
        samlResult.insert("userAuthCookie", userAuthCookie);
    }

    // Check the SAML result
    if (samlResult.contains("username")
            && (samlResult.contains("preloginCookie") || samlResult.contains("userAuthCookie"))) {
        LOGI << "Got the SAML authentication information successfully.";

        emit success(samlResult);
        accept();
    }
}

void SAMLLoginWindow::onLoadFinished()
{
     LOGI << "Load finished " << webView->page()->url().toString();
     if (tryAutomatedLogin()) {
         return;
     }
     webView->page()->toHtml([this] (const QString &html) { this->handleHtml(html); });
}

void SAMLLoginWindow::retryAutomatedLogin()
{
    if (failed || automatedLoginRetryScheduled || automatedLoginAttempts >= MAX_AUTOMATED_LOGIN_ATTEMPTS) {
        return;
    }

    automatedLoginRetryScheduled = true;
    QTimer::singleShot(AUTOMATED_LOGIN_RETRY_DELAY, this, [this]() {
        automatedLoginRetryScheduled = false;
        if (failed) {
            return;
        }

        if (webView->page()->url().scheme() == "chrome-error") {
            automatedLoginAttempts++;
            webView->reload();
            retryAutomatedLogin();
            return;
        }

        tryAutomatedLogin();
    });
}

bool SAMLLoginWindow::tryAutomatedLogin()
{
    const QByteArray enabled = qgetenv("GPAGENT_AUTO_RECONNECT");
    if (enabled != "1" && enabled.toLower() != "true") {
        return false;
    }

    if (!credentialsLoaded) {
        std::string credentialError;
        credentials = OnePasswordCredentialProvider().fetch(&credentialError);
        credentialsLoaded = true;
        if (!credentials.isValid()) {
            if (!credentialError.empty()) {
                LOGW << "1Password SAML automation skipped: " << QString::fromStdString(credentialError);
            }
            return false;
        }
    }

    std::string error;
    const auto script = SamlLoginAutomation().buildScript(credentials.username, credentials.password, credentials.totp, &error);
    if (script.empty()) {
        if (!error.empty()) {
            LOGW << "SAML automation skipped: " << QString::fromStdString(error);
        }
        return false;
    }

    automatedLoginAttempts++;
    webView->page()->runJavaScript(QString::fromStdString(script), [this](const QVariant &result) {
        if (!result.toBool()) {
            LOGW << "SAML automation script did not find configured fields";
            webView->page()->toHtml([this] (const QString &html) {
                if (SamlLoginAutomation::isTooManyAttemptsPage(html.toStdString())) {
                    LOGW << "SAML automation paused because the identity provider reported too many attempts";
                    failed = true;
                    emit fail("ERR004", "Identity provider reported too many login attempts.");
                    return;
                }
                if (SamlLoginAutomation::isRejectedLoginPage(html.toStdString())) {
                    LOGW << "SAML automation stopped after identity provider rejected the submitted credentials";
                    failed = true;
                    emit fail("ERR003", "Identity provider rejected the submitted credentials.");
                    return;
                }
                retryAutomatedLogin();
                this->handleHtml(html);
            });
            return;
        }

        retryAutomatedLogin();
    });
    return true;
}

void SAMLLoginWindow::handleHtml(const QString &html)
{
    // try to check the html body and extract from there
    const auto samlAuthStatus = parseTag("saml-auth-status", html);

    if (samlAuthStatus == "1") {
        const auto preloginCookie = parseTag("prelogin-cookie", html);
        const auto username = parseTag("saml-username", html);
        const auto userAuthCookie = parseTag("portal-userauthcookie", html);

        checkSamlResult(username, preloginCookie, userAuthCookie);
    } else if (samlAuthStatus == "-1") {
        LOGI << "SAML authentication failed...";
        failed = true;
        emit fail("ERR002", "Authentication failed, please try again.");
    } else {
        show();
    }
}

QString SAMLLoginWindow::parseTag(const QString &tag, const QString &html) {
    const QRegularExpression expression(QString("<%1>(.*)</%1>").arg(tag));
    return expression.match(html).captured(1);
}
