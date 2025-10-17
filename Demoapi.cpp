#include <iostream>
#include <string>
#include <curl/curl.h>
#include "nlohmann/json.hpp" 
#include <chrono>
#include <thread>
#include<mutex>
using namespace std;
using json = nlohmann::json;
std::mutex mtx;  // mutex do synchronizacji
double btc_price = 0.0;
// Funkcja callback do zapisywania danych pobranych przez CURL
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}
void Get_values()
{
    CURL* curl;
    CURLcode res;
    curl = curl_easy_init();
    if(!curl) return;

    while(true) {
        std::string readBuffer;
        curl_easy_setopt(curl, CURLOPT_URL, "https://api.coingecko.com/api/v3/simple/price?ids=bitcoin&vs_currencies=usd");
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
    //    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);

        res = curl_easy_perform(curl);
        if(res == CURLE_OK) {
            json j = json::parse(readBuffer);
            std::lock_guard<std::mutex> lock(mtx);
            btc_price = j["bitcoin"]["usd"];
        }

        curl_easy_cleanup(curl);
        std::this_thread::sleep_for(std::chrono::seconds(1)); // pobieranie co 1 sekundę
    }
}

int main() {
    std::thread background(Get_values); 
    background.detach();

    while(true) {
        {
            std::lock_guard<std::mutex> lock(mtx);
            std::cout << "Aktualna cena BTC: " << btc_price << " USD" << std::endl;
        }
        std::this_thread::sleep_for(std::chrono::seconds(2));
        std::cout << "sperma" << std::endl;
    }
}

