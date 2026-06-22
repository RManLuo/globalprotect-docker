#include "credentialprovider.h"

#include <array>
#include <cerrno>
#include <cstring>
#include <cstdlib>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

namespace
{
std::string getenvOrEmpty(const char *name)
{
    const char *value = std::getenv(name);
    return value == nullptr ? "" : value;
}

std::string trim(std::string value)
{
    while (!value.empty() && (value.back() == '\n' || value.back() == '\r' || value.back() == ' ' || value.back() == '\t')) {
        value.pop_back();
    }
    size_t start = 0;
    while (start < value.size() && (value[start] == ' ' || value[start] == '\t' || value[start] == '\n' || value[start] == '\r')) {
        start++;
    }
    return value.substr(start);
}

bool runProcess(const std::vector<std::string> &args, std::string *output)
{
    std::array<int, 2> pipeFd;
    if (pipe(pipeFd.data()) != 0) {
        return false;
    }

    const pid_t pid = fork();
    if (pid < 0) {
        close(pipeFd[0]);
        close(pipeFd[1]);
        return false;
    }

    if (pid == 0) {
        dup2(pipeFd[1], STDOUT_FILENO);
        dup2(pipeFd[1], STDERR_FILENO);
        close(pipeFd[0]);
        close(pipeFd[1]);

        std::vector<char *> argv;
        argv.reserve(args.size() + 1);
        for (const auto &arg : args) {
            argv.push_back(const_cast<char *>(arg.c_str()));
        }
        argv.push_back(nullptr);
        execvp(argv[0], argv.data());
        _exit(127);
    }

    close(pipeFd[1]);
    output->clear();

    char buffer[256];
    ssize_t bytesRead = 0;
    while ((bytesRead = read(pipeFd[0], buffer, sizeof(buffer))) > 0) {
        output->append(buffer, static_cast<size_t>(bytesRead));
    }
    close(pipeFd[0]);

    int status = 0;
    if (waitpid(pid, &status, 0) < 0) {
        return false;
    }

    return WIFEXITED(status) && WEXITSTATUS(status) == 0;
}

std::vector<std::string> baseOpArgs()
{
    const std::string opPath = getenvOrEmpty("GPAGENT_OP_PATH").empty() ? "op" : getenvOrEmpty("GPAGENT_OP_PATH");
    const std::string item = getenvOrEmpty("GPAGENT_OP_ITEM");
    std::vector<std::string> args { opPath, "item", "get", item };

    const std::string vault = getenvOrEmpty("GPAGENT_OP_VAULT");
    if (!vault.empty()) {
        args.push_back("--vault");
        args.push_back(vault);
    }

    return args;
}
}

bool OnePasswordCredentials::isValid() const
{
    return !username.empty() && !password.empty() && !totp.empty();
}

OnePasswordCredentials OnePasswordCredentialProvider::fetch(std::string *error) const
{
    if (error != nullptr) {
        error->clear();
    }

    if (getenvOrEmpty("GPAGENT_OP_ITEM").empty()) {
        if (error != nullptr) {
            *error = "GPAGENT_OP_ITEM is required";
        }
        return {};
    }

    OnePasswordCredentials credentials;
    credentials.username = readField("username", error);
    if (credentials.username.empty()) {
        return {};
    }

    credentials.password = readField("password", error);
    if (credentials.password.empty()) {
        return {};
    }

    credentials.totp = readTotp(error);
    if (credentials.totp.empty()) {
        return {};
    }

    if (error != nullptr) {
        error->clear();
    }
    return credentials;
}

std::string OnePasswordCredentialProvider::readField(const std::string &field, std::string *error) const
{
    auto args = baseOpArgs();
    args.push_back("--fields");
    args.push_back("label=" + field);
    args.push_back("--reveal");

    std::string output;
    if (!runProcess(args, &output)) {
        if (error != nullptr) {
            *error = "op command failed while reading " + field;
        }
        return "";
    }

    return trim(output);
}

std::string OnePasswordCredentialProvider::readTotp(std::string *error) const
{
    auto args = baseOpArgs();
    args.push_back("--otp");

    std::string output;
    if (!runProcess(args, &output)) {
        if (error != nullptr) {
            *error = "op command failed while reading TOTP";
        }
        return "";
    }

    return trim(output);
}
