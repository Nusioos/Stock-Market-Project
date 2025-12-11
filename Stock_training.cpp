#include <iostream>
#include <sqlite3.h>
#include <cstdio>
#include <vector>
#include <curl/curl.h>
#include <string>
#include "Api_connetion.hpp"
using namespace std;

class SQLmanager
{

    struct stock
    {
        string symbol;
        double price;
        string API_URL;
    };

public:
    sqlite3 *db;
    char *errMsg = nullptr;
    stock stock_object;
    bool Initialize_database()
    {
        if (sqlite3_open("test.db", &db))
        {
            cerr << "fail" << sqlite3_errmsg(db) << endl;
            return 1;
        }
        cout << "Success" << endl;

        const char *createTableSQL =
            "CREATE TABLE IF NOT EXISTS stocks ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "symbol TEXT NOT NULL, "
            "price REAL,"
            "API_URL TEXT NOT NULL);";

        if (sqlite3_exec(db, createTableSQL, nullptr, nullptr, &errMsg) != SQLITE_OK)
        {
            std::cerr << "Błąd tworzenia tabeli: " << errMsg << std::endl;
            sqlite3_free(errMsg);
        }
        else
        {
            std::cout << "Tabela gotowa!" << std::endl;
        }
        return true;
    }
    void Insert_to_table(stock row, sqlite3 *db, char *errMsg)
    {
        const char *sql = "INSERT INTO stocks (symbol, price, API_URL) VALUES (?, ?, ?)";
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
    void Delete_database()
    {
        int result = remove("test.db");
        if (result == 0)
        {
            cout << "Baza usunieta " << endl;
        }
        else
        {
            cout << "Error nie da sie usunac bazy" << endl;
        }
    }

    bool IS_database_empty(sqlite3 *db, int list_size, int &amount_of_elements_added)
    {
        sqlite3_stmt *stmt;

        if (sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM stocks;", -1, &stmt, nullptr) != SQLITE_OK)
        {
            return false;
        }
        int row_count = 0;
        if (sqlite3_step(stmt) == SQLITE_ROW)
        {
            row_count = sqlite3_column_int(stmt, 0);
        }
        sqlite3_finalize(stmt);
        amount_of_elements_added = list_size - row_count;
        if (row_count == 0)
            return true;
        if (row_count < list_size)
            return true;

        return false;
    }
};
class SQL_list_of_stocks : public SQLmanager
{
private:
    vector<tuple<string, double, string>> StockList = {
        {"solana", 0, "https://api.coingecko.com/api/v3/simple/price?ids=solana&vs_currencies=usd"},
        {"bitcoin", 0, "https://api.coingecko.com/api/v3/simple/price?ids=bitcoin&vs_currencies=usd"},
        {"ethereum", 0, "https://api.coingecko.com/api/v3/simple/price?ids=ethereum&vs_currencies=usd"}
         }; // add from left side to not crash the adding algorithm  for exemple: {new_element},{old_element}  not : {old_element},{new_element}

public:
    int Stock_elements_size = StockList.size();
    void Insert_basic_stocks(SQLmanager *manager, sqlite3 *db, char *errMsg, int &amount_of_elements_added)
    {
        for (int i = amount_of_elements_added - 1; i >= 0; i--)
        {
            manager->stock_object.symbol = get<0>(StockList[i]);
            manager->stock_object.price = get<1>(StockList[i]);
            manager->stock_object.API_URL = get<2>(StockList[i]);
            manager->Insert_to_table(manager->stock_object, db, errMsg);
        }
    }
    void fill_stocks(vector<unique_ptr<Api_connection>> &stocks, SQLmanager *manager)
    {
        for (int i = 0; i < Stock_elements_size; i++)
        {
            auto obj = make_unique<Api_connection>(get<2>(StockList[i]), get<0>(StockList[i]), "usd");
            stocks.push_back(move(obj));
        }
    }
    void stock_element_change_price_in_database(sqlite3 *db, double price, string name)
    {
        sqlite3_stmt *stmt;
        string sql = "UPDATE stocks SET price = ? WHERE symbol = ?";

        int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
        if (rc != SQLITE_OK)
        {
            cout << "Error preparing update statement! " << sqlite3_errmsg(db) << endl;
            return;
        }

        sqlite3_bind_double(stmt, 1, price);                         // binding price
        sqlite3_bind_text(stmt, 2, name.c_str(), -1, SQLITE_STATIC); // symbol

        rc = sqlite3_step(stmt);
        if (rc != SQLITE_DONE)
        {
            cout << "Error updating price: " << sqlite3_errmsg(db) << endl;
        }
        else
        {
            cout << "Successfully updated " << name << " price to " << price << endl;
        }

        sqlite3_finalize(stmt);
    }
};
bool Do_ywt_cleardatabase(bool &Is_clearing_database)
{
    cout << "do tyou want to clear database 0-NO 1-YES :" << endl;
    cin >> Is_clearing_database;
    cout << "\033[2J\033[1;1H"; // clearing console
    return Is_clearing_database;
}
int main()
{
    SQLmanager *manager = new SQLmanager();
    SQL_list_of_stocks List_stocks;
    manager->Initialize_database();
    int amount_of_elements_added = 0;
    bool Is_table_empty = manager->IS_database_empty(manager->db, List_stocks.Stock_elements_size, amount_of_elements_added);
    bool Is_clearing_database = Do_ywt_cleardatabase(Is_clearing_database);
    if (Is_clearing_database)
    {
        manager->Delete_database();
        exit(0);
    }
    if (Is_table_empty)
    {
        cout << "Adding elements to database: " << endl;
        List_stocks.Insert_basic_stocks(manager, manager->db, manager->errMsg, amount_of_elements_added);
    }

    auto callback = [](void *, int argc, char **argv, char **colNames) -> int
    {
        std::cout << "Wynik:" << std::endl;
        for (int i = 0; i < argc; i++)
        {
            std::cout << colNames[i] << " = " << (argv[i] ? argv[i] : "NULL") << std::endl;
        }
        std::cout << "-------------------" << std::endl;
        return 0;
    };

    const char *selectSQL = "SELECT * FROM stocks;";
    if (sqlite3_exec(manager->db, selectSQL, callback, nullptr, &manager->errMsg) != SQLITE_OK)
    {
        std::cerr << "Blad SELECT: " << manager->errMsg << std::endl;
        sqlite3_free(manager->errMsg);
    }

    /*auto find = [](void *, int argc, char **argv, char **colNames) -> int
    {
        cout << "Funkcja find" << endl;
        for (int i = 0; i < argc; i++)
        {
            if (argv[i] && string(argv[i]) == "BTC")
            {
                cout << "Znaleziono Bitcoin!" << endl;
            }
        }
        return 0;
    };*/
    vector<unique_ptr<Api_connection>> stocks;
    List_stocks.fill_stocks(stocks, manager);
    while (1)
    {
        cout << "PIck the stock that you want to visit(Name):" << endl;
        string pick_stock;
        cin >> pick_stock;
        for (int i = 0; i < List_stocks.Stock_elements_size; i++)
        {
            if (pick_stock == stocks[i]->currname)
            {
                /*cout << stocks[i]->URL_of_API << endl;
                cout << stocks[i]->currname << endl;
                cout << stocks[i]->currency << endl;*/
                double temp_price = 0;
                stocks[i]->Initialize(stocks[i]->URL_of_API, stocks[i]->currname, stocks[i]->currency, temp_price);
                List_stocks.stock_element_change_price_in_database(manager->db, temp_price, stocks[i]->currname);
            }
        }
        if (sqlite3_exec(manager->db, selectSQL, callback, nullptr, &manager->errMsg) != SQLITE_OK)
        {
            std::cerr << "Blad SELECT: " << manager->errMsg << std::endl;
            sqlite3_free(manager->errMsg);
        }
    }
    sqlite3_close(manager->db);
    delete manager;
}
