#ifndef SERVICECHECK_H
#define SERVICECHECK_H

#include <string>

bool service_exists(const std::string& service);
bool service_exists(const char* service);

#endif // SERVICECHECK_H
