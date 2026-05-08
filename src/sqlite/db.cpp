#include "sqlite/db.h"

#include "sqlite/statement.h"

namespace fountain::sqlite {

Database::~Database() {
    Close();
}

Database::Database(Database &&other) noexcept : db_(other.db_) {
    other.db_ = nullptr;
}

Database &Database::operator=(Database &&other) noexcept {
    if (this != &other) {
        Close();
        db_ = other.db_;
        other.db_ = nullptr;
    }
    return *this;
}

bool Database::Open(const std::string &path) {
    Close();
    const int rc = sqlite3_open_v2(
        path.c_str(),
        &db_,
        SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX,
        nullptr
    );
    if (rc != SQLITE_OK) {
        Close();
        return false;
    }
    return true;
}

bool Database::IsOpen() const {
    return db_ != nullptr;
}

void Database::Close() {
    if (db_ != nullptr) {
        sqlite3_close(db_);
        db_ = nullptr;
    }
}

bool Database::Exec(const std::string &sql) const {
    if (db_ == nullptr) {
        return false;
    }
    char *err = nullptr;
    const int rc = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &err);
    if (err != nullptr) {
        sqlite3_free(err);
    }
    return rc == SQLITE_OK;
}

Statement Database::Prepare(const std::string &sql) const {
    if (db_ == nullptr) {
        return Statement();
    }
    sqlite3_stmt *stmt = nullptr;
    const int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return Statement();
    }
    return Statement(stmt);
}

}  // namespace fountain::sqlite
