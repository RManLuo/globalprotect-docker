#ifndef CREDENTIALPROVIDER_H
#define CREDENTIALPROVIDER_H

#include <string>

struct OnePasswordCredentials
{
    std::string username;
    std::string password;
    std::string totp;

    bool isValid() const;
};

class OnePasswordCredentialProvider
{
public:
    OnePasswordCredentials fetch(std::string *error = nullptr) const;

private:
    std::string readField(const std::string &field, std::string *error) const;
    std::string readTotp(std::string *error) const;
};

#endif // CREDENTIALPROVIDER_H
