#include "stdafx.h"

#define FALSE_ON_NULL(obj) if(NULL == (obj)){\
    return false;\
    }
#define RESULT_OK(exp) (SQLITE_OK == (exp))

sqlite_value::sqlite_value():value_(), is_null_(true){}

sqlite_value::sqlite_value(const char* value): value_(value), is_null_(value == NULL){}

void sqlite_value::swap(sqlite_value& other){
	value_.swap(other.value_);
	std::swap(is_null_, other.is_null_);
}

sqlite_value::sqlite_value(const sqlite_value& other):value_(other.value_), is_null_(other.is_null_){}

sqlite_value& sqlite_value::operator=(const sqlite_value& other){
	sqlite_value temp(other);
	swap(temp);
	return *this;
}

int sqlite_value::as_int() const{
	if( is_null_ ){
		return 0;
	}
	return atoi( value_.c_str() );
}

__int64 sqlite_value::as_int64() const {
	if(is_null_){
		return 0;
	}
	return _atoi64(value_.c_str());
}

const char* sqlite_value::as_string() const{
	if( is_null_ ){
		return NULL;
	}
	return value_.c_str();
}

bool sqlite_value::as_bool() const{
	if( is_null_ ){
		return false;
	}
	return atoi( value_.c_str() ) != 0;
}

bool sqlite_value::is_null() const{
	return is_null_;
}

sqlite_row::sqlite_row():row_data_(){}

sqlite_row::sqlite_row( const sqlite_row& other ) : row_data_(other.row_data_){}

void sqlite_row::swap( sqlite_row& other ){
	row_data_.swap( other.row_data_ );
}

sqlite_row& sqlite_row::operator=(const sqlite_row& other){
	sqlite_row temp(other);
	swap(temp);
	return *this;
}

void sqlite_row::add_column(const char* name, const sqlite_value& value){
	row_data_.insert(std::make_pair(std::string(name), value));
}

bool sqlite_row::has_column(const char* name){
	std::string col_name(name);
	std::map<std::string, sqlite_value>::const_iterator it = row_data_.find( col_name );
	return it != row_data_.end();
}

sqlite_value sqlite_row::operator[] (const char* name){
	std::string col_name(name);
	std::map<std::string, sqlite_value>::const_iterator it = row_data_.find( col_name );
	if( it == row_data_.end() ){
		return sqlite_value();
	}
	return it->second;
}

sqlite_result_set::sqlite_result_set() : row_list_(){}

void sqlite_result_set::add_row(const sqlite_row& row){
	row_list_.push_back( row );
}

int sqlite_result_set::rows_count() const{
	return (int)row_list_.size();
}

sqlite_row sqlite_result_set::get_row(int index) const{
	if( (size_t) index < row_list_.size() ){
		int start = 0;
		const_iterator it = row_list_.begin();
		while( start < index ){
			++start;
			++it;
		}
		return sqlite_row( *it );
	}
	return sqlite_row();
}

std::list<sqlite_row> sqlite_result_set::all() const{
	return row_list_;
}

sqlite_result_set::const_iterator sqlite_result_set::result_const_iterator() const{
	return row_list_.begin();
}

sqlite_result_set::iterator sqlite_result_set::result_iterator(){
	return row_list_.begin();
}

sqlite_connection::sqlite_connection() : db_(NULL), last_error_(), is_open_(false){};
sqlite_connection::~sqlite_connection(){
	if(db_ != NULL){
		sqlite3_close(db_);
		db_ = NULL;
	}
}

bool sqlite_connection::open(const char* filename){
	if( sqlite3_open(filename, &db_) != SQLITE_OK ){
		last_error_ = sqlite3_errmsg( db_ );
		is_open_ = false;
		return is_open_;
	}
	is_open_ = true;
	return is_open_;
}

bool sqlite_connection::open(const wchar_t* filename){
    if( sqlite3_open16(filename, &db_) != SQLITE_OK ){
        last_error_ = sqlite3_errmsg( db_ );
        is_open_ = false;
        return is_open_;
    }
    is_open_ = true;
    return is_open_;
}

bool sqlite_connection::is_open() const{
	return is_open_;
}

void sqlite_connection::close(){
	if( db_ != NULL ){
		sqlite3_close( db_ );
		db_ = NULL;
		is_open_ = false;
	}
}

const std::string& sqlite_connection::last_error() const{
	return last_error_;
}

bool sqlite_connection::execute(const char* sql){
	if( db_ ==  NULL ){
		last_error_ = "database connection haven't opened yet.";
		return false;
	}

	char* errmsg = NULL;
	if( sqlite3_exec( db_, sql, NULL, NULL, &errmsg) != SQLITE_OK ){
		if(errmsg != NULL){
			last_error_ = errmsg;
			sqlite3_free( errmsg );
		}
		return false;
	}
	if(errmsg != NULL){
		sqlite3_free(errmsg);
	}
	return true;
}

__int64 sqlite_connection::last_insert_id(){
	return sqlite3_last_insert_rowid(db_);
}

int sqlite_connection::sqlite_exec_callback(void* param, int colc, char** coltxt, char** colname){
	sqlite_result_set* result_set = static_cast<sqlite_result_set*>(param);
	sqlite_row arow;
	for( int i = 0; i < colc; ++i ){
		arow.add_column( colname[i], sqlite_value(coltxt[i]) );
	}
	result_set->add_row( arow );
	return 0;
}

std::shared_ptr<sqlite_result_set> sqlite_connection::query(const char* sql){
	std::shared_ptr<sqlite_result_set> result_set( new sqlite_result_set() );

	if( db_ ==  NULL ){
		last_error_ = "database connection haven't opened yet.";
		return result_set;
	}

	char* errmsg = NULL;
	if( SQLITE_OK != \
		sqlite3_exec( db_, sql, & sqlite_connection::sqlite_exec_callback, (void*) result_set.get(), &errmsg ) ){
		if( errmsg != NULL ){
			last_error_ = errmsg;
			sqlite3_free(errmsg);
		}
		return result_set;
	}
	if(errmsg != NULL){ //this should not be reachable
		sqlite3_free(errmsg);
	}
	return result_set;
}

std::shared_ptr<sqlite_command> sqlite_connection::create_command(const wchar_t* sql){
    return std::shared_ptr<sqlite_command>(new sqlite_command(*this, sql));
}

std::shared_ptr<sqlite_command> sqlite_connection::create_command(const char* sql){
    return std::shared_ptr<sqlite_command>(new sqlite_command(*this, sql));
}

sqlite_command::sqlite_command(sqlite_connection& conn, const wchar_t* sql): stmt_(NULL){
    sqlite3_prepare16_v2(conn.db_, sql, -1, &stmt_, NULL);
}

sqlite_command::sqlite_command(sqlite_connection& conn, const char* sql) : stmt_(NULL){
    sqlite3_prepare_v2(conn.db_, sql, -1, &stmt_, NULL);
}

bool sqlite_command::bind_blob(int param, const void* data, int bytes){
    FALSE_ON_NULL(stmt_);
    return RESULT_OK(sqlite3_bind_blob(stmt_, param, data, bytes, SQLITE_TRANSIENT));
}

bool sqlite_command::bind_double(int param, double value){
    FALSE_ON_NULL(stmt_);
    return RESULT_OK(sqlite3_bind_double(stmt_, param, value));
}

bool sqlite_command::bind_int(int param, int value){
    FALSE_ON_NULL(stmt_);
    return RESULT_OK(sqlite3_bind_int(stmt_, param, value));
}

bool sqlite_command::bind_int64(int param, __int64 value){
    FALSE_ON_NULL(stmt_);
    return RESULT_OK(sqlite3_bind_int64(stmt_, param, value));
}

bool sqlite_command::bind_str(int param, const char* value){
    FALSE_ON_NULL(stmt_);
    return RESULT_OK(sqlite3_bind_text(stmt_, param, value, -1, SQLITE_TRANSIENT));
}

bool sqlite_command::bind_wstr(int param, const wchar_t* value){
    FALSE_ON_NULL(stmt_);
    return RESULT_OK(sqlite3_bind_text16(stmt_, param, value, -1, SQLITE_TRANSIENT));
}

bool sqlite_command::execute(){
    return SQLITE_DONE == sqlite3_step(stmt_);
}

bool sqlite_command::reset(){
    return RESULT_OK(sqlite3_reset(stmt_));
}

bool sqlite_command::clear(){
    sqlite3_clear_bindings(stmt_);
    return true;
}

sqlite_command::~sqlite_command(){
    if(stmt_ != NULL){
        sqlite3_finalize(stmt_);
        stmt_ = NULL;
    }
}

std::shared_ptr<sqlite_reader> sqlite_command::execute_reader(){
    if(NULL == stmt_){
        return std::shared_ptr<sqlite_reader>(NULL);
    }
    return std::shared_ptr<sqlite_reader>(new sqlite_reader(*this));
}

sqlite_reader::sqlite_reader(sqlite_command& cmd) : command_(cmd){
}

const void* sqlite_reader::blob(int col, int& bytes){
    bytes = 0;
    const void* data = sqlite3_column_blob(command_.stmt_, col);
    bytes = sqlite3_column_bytes(command_.stmt_, col);
    return data;
}

std::wstring sqlite_reader::wstr(int col){
    const wchar_t* text = (const wchar_t*)sqlite3_column_text16(command_.stmt_, col);
    int len = sqlite3_column_bytes16(command_.stmt_, col) / sizeof(wchar_t);
    return std::wstring(text, text+len);
}

std::string sqlite_reader::str(int col){
    const char* text = (const char*)sqlite3_column_text(command_.stmt_, col);
    int len = sqlite3_column_bytes(command_.stmt_, col);
    return std::string(text, text+len);
}

int sqlite_reader::ival(int col){
    return sqlite3_column_int(command_.stmt_, col);
}

__int64 sqlite_reader::i64val(int col){
    return sqlite3_column_int64(command_.stmt_, col);
}

double sqlite_reader::dblval(int col){
    return sqlite3_column_double(command_.stmt_, col);
}

bool sqlite_reader::read(){
    return SQLITE_ROW == sqlite3_step(command_.stmt_);
}