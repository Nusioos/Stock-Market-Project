#include <iostream>
#include <string>
#include <curl/curl.h>
#include "nlohmann/json.hpp" // <- Zwróć uwagę na <>, nie ""

using json = nlohmann::json;

// Funkcja callback do zapisywania danych pobranych przez CURL
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

int main() {
    CURL* curl;
    CURLcode res;
    std::string readBuffer;

    curl = curl_easy_init();
    if(curl) {
        curl_easy_setopt(curl, CURLOPT_URL, "https://api.coingecko.com/api/v3/simple/price?ids=bitcoin&vs_currencies=usd");
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);

        if(res == CURLE_OK) {
            json j = json::parse(readBuffer);
            double btc_price = j["bitcoin"]["usd"];
            std::cout << "Cena BTC: " << btc_price << " USD" << std::endl;
        } else {
            std::cerr << "Błąd pobierania danych: " << curl_easy_strerror(res) << std::endl;
        }
    }
    return 0;
}
