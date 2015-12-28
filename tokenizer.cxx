#include "stdafx.h"

const std::wstring empty_str(L"");

bool string_tokenizer::has_more() const {
	return this->_current_pos < this->_source.length();
}

std::wstring string_tokenizer::next() {

	if (!this->has_more()) {
		return empty_str;
	}

	if (this->_current_pos > 0) {
		this->_current_pos += wcslen(this->_delimiter);
	}

	bool quoted = this->_source[this->_current_pos] == L'"';
	size_t pos = std::wstring::npos;
	if (!quoted)
	{
		pos = this->_source.find(this->_delimiter, this->_current_pos);
	}
	else {
		pos = this->_current_pos;
		bool quote_closed = false;
		do {
			pos = this->_source.find(this->_delimiter, pos);
			if (pos == std::wstring::npos) {
				break;
			}

			size_t cursor = pos - 1;
			while (cursor > this->_current_pos && this->_source[cursor--] == L'"');
			quote_closed = ((pos - cursor - 2) & 1) == 1;
			if (quote_closed) {
				break;
			}
			else {
				pos += wcslen(this->_delimiter);
			}
		} while (true);
	}
	if (pos == std::wstring::npos) {
		pos = this->_source.length();
	}

	const wchar_t* token = this->_source.c_str() + this->_current_pos;
	size_t size = pos - this->_current_pos;
	this->_current_pos = pos;
	if (size == 0) {
		return empty_str;
	}
	
	return std::move(std::wstring(token, size));
}

void trim(std::wstring& str, const wchar_t* str_to_trim) {
	// Trim at the begin
	size_t len_to_trim(0);
	size_t cursor(0);
	size_t trim_str_len = wcslen(str_to_trim);
	while (cursor < str.length() && str[cursor] == str_to_trim[cursor % trim_str_len]) cursor++;
	if (cursor > 0) {
		len_to_trim = (cursor / trim_str_len) * trim_str_len;
		str.erase(0, len_to_trim);
	}

	// Trim at the end
	cursor = 0;
	while (str.length() - cursor - 1 >= 0 && str[str.length() - 1 - cursor] == str_to_trim[trim_str_len - 1 - cursor % trim_str_len]) cursor++;
	if (cursor > 0) {
		len_to_trim = (cursor / trim_str_len) * trim_str_len;
		str.erase(str.length() - len_to_trim, len_to_trim);
	}
}

std::wstring join(std::vector<std::wstring>& strings, const std::wstring& connector) {
	std::wstring result;
	for (size_t index = 0; index < strings.size(); ++index) {
		result.append(strings[index]);
		if (index != strings.size() - 1) {
			result.append(connector);
		}
	}

	return std::move(result);
}

std::wstring repeat(const std::wstring& string, const int count, const std::wstring& connector) {
	std::wstring result;
	for (int index = 0; index < count; ++index) {
		result.append(string);
		if (index != count - 1) {
			result.append(connector);
		}
	}

	return std::move(result);
}