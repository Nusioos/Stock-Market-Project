
#ifndef Api_connection_HPP
#define Api_connection_HPP

#include <iostream>
#include <string>
#include <curl/curl.h>
#include "nlohmann/json.hpp"
#include <chrono>
#include <thread>
#include <mutex>
#include <deque>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <atomic>
#include <condition_variable>
#include <sqlite3.h>
using namespace std;
using json = nlohmann::json;
class Api_connection_base
{
public:
    virtual void Get_values(string URL_, string name_of_curr, string currency) = 0;
    virtual ~Api_connection_base() {}
    // User var
    virtual bool BuyStock(const string &user_name, double amount, double price) = 0;
    virtual bool SellStock(const string &user_name, double amount, double price) = 0;
    virtual double GetPortfolioValue(const string &user_name) = 0;
};
class Graph_maker
{
public:
    Graph_maker(size_t Bitcoin_prize, deque<size_t> Stocks, size_t maksprice)
    {
        size_t minPrice = Stocks[0];
        size_t maxPrice = Stocks[0];
        const int HEIGHT = 30;

        for (auto p : Stocks)
        {
            if (p < minPrice)
                minPrice = p;
            if (p > maxPrice)
                maxPrice = p;
        }

        for (int y = HEIGHT; y >= 0; y--)
        {
            cout << "|";
            for (size_t x = 0; x < Stocks.size(); x++)
            {
                size_t sum;
                if (maxPrice != minPrice)
                    sum = (Stocks[x] - minPrice) * HEIGHT / (maxPrice - minPrice);
                else
                    sum = HEIGHT;

                if (sum == y)
                {
                    if (x == 0)
                        cout << "x";
                    else if (x >= 0 && Stocks[x] > Stocks[x - 1])
                        cout << "\033[1;32mx\033[0m";
                    else if (x >= 0 && Stocks[x] < Stocks[x - 1])
                        cout << "\033[1;31mx\033[0m";
                    else
                        cout << "\033[1;33mx\033[0m";
                }

                else
                {
                    cout << " ";
                }
            }

            cout << endl;
        }

        cout << "+";
        for (size_t i = 0; i < Stocks.size(); i++)
            cout << "-";
        cout << endl;
    }
};
class Api_connection : public Api_connection_base
{
private:
    double curr = 0;
    mutex mtx;
    atomic<double> btc_price = 0.0;
    atomic<double> prev_btc_price = 0.0;
    atomic<bool> new_data_available{false};
    deque<size_t> btc_price_dequeue;
    atomic<bool> stop_thread{false};
    thread background_thread;
    condition_variable cv;
    mutex cv_mtx;

    // User var
    sqlite3 *user_db;
    string current_user_name;

public:
    string URL_of_API;
    string currname;
    string currency;
    // Stock constructor
    // Api_connection(string URL_of_API, string currname, string currency) : URL_of_API(URL_of_API), currname(currname), currency(currency) {} // add the URL to the API you need after that name that your currency has and currency i only use US dollars so it's for future development
    Api_connection(string URL_of_API, string currname, string currency,
                   sqlite3 *user_db = nullptr, const string &user_name = "")
        : URL_of_API(URL_of_API), currname(currname), currency(currency),
          user_db(user_db), current_user_name(user_name) {}
    // User Constructor
    //  Api_connection(string URL_of_API, string currname, string currency, sqlite3 *user_db) : URL_of_API(URL_of_API), currname(currname), currency(currency), user_db(user_db) {}
    ~Api_connection()
    {
        stop_thread = true;
        if (background_thread.joinable())
        {
            background_thread.join();
        }
    }
    // Delete copy constructor
    Api_connection(const Api_connection &) = delete;
    Api_connection &operator=(const Api_connection &) = delete;

    // Allow moving
    Api_connection(Api_connection &&) = default;
    Api_connection &operator=(Api_connection &&) = default;

    void Initialize(string &URL_of_API, string &currname, string &currency, double &temp_price_to_retrive)
    {
        curl_global_init(CURL_GLOBAL_DEFAULT);
        stop_thread = false;
        new_data_available = false;
        background_thread = thread(&Api_connection::Get_values, this, URL_of_API, currname, currency);
        background_thread.detach();

        while (true)
        {
            if (stop_thread)
                break;

            {
                if (kbhit())
                {
                    char ch = getch();
                    if (ch == 'Q' || ch == 'q')
                    {
                        cout << "Closing section..." << endl;
                        break;
                    }
                    else if (ch == 'M' || ch == 'm')
                    {
                        cout << "\n=== Opening Trading Menu ===" << endl;
                        ShowBuySellMenu(current_user_name); // username
                        cout << "\nResuming price display..." << endl;
                        continue;
                    }
                }

                unique_lock<mutex> lock(cv_mtx);
                cv.wait_for(lock, chrono::seconds(1), [this]()
                            { return new_data_available.load() || stop_thread.load(); });
                if (new_data_available.load())
                {
                    new_data_available.store(false);
                    cout << "\033[2J\033[1;1H";
                    cout << "price of " << currname << ": " << btc_price << " USD" << endl;
                    UpdatePortfolioPrice(current_user_name, btc_price.load());
                    double price_value = btc_price.load();
                    size_t maks_price = max(curr, price_value);
                    Graph_maker(btc_price, btc_price_dequeue, maks_price);
                    prev_btc_price.store(btc_price.load(), std::memory_order_relaxed);
                    temp_price_to_retrive = prev_btc_price;
                    cout << "To stop the program press Q" << endl;
                }
            }
            this_thread::sleep_for(chrono::seconds(1));
        }
        stop_thread = true;
        if (background_thread.joinable())
        {
            stop_thread = true;
            background_thread.join();
        }
    }

    void UpdatePortfolioPrice(const string &user_name, double new_price)
    {
        if (!user_db || user_name.empty())
            return;

        sqlite3_stmt *stmt;
        const char *sql = "UPDATE Wallet SET current_price = ? WHERE user_id = (SELECT id FROM Users WHERE name = ?) AND stock_name = ?";

        if (sqlite3_prepare_v2(user_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        {
            cerr << "Error updating portfolio price: " << sqlite3_errmsg(user_db) << endl;
            return;
        }

        sqlite3_bind_double(stmt, 1, new_price);
        sqlite3_bind_text(stmt, 2, user_name.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 3, currname.c_str(), -1, SQLITE_STATIC);

        if (sqlite3_step(stmt) != SQLITE_DONE)
        {/*Finalizing query*/}

        sqlite3_finalize(stmt);
    }
    bool kbhit()
    {
        struct termios oldt, newt;
        int ch;
        tcgetattr(STDIN_FILENO, &oldt); // terminal state
        newt = oldt;
        newt.c_lflag &= ~(ICANON | ECHO); //  non blocking type
        newt.c_cc[VMIN] = 1;              // min len to read
        newt.c_cc[VTIME] = 0;             // waiting for data
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);

        // set file to not blockign type
        fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);

        ch = getchar();
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt); // go back to old terminal state
        fcntl(STDIN_FILENO, F_SETFL, 0);         // chaning the type back again

        if (ch != EOF)
        {
            ungetc(ch, stdin); // going back to sttarter buffer
            return true;
        }
        return false;
    }
    char getch()
    {
        return getchar();
    }
    static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp) // Used in Get_values
    {
        ((std::string *)userp)->append((char *)contents, size * nmemb);
        return size * nmemb;
    }
    // Buy sell section for User
    void ShowBuySellMenu(const string &user_name)
    {
        cout << "\n=== DEBUG BuyStock ===" << endl;
        cout << "user_db pointer: " << user_db << endl;
        cout << "current_user_name in object: " << current_user_name << endl;
        cout << "user_name parameter: " << user_name << endl;
        if (!user_db)
        {
            cout << "User database not connected!" << endl;
            return;
        }

        while (true)
        {
            cout << "\n=== " << currname << " Trading ===" << endl;
            cout << "Current Price: $" << btc_price.load() << endl;
            cout << "1. Buy " << currname << endl;
            cout << "2. Sell " << currname << endl;
            cout << "3. Check Portfolio" << endl;
            cout << "4. Back to main menu" << endl;
            cout << "Choice: ";

            int choice;
            cin >> choice;

            if (choice == 1)
            {
                cout << "Enter amount to buy: ";
                double amount;
                cin >> amount;
                double current_price = btc_price.load();
                BuyStock(user_name, amount, current_price);
            }
            else if (choice == 2)
            {
                cout << "Enter amount to sell: ";
                double amount;
                cin >> amount;
                double current_price = btc_price.load();
                SellStock(user_name, amount, current_price);
            }
            else if (choice == 3)
            {
                double portfolio_value = GetPortfolioValue(user_name);
                cout << "Total portfolio value: $" << portfolio_value << endl;
            }
            else if (choice == 4)
            {
                break;
            }
        }
    }

    bool BuyStock(const string &user_name, double amount, double price) override
    {
        if (!user_db)
            return false;

        cout << "How much " << currname << " do you want to buy? ";

        if (amount <= 0)
        {
            cout << "Invalid amount!" << endl;
            return false;
        }

        double current_price = btc_price.load();
        double total_cost = amount * current_price;

        cout << "Total cost: $" << total_cost << endl;
        cout << "Confirm purchase? (y/n): ";
        char confirm;
        cin >> confirm;

        if (confirm != 'y' && confirm != 'Y')
        {
            cout << "Purchase cancelled." << endl;
            return false;
        }

        // 1.Check for money
        sqlite3_stmt *stmt;
        const char *sql = "SELECT funds FROM Users WHERE name = ?";

        if (sqlite3_prepare_v2(user_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        {
            cerr << "Database error: " << sqlite3_errmsg(user_db) << endl;
            return false;
        }

        sqlite3_bind_text(stmt, 1, user_name.c_str(), -1, SQLITE_STATIC);

        double user_funds = 0;
        if (sqlite3_step(stmt) == SQLITE_ROW)
        {
            user_funds = sqlite3_column_double(stmt, 0);
        }
        sqlite3_finalize(stmt);

        if (user_funds < total_cost)
        {
            cout << "Insufficient funds! You have: $" << user_funds << endl;
            return false;
        }

        // 2. Add to use wallet
        const char *update_wallet =
            "INSERT INTO Wallet (user_id, stock_name, amount, price_paid, current_price) "
            "VALUES ((SELECT id FROM Users WHERE name = ?), ?, ?, ?, ?) "
            "ON CONFLICT(user_id, stock_name) DO UPDATE SET "
            "amount = amount + ?, "
            "price_paid = ?, "
            "current_price = ?";

        if (sqlite3_prepare_v2(user_db, update_wallet, -1, &stmt, nullptr) != SQLITE_OK)
        {
            cerr << "Database error: " << sqlite3_errmsg(user_db) << endl;
            return false;
        }

        // Binding: 1-5  INSERT 6-8  UPDATE
        sqlite3_bind_text(stmt, 1, user_name.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, currname.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_double(stmt, 3, amount);
        sqlite3_bind_double(stmt, 4, price);
        sqlite3_bind_double(stmt, 5, price);

        sqlite3_bind_double(stmt, 6, amount);
        sqlite3_bind_double(stmt, 7, price);
        sqlite3_bind_double(stmt, 8, price);

        if (sqlite3_step(stmt) != SQLITE_DONE)
        {
            cerr << "Purchase failed: " << sqlite3_errmsg(user_db) << endl;
            sqlite3_finalize(stmt);
            return false;
        }
        sqlite3_finalize(stmt);

        // 3. Take the money out
        const char *update_funds = "UPDATE Users SET funds = funds - ? WHERE name = ?";
        if (sqlite3_prepare_v2(user_db, update_funds, -1, &stmt, nullptr) != SQLITE_OK)
        {
            cerr << "Database error: " << sqlite3_errmsg(user_db) << endl;
            return false;
        }

        sqlite3_bind_double(stmt, 1, total_cost);
        sqlite3_bind_text(stmt, 2, user_name.c_str(), -1, SQLITE_STATIC);

        if (sqlite3_step(stmt) != SQLITE_DONE)
        {
            cerr << "Failed to update funds: " << sqlite3_errmsg(user_db) << endl;
            sqlite3_finalize(stmt);
            return false;
        }

        sqlite3_finalize(stmt);
        cout << "Successfully bought " << amount << " " << currname << " for $" << total_cost << endl;
        return true;
    }

    bool SellStock(const string &user_name, double amount, double price) override
    {
        if (!user_db)
            return false;

        // check amount of stck that user has
        sqlite3_stmt *stmt;
        const char *sql = "SELECT amount FROM Wallet WHERE stock_name = ? AND user_id = (SELECT id FROM Users WHERE name = ?)";

        if (sqlite3_prepare_v2(user_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        {
            cerr << "Database error: " << sqlite3_errmsg(user_db) << endl;
            return false;
        }

        sqlite3_bind_text(stmt, 1, currname.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, user_name.c_str(), -1, SQLITE_STATIC);

        double owned_amount = 0;
        if (sqlite3_step(stmt) == SQLITE_ROW)
        {
            owned_amount = sqlite3_column_double(stmt, 0);
        }
        sqlite3_finalize(stmt);

        if (owned_amount <= 0)
        {
            cout << "You don't own any " << currname << "!" << endl;
            return false;
        }

        cout << "You own " << owned_amount << " " << currname << endl;
        cout << "How much do you want to sell? ";

        if (amount <= 0 || amount > owned_amount)
        {
            cout << "Invalid amount!" << endl;
            return false;
        }

        double current_price = btc_price.load();
        double total_value = amount * current_price;

        cout << "You will receive: $" << total_value << endl;
        cout << "Confirm sale? (y/n): ";
        char confirm;
        cin >> confirm;

        if (confirm != 'y' && confirm != 'Y')
        {
            cout << "Sale cancelled." << endl;
            return false;
        }

        // finally sell your stocks
        if (owned_amount == amount)
        {
            // all in - delete the records
            const char *delete_sql = "DELETE FROM Wallet WHERE stock_name = ? AND user_id = (SELECT id FROM Users WHERE name = ?)";
            if (sqlite3_prepare_v2(user_db, delete_sql, -1, &stmt, nullptr) != SQLITE_OK)
            {
                cerr << "Database error: " << sqlite3_errmsg(user_db) << endl;
                return false;
            }
        }
        else
        {
            // user only sells a part of his crap
            const char *update_sql = "UPDATE Wallet SET amount = amount - ?, current_price = ? WHERE stock_name = ? AND user_id = (SELECT id FROM Users WHERE name = ?)";
            // const char *update_sql = "UPDATE Wallet SET amount = amount - ? WHERE stock_name = ? AND user_id = (SELECT id FROM Users WHERE name = ?)";
            if (sqlite3_prepare_v2(user_db, update_sql, -1, &stmt, nullptr) != SQLITE_OK)
            {
                cerr << "Database error: " << sqlite3_errmsg(user_db) << endl;
                return false;
            }
            sqlite3_bind_double(stmt, 1, amount);
            sqlite3_bind_double(stmt, 2, current_price); // DODAJ aktualną cenę
            sqlite3_bind_text(stmt, 3, currname.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 4, user_name.c_str(), -1, SQLITE_STATIC);
        }

        if (sqlite3_step(stmt) != SQLITE_DONE)
        {
            cerr << "Sale failed: " << sqlite3_errmsg(user_db) << endl;
            sqlite3_finalize(stmt);
            return false;
        }
        sqlite3_finalize(stmt);

        // add funds
        const char *update_funds = "UPDATE Users SET funds = funds + ? WHERE name = ?";
        if (sqlite3_prepare_v2(user_db, update_funds, -1, &stmt, nullptr) != SQLITE_OK)
        {
            cerr << "Database error: " << sqlite3_errmsg(user_db) << endl;
            return false;
        }

        sqlite3_bind_double(stmt, 1, total_value);
        sqlite3_bind_text(stmt, 2, user_name.c_str(), -1, SQLITE_STATIC);

        if (sqlite3_step(stmt) != SQLITE_DONE)
        {
            cerr << "Failed to update funds: " << sqlite3_errmsg(user_db) << endl;
            sqlite3_finalize(stmt);
            return false;
        }

        sqlite3_finalize(stmt);
        cout << "Successfully sold " << amount << " " << currname << " for $" << total_value << endl;
        return true;
    }

    double GetPortfolioValue(const string &user_name)
    {
        if (!user_db)
            return 0;

        cout << "\n=== Portfolio ===" << endl;

        // take the info about funds
        sqlite3_stmt *stmt;
        const char *sql = "SELECT funds FROM Users WHERE name = ?";

        if (sqlite3_prepare_v2(user_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        {
            cerr << "Database error: " << sqlite3_errmsg(user_db) << endl;
            return 0;
        }

        sqlite3_bind_text(stmt, 1, user_name.c_str(), -1, SQLITE_STATIC);

        double funds = 0;
        if (sqlite3_step(stmt) == SQLITE_ROW)
        {
            funds = sqlite3_column_double(stmt, 0);
        }
        sqlite3_finalize(stmt);

        cout << "Cash: $" << funds << endl;

        //  take the stocks
        const char *stocks_sql =
            "SELECT stock_name, amount, price_paid, current_price FROM Wallet "
            "WHERE user_id = (SELECT id FROM Users WHERE name = ?)";

        if (sqlite3_prepare_v2(user_db, stocks_sql, -1, &stmt, nullptr) != SQLITE_OK)
        {
            cerr << "Database error: " << sqlite3_errmsg(user_db) << endl;
            return funds;
        }

        sqlite3_bind_text(stmt, 1, user_name.c_str(), -1, SQLITE_STATIC);
        //  double avg_buy_price = sqlite3_column_double(stmt, 2);
        // double current_price_db = sqlite3_column_double(stmt, 3);
        double total_value_at_buy = funds;
        double total_value_current = funds;
        bool has_stocks = false;

        while (sqlite3_step(stmt) == SQLITE_ROW)
        {
            has_stocks = true;
            string stock_name = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0)); // force change (I dont want to bother with static_cast)
            double amount = sqlite3_column_double(stmt, 1);
            double avg_buy_price = sqlite3_column_double(stmt, 2);
            double current_price_db = sqlite3_column_double(stmt, 3);

            double value_at_buy = amount * avg_buy_price;
            double value_current = amount * current_price_db;
            double profit_loss = value_current - value_at_buy;
            double profit_loss_percent = (avg_buy_price > 0) ? (profit_loss / value_at_buy * 100) : 0; // Fancy notation

            cout << "\nStock: " << stock_name << endl;
            cout << "  Amount: " << amount << " shares" << endl;
            cout << "  Avg Buy Price: $" << avg_buy_price << endl;
            cout << "  Current Price: $" << current_price_db << endl;
            cout << "  Invested: $" << value_at_buy << endl;
            cout << "  Current Value: $" << value_current << endl;
            cout << "  P/L: $" << profit_loss << " (" << profit_loss_percent << "%)" << endl;

            total_value_at_buy += value_at_buy;
            total_value_current += value_current;
        }

        if (!has_stocks)
        {
            cout << "\nNo stocks in portfolio." << endl;
        }

        sqlite3_finalize(stmt);

        cout << "\n=== Summary ===" << endl;
        cout << "Total Invested: $" << total_value_at_buy << endl;
        cout << "Total Current Value: $" << total_value_current << endl;
        cout << "Cash: $" << funds << endl;
        cout << "=== Portfolio Value: $" << total_value_current << " ===" << endl;

        return total_value_current;
    }

    // end of user stuff
    void Get_values(string URL_, string name_of_curr, string currency) // Used in Api connection
    {
        while (!stop_thread.load())
        {

            CURL *curl = curl_easy_init();
            if (curl)
            {
                string readBuffer;
                curl_easy_setopt(curl, CURLOPT_URL, URL_.c_str());
                curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
                curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

                curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
                curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 3L);
                CURLcode res = curl_easy_perform(curl);
                if (res == CURLE_OK)
                {
                    try
                    {
                        json j = json::parse(readBuffer);

                        if (j.contains(name_of_curr) && j[name_of_curr].contains(currency))
                        {
                            auto price = j[name_of_curr][currency];

                            if (price.is_number())
                            {
                                btc_price.store(price.get<double>());

                                if (btc_price_dequeue.size() >= 70)
                                {
                                    btc_price_dequeue.pop_front();
                                }
                                btc_price_dequeue.push_back((size_t)btc_price);

                                new_data_available.store(true);
                                cv.notify_all();
                            }
                        }
                    }
                    catch (...)
                    {
                        cerr << "diffrent error" << endl;
                    }
                }

                curl_easy_cleanup(curl);
            }
            for (int i = 0; i < 100; i++) // 100 × 100ms = 10s
            {
                if (stop_thread.load())
                    break;
                this_thread::sleep_for(chrono::milliseconds(100));
            }
        }
        cout << "done finishing..." << endl;
    }
};
#endif