
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
using namespace std;
using json = nlohmann::json;
class Api_connection_base
{
public:
    virtual void Get_values(string URL_, string name_of_curr, string currency) = 0;
    virtual ~Api_connection_base() {}
};
class Graph_maker
{
public:
    Graph_maker(size_t Bitcoin_prize, deque<size_t> Stocks, size_t maksprice)
    {
        size_t minPrice = Stocks[0];
        size_t maxPrice = Stocks[0];
        const int HEIGHT = 20;

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
    double btc_price = 0.0;
    double prev_btc_price = 0.0;
    deque<size_t> btc_price_dequeue;
    atomic<bool> stop_thread{false};
    thread background_thread;

public:
    string URL_of_API;
    string currname;
    string currency;
    Api_connection(string URL_of_API, string currname, string currency) : URL_of_API(URL_of_API), currname(currname), currency(currency) {} // add the URL to the API you need after that name that your currency has and currency i only use US dollars so it's for future development

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
        background_thread = thread(&Api_connection::Get_values, this, URL_of_API, currname, currency);
        background_thread.detach();

        this_thread::sleep_for(chrono::seconds(10));
        while (true)
        {

            {
                if (kbhit())
                {
                    char ch = getch();
                    if (ch == 'Q' || ch == 'q')
                    {
                        cout << "Closing section..." << endl;
                        break;
                    }
                }
                // Get_values(URL_of_API, currname, currency);
                lock_guard<mutex> lock(mtx);
                if (btc_price != prev_btc_price)
                {

                    cout << "\033[2J\033[1;1H";
                    cout << "price of " << currname << ": " << btc_price << " USD" << endl;
                    size_t maks_price = max(curr, btc_price);
                    Graph_maker(btc_price, btc_price_dequeue, maks_price);
                    prev_btc_price = btc_price;
                    temp_price_to_retrive = prev_btc_price;
                    cout << "To stop the program press Q" << endl;
                }
            }
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
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
    void Get_values(string URL_, string name_of_curr, string currency) // Used in Api connection
    {

        while (!stop_thread)
        {

            CURL *curl = curl_easy_init();
            if (curl)
            {
                // cout << "Fetching from URL: " << URL_ << endl;

                string readBuffer;
                curl_easy_setopt(curl, CURLOPT_URL, URL_.c_str());
                curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
                curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

                CURLcode res = curl_easy_perform(curl);
                if (res == CURLE_OK)
                {
                    int latest = 0;
                    try
                    {
                        json j = json::parse(readBuffer);
                        // cout << "API Response: " << j.dump(2) << endl;

                        if (j.contains(name_of_curr) && j[name_of_curr].contains(currency))
                        {
                            auto price = j[name_of_curr][currency];

                            if (price.is_number())
                            {
                                lock_guard<mutex> lock(mtx);
                                btc_price = price.get<double>();

                                if (btc_price_dequeue.size() > 30)
                                {
                                    btc_price_dequeue.pop_front();
                                }
                                btc_price_dequeue.push_back((size_t)btc_price);
                            }
                            else
                            {
                                cerr << "Invalid data received for " << name_of_curr << " in currency " << currency << endl;
                            }
                        }
                        else
                        {
                            cerr << "The expected data for " << name_of_curr << " in currency " << currency << " was not found." << endl;
                        }
                    }
                    catch (std::exception &e)
                    {
                        cerr << "Standard exception: " << e.what() << endl;
                    }
                    catch (...)
                    {
                        cerr << "other error uknown" << endl;
                    }
                }
                else
                {
                    cerr << "error CURL: " << curl_easy_strerror(res) << endl;
                }

                curl_easy_cleanup(curl);
            }

            std::this_thread::sleep_for(std::chrono::seconds(15));
        }
    }
};

#endif