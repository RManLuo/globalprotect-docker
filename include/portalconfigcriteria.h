#ifndef PORTALCONFIGCRITERIA_H
#define PORTALCONFIGCRITERIA_H

#include <QtCore/QByteArray>
#include <QtCore/QString>
#include <QtCore/QStringList>

class PortalConfigCriteria
{
public:
    PortalConfigCriteria();

    static PortalConfigCriteria parse(const QByteArray &xml);
    static QByteArray certificateReportFromPemFile(const QString &pemFile);
    static QString defaultCertificatePemFile();
    static bool hasCertificatePemFile();

    bool requiresCscRequest() const;
    QByteArray cscData() const;
    QString cscDigest() const;
    QString configDigest() const;
    QString portalCscAuthCookie() const;

private:
    QString m_portalCscAuthCookie;
    QString m_cscDigest;
    QString m_configDigest;
    QStringList m_issuerDerValues;

    QByteArray categoryData() const;
    static QString xmlText(const QByteArray &xml, const QString &tagName);
    static QStringList certIssuerDerValues(const QByteArray &xml);
    static QString xmlEscape(const QString &value);
    static QString pemSubject(const QString &pem);
};

#endif // PORTALCONFIGCRITERIA_H
