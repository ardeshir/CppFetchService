#include <iostream>  
#include <pqxx/pqxx>  
#include <curl/curl.h>  
#include <chrono>  
#include <thread>  
#include <ctime>  
  
size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* s) {  
    size_t newLength = size * nmemb;  
    size_t oldLength = s->size();  
    try {  
        s->resize(oldLength + newLength);  
    } catch (std::bad_alloc &e) {  
        return 0;  
    }  
    std::copy((char*)contents, (char*)contents + newLength, s->begin() + oldLength);  
    return size * nmemb;  
}  
  
int main() {  
    const std::string connectionStr = "dbname=searchdb user=admin password=admin hostaddr=127.0.0.1 port=5432";  
    pqxx::connection conn(connectionStr);  
    CURL* curl = curl_easy_init();  
  
    if (!curl) {  
        std::cerr << "Failed to initialize CURL" << std::endl;  
        return 1;  
    }  
  
    while (true) {  
        pqxx::work txn(conn);  
        pqxx::result r = txn.exec("SELECT id, url FROM urls WHERE last_checked IS NULL OR last_checked < NOW() - INTERVAL '24 hours'");  
  
        if (r.empty()) {  
            std::cout << "No URLs to fetch, sleeping for 1 minute..." << std::endl;  
            txn.commit();  
            std::this_thread::sleep_for(std::chrono::minutes(1));  
            continue;  
        }  
  
        for (const auto& row : r) {  
            int url_id = row[0].as<int>();  
            std::string url = row[1].as<std::string>();  
            std::string response_string;  
  
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());  
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);  
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);  
  
            CURLcode res = curl_easy_perform(curl);  
            if (res != CURLE_OK) {  
                std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;  
                continue;  
            }  
  
            try {  
                pqxx::work insert_txn(conn);  
                insert_txn.exec_params(  
                    "INSERT INTO content (url_id, content) VALUES ($1, $2)",  
                    url_id, response_string  
                );  
                insert_txn.exec_params(  
                    "UPDATE urls SET last_checked = NOW() WHERE id = $1",  
                    url_id  
                );  
                insert_txn.commit();  
            } catch (const std::exception &e) {  
                std::cerr << "Database error: " << e.what() << std::endl;  
            }  
        }  
        txn.commit();  
    }  
  
    curl_easy_cleanup(curl);  
    return 0;  
}  