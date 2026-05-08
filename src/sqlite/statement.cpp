#include "sqlite/statement.h"

namespace fountain::sqlite {

Statement::~Statement() {
    if (stmt_ != nullptr) {
        sqlite3_finalize(stmt_);
    }
}

Statement::Statement(Statement &&other) noexcept
    : stmt_(other.stmt_), last_step_code_(other.last_step_code_) {
    other.stmt_ = nullptr;
    other.last_step_code_ = SQLITE_OK;
}

Statement &Statement::operator=(Statement &&other) noexcept {
    if (this != &other) {
        if (stmt_ != nullptr) {
            sqlite3_finalize(stmt_);
        }
        stmt_ = other.stmt_;
        last_step_code_ = other.last_step_code_;
        other.stmt_ = nullptr;
        other.last_step_code_ = SQLITE_OK;
    }
    return *this;
}

bool Statement::Step() {
    if (stmt_ == nullptr) {
        return false;
    }
    last_step_code_ = sqlite3_step(stmt_);
    return last_step_code_ == SQLITE_ROW;
}

void Statement::Reset() {
    if (stmt_ != nullptr) {
        sqlite3_reset(stmt_);
        sqlite3_clear_bindings(stmt_);
        last_step_code_ = SQLITE_OK;
    }
}

bool Statement::BindInt(const int index, const int value) {
    return stmt_ != nullptr && sqlite3_bind_int(stmt_, index, value) == SQLITE_OK;
}

bool Statement::BindInt64(const int index, const std::int64_t value) {
    return stmt_ != nullptr && sqlite3_bind_int64(stmt_, index, value) == SQLITE_OK;
}

bool Statement::BindDouble(const int index, const double value) {
    return stmt_ != nullptr && sqlite3_bind_double(stmt_, index, value) == SQLITE_OK;
}

bool Statement::BindText(const int index, const std::string &value) {
    return stmt_ != nullptr &&
           sqlite3_bind_text(stmt_, index, value.c_str(), static_cast<int>(value.size()), SQLITE_TRANSIENT) ==
               SQLITE_OK;
}

bool Statement::BindNull(const int index) {
    return stmt_ != nullptr && sqlite3_bind_null(stmt_, index) == SQLITE_OK;
}

std::int64_t Statement::ColumnInt64(const int index) const {
    return sqlite3_column_int64(stmt_, index);
}

std::string Statement::ColumnText(const int index) const {
    const unsigned char *text = sqlite3_column_text(stmt_, index);
    if (text == nullptr) {
        return "";
    }
    return reinterpret_cast<const char *>(text);
}

}  // namespace fountain::sqlite
