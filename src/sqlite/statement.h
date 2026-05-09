#ifndef FOUNTAIN_SRC_SQLITE_STATEMENT_H_
#define FOUNTAIN_SRC_SQLITE_STATEMENT_H_

#include <sqlite3.h>

#include <cstdint>
#include <string>

namespace fountain::sqlite {

class Statement {
public:
    Statement() = default;
    explicit Statement(sqlite3_stmt *stmt) : stmt_(stmt) {}
    ~Statement();

    Statement(const Statement &) = delete;
    Statement &operator=(const Statement &) = delete;
    Statement(Statement &&other) noexcept;
    Statement &operator=(Statement &&other) noexcept;

    bool IsValid() const { return stmt_ != nullptr; }
    bool Step();
    bool Done() const { return last_step_code_ == SQLITE_DONE; }
    void Reset();

    bool BindInt(int index, int value);
    bool BindInt64(int index, std::int64_t value);
    bool BindDouble(int index, double value);
    bool BindText(int index, const std::string &value);
    bool BindNull(int index);

    std::int64_t ColumnInt64(int index) const;
    std::string ColumnText(int index) const;
    int LastStepCode() const { return last_step_code_; }

private:
    sqlite3_stmt *stmt_ = nullptr;
    int last_step_code_ = SQLITE_OK;
};

}  // namespace fountain::sqlite
#endif
