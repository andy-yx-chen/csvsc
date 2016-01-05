// csvsc.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <fcntl.h>
#include <io.h>


int _tmain(int argc, wchar_t** argv)
{
	_setmode(_fileno(stdout), _O_U16TEXT);
	if (argc != 2) {
		std::wcout << L"usage:csvsc <csv-file>" << std::endl;
		return 0;
	}

	std::wcout << L"loading..." << std::endl;
	csv_container container(argv[1]);
	std::shared_ptr<command_provider> provider = create_command_provider();
	std::shared_ptr<error_context> err_context(new console_error_context());
	if (!container.run(provider, err_context)) {
		std::wcout << L"ERR: " << container.error() << std::endl;
	}

    return 0;
}

