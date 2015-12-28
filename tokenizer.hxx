#pragma once
class string_tokenizer {
public:
	string_tokenizer(const std::wstring& source, const wchar_t* delimiter)
		: _source(source), _delimiter(delimiter), _current_pos(0) {

	}

private:
	const std::wstring& _source;
	const wchar_t* _delimiter;
	size_t _current_pos;

public:
	bool has_more() const;

	std::wstring next();
};

void trim(std::wstring& str, const wchar_t* str_to_trim);

std::wstring join(std::vector<std::wstring>& strings, const std::wstring& connector);

std::wstring repeat(const std::wstring& string, const int count, const std::wstring& connector);

extern const std::wstring empty_str;