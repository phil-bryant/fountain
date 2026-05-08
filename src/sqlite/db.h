#pragma once

#include <sqlite3.h>

#include <string>

namespace fountain::sqlite {

class Statement;

class Database {
public:
    Database() = default;
    ~Database();

    Database(const Database &) = delete;
    Database &operator=(const Database &) = delete;
    Database(Database &&other) noexcept;
    Database &operator=(Database &&other) noexcept;

    bool Open(const std::string &path);
    bool IsOpen() const;
    void Close();
    bool Exec(const std::string &sql) const;
    Statement Prepare(const std::string &sql) const;
    sqlite3 *raw() const { return db_; }

private:
    sqlite3 *db_ = nullptr;
};

}  // namespace fountain::sqlite
