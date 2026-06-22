#ifndef AUTORECONNECTPOLICY_H
#define AUTORECONNECTPOLICY_H

#include <cstddef>

class AutoReconnectPolicy
{
public:
    explicit AutoReconnectPolicy(bool enabled);

    bool shouldRetry(bool explicitDisconnect) const;
    int nextDelaySeconds() const;
    void recordAttempt();
    void reset();

private:
    bool m_enabled;
    size_t m_attempts { 0 };
};

#endif // AUTORECONNECTPOLICY_H
