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
    double funds;
    struct bought_stock
    {
        string name_of_a_stock;
        int amount;
        double price_stock;
    };

public:

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

        const char *createTableSQL =
            "CREATE TABLE IF NOT EXISTS Users ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "User_Name TINYTEXT NOT NULL, "
            "funds REAL"
            "amount INTEGER NOT NULL, "
            "Stock_Name TINYTEXT NOT NULL, "
            "price_stock REAL);";

        if (sqlite3_exec(database_user, createTableSQL, nullptr, nullptr, &errmsg) != SQLITE_OK)
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
  

    //User musi miec pieniadze
    //mozliwosci kupowanie stocks
    //jesli User zakupi fajnie by było gdyby widział wartość portfela
    //user ma ID,imie,funds,amount,nazwa_stock,stock_price  
};
#endif
