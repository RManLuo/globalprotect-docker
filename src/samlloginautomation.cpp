#include "samlloginautomation.h"

#include <algorithm>
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

bool SamlLoginSelectors::hasAnyField() const
{
    return !username.empty() || !password.empty() || !totp.empty();
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

    if (!m_selectors.hasAnyField() || m_selectors.submit.empty()) {
        if (error != nullptr) {
            *error = "SAML login selector configuration is incomplete";
        }
        return "";
    }

    std::ostringstream script;
    script
        << "(function(){"
        << "const submit=document.querySelector(" << jsString(m_selectors.submit) << ");"
        << "if(!submit){return false;}"
        << "const visible=function(el){if(!el){return false;}const r=el.getBoundingClientRect();const s=getComputedStyle(el);return !!(r.width&&r.height)&&s.display!=='none'&&s.visibility!=='hidden';};"
        << "const textFor=function(el){const label=el.id?document.querySelector('label[for=\"'+CSS.escape(el.id)+'\"]'):null;return ((label?label.innerText:'')+' '+(el.placeholder||'')+' '+(el.getAttribute('aria-label')||'')).toLowerCase();};"
        << "const looksTotp=function(el){const text=textFor(el);return el.autocomplete==='one-time-code'||/\\b(code|otp|totp|authenticator|verification)\\b/.test(text);};"
        << "const looksPassword=function(el){return el.type==='password'||/password/.test(textFor(el));};"
        << "const used=new Set();"
        << "const fill=function(selector,value,kind){if(!selector){return false;}const el=document.querySelector(selector);if(!visible(el)||used.has(el)){return false;}if(kind==='totp'&&looksPassword(el)&&!looksTotp(el)){return false;}if(kind==='password'&&looksTotp(el)){return false;}used.add(el);el.focus();el.value=value;el.dispatchEvent(new Event('input',{bubbles:true}));el.dispatchEvent(new Event('change',{bubbles:true}));return true;};"
        << "let filled=false;"
        << "filled=fill(" << jsString(m_selectors.username) << "," << jsString(username) << ",'username')||filled;"
        << "filled=fill(" << jsString(m_selectors.password) << "," << jsString(password) << ",'password')||filled;"
        << "filled=fill(" << jsString(m_selectors.totp) << "," << jsString(totp) << ",'totp')||filled;"
        << "if(!filled){return false;}"
        << "submit.click();"
        << "return true;"
        << "})()";

    return script.str();
}

bool SamlLoginAutomation::isRejectedLoginPage(const std::string &html)
{
    std::string lower = html;
    std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return lower.find("unable to sign in") != std::string::npos;
}
