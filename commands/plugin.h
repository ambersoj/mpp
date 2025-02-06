#ifndef PLUGIN_H
#define PLUGIN_H

#include <string>

extern "C" {
    void execute(const std::string& args);
    std::string getHelp();
}

#endif // PLUGIN_H
