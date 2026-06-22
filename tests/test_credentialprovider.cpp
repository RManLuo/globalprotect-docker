#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

#include "credentialprovider.h"

static void require(bool condition, const char *message)
{
    if (!condition) {
        std::cerr << message << "\n";
        std::exit(1);
    }
}

static std::string writeFakeOp(const std::string &body)
{
    char path[] = "/tmp/fake-op-XXXXXX";
    const int fd = mkstemp(path);
    require(fd >= 0, "failed to create fake op script");
    close(fd);

    std::ofstream script(path);
    script << "#!/bin/sh\n" << body << "\n";
    script.close();
    chmod(path, 0700);
    return path;
}

static void clearOpEnvironment()
{
    unsetenv("GPAGENT_OP_PATH");
    unsetenv("GPAGENT_OP_ITEM");
    unsetenv("GPAGENT_OP_VAULT");
}

int main()
{
    clearOpEnvironment();

    const std::string fakeOp = writeFakeOp(
        "case \"$*\" in\n"
        "  *\"--fields username\"*) echo alice ;;\n"
        "  *\"--fields password\"*) echo correct-horse-battery-staple ;;\n"
        "  *\"--otp\"*) echo 123456 ;;\n"
        "  *) echo unexpected >&2; exit 2 ;;\n"
        "esac");

    setenv("GPAGENT_OP_PATH", fakeOp.c_str(), 1);
    setenv("GPAGENT_OP_ITEM", "GlobalProtect", 1);

    {
        OnePasswordCredentialProvider provider;
        std::string error;
        const auto credentials = provider.fetch(&error);

        require(credentials.isValid(), "expected valid credentials");
        require(credentials.username == "alice", "expected username from fake op");
        require(credentials.password == "correct-horse-battery-staple", "expected password from fake op");
        require(credentials.totp == "123456", "expected TOTP from fake op");
        require(error.empty(), "expected empty error after successful fetch");
    }

    {
        unsetenv("GPAGENT_OP_ITEM");
        OnePasswordCredentialProvider provider;
        std::string error;
        const auto credentials = provider.fetch(&error);

        require(!credentials.isValid(), "expected missing item to fail");
        require(error.find("GPAGENT_OP_ITEM") != std::string::npos, "expected missing item error");
    }

    {
        const std::string failingOp = writeFakeOp("echo super-secret-value >&2; exit 9");
        setenv("GPAGENT_OP_PATH", failingOp.c_str(), 1);
        setenv("GPAGENT_OP_ITEM", "GlobalProtect", 1);

        OnePasswordCredentialProvider provider;
        std::string error;
        const auto credentials = provider.fetch(&error);

        require(!credentials.isValid(), "expected failing op to fail credential fetch");
        require(error.find("super-secret-value") == std::string::npos, "expected redacted op failure");
        require(error.find("op command failed") != std::string::npos, "expected redacted op failure message");
    }

    clearOpEnvironment();
    return 0;
}
