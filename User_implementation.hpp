#ifndef User_implementation_HPP
#define User_implementation_HPP

#include <iostream>
#include <sqlite3.h>
#include <cstdio>
#include <vector>
#include <curl/curl.h>
#include <string>
#include <chrono>
#include <thread>
#include <mutex>
#include <deque>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <atomic>
#include <condition_variable>

using namespace std;

class UserManager
{
private:
    struct bought_stock
    {
        string name_of_a_stock;
        int amount;
        double price_stock;
    };
    struct User
    {
        string name;
        double funds;
        vector<bought_stock> wallet;
    };

public:
    User User_object;
    sqlite3 *database_user;
    char *errmsg = 0;

    bool Initialize_database_user()
    {
        if (sqlite3_open("User_info.db", &database_user))
        {
            cerr << "fail" << sqlite3_errmsg(database_user) << endl;
            return 1;
        }
        cout << "Success" << endl;

        const char *createUserSQL =
            "CREATE TABLE IF NOT EXISTS Users ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "name TEXT NOT NULL UNIQUE, "
            "funds REAL DEFAULT 10000.0);"; 

        const char *createWalletSQL =
            "CREATE TABLE IF NOT EXISTS Wallet ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "user_id INTEGER NOT NULL, "      // which user
            "stock_name TEXT NOT NULL, "     
            "amount INTEGER NOT NULL, "       
            "FOREIGN KEY (user_id) REFERENCES Users(id) ON DELETE CASCADE, "// if suer is deleted everything is
            "UNIQUE(user_id, stock_name));"; //no duplictaes

        if (sqlite3_exec(database_user, createUserSQL, nullptr, nullptr, &errmsg) != SQLITE_OK)
        {
            std::cerr << "Błąd tworzenia tabeli: " << errmsg << std::endl;
            sqlite3_free(errmsg);
        }
        else
        {
            std::cout << "Tabela gotowa!" << std::endl;
        }
        return true;
    }
    void Delete_database()
    {
        bool result = remove("User_info.db");
        if (result == 0)
        {
            cout << "Baza usunieta " << endl;
        }
        else
        {
            cout << "Error nie da sie usunac bazy" << endl;
        }
    }
    bool IS_database_empty(sqlite3 *db)
    {
        sqlite3_stmt *stmt;

        if (sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM Users;", -1, &stmt, nullptr) != SQLITE_OK)
        {
            return false;
        }
        int row_count = 0;
        if (sqlite3_step(stmt) == SQLITE_ROW)
        {
            row_count = sqlite3_column_int(stmt, 0);
        }
        sqlite3_finalize(stmt);
        if (row_count == 0)
            return true;

        return false;
    }
    bool Is_user_already_indatabase(sqlite3 *db, string name)
    {
        sqlite3_stmt *stmt;
        const char *sql_query = "SELECT 1 FROM Users WHERE name = ? LIMIT 1";

        if (sqlite3_prepare_v2(db, sql_query, -1, &stmt, nullptr) != SQLITE_OK)
        {
            return false;
        }

        sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT); // stament,paramter number,what length,-1 (SQL figure it out),copy=Transient/move_og=Static

        bool exists = (sqlite3_step(stmt) == SQLITE_ROW);
        sqlite3_finalize(stmt);

        return exists;
    }

    void Initialize_User(User row, sqlite3 *db, char *errMsg)
    {
        const char *sql = "INSERT INTO stocks (User_Name, funds, API_URL) VALUES (?, ?, ?)";
        sqlite3_stmt *stmt;
        int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
        if (rc != SQLITE_OK)
        {
            cout << "error Insert";
            return;
        }
        sqlite3_bind_text(stmt, 1, row.symbol.c_str(), -1, SQLITE_STATIC); // binding the variables
        sqlite3_bind_double(stmt, 2, row.price);
        sqlite3_bind_text(stmt, 3, row.API_URL.c_str(), -1, SQLITE_STATIC);

        rc = sqlite3_step(stmt); // do query
        if (rc != SQLITE_DONE)
        {
            cout << "query error: " << sqlite3_errmsg(db) << endl;
        }
        sqlite3_finalize(stmt);
    }
    // User musi miec pieniadze
    // mozliwosci kupowanie stocks
    // jesli User zakupi fajnie by było gdyby widział wartość portfela
    // user ma ID,imie,funds,amount,nazwa_stock,stock_price DONE
};
#endif
