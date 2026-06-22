#ifndef SAMLLOGINAUTOMATION_H
#define SAMLLOGINAUTOMATION_H

#include <string>

struct SamlLoginSelectors
{
    std::string username;
    std::string password;
    std::string totp;
    std::string submit;

    bool hasAnyField() const;
};

class SamlLoginAutomation
{
public:
    SamlLoginAutomation();
    explicit SamlLoginAutomation(SamlLoginSelectors selectors);

    std::string buildScript(const std::string &username,
                            const std::string &password,
                            const std::string &totp,
                            std::string *error = nullptr) const;

    static bool isRejectedLoginPage(const std::string &html);

private:
    SamlLoginSelectors m_selectors;
};

#endif // SAMLLOGINAUTOMATION_H
