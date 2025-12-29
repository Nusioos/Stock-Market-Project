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
using namespace std;

using json = nlohmann::json;
mutex mtx;
double btc_price = 0.0;
deque<size_t> btc_price_dequeue;
void Generate_graph(size_t Bitcoin_prize, deque<size_t> Stocks, size_t maksprice);
static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((string *)userp)->append((char *)contents, size * nmemb);
    return size * nmemb;
}
void Get_values()
{

    while (true)
    {

        CURL *curl = curl_easy_init();
        if (curl)
        {
            std::string readBuffer;
            curl_easy_setopt(curl, CURLOPT_URL, "https://api.coingecko.com/api/v3/simple/price?ids=bitcoin&vs_currencies=usd");
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
                    btc_price = j["bitcoin"]["usd"];
                    if (btc_price_dequeue.size() > 10)
                    {
                        btc_price_dequeue.pop_front();
                    }
                    btc_price_dequeue.push_back((size_t)btc_price);
                }
                catch (exception &e)
                {
                    cout << "error" << endl;
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
int main()
{
    double curr = 0;
    double prev_btc_price = 0.0;
    curl_global_init(CURL_GLOBAL_DEFAULT);
    thread background(Get_values);
    background.detach();
    std::this_thread::sleep_for(std::chrono::seconds(10));
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
            lock_guard<mutex> lock(mtx);
            if (btc_price != prev_btc_price)
            {
                cout << "price of BTC: " << btc_price << " USD" << endl;
                size_t maks_price = max(curr, btc_price);
                Generate_graph(btc_price, btc_price_dequeue, maks_price);
                prev_btc_price = btc_price;
                cout << "To stop the program press Q" << endl;
            }
        }
        std::this_thread::sleep_for(std::chrono::seconds(20));
    }
}
void Generate_graph(size_t Bitcoin_prize, deque<size_t> Stocks, size_t maksprice)
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
