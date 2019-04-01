#ifndef STRING_HELPERS_H
#define STRING_HELPERS_H

// STL
#include <list>
#include <string>

// PUT
#include <put/cxxutils/posix_helpers.h>


#ifndef LIST_DELIM
#define LIST_DELIM ','
#endif

// explode string by deliminator and remove whitespaces
std::list<std::string> clean_explode(const std::string& str, char delim) noexcept;

int16_t convert_to_runlevel(const std::string& str, int16_t invalid_value);

posix::Signal::EId decode_signal_name(const std::string& signal_name) noexcept;


#endif // STRING_HELPERS_H
