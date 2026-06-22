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

    {
        require(SamlLoginAutomation::isTooManyAttemptsPage("<main>Too many attempts. Try again later.</main>"),
                "throttled login page should be detected");
        require(SamlLoginAutomation::isTooManyAttemptsPage("<main>Too many failed attempts</main>"),
                "failed-attempt throttling page should be detected");
        require(SamlLoginAutomation::isTooManyAttemptsPage("<main>Your account is temporarily locked due to too many sign-in attempts</main>"),
                "temporary lockout page should be detected");
        require(!SamlLoginAutomation::isTooManyAttemptsPage("<main>Unable to sign in</main>"),
                "bad credentials page should not be treated as throttling");
        require(!SamlLoginAutomation::isTooManyAttemptsPage("<main>Verify with your password</main>"),
                "normal password page should not be treated as throttling");
    }

    {
        unsetenv("GPAGENT_SAML_THROTTLE_SLEEP_SECONDS");
        require(SamlLoginAutomation::throttleSleepSeconds() == 300,
                "default throttle sleep should be 300 seconds");

        setenv("GPAGENT_SAML_THROTTLE_SLEEP_SECONDS", "7", 1);
        require(SamlLoginAutomation::throttleSleepSeconds() == 7,
                "valid throttle sleep override should be used");

        setenv("GPAGENT_SAML_THROTTLE_SLEEP_SECONDS", "0", 1);
        require(SamlLoginAutomation::throttleSleepSeconds() == 300,
                "zero throttle sleep should fall back to default");

        setenv("GPAGENT_SAML_THROTTLE_SLEEP_SECONDS", "invalid", 1);
        require(SamlLoginAutomation::throttleSleepSeconds() == 300,
                "invalid throttle sleep should fall back to default");

        unsetenv("GPAGENT_SAML_THROTTLE_SLEEP_SECONDS");
    }

    return 0;
}
