#include "samlloginautomation.h"

#include <cstdlib>
#include <sstream>
#include <utility>

namespace
{
std::string getenvOrEmpty(const char *name)
{
    const char *value = std::getenv(name);
    return value == nullptr ? "" : value;
}

std::string jsString(const std::string &value)
{
    std::string escaped;
    escaped.reserve(value.size() + 2);
    escaped.push_back('"');
    for (const char ch : value) {
        switch (ch) {
            case '\\':
                escaped += "\\\\";
                break;
            case '"':
                escaped += "\\\"";
                break;
            case '\n':
                escaped += "\\n";
                break;
            case '\r':
                escaped += "\\r";
                break;
            case '\t':
                escaped += "\\t";
                break;
            default:
                escaped.push_back(ch);
                break;
        }
    }
    escaped.push_back('"');
    return escaped;
}
}

bool SamlLoginSelectors::isComplete() const
{
    return !username.empty() && !password.empty() && !totp.empty() && !submit.empty();
}

SamlLoginAutomation::SamlLoginAutomation()
    : m_selectors {
        getenvOrEmpty("GPAGENT_LOGIN_USERNAME_SELECTOR"),
        getenvOrEmpty("GPAGENT_LOGIN_PASSWORD_SELECTOR"),
        getenvOrEmpty("GPAGENT_LOGIN_TOTP_SELECTOR"),
        getenvOrEmpty("GPAGENT_LOGIN_SUBMIT_SELECTOR")
      }
{
}

SamlLoginAutomation::SamlLoginAutomation(SamlLoginSelectors selectors)
    : m_selectors(std::move(selectors))
{
}

std::string SamlLoginAutomation::buildScript(const std::string &username,
                                             const std::string &password,
                                             const std::string &totp,
                                             std::string *error) const
{
    if (error != nullptr) {
        error->clear();
    }

    if (!m_selectors.isComplete()) {
        if (error != nullptr) {
            *error = "SAML login selector configuration is incomplete";
        }
        return "";
    }

    std::ostringstream script;
    script
        << "(function(){"
        << "const username=document.querySelector(" << jsString(m_selectors.username) << ");"
        << "const password=document.querySelector(" << jsString(m_selectors.password) << ");"
        << "const totp=document.querySelector(" << jsString(m_selectors.totp) << ");"
        << "const submit=document.querySelector(" << jsString(m_selectors.submit) << ");"
        << "if(!username||!password||!totp||!submit){return false;}"
        << "username.focus();username.value=" << jsString(username) << ";username.dispatchEvent(new Event('input',{bubbles:true}));"
        << "password.focus();password.value=" << jsString(password) << ";password.dispatchEvent(new Event('input',{bubbles:true}));"
        << "totp.focus();totp.value=" << jsString(totp) << ";totp.dispatchEvent(new Event('input',{bubbles:true}));"
        << "submit.click();"
        << "return true;"
        << "})()";

    return script.str();
}
