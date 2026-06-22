#include "autoreconnectpolicy.h"

#include <array>

namespace
{
constexpr std::array<int, 5> delays { 30, 60, 120, 240, 300 };
}

AutoReconnectPolicy::AutoReconnectPolicy(bool enabled)
    : m_enabled(enabled)
{
}

bool AutoReconnectPolicy::shouldRetry(bool explicitDisconnect) const
{
    return m_enabled && !explicitDisconnect && m_attempts < delays.size();
}

int AutoReconnectPolicy::nextDelaySeconds() const
{
    if (m_attempts >= delays.size()) {
        return delays.back();
    }
    return delays.at(m_attempts);
}

void AutoReconnectPolicy::recordAttempt()
{
    if (m_attempts < delays.size()) {
        m_attempts++;
    }
}

void AutoReconnectPolicy::reset()
{
    m_attempts = 0;
}
