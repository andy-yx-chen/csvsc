#pragma once

#define INMEM_TBL L"file_data"

struct context {
	std::vector<std::wstring> columns;
	sqlite_connection* data;
};

class command {
public:
	virtual ~command() {
		//
	}

	command() {}

	virtual bool execute() = 0;

	bool has_error() const {
		return this->_error.size() > 0;
	}

	const std::wstring& error() const {
		return this->_error;
	}

private:
	command(const command&);
	command& operator = (const command&);

protected:
	std::wstring _error;
};

class command_provider {
public:
	command_provider() {}

	virtual ~command_provider() {}
public:
	virtual std::shared_ptr<command> next_command(context* context) = 0;
};

class error_context {
public:
	virtual void on_command_error(const std::wstring& error) = 0;
};

class console_error_context : public error_context {
public:
	virtual void on_command_error(const std::wstring& error) override;
};

class csv_container {
public:
	csv_container(const wchar_t* csv_file);
	~csv_container();
public:
	bool run(std::shared_ptr<command_provider> command_provider, std::shared_ptr<error_context> error_context);

	const std::wstring& error() const {
		return this->_error;
	}
private:
	sqlite_connection* _data_connection;
	context* _context;
	std::wstring _error;
};

class exit_command : public command {
public:
	virtual bool execute() override {
		return true;
	}
};

class save_command : public command {
public:
	save_command(context* context, const std::wstring& file_name, const std::wstring& order_by)
		: _context(context), _file_name(file_name), _order_by(order_by) {
		// empty
	}

	virtual bool execute() override;

private:
	context* _context;
	std::wstring _file_name;
	std::wstring _order_by;
};

class add_command : public command {
public:
	add_command(context* context)
		: _context(context), _values() {
		// empty
	}

	void add_value(const std::wstring& value) {
		this->_values.push_back(value);
	}

	virtual bool execute() override;
private:
	context* _context;
	std::vector<std::wstring> _values;
};

class value_provider {
public:
	virtual ~value_provider() {
		//
	}

	virtual bool no_more() = 0;
	virtual std::wstring next_value() = 0;
};

class constant_value_provider : public value_provider {
public:
	constant_value_provider(const std::wstring& value)
		:_value(value) {
		//
	}

public:
	virtual bool no_more() override {
		return true;
	}

	virtual std::wstring next_value() override {
		return this->_value;
	}

private:
	std::wstring _value;
};

class file_value_provider : public value_provider {
public:
	file_value_provider(const std::wstring& file)
	: _input(file){
		this->_input.imbue(std::locale(this->_input.getloc(), new std::codecvt_utf8<wchar_t, 0x10ffff, std::consume_header>()));
	}

	virtual ~file_value_provider() override{
		this->_input.close();
	}

	virtual bool no_more() override {
		return this->_input.bad() || this->_input.eof();
	}

	virtual std::wstring next_value() override {
		if (this->no_more()) {
			return std::move(empty_str);
		}

		std::wstring value;
		std::getline(this->_input, value);
		return std::move(value);
	}
private:
	std::wifstream _input;
};

class csv_file_value_provider : public value_provider {
public:
	csv_file_value_provider(const std::wstring& file, const std::wstring& column);

	virtual ~csv_file_value_provider() override {
		this->_input.close();
	}

	virtual bool no_more() override {
		return this->_input.bad() || this->_input.eof() || this->_column == -1;
	}

	virtual std::wstring next_value() override;
private:
	std::wifstream _input;
	int _column;
};

class batch_add_command : public command {
public:
	batch_add_command(context* context)
		: _context(context) {
		// empty
	}

	void add_value_provider(std::shared_ptr<value_provider>& value_provider) {
		this->_value_providers.push_back(value_provider);
	}

	virtual bool execute() override;
private:
	context* _context;
	std::vector<std::shared_ptr<value_provider>> _value_providers;
};

class show_command : public command {
public:
	show_command(context* context, const std::wstring& query, const std::wstring& order_by)
		: _context(context), _query(query), _order_by(order_by), _columns() {
		// empty
	}

	virtual bool execute() override;

	void add_column(const std::wstring& column) {
		this->_columns.push_back(column);
	}

private:
	std::wstring _query;
	std::wstring _order_by;
	std::vector<std::wstring> _columns;
	context* _context;
};

class update_command : public command {
public:
	update_command(context* context, const std::wstring& query, const std::wstring& set_values)
		: _context(context), _query(query), _set_values(set_values) {
		// empty
	}

	virtual bool execute() override;

private:
	context* _context;
	std::wstring _query;
	std::wstring _set_values;
};

class delete_command : public command {
public:
	delete_command(context* context, const std::wstring& query)
		: _context(context), _query(query) {
		// empty
	}

	virtual bool execute() override;
private:
	std::wstring _query;
	context* _context;
};

class delete_column_command : public command {
public:
	delete_column_command(context* context, const std::wstring& column)
		: _context(context), _column(column){}

	virtual bool execute() override;
private:
	context* _context;
	std::wstring _column;
};

class add_column_command : public command {
public:
	add_column_command(context* context, const std::wstring& column, const std::wstring& default_value)
		: _context(context), _column(column), _default_value(default_value) {}

	virtual bool execute() override;
private:
	context* _context;
	std::wstring _column;
	std::wstring _default_value;
};

std::shared_ptr<command_provider> create_command_provider();