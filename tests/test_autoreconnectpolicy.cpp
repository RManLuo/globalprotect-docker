#include <cstdlib>
#include <iostream>

#include "autoreconnectpolicy.h"

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
        AutoReconnectPolicy policy(false);
        require(!policy.shouldRetry(false), "disabled policy should not retry");
    }

    {
        AutoReconnectPolicy policy(true);
        require(policy.shouldRetry(false), "enabled policy should retry first unexpected disconnect");
        require(policy.nextDelaySeconds() == 30, "expected first reconnect delay");
        policy.recordAttempt();
        require(policy.shouldRetry(false), "enabled policy should retry second unexpected disconnect");
        require(policy.nextDelaySeconds() == 60, "expected second reconnect delay");
        policy.recordAttempt();
        require(policy.nextDelaySeconds() == 120, "expected third reconnect delay");
        policy.recordAttempt();
        require(policy.nextDelaySeconds() == 240, "expected fourth reconnect delay");
        policy.recordAttempt();
        require(policy.nextDelaySeconds() == 300, "expected fifth reconnect delay");
        policy.recordAttempt();
        require(!policy.shouldRetry(false), "policy should stop after retry budget is exhausted");
    }

    {
        AutoReconnectPolicy policy(true);
        require(!policy.shouldRetry(true), "explicit disconnect should suppress reconnect");
        require(policy.shouldRetry(false), "unexpected disconnect should still retry");
    }

    {
        AutoReconnectPolicy policy(true);
        policy.recordAttempt();
        policy.recordAttempt();
        require(policy.nextDelaySeconds() == 120, "expected advanced delay before reset");
        policy.reset();
        require(policy.nextDelaySeconds() == 30, "reset should restore first delay");
    }

    return 0;
}
