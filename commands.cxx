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

bool reduce_command::execute() {
	if (this->_columns.size() != this->_values.size() || this->_columns.size() == 0) {
		this->_error = L"columns and values are not matched or no column is specified.";
	}
	else {
		std::wstring rename_sql(L"ALTER TABLE ");
		rename_sql.append(INMEM_TBL);
		rename_sql.append(L" RENAME TO TEMPTBL");
		std::shared_ptr<sqlite_command> cmd = this->_context->data->create_command(rename_sql.c_str());
		if (!cmd->execute()) {
			this->_error = L"operation failed, please exit and reopen the csv file (SYSTEM ERROR.step.1).";
			return false;
		}

		std::wstring create_table_script(L"CREATE TABLE ");
		create_table_script.append(INMEM_TBL);
		create_table_script.append(L"([");
		create_table_script.append(join(this->_columns, L"] TEXT,["));
		create_table_script.append(L"] TEXT)");
		cmd = this->_context->data->create_command(create_table_script.c_str());
		if (!cmd->execute()) {
			this->_error = L"operation failed, please exit and reopen the csv file (SYSTEM ERROR.step.2).";
			return false;
		}

		std::wstring copy_data_sql(L"INSERT INTO ");
		copy_data_sql.append(INMEM_TBL);
		copy_data_sql.append(L"([");
		copy_data_sql.append(join(this->_columns, L"],["));
		copy_data_sql.append(L"]) SELECT ");
		copy_data_sql.append(join(this->_values, L","));
		copy_data_sql.append(L" FROM TEMPTBL ");
		copy_data_sql.append(L"GROUP BY ");
		copy_data_sql.append(this->_group_by);
		if (this->_order_by.length() > 0) {
			copy_data_sql.append(L" ORDER BY ");
			copy_data_sql.append(this->_order_by);
		}

		cmd = this->_context->data->create_command(copy_data_sql.c_str());
		if (!cmd->execute()) {
			this->_error = L"operation failed, please exit and reopen the csv file (SYSTEM ERROR.step.3).";
			return false;
		}

		cmd = this->_context->data->create_command(L"DROP TABLE TEMPTBL");
		if (!cmd->execute()) {
			this->_error = L"operation failed, please exit and reopen the csv file (SYSTEM ERROR.step.4).";
			return false;
		}
		
		// save the columns information
		this->_context->columns.erase(this->_context->columns.begin(), this->_context->columns.end());
		this->_context->columns.insert(this->_context->columns.begin(), this->_columns.begin(), this->_columns.end());
	}

	return false;
}

bool values_as_columns_command::execute() {
	// read the columns
	std::vector<std::wstring> columns;
	columns.push_back(this->_group_by);
	std::wstring sql(L"SELECT DISTINCT [");
	sql.append(this->_column_source);
	sql.append(L"] FROM ");
	sql.append(INMEM_TBL);
	std::shared_ptr<sqlite_command> cmd = this->_context->data->create_command(sql.c_str());
	std::shared_ptr<sqlite_reader> reader = cmd->execute_reader();
	while (reader->read()) {
		columns.push_back(reader->wstr(0));
	}

	if (columns.size() == 1) {
		this->_error = L"No value for the value expression";
		return false;
	}

	// read the data
	typedef std::unordered_map<std::wstring, std::wstring> data_item;
	typedef std::shared_ptr<data_item> data_item_ptr;
	typedef std::unordered_map<std::wstring, data_item_ptr> data_container;
	data_container data;
	std::wstring select_sql(L"SELECT [");
	select_sql.append(this->_group_by);
	select_sql.append(L"],[");
	select_sql.append(this->_column_source);
	select_sql.append(L"],");
	select_sql.append(this->_value_column);
	select_sql.append(L" FROM ");
	select_sql.append(INMEM_TBL);
	select_sql.append(L" ORDER BY ");
	select_sql.append(this->_group_by);
	cmd = this->_context->data->create_command(select_sql.c_str());
	reader = cmd->execute_reader();
	if (!reader) {
		this->_error = L"execution failed, no data.";
		return false;
	}

	while (reader->read()) {
		std::wstring key(reader->wstr(0));
		data_container::const_iterator it = data.find(key);
		data_item_ptr item;
		if (it == data.end()) {
			item = data_item_ptr(new data_item());
			data.insert(std::make_pair(key, item));
		}
		else {
			item = it->second;
		}

		std::wstring column(reader->wstr(1));
		if (item->find(column) == item->end()) {
			item->insert(std::make_pair(column, reader->wstr(2)));
		}
	}

	if (data.size() == 0) {
		this->_error = L"operation failed, data not found (SYSTEM ERROR.step.1).";
		return false;
	}

	// now, okay to drop the old table
	std::wstring drop_tbl_sql(L"DROP TABLE ");
	drop_tbl_sql.append(INMEM_TBL);
	cmd = this->_context->data->create_command(drop_tbl_sql.c_str());
	if (!cmd->execute()) {
		this->_error = L"operation failed, please exit and reopen the csv file (SYSTEM ERROR.step.2).";
		return false;
	}

	// recreate the table with new schema
	std::wstring create_table_script(L"CREATE TABLE ");
	create_table_script.append(INMEM_TBL);
	create_table_script.append(L"([");
	create_table_script.append(join(columns, L"] TEXT,["));
	create_table_script.append(L"] TEXT)");
	cmd = this->_context->data->create_command(create_table_script.c_str());
	if (!cmd->execute()) {
		this->_error = L"operation failed, please exit and reopen the csv file (SYSTEM ERROR.step.3).";
		return false;
	}

	std::wstring insert_sql(L"INSERT INTO ");
	insert_sql.append(INMEM_TBL);
	insert_sql.append(L"([");
	insert_sql.append(join(columns, L"],["));
	insert_sql.append(L"]) VALUES(");
	insert_sql.append(repeat(L"?", (int)columns.size(), L","));
	insert_sql.append(L")");
	for (data_container::const_iterator it = data.begin(); it != data.end(); ++it) {
		std::shared_ptr<sqlite_command> insert_command = this->_context->data->create_command(insert_sql.c_str());
		insert_command->bind_wstr(1, it->first.c_str());
		for (size_t i = 1; i < columns.size(); ++i) {
			data_item_ptr item = it->second;
			data_item::const_iterator value_it = item->find(columns[i]);
			if (value_it == item->end()) {
				insert_command->bind_wstr((int)i + 1, this->_default_value.c_str());
			}
			else {
				insert_command->bind_wstr((int)i + 1, value_it->second.c_str());
			}
		}

		if (!insert_command->execute()) {
			this->_error = L"operation failed, please exit and reopen the csv file (SYSTEM ERROR.step.4).";
			return false;
		}
	}

	// save the columns information
	this->_context->columns.erase(this->_context->columns.begin(), this->_context->columns.end());
	this->_context->columns.insert(this->_context->columns.begin(), columns.begin(), columns.end());
	return false;
}