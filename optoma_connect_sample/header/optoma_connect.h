#ifndef _OPTOMA_CONNECT_H_
#define _OPTOMA_CONNECT_H_

#ifdef OPTOMACONNECT_EXPORTS
#define OPTOMA_API __declspec(dllexport)
#else
#define OPTOMA_API
#endif

#include <vector>
#include <string>

OPTOMA_API void initial();
OPTOMA_API void deinitial();
OPTOMA_API std::vector<std::string> get_device_list();
OPTOMA_API void connect(std::string ssid);
OPTOMA_API void disconnect();

#endif