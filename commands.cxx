#include "stdafx.h"

bool save_command::execute() {
	std::wstring sql(L"SELECT [");
	sql.append(join(this->_context->columns, L"],["));
	sql.append(L"] FROM ");
	sql.append(INMEM_TBL);
	sql.append(L" ORDER BY ");
	if (this->_order_by.length() > 0) {
		sql.append(this->_order_by);
	}
	else {
		sql.append(L"[");
		sql.append(join(this->_context->columns, L"],["));
		sql.append(L"]");
	}

	std::shared_ptr<sqlite_command> command = this->_context->data->create_command(sql.c_str());
	std::shared_ptr<sqlite_reader> reader = command->execute_reader();
	if (reader == nullptr) {
		this->_error = L"failed to retrive data from memory buffer (SYSTEM ERROR).";
	}
	else {
		std::ofstream out(this->_file_name);
		// utf8 BOM
		out.put((char)0xEF);
		out.put((char)0xBB);
		out.put((char)0xBF);
		std::wstring_convert<std::codecvt_utf8<wchar_t>> utf8conv;
		out << utf8conv.to_bytes(join(this->_context->columns, L",")) << std::endl;
		while (reader->read()) {
			size_t index = 1;
			while (index < this->_context->columns.size()) {
				out << utf8conv.to_bytes(reader->wstr((int)index - 1)) << ",";
				index++;
			}

			out << utf8conv.to_bytes(reader->wstr((int)index - 1)) << std::endl;
		}

		out.flush();
		out.close();
	}
	return false;
}

bool add_command::execute() {
	if (this->_values.size() != this->_context->columns.size()) {
		this->_error = L"more or less values than expected";
	}
	else {
		std::wstring insert_sql(L"INSERT INTO ");
		insert_sql.append(INMEM_TBL);
		insert_sql.append(L" VALUES(");
		insert_sql.append(repeat(L"?", (int)this->_context->columns.size(), L","));
		insert_sql.append(L")");
		std::shared_ptr<sqlite_command> insert_command = this->_context->data->create_command(insert_sql.c_str());
		for (size_t i = 1; i <= this->_values.size(); ++i) {
			insert_command->bind_wstr((int)i, this->_values[(int)i - 1].c_str());
		}

		if (!insert_command->execute()) {
			this->_error = L"failed to add values into buffer (SYSTEM ERROR)";
		}
	}

	return false;
}

bool show_command::execute() {
	std::vector<std::wstring>& columns(this->_columns.size() > 0 ? this->_columns : this->_context->columns);
	std::wstring sql(L"SELECT [");
	sql.append(join(columns, L"],["));
	sql.append(L"] FROM ");
	sql.append(INMEM_TBL);
	if (this->_query.length() > 0) {
		sql.append(L" WHERE ");
		sql.append(this->_query);
	}

	if (this->_order_by.length() > 0) {
		sql.append(L" ORDER BY ");
		sql.append(this->_order_by);
	}

	std::shared_ptr<sqlite_command> command = this->_context->data->create_command(sql.c_str());
	std::shared_ptr<sqlite_reader> reader = command->execute_reader();
	if (reader == nullptr) {
		this->_error = L"failed to retrive data from memory buffer (SYSTEM ERROR).";
	}
	else {
		std::wcout << join(columns, L",") << std::endl;
		while (reader->read()) {
			size_t index = 1;
			while (index < columns.size()) {
				std::wcout << reader->wstr((int)index - 1) << L",";
				index++;
			}

			std::wcout << reader->wstr((int)index - 1) << std::endl;
		}
	}

	return false;
}

bool batch_add_command::execute() {
	if (this->_context->columns.size() != this->_value_providers.size()) {
		this->_error = L"more or less value providers expected";
	}
	else {
		bool stop = false;
		while (!stop) {
			stop = true;
			std::shared_ptr<add_command> addcmd(new add_command(this->_context));
			for (size_t i = 0; i < this->_value_providers.size(); ++i) {
				addcmd->add_value(this->_value_providers[i]->next_value());
				stop &= this->_value_providers[i]->no_more();
			}

			addcmd->execute();
			if (addcmd->has_error()) {
				this->_error = addcmd->error();
				break;
			}
		}
	}

	return false;
}

bool update_command::execute() {
	std::wstring sql(L"UPDATE ");
	sql.append(INMEM_TBL);
	sql.append(L" SET ");
	sql.append(this->_set_values);
	sql.append(L" WHERE ");
	sql.append(this->_query);
	std::shared_ptr<sqlite_command> command = this->_context->data->create_command(sql.c_str());
	if (!command->execute()) {
		this->_error = L"failed to do the update (SYSTEM ERROR)";
	}

	return false;
}

bool delete_command::execute() {
	std::wstring sql(L"DELETE FROM ");
	sql.append(INMEM_TBL);
	sql.append(L" WHERE ");
	sql.append(this->_query);
	std::shared_ptr<sqlite_command> command = this->_context->data->create_command(sql.c_str());
	if (!command->execute()) {
		this->_error = L"failed to remove the data you specified (SYSTEM ERROR)";
	}

	return false;
}

bool delete_column_command::execute() {
	std::vector<std::wstring>::iterator it = std::find(this->_context->columns.begin(), this->_context->columns.end(), this->_column);
	if (it != this->_context->columns.end()) {
		this->_context->columns.erase(it);
		/* The following code is not necessary, as once we drop the column from the context, then */
		/* the column will not be shown nor saved*/
		/*
		std::wstring rename_sql(L"ALTER TABLE ");
		rename_sql.append(INMEM_TBL);
		rename_sql.append(L" RENAME TO TEMPTBL");
		std::shared_ptr<sqlite_command> cmd = this->_context->data->create_command(rename_sql.c_str());
		if (!cmd->execute()) {
			this->_error = L"failed to completely remove the column, please exit and reopen the csv file (SYSTEM ERROR.step.1).";
			return false;
		}

		std::wstring create_table_script(L"CREATE TABLE ");
		create_table_script.append(INMEM_TBL);
		create_table_script.append(L"([");
		create_table_script.append(join(this->_context->columns, L"] TEXT,["));
		create_table_script.append(L"] TEXT)");
		cmd = this->_context->data->create_command(create_table_script.c_str());
		if (!cmd->execute()) {
			this->_error = L"failed to completely remove the column, please exit and reopen the csv file (SYSTEM ERROR.step.2).";
			return false;
		}

		std::wstring copy_data_sql(L"INSERT INTO ");
		copy_data_sql.append(INMEM_TBL);
		copy_data_sql.append(L"([");
		copy_data_sql.append(join(this->_context->columns, L"],["));
		copy_data_sql.append(L"]) SELECT [");
		copy_data_sql.append(join(this->_context->columns, L"],["));
		copy_data_sql.append(L"] FROM TEMPTBL");
		cmd = this->_context->data->create_command(copy_data_sql.c_str());
		if (!cmd->execute()) {
			this->_error = L"failed to completely remove the column, please exit and reopen the csv file (SYSTEM ERROR.step.3).";
			return false;
		}

		cmd = this->_context->data->create_command(L"DROP TABLE TEMPTBL");
		if (!cmd->execute()) {
			this->_error = L"failed to completely remove the column, please exit and reopen the csv file (SYSTEM ERROR.step.4).";
			return false;
		}
		*/
	}
	else {
		this->_error = L"column no found.";
	}

	return false;
}

bool add_column_command::execute() {
	std::wstring add_column_sql(L"ALTER TABLE ");
	add_column_sql.append(INMEM_TBL);
	add_column_sql.append(L" ADD COLUMN [");
	add_column_sql.append(this->_column);
	add_column_sql.append(L"] TEXT DEFAULT '");
	add_column_sql.append(this->_default_value);
	add_column_sql.append(L"'");
	std::shared_ptr<sqlite_command> cmd = this->_context->data->create_command(add_column_sql.c_str());
	if (!cmd->execute()) {
		this->_error = L"failed to apply the change (SYSTEM ERROR).";
	}
	else {
		this->_context->columns.push_back(this->_column);
	}

	return false;
}