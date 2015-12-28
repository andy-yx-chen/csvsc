#include "stdafx.h"

csv_container::csv_container(const wchar_t* csv_file) 
	: _error(){
	std::wifstream input(csv_file);
	if (input.good()) {
		input.imbue(std::locale(input.getloc(), new std::codecvt_utf8<wchar_t, 0x10ffff, std::consume_header>()));
		this->_context = new context;
		this->_data_connection = new sqlite_connection;
		std::wstring header;
		std::getline(input, header);

		string_tokenizer tokenizer(header, L",");
		while (tokenizer.has_more()) {
			std::wstring column = tokenizer.next();
			trim(column, L" ");
			this->_context->columns.push_back(column);
		}

		if (this->_context->columns.size() == 0) {
			this->_error = L"no column info";
			delete this->_context;
			delete this->_data_connection;
			this->_context = nullptr;
			this->_data_connection = nullptr;
			input.close();
			return;
		}
		

		std::wstring create_table_script(L"CREATE TABLE ");
		create_table_script.append(INMEM_TBL);
		create_table_script.append(L"([");
		create_table_script.append(join(this->_context->columns, L"] TEXT,["));
		create_table_script.append(L"] TEXT)");
		this->_data_connection->open(L":memory:");
		std::shared_ptr<sqlite_command> cmd = this->_data_connection->create_command(create_table_script.c_str());
		if (!cmd->execute()) {
			this->_error = L"failed to initialize the memory";
			this->_data_connection->close();
			delete this->_context;
			delete this->_data_connection;
			this->_context = nullptr;
			this->_data_connection = nullptr;
			input.close();
			return;
		}

		int row_index = 0;

		std::wstring insert_sql(L"INSERT INTO ");
		insert_sql.append(INMEM_TBL);
		insert_sql.append(L"([");
		insert_sql.append(join(this->_context->columns, L"],["));
		insert_sql.append(L"]) VALUES(");
		insert_sql.append(repeat(L"?", (int)this->_context->columns.size(), L","));
		insert_sql.append(L")");
		while (!input.eof()) {
			std::wstring line;
			std::getline(input, line);
			++row_index;
			if (line.size() == 0) {
				continue;
			}

			int values_count = 0;
			string_tokenizer values(line, L",");
			std::shared_ptr<sqlite_command> insert_command = this->_data_connection->create_command(insert_sql.c_str());
			while (values.has_more()) {
				std::wstring value = values.next();
				if (value.length() > 0) {
					trim(value, L" ");
				}

				values_count++;
				insert_command->bind_wstr(values_count, value.c_str());
			}

			if (values_count != this->_context->columns.size() || !insert_command->execute()) {
				this->_error = L"invalid row at " + row_index;
				this->_data_connection->close();
				delete this->_context;
				delete this->_data_connection;
				this->_context = nullptr;
				this->_data_connection = nullptr;
				input.close();
				return;
			}
		}
	}
	else {
		this->_error = L"invalid file";
	}
}

csv_container::~csv_container() {
	if (this->_context != nullptr) {
		delete this->_context;
	}

	if (this->_data_connection != nullptr) {
		this->_data_connection->close();
		delete this->_data_connection;
	}
}

bool csv_container::run(std::shared_ptr<command_provider> command_provider, std::shared_ptr<error_context> err_context) {
	if (this->_data_connection == nullptr) {
		return false;
	}

	this->_context->data = this->_data_connection;
	while (true) {
		std::shared_ptr<command> command = command_provider->next_command(this->_context);
		if (command->execute()) {
			break;
		}

		if (command->has_error()) {
			err_context->on_command_error(command->error());
		}
	}

	return true;
}