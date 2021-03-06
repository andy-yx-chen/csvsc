#include "stdafx.h"
#define SAVE_USAGE L"save\n\
file:> save-to-file-path.csv\n\
order:> [col1] [ASC|DESC], [col2] [ASC|DESC],..., example, [col1],[col2],[col3], default to all columns in ASC\n\
"
#define EXIT_USAGE L"exit\n\
"
#define ADD_USAGE L"add\n\
values:> value1,value2,value3... (values must match the columns)\n\
"
#define BATCH_ADD_USAGE L"batchadd\n\
values:> value-provider1,value-provide2,value-provider3... (value providers must match the columns)\n\
value provider:\n\
    const-provider, const string\n\
    file-provider, @<file-name>\n\
    csv-provider, @<csv-file-name>|column-name\n\
example,\n\
values:> @countries.txt,const-value,@employees.csv/Name\n\
"
#define SHOW_USAGE L"show\n\
columns:> [col1],[col2],[col3],...\n\
query:> [col1] = 'value1' AND [col2] <> 'value2' OR NOT ([col3] = 'value3')\n\
order:> [col1] [ASC|DESC] (or could be empty)\n\
"
#define UPDATE_USAGE L"update\n\
set:> [col1]='value1',[col2]='value2',...\n\
for:> [col1] = 'value1' AND [col2] <> 'value2' OR NOT ([col3] = 'value3')\n\
"
#define DELETE_USAGE L"delete\n\
what:> [col1] = 'value1' AND [col2] <> 'value2' OR NOT ([col3] = 'value3')\n\
"
#define DROP_COLUMN_USAGE L"dropcolumn\n\
column:> col1\n"

#define ADD_COLUMN_USAGE L"addcolumn\n\
column:> coln\n\
default:> default-value\n"

#define REDUCE_USAGE L"reduce\n\
by:> [col1],[col2]\n\
columns:> col1,col2,col3...\n\
values:>[col1],[col2],CAST((CAST([col4] AS REAL) - CAST([col5] AS REAL))/CAST([col4] AS REAL) AS TEXT)\n\
order:> [col1]\n"

#define VALUES_AS_COLUMNS_USAGE L"vac\n\
values-as-columns, which is turning the values into columns for a csv file, e.g.,\n\
Year,Product,Revenue\n\
2015,A,100000\n\
2015,B,150000\n\
2015,C,100000\n\
2014,A,90000\n\
2014,B,130000\n\
with the following parameters\n\
by:> Year\n\
from:> Product\n\
value:> Revenue\n\
default:> 0\n\
would turn the table into\n\
Year,A,B,C\n\
2015,100000,150000,100000\n\
2014,90000,130000,0\n\
with this change, it would be very easy for you to draw charts in excel\n"

void console_error_context::on_command_error(const std::wstring& error) {
	std::wcout << L"ERR: " << error << std::endl;
}

typedef std::basic_istream<wchar_t> istream;

class console_cmd_generator {
public:
	console_cmd_generator(const std::wstring& usage)
		: _usage(usage), _input(std::wcin) {}

    console_cmd_generator(const std::wstring& usage, istream& input)
        : _usage(usage), _input(input) {}

	virtual ~console_cmd_generator() {}

	virtual std::shared_ptr<command> generate(context* context) const = 0;

	const std::wstring& usage() const {
		return this->_usage;
	}

protected:
    void read_line(std::wstring& line) const{
        std::getline(_input, line);
    }

private:
	console_cmd_generator(const console_cmd_generator&);
	console_cmd_generator& operator= (const console_cmd_generator&);

private:
	std::wstring _usage;
    istream& _input;
};

class save_cmd_generator : public console_cmd_generator {
public:
	save_cmd_generator() : console_cmd_generator(SAVE_USAGE) {
	}

    save_cmd_generator(istream& in) : console_cmd_generator(SAVE_USAGE, in) {
    }

	virtual std::shared_ptr<command> generate(context* context) const override;
};

std::shared_ptr<command> save_cmd_generator::generate(context* context) const {
	std::wstring file_name;
	do {
		std::wcout << L"file:> ";
        read_line(file_name);
	} while (file_name.length() == 0);

	std::wstring order_by;
	std::wcout << L"order:> ";
	read_line(order_by);
	return std::move(std::shared_ptr<command>(new save_command(context, file_name, order_by)));
}

class exit_cmd_generator : public console_cmd_generator {
public:
	exit_cmd_generator() : console_cmd_generator(EXIT_USAGE) {
	}

	virtual std::shared_ptr<command> generate(context* context) const override {
		return std::move(std::shared_ptr<command>(new exit_command()));
	}
};

class add_cmd_generator : public console_cmd_generator {
public:
	add_cmd_generator() : console_cmd_generator(ADD_USAGE) {
	}

    add_cmd_generator(istream& in) : console_cmd_generator(ADD_USAGE, in) {
    }
	virtual std::shared_ptr<command> generate(context* context) const override;
};

std::shared_ptr<command> add_cmd_generator::generate(context* context) const {
	std::wstring values;
	do {
		std::wcout << L"values:> ";
		read_line(values);
	} while (values.length() == 0);

	add_command* cmd = new add_command(context);
	string_tokenizer tokenizer(values, L",");
	while (tokenizer.has_more()) {
		cmd->add_value(tokenizer.next());
	}

	return std::move(std::shared_ptr<command>(cmd));
}

class batch_add_cmd_generator : public console_cmd_generator {
public:
	batch_add_cmd_generator() : console_cmd_generator(BATCH_ADD_USAGE) {
	}

    batch_add_cmd_generator(istream& in) : console_cmd_generator(BATCH_ADD_USAGE, in) {
    }

	virtual std::shared_ptr<command> generate(context* context) const override;
};

std::shared_ptr<command> batch_add_cmd_generator::generate(context* context) const {
	std::wstring values;
	do {
		std::wcout << L"values:> ";
		read_line(values);
	} while (values.length() == 0);

	batch_add_command* cmd = new batch_add_command(context);
	string_tokenizer tokenizer(values, L",");
	while (tokenizer.has_more()) {
		std::wstring value = tokenizer.next();
		if (value.length() > 0 && value[0] == L'@') {
			size_t pos = value.find(L'|');
			if (pos == std::wstring::npos) {
				cmd->add_value_provider(std::move(std::shared_ptr<value_provider>(new file_value_provider(value.substr(1)))));
			}
			else {
				cmd->add_value_provider(std::move(std::shared_ptr<value_provider>(new csv_file_value_provider(value.substr(1, pos - 1), value.substr(pos + 1)))));
			}
		}
		else {
			cmd->add_value_provider(std::move(std::shared_ptr<value_provider>(new constant_value_provider(value))));
		}
	}

	return std::move(std::shared_ptr<command>(cmd));
}

class show_cmd_generator : public console_cmd_generator {
public:
	show_cmd_generator() : console_cmd_generator(SHOW_USAGE) {
	}

    show_cmd_generator(istream& in) : console_cmd_generator(SHOW_USAGE, in) {
    }

	virtual std::shared_ptr<command> generate(context* context) const override;
};

std::shared_ptr<command> show_cmd_generator::generate(context* context) const {
	std::wstring columns;
	do {
		std::wcout << L"columns:> ";
		read_line(columns);
	} while (columns.length() == 0);
	
	std::wstring query;
	std::wstring order_by;
	std::wcout << L"query:> ";
	read_line(query);
	std::wcout << L"order:> ";
	read_line(order_by);
	string_tokenizer tokenizer(columns, L",");
	show_command* cmd = new show_command(context, query, order_by);
	while (tokenizer.has_more()) {
		std::wstring column = tokenizer.next();
		if (column.length() > 0) {
			cmd->add_column(column);
		}
	}

	return std::move(std::shared_ptr<command>(cmd));
}

class update_cmd_generator : public console_cmd_generator {
public:
	update_cmd_generator() : console_cmd_generator(UPDATE_USAGE) {
	}

    update_cmd_generator(istream& in) : console_cmd_generator(UPDATE_USAGE, in) {
    }

	virtual std::shared_ptr<command> generate(context* context) const override;
};

std::shared_ptr<command> update_cmd_generator::generate(context* context) const {
	std::wstring query;
	std::wstring set_str;
	std::wcout << L"set:> ";
	read_line(set_str);
	std::wcout << L"query:> ";
	read_line(query);
	return std::move(std::shared_ptr<command>(new update_command(context, query, set_str)));
}

class delete_cmd_generator : public console_cmd_generator {
public:
	delete_cmd_generator() : console_cmd_generator(DELETE_USAGE) {
	}

    delete_cmd_generator(istream& in) : console_cmd_generator(DELETE_USAGE, in) {
    }

	virtual std::shared_ptr<command> generate(context* context) const override;
};

std::shared_ptr<command> delete_cmd_generator::generate(context* context) const {
	std::wstring query;
	std::wcout << L"what:> ";
	read_line(query);
	return std::move(std::shared_ptr<command>(new delete_command(context, query)));
}

class drop_column_cmd_generator : public console_cmd_generator {
public:
	drop_column_cmd_generator() : console_cmd_generator(DROP_COLUMN_USAGE){}

    drop_column_cmd_generator(istream& in) : console_cmd_generator(DROP_COLUMN_USAGE, in) {}

	virtual std::shared_ptr<command> generate(context* context) const override {
		std::wstring column;
		std::wcout << L"column:> ";
		read_line(column);
		return std::move(std::shared_ptr<command>(new delete_column_command(context, column)));
	}
};

class add_column_cmd_generator : public console_cmd_generator {
public:
	add_column_cmd_generator() : console_cmd_generator(ADD_COLUMN_USAGE) {}

    add_column_cmd_generator(istream& in) : console_cmd_generator(ADD_COLUMN_USAGE, in) {}

	virtual std::shared_ptr<command> generate(context* context) const override {
		std::wstring column;
		std::wcout << L"column:> ";
		read_line(column);
		std::wstring default_value;
		std::wcout << L"default:> ";
		read_line(default_value);
		return std::move(std::shared_ptr<command>(new add_column_command(context, column, default_value)));
	}
};

class reduce_cmd_generator : public console_cmd_generator {
public:
	reduce_cmd_generator() : console_cmd_generator(REDUCE_USAGE) {}

    reduce_cmd_generator(istream& in) : console_cmd_generator(REDUCE_USAGE, in) {}

	virtual std::shared_ptr<command> generate(context* context) const override {
		std::wstring group_by;
		do {
			std::wcout << L"by:> ";
			read_line(group_by);
		} while (group_by.length() == 0);

		std::wstring columns;
		do {
			std::wcout << L"columns:> ";
			read_line(columns);
		} while (columns.length() == 0);

		std::wstring values;
		do {
			std::wcout << L"values:> ";
			read_line(values);
		} while (values.length() == 0);

		std::wstring order_by;
		std::wcout << L"order:> ";
		read_line(order_by);

		string_tokenizer tokenizer(columns, L",");
		reduce_command* cmd = new reduce_command(context, group_by, order_by);

		while (tokenizer.has_more()) {
			std::wstring column = tokenizer.next();
			if (column.length() > 0) {
				cmd->add_column(column);
			}
		}

		string_tokenizer value_tokenizer(values, L",");
		while (value_tokenizer.has_more()) {
			std::wstring value = value_tokenizer.next();
			if (value.length() > 0) {
				cmd->add_value(value);
			}
		}

		return std::move(std::shared_ptr<command>(cmd));
	}
};

class vac_cmd_generator : public console_cmd_generator {
public:
	vac_cmd_generator() : console_cmd_generator(VALUES_AS_COLUMNS_USAGE) {}

    vac_cmd_generator(istream& in) : console_cmd_generator(VALUES_AS_COLUMNS_USAGE, in) {}

	virtual std::shared_ptr<command> generate(context* context) const override {
		std::wstring group_by;
		do {
			std::wcout << L"by:> ";
			read_line(group_by);
		} while (group_by.length() == 0);

		std::wstring column_source;
		do {
			std::wcout << L"from:> ";
			read_line(column_source);
		} while (column_source.length() == 0);

		std::wstring value_expression;
		do {
			std::wcout << L"value:> ";
			read_line(value_expression);
		} while (value_expression.length() == 0);

		std::wstring default_value;
		std::wcout << L"default:> ";
		read_line(default_value);

		return std::move(std::shared_ptr<command>(new values_as_columns_command(context, group_by, column_source, value_expression, default_value)));
	}
};

class console_command_provider : public command_provider {
public:
	console_command_provider(istream& in);
public:
	virtual std::shared_ptr<command> next_command(context* context) override;

private:
	std::unordered_map<std::wstring, std::shared_ptr<console_cmd_generator>> _generators;
    istream& _input;
};

console_command_provider::console_command_provider(istream& in) : _generators(), _input(in) {
	this->_generators.insert(std::make_pair(L"add", std::move(std::shared_ptr<console_cmd_generator>(new add_cmd_generator(_input)))));
	this->_generators.insert(std::make_pair(L"save", std::move(std::shared_ptr<console_cmd_generator>(new save_cmd_generator(_input)))));
	this->_generators.insert(std::make_pair(L"exit", std::move(std::shared_ptr<console_cmd_generator>(new exit_cmd_generator()))));
	this->_generators.insert(std::make_pair(L"batchadd", std::move(std::shared_ptr<console_cmd_generator>(new batch_add_cmd_generator(_input)))));
	this->_generators.insert(std::make_pair(L"show", std::move(std::shared_ptr<console_cmd_generator>(new show_cmd_generator(_input)))));
	this->_generators.insert(std::make_pair(L"update", std::move(std::shared_ptr<console_cmd_generator>(new update_cmd_generator(_input)))));
	this->_generators.insert(std::make_pair(L"delete", std::move(std::shared_ptr<console_cmd_generator>(new delete_cmd_generator(_input)))));
	this->_generators.insert(std::make_pair(L"dropcolumn", std::move(std::shared_ptr<console_cmd_generator>(new drop_column_cmd_generator(_input)))));
	this->_generators.insert(std::make_pair(L"addcolumn", std::move(std::shared_ptr<console_cmd_generator>(new add_column_cmd_generator(_input)))));
	this->_generators.insert(std::make_pair(L"reduce", std::move(std::shared_ptr<console_cmd_generator>(new reduce_cmd_generator(_input)))));
	this->_generators.insert(std::make_pair(L"vac", std::move(std::shared_ptr<console_cmd_generator>(new vac_cmd_generator(_input)))));
}

std::shared_ptr<command> console_command_provider::next_command(context* context) {
	while (true) {
		std::wstring cmd;
		std::wcout << L"command:> ";
		std::getline(_input, cmd);
		if (cmd.length() > 0) {
			std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::tolower);
			if (cmd == L"help") {
				for (std::unordered_map<std::wstring, std::shared_ptr<console_cmd_generator>>::const_iterator it = this->_generators.begin();
					it != this->_generators.end();
					++it) {
					std::wcout << it->second->usage() << std::endl;
				}
			}
			else if (cmd == L"columns") {
				for (std::vector<std::wstring>::const_iterator it = context->columns.begin(); it != context->columns.end(); ++it) {
					std::wcout << *it << std::endl;
				}
			}
			else {
				std::unordered_map<std::wstring, std::shared_ptr<console_cmd_generator>>::const_iterator it = this->_generators.find(cmd);
				if (it == this->_generators.end()) {
					std::wcout << L"bad command " << cmd << std::endl;
				}
				else {
					return it->second->generate(context);
				}
			}
		}
	}
}

std::shared_ptr<command_provider> create_command_provider(istream& in) {
	return std::move(std::shared_ptr<command_provider>(new console_command_provider(in)));
}