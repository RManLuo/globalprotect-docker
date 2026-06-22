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
        selectors.username = "input[name=\"identifier\"]";
        selectors.submit = "input[type=\"submit\"]";

        SamlLoginAutomation automation(selectors);
        std::string error;
        const auto script = automation.buildScript("alice", "password", "123456", &error);

        require(!script.empty(), "staged username-only selectors should build script");
        require(error.empty(), "staged username script should not set error");
        require(script.find("input[name=\\\"identifier\\\"]") != std::string::npos, "script should include username selector");
        require(script.find("alice") != std::string::npos, "script should include username value");
        require(script.find("password.value") == std::string::npos, "script should not require password selector");
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
        require(script.find("#username") != std::string::npos, "script should include username selector");
        require(script.find("button[type=submit]") != std::string::npos, "script should include submit selector");
        require(script.find("al\\\"ice") != std::string::npos, "script should escape double quotes");
        require(script.find("pa\\\\ss\\nword") != std::string::npos, "script should escape backslash and newline");
        require(script.find("123456") != std::string::npos, "script should include generated TOTP");
        require(script.find(".click()") != std::string::npos, "script should submit the form");
    }

    {
        require(SamlLoginAutomation::isRejectedLoginPage("<main>Unable to sign in</main>"),
                "Okta rejected login page should be detected");
        require(!SamlLoginAutomation::isRejectedLoginPage("<main>Verify with your password</main>"),
                "normal password page should not be treated as rejected");
    }

    return 0;
}
