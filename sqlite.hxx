#pragma once

class sqlite_value{
public:
	sqlite_value();
	explicit sqlite_value(const char* );
	sqlite_value(const sqlite_value&);
	sqlite_value& operator = (const sqlite_value&);

public:
	void swap(sqlite_value&);

public:
	int as_int() const;
	__int64 as_int64() const;
	const char* as_string() const;
	bool as_bool() const;
	bool is_null() const;
	
private:
	std::string value_;
	bool is_null_;
};

class sqlite_row{
public:
	sqlite_row();
	sqlite_row(const sqlite_row&);
	sqlite_row& operator = (const sqlite_row&);

public:
	void swap(sqlite_row&);

public:
	void add_column(const char*, const sqlite_value& );
	sqlite_value operator[] (const char*);
	bool has_column(const char*);

private:
	std::map<std::string, sqlite_value> row_data_;
};

class sqlite_result_set{
public:
	typedef std::list<sqlite_row>::const_iterator const_iterator;
	typedef std::list<sqlite_row>::iterator iterator;

public:
	sqlite_result_set();

private:
	sqlite_result_set(const sqlite_result_set&);
	sqlite_result_set& operator = (const sqlite_result_set&);

public:
	void add_row(const sqlite_row&);

	int rows_count() const;
	sqlite_row get_row(int) const;
	std::list<sqlite_row> all() const;

	const_iterator result_const_iterator() const;
	iterator result_iterator();
	
private:
	std::list<sqlite_row> row_list_;
};

class sqlite_command;
class sqlite_reader;

class sqlite_connection{
public:
	sqlite_connection();
	~sqlite_connection();
	
private:
	sqlite_connection(const sqlite_connection&);
	sqlite_connection& operator = (const sqlite_connection&);

public:
	bool open(const char*);
    bool open(const wchar_t*);
	void close();
	std::shared_ptr<sqlite_result_set> query(const char*);//execute R
	bool execute(const char*);//execute CUD
	const std::string& last_error() const;
	bool is_open() const;
    std::shared_ptr<sqlite_command> create_command(const wchar_t* sql);
    std::shared_ptr<sqlite_command> create_command(const char* sql);
	__int64 last_insert_id();

private:
	static int sqlite_exec_callback(void* param, int colc, char** coltxt, char** colname);

private:
	sqlite3* db_;
	std::string last_error_;
	bool is_open_;
    friend class sqlite_command;
};

class sqlite_command{
public:
    sqlite_command(sqlite_connection& conn, const wchar_t* sql);
    sqlite_command(sqlite_connection& conn, const char* sql);
    ~sqlite_command();
private:
    //disable copy constructor and assignment operator
    sqlite_command(const sqlite_command&);
    sqlite_command& operator=(const sqlite_command&);

public:
    bool bind_int(int param, int value);
    bool bind_int64(int param, __int64 value);
    bool bind_str(int param, const char* value);
    bool bind_wstr(int param, const wchar_t* value);
    bool bind_double(int param, double value);
    bool bind_blob(int param, const void* data, int data_bytes);
    bool execute();
    std::shared_ptr<sqlite_reader> execute_reader();
    bool reset();
    bool clear();

private:
    sqlite3_stmt* stmt_;

    friend class sqlite_reader;
};

class sqlite_reader{
public:
    sqlite_reader(sqlite_command& command);
private:
    //disable copy constructor and assignment operator
    sqlite_reader(const sqlite_reader&);
    sqlite_reader& operator=(const sqlite_reader&);
public:
    const void* blob(int col, int& bytes);
    std::wstring wstr(int col);
    std::string str(int col);
    int ival(int col);
    __int64 i64val(int col);
    double dblval(int col);
    bool read();

private:
    sqlite_command& command_;
};