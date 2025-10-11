#include <iostream>
#include <sqlite3.h>
#include <cstdio> 
#include <vector>
using namespace std;

class SQLmanager 
{
public:
struct stock
{
string symbol;
double price;
};

stock s;
void Insert_to_table(stock row,sqlite3 *db,char *errMsg)
{
   string query="INSERT INTO stocks (symbol, price) VALUES ('" + row.symbol + "', " + std::to_string(row.price) + ");";

    if (sqlite3_exec(db, query.c_str(), nullptr, nullptr, &errMsg) != SQLITE_OK) {
       cout << "Blad INSERT: " << errMsg << endl;
        sqlite3_free(errMsg);
    } 
}

void Delete_database()
{
int result = remove("test.db");
if (result == 0) {
    cout << "Baza usunieta " <<endl;
} else {
    cout << "Error nie da sie usunac bazy" << endl;
}
}
};
 class SQL_list_of_stocks :SQLmanager
{
public:
    vector<tuple<string, int>> StockList = {
        {"Bitcoin", 30000},
        {"Ethereum", 2000},
        {"Cardano", 1},
        {"Solana", 35},
        {"Polkadot", 20},
        {"Dogecoin", 0},
        {"Litecoin", 150},
        {"Avalanche", 25},
        {"Chainlink", 15},
        {"Shiba", 0}
    };
};


int main() {
    sqlite3 *db;
    char *errMsg =nullptr;
    if(sqlite3_open("test.db",&db))
    {
        cerr << "fail" << sqlite3_errmsg(db) << endl;
        return 1;
    }
    cout << "Success" << endl;
  
      const char *createTableSQL =
        "CREATE TABLE IF NOT EXISTS stocks ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "symbol TEXT NOT NULL, "
        "price REAL);";

    if (sqlite3_exec(db, createTableSQL, nullptr, nullptr, &errMsg) != SQLITE_OK) {
        std::cerr << "Błąd tworzenia tabeli: " << errMsg << std::endl;
        sqlite3_free(errMsg);
    } else {
        std::cout << "Tabela gotowa!" << std::endl;
    }

    SQLmanager* manager=new SQLmanager();
    SQL_list_of_stocks Lista;

 for(int i=0;i<Lista.StockList.size();i++)
  {
    manager->s.symbol = get<0>(Lista.StockList[i]); //get for the tupples 
    manager->s.price  = get<1>(Lista.StockList[i]); 
    manager->Insert_to_table(manager->s,db,errMsg);
  }

 //  manager->Delete_database(); //DELETES WHOLE DATABASE

  /*const char *insertSQL = "INSERT INTO stocks (symbol, price) VALUES ('BTC', 65000.0);";
    if (sqlite3_exec(db, insertSQL, nullptr, nullptr, &errMsg) != SQLITE_OK) {
        std::cerr << "Błąd INSERT: " << errMsg << std::endl;
        sqlite3_free(errMsg);
    } else {
        std::cout << "Dodano dane!" << std::endl;
    }*/
       auto callback = [](void*, int argc, char **argv, char **colNames) -> int {
        std::cout << "Wynik:" << std::endl;
        for (int i = 0; i < argc; i++) {
            std::cout << colNames[i] << " = " << (argv[i] ? argv[i] : "NULL") << std::endl;
        }
        std::cout << "-------------------" << std::endl;
        return 0;
    };



     auto find =[](void*,int argc,char **argv,char **colNames)->int
     {
cout << "Funkcja find" << endl;
for(int i=0;i<argc ;i++)
{
     if (argv[i] && string(argv[i]) == "BTC") {
            cout << "Znaleziono Bitcoin!" << endl;
        }
}
return 0;
     };


        const char *selectSQL = "SELECT * FROM stocks;";
    if (sqlite3_exec(db, selectSQL, callback, nullptr, &errMsg) != SQLITE_OK) {
        std::cerr << "Blad SELECT: " << errMsg << std::endl;
        sqlite3_free(errMsg);
    }
     sqlite3_close(db);
     delete manager;
}
