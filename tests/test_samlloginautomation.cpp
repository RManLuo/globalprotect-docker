#include <cstdlib>
#include <iostream>
#include <string>

#include "samlloginautomation.h"

static void require(bool condition, const char *message)
{
    if (!condition) {
        std::cerr << message << "\n";
        std::exit(1);
    }
}

int main()
{
    {
        SamlLoginAutomation automation;
        std::string error;

        require(automation.buildScript("alice", "password", "123456", &error).empty(), "missing selectors should not build script");
        require(error.find("selector") != std::string::npos, "missing selectors should report selector error");
    }

    {
        SamlLoginSelectors selectors;
        selectors.username = "#username";
        selectors.password = "#password";
        selectors.totp = "#totp";
        selectors.submit = "button[type=submit]";

        SamlLoginAutomation automation(selectors);
        std::string error;
        const auto script = automation.buildScript("al\"ice", "pa\\ss\nword", "123456", &error);

        require(!script.empty(), "complete selectors should build script");
        require(error.empty(), "successful script build should not set error");
        require(script.find("document.querySelector(\"#username\")") != std::string::npos, "script should query username field");
        require(script.find("document.querySelector(\"button[type=submit]\")") != std::string::npos, "script should query submit button");
        require(script.find("al\\\"ice") != std::string::npos, "script should escape double quotes");
        require(script.find("pa\\\\ss\\nword") != std::string::npos, "script should escape backslash and newline");
        require(script.find("123456") != std::string::npos, "script should include generated TOTP");
        require(script.find(".click()") != std::string::npos, "script should submit the form");
    }

    return 0;
}
