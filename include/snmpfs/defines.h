#pragma once

#include <stdint.h>
#include <string>

namespace snmpfs {

	inline std::string	APP_AUTHOR			= "Rudolf Ortner";
	inline std::string	APP_NAME			= "snmpfs";
	inline uint8_t		APP_VERSION_MAJOR	= 1;
	inline uint8_t		APP_VERSION_MINOR	= 0;
	inline uint8_t		APP_VERSION_PATCH	= 0;
	inline std::string	APP_VERSION			= std::to_string(APP_VERSION_MAJOR) + "." + std::to_string(APP_VERSION_MINOR) + "." + std::to_string(APP_VERSION_PATCH);

	inline int32_t DEFAULT_INTERVAL			= 5 * 60;

}	// namespace snmpfs
