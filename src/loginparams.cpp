#include <QtCore/QUrlQuery>
#include <QtCore/QSysInfo>

#include "loginparams.h"
#include "gphelper.h"

using namespace gpclient::helper;

LoginParams::LoginParams(const QString clientos)
{
    params.addQueryItem("prot", QUrl::toPercentEncoding("https:"));
    params.addQueryItem("server", "");
    params.addQueryItem("inputStr", "");
    params.addQueryItem("jnlpReady", "jnlpReady");
    params.addQueryItem("user", "");
    params.addQueryItem("passwd", "");
    params.addQueryItem("computer", QUrl::toPercentEncoding(QSysInfo::machineHostName()));
    params.addQueryItem("ok", "Login");
    params.addQueryItem("direct", "yes");
    params.addQueryItem("clientVer", "4100");

    // add the clientos parameter if not empty
    if (!clientos.isEmpty()) {
        params.addQueryItem("clientos", clientos);
    }
    const QString clientGpVersion = QString::fromLocal8Bit(qgetenv("GPAGENT_CLIENTGPVERSION"));
    params.addQueryItem("clientgpversion", clientGpVersion.isEmpty() ? "6.3.3-638" : clientGpVersion);

    auto osVersion = settings::get("os-version", "").toString();
    if (osVersion.isEmpty()) {
        osVersion = QSysInfo::prettyProductName();
    }
    params.addQueryItem("os-version", QUrl::toPercentEncoding(osVersion));

    params.addQueryItem("portal-userauthcookie", "");
    params.addQueryItem("portal-prelogonuserauthcookie", "");
    params.addQueryItem("prelogin-cookie", "");
    params.addQueryItem("ipv6-support", "yes");

    QString hostId = QString::fromLocal8Bit(qgetenv("GPAGENT_HOST_ID"));
    if (hostId.isEmpty()) {
        hostId = QString::fromLocal8Bit(QSysInfo::machineUniqueId());
    }
    params.addQueryItem("host-id", hostId);

    const QString serialNo = QString::fromLocal8Bit(qgetenv("GPAGENT_SERIALNO"));
    params.addQueryItem("serialno", serialNo);
    params.addQueryItem("csc-digest", "");
    params.addQueryItem("config-digest", "");
    params.addQueryItem("csc-support", "yes");
}

LoginParams::~LoginParams()
{
}

void LoginParams::setUser(const QString user)
{
    updateQueryItem("user", user);
}

void LoginParams::setServer(const QString server)
{
    updateQueryItem("server", server);
}

void LoginParams::setPassword(const QString password)
{
    updateQueryItem("passwd", password);
}

void LoginParams::setUserAuthCookie(const QString cookie)
{
    updateQueryItem("portal-userauthcookie", cookie);
}

void LoginParams::setPrelogonAuthCookie(const QString cookie)
{
    updateQueryItem("portal-prelogonuserauthcookie", cookie);
}

void LoginParams::setPreloginCookie(const QString cookie)
{
    updateQueryItem("prelogin-cookie", cookie);
}

void LoginParams::setInputStr(const QString inputStr)
{
    updateQueryItem("inputStr", inputStr);
}

void LoginParams::setHostId(const QString hostId)
{
    updateQueryItem("host-id", hostId);
}

void LoginParams::setSerialNo(const QString serialNo)
{
    updateQueryItem("serialno", serialNo);
}

void LoginParams::setConfigDigest(const QString digest)
{
    updateQueryItem("config-digest", digest);
}

void LoginParams::setCscDigest(const QString digest)
{
    updateQueryItem("csc-digest", digest);
}

void LoginParams::setCscSupport(const QString support)
{
    updateQueryItem("csc-support", support);
}

QByteArray LoginParams::toUtf8() const
{
    return params.toString().toUtf8();
}

void LoginParams::updateQueryItem(const QString key, const QString value)
{
    if (params.hasQueryItem(key)) {
        params.removeQueryItem(key);
    }
    params.addQueryItem(key, QUrl::toPercentEncoding(value));
}
