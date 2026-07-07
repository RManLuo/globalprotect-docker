#include <QtCore/QCryptographicHash>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QProcess>
#include <QtCore/QRegularExpression>
#include <QtCore/QTextStream>
#include <QtCore/QXmlStreamReader>

#include "portalconfigcriteria.h"

PortalConfigCriteria::PortalConfigCriteria()
{
}

PortalConfigCriteria PortalConfigCriteria::parse(const QByteArray &xml)
{
    PortalConfigCriteria criteria;
    criteria.m_portalCscAuthCookie = xmlText(xml, "portal-csc-auth-cookie");
    criteria.m_cscDigest = xmlText(xml, "csc-digest");
    criteria.m_configDigest = xmlText(xml, "config-digest");
    criteria.m_issuerDerValues = certIssuerDerValues(xml);
    return criteria;
}

QString PortalConfigCriteria::defaultCertificatePemFile()
{
    const QByteArray configured = qgetenv("HIP_CERTIFICATE_PEM_FILE");
    if (!configured.isEmpty()) {
        return QString::fromLocal8Bit(configured);
    }

    return "/root/hip-certificates.pem";
}

bool PortalConfigCriteria::hasCertificatePemFile()
{
    const QFile file(defaultCertificatePemFile());
    return file.exists() && file.size() > 0;
}

bool PortalConfigCriteria::requiresCscRequest() const
{
    return !m_portalCscAuthCookie.isEmpty() && hasCertificatePemFile();
}

QByteArray PortalConfigCriteria::cscData() const
{
    const QByteArray categories = categoryData();
    if (categories.isEmpty()) {
        return QByteArray();
    }

    QByteArray data;
    data.append("<hip-report>");
    data.append("<portal-csc-auth-cookie>");
    data.append(m_portalCscAuthCookie.toUtf8());
    data.append("</portal-csc-auth-cookie>");
    data.append("<csc-digest>");
    data.append(cscDigest().toUtf8());
    data.append("</csc-digest>");
    data.append("<categories>");
    data.append(categories);
    data.append("</categories>");
    data.append("</hip-report>");
    return data;
}

QString PortalConfigCriteria::cscDigest() const
{
    const QByteArray data = categoryData();
    if (data.isEmpty()) {
        return m_cscDigest;
    }

    return QString::fromLatin1(QCryptographicHash::hash(data, QCryptographicHash::Md5).toHex());
}

QString PortalConfigCriteria::configDigest() const
{
    return m_configDigest;
}

QString PortalConfigCriteria::portalCscAuthCookie() const
{
    return m_portalCscAuthCookie;
}

QByteArray PortalConfigCriteria::certificateReportFromPemFile(const QString &pemFile)
{
    QFile file(pemFile);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QByteArray();
    }

    const QString contents = QString::fromUtf8(file.readAll());
    const QRegularExpression certRe("-----BEGIN CERTIFICATE-----[\\s\\S]*?-----END CERTIFICATE-----");
    QRegularExpressionMatchIterator it = certRe.globalMatch(contents);

    QString report;
    QTextStream out(&report);
    out << "\t\t<entry name=\"certificate\">\n";
    out << "\t\t\t<list>\n";

    int count = 0;
    while (it.hasNext()) {
        const QString pem = it.next().captured(0).trimmed();
        const QString subject = pemSubject(pem);
        out << "\t\t\t\t<entry>\n";
        out << "\t\t\t\t\t<subject>" << xmlEscape(subject.isEmpty() ? "/CN=unknown" : subject) << "</subject>\n";
        out << "\t\t\t\t\t<pem>" << pem << "</pem>\n";
        out << "\t\t\t\t</entry>\n";
        count++;
        break;
    }

    out << "\t\t\t</list>\n";
    out << "\t\t</entry>\n";

    if (count == 0) {
        return QByteArray();
    }

    return report.toUtf8();
}

QByteArray PortalConfigCriteria::categoryData() const
{
    return certificateReportFromPemFile(defaultCertificatePemFile());
}

QString PortalConfigCriteria::xmlText(const QByteArray &xml, const QString &tagName)
{
    QXmlStreamReader reader(xml);
    while (!reader.atEnd()) {
        reader.readNext();
        if (reader.isStartElement() && reader.name() == tagName) {
            return reader.readElementText().trimmed();
        }
    }

    return QString();
}

QStringList PortalConfigCriteria::certIssuerDerValues(const QByteArray &xml)
{
    QStringList values;
    QXmlStreamReader reader(xml);
    while (!reader.atEnd()) {
        reader.readNext();
        if (reader.isStartElement() && reader.name() == "issuer-der") {
            const QString value = reader.readElementText().trimmed();
            if (!value.isEmpty()) {
                values.append(value);
            }
        }
    }

    return values;
}

QString PortalConfigCriteria::xmlEscape(const QString &value)
{
    QString escaped = value;
    escaped.replace("&", "&amp;");
    escaped.replace("<", "&lt;");
    escaped.replace(">", "&gt;");
    escaped.replace("\"", "&quot;");
    escaped.replace("'", "&apos;");
    return escaped;
}

QString PortalConfigCriteria::pemSubject(const QString &pem)
{
    QProcess openssl;
    openssl.start("openssl", QStringList() << "x509" << "-noout" << "-subject" << "-nameopt" << "compat");
    if (!openssl.waitForStarted(1000)) {
        return QString();
    }

    openssl.write(pem.toUtf8());
    openssl.closeWriteChannel();

    if (!openssl.waitForFinished(3000) || openssl.exitCode() != 0) {
        return QString();
    }

    QString subject = QString::fromUtf8(openssl.readAllStandardOutput()).trimmed();
    subject.remove(QRegularExpression("^subject=\\s*"));
    return subject;
}
