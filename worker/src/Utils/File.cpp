#define MS_CLASS "Utils::File"
// #define MS_LOG_DEV

#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "Utils.hpp"
#include <cerrno>
#include <sys/stat.h> // stat()

#ifndef _WIN32
#include <unistd.h>   // access(), R_OK
#endif


namespace Utils
{
	void Utils::File::CheckFile(const char* file)
	{
		MS_TRACE();

		struct stat fileStat; // NOLINT(cppcoreguidelines-pro-type-member-init)
		int err;

		// Ensure the given file exists.
		err = stat(file, &fileStat);

		if (err != 0)
			MS_THROW_ERROR("cannot read file '%s': %s", file, std::strerror(errno));

		// Ensure it is a regular file.
		if ((fileStat.st_mode & S_IFREG) ==0)
			MS_THROW_ERROR("'%s' is not a regular file", file);

#ifndef _WIN32
		// Ensure it is readable.
		err = access(file, R_OK);

		if (err != 0)
			MS_THROW_ERROR("cannot read file '%s': %s", file, std::strerror(errno));
#endif
	}
} // namespace Utils
