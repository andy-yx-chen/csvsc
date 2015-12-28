#include "stdafx.h"

csv_file_value_provider::csv_file_value_provider(const std::wstring& file, const std::wstring& column)
	: _input(file), _column(-1) {
	if (this->_input.good()) {
		this->_input.imbue(std::locale(this->_input.getloc(), new std::codecvt_utf8<wchar_t, 0x10ffff, std::consume_header>()));
		std::wstring header;
		std::getline(this->_input, header);
		int index = 0;
		string_tokenizer tokenizer(header, L",");
		while (tokenizer.has_more()) {
			std::wstring col = tokenizer.next();
			trim(col, L" ");
			if (col == column) {
				this->_column = index;
				break;
			}

			++index;
		}
	}
}

std::wstring csv_file_value_provider::next_value() {
	if (this->no_more()) {
		return std::move(empty_str);
	}

	std::wstring line;
	std::getline(this->_input, line);
	string_tokenizer tokenizer(line, L",");
	int index = 0;
	while (tokenizer.has_more()) {
		std::wstring value = tokenizer.next();
		if (index == this->_column) {
			return std::move(value);
		}
	}

	return std::move(empty_str);
}