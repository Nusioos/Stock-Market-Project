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
    deque<size_t> btc_price_dequeue;

public:
    Api_connection(string URL_of_API,string currname , string currency)
    {
        curl_global_init(CURL_GLOBAL_DEFAULT);
        thread background(&Api_connection::Get_values, this, URL_of_API, currname, currency);
        background.detach();
        std::this_thread::sleep_for(std::chrono::seconds(10));
        while (true)
        {
            {
                lock_guard<mutex> lock(mtx);
                std::cout << "Aktualna cena BTC: " << btc_price << " USD" << std::endl;
                size_t maks_price = max(curr, btc_price);
                Graph_maker(btc_price, btc_price_dequeue, maks_price);
            }
            std::this_thread::sleep_for(std::chrono::seconds(20));
            std::cout << "sperma" << std::endl;
        }
    }

    static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
    {
        ((std::string *)userp)->append((char *)contents, size * nmemb);
        return size * nmemb;
    }
    void Get_values(string URL_, string name_of_curr, string currency)
    {

        while (true)
        {

            CURL *curl = curl_easy_init();
            if (curl)
            {
                std::string readBuffer;
                curl_easy_setopt(curl, CURLOPT_URL, URL_);
                curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
                curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

                CURLcode res = curl_easy_perform(curl);
                if (res == CURLE_OK)
                {
                    int latest = 0;
                    try
                    {
                        json j = json::parse(readBuffer);
                        lock_guard<mutex> lock(mtx);
                        btc_price = j[name_of_curr][currency];
                        if (btc_price_dequeue.size() > 200)
                        {
                            btc_price_dequeue.pop_front();
                        }
                        btc_price_dequeue.push_back((size_t)btc_price);
                    }
                    catch (exception &e)
                    {
                        cout << "blad" << endl;
                    }
                }
                else
                {
                    cerr << "Błąd CURL: " << curl_easy_strerror(res) << endl;
                }

                curl_easy_cleanup(curl);
            }

            std::this_thread::sleep_for(std::chrono::seconds(15));
        }
    }
};

#endif