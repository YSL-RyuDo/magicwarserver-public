#pragma once
#include <mysql/mysql.h>
#include <memory>
#include <string>
#include <vector>

// Values are always bound, including numeric values. SQL text is static.
// Results are copied before the statement is closed.
inline bool PreparedQuery(MYSQL* db, const char* sql,
    const std::vector<std::string>& values,
    std::vector<std::vector<std::string>>* rows = nullptr)
{
    if (!db) return false;
    std::unique_ptr<MYSQL_STMT, decltype(&mysql_stmt_close)>
        stmt(mysql_stmt_init(db), mysql_stmt_close);
    if (!stmt || mysql_stmt_prepare(stmt.get(), sql,
        static_cast<unsigned long>(std::char_traits<char>::length(sql))) != 0)
        return false;
    if (mysql_stmt_param_count(stmt.get()) != values.size()) return false;

    std::vector<MYSQL_BIND> params(values.size());
    std::vector<unsigned long> lengths(values.size());
    for (size_t i = 0; i < values.size(); ++i)
    {
        lengths[i] = static_cast<unsigned long>(values[i].size());
        params[i].buffer_type = MYSQL_TYPE_STRING;
        params[i].buffer = const_cast<char*>(values[i].data());
        params[i].buffer_length = lengths[i];
        params[i].length = &lengths[i];
    }
    if (!params.empty() && mysql_stmt_bind_param(stmt.get(), params.data()) != 0)
        return false;
    if (mysql_stmt_execute(stmt.get()) != 0) return false;
    if (!rows) return mysql_stmt_field_count(stmt.get()) == 0;

    rows->clear();
    const auto count = mysql_stmt_field_count(stmt.get());
    if (!count || mysql_stmt_store_result(stmt.get()) != 0) return false;
    // First fetch lengths only; then copy each column using fetch_column.
    std::vector<MYSQL_BIND> bindings(count);
    std::vector<unsigned long> sizes(count);
    for (unsigned int i = 0; i < count; ++i)
    {
        bindings[i].buffer_type = MYSQL_TYPE_STRING;
        bindings[i].length = &sizes[i];
    }
    if (mysql_stmt_bind_result(stmt.get(), bindings.data()) != 0) return false;
    for (;;)
    {
        const int status = mysql_stmt_fetch(stmt.get());
        if (status == MYSQL_NO_DATA) return true;
        if (status != 0 && status != MYSQL_DATA_TRUNCATED) return false;
        std::vector<std::string> row(count);
        for (unsigned int i = 0; i < count; ++i)
        {
            if (sizes[i] > 1024 * 1024) return false;
            row[i].resize(sizes[i]);
            MYSQL_BIND column{};
            column.buffer_type = MYSQL_TYPE_STRING;
            column.buffer = row[i].empty() ? nullptr : &row[i][0];
            column.buffer_length = sizes[i];
            if (mysql_stmt_fetch_column(stmt.get(), &column, i, 0) != 0)
                return false;
        }
        rows->push_back(std::move(row));
    }
}
