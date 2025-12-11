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
            "CREATE TABLE IF NOT EXISTS User ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "amount INTEGER NOT NULL, "
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
};
#endif
