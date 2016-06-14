// csvsc.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <fcntl.h>
#include <io.h>


int _tmain(int argc, wchar_t** argv)
{
	_setmode(_fileno(stdout), _O_U16TEXT);
	if (argc < 2) {
		std::wcout << L"usage:csvsc <csv-file> [-i <script-file>]" << std::endl;
		return 0;
	}

	std::wcout << L"loading..." << std::endl;
	csv_container container(argv[1]);
    std::wifstream* script_input = nullptr;
    if (argc > 3 && ::_wcsicmp(argv[2], L"-i") == 0) {
        script_input = new std::wifstream(argv[3]);
        if (script_input->good()) {
            script_input->imbue(std::locale(script_input->getloc(), new std::codecvt_utf8<wchar_t, 0x10ffff, std::consume_header>()));
        }
        else {
            script_input->close();
            script_input = nullptr;
        }
    }

	std::shared_ptr<command_provider> provider = create_command_provider(script_input == nullptr ? (std::basic_istream<wchar_t>&) std::wcin : (std::basic_istream<wchar_t>&) *script_input);
	std::shared_ptr<error_context> err_context(new console_error_context());
	if (!container.run(provider, err_context)) {
		std::wcout << L"ERR: " << container.error() << std::endl;
	}

    return 0;
}

