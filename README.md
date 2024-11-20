# C++ client application fetch.cpp that interacts with a PostgreSQL database.

Below is a detailed guide, including database setup, C++ code, and build configuration with CMake.

##### Step 1: Install PostgreSQL and Development Libraries on Ubuntu
 

Install PostgreSQL:
```bash
sudo apt update  
sudo apt install postgresql postgresql-contrib  
``` 
2. Install PostgreSQL development libraries:
```bash
sudo apt install libpqxx-dev 
```
3. Install other necessary tools:
```bash
sudo apt install cmake g++ libcurl4-openssl-dev
```
#### Step 2: Setup PostgreSQL Database and Tables
 

- Switch to the PostgreSQL user:
```bash
sudo -i -u postgres  
``` 
2. Create a new database and user:
```bash
sh createdb mydatabase psql -c "CREATE USER myuser WITH ENCRYPTED PASSWORD 'mypassword';" psql -c "GRANT ALL PRIVILEGES ON DATABASE mydatabase TO myuser;"
```
3. Create the urls and content tables:

```bash
psql -d mydatabase


CREATE TABLE urls (  
    id SERIAL PRIMARY KEY,  
    url TEXT NOT NULL,  
    last_checked TIMESTAMP  
);  

CREATE TABLE content (  
    id SERIAL PRIMARY KEY,  
    url_id INTEGER REFERENCES urls(id),  
    content TEXT NOT NULL,  
    fetched_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP  
);  
\q  
```  
### create correct permissions for the tables

```sql
searchdb=#  GRANT ALL PRIVILEGES ON TABLE urls TO myuser;

searchdb=#  GRANT ALL PRIVILEGES ON TABLE content TO myuser;
``` 

#### Step 3: Write the C++ Code (fetch.cpp) - Continued
 
Here's the continuation and completion of the fetch.cpp program:

```cpp
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
    const std::string connectionStr = "dbname=mydatabase user=myuser password=mypassword hostaddr=127.0.0.1 port=5432";  
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
``` 

##### Step 4: Create the CMakeLists.txt File 

Create a file named CMakeLists.txt in the same directory as your fetch.cpp file with the following content:

```text
cmake_minimum_required(VERSION 3.10)  
project(fetch_service)  
  
set(CMAKE_CXX_STANDARD 17)  
  
# Find PostgreSQL and libpqxx packages  
find_package(PostgreSQL REQUIRED)  
find_package(PkgConfig REQUIRED)  
pkg_check_modules(PQXX REQUIRED libpqxx)  
  
# Include directories for PostgreSQL and libpqxx  
include_directories(${PostgreSQL_INCLUDE_DIRS})  
include_directories(${PQXX_INCLUDE_DIRS})  
  
# Add the executable  
add_executable(fetch fetch.cpp)  
  
# Link libraries for PostgreSQL, libpqxx, and CURL  
target_link_libraries(fetch ${PostgreSQL_LIBRARIES} ${PQXX_LIBRARIES} curl)   
``` 

#### Step 5: Build the Project

Create a build directory:
```bash
mkdir build  
cd build  
```
 
2. Run CMake to configure the project:

```bash 
sh cmake ..
```

This command will generate the necessary build files using the `CMakeLists.txt` configuration.  
 
3. Compile the project:
```bash 
sh make
```

This will compile the `fetch.cpp` file and link it against the PostgreSQL and CURL libraries, producing an executable named `fetch`.  
 

#### Step 6: Run the Application
 
- Run the executable:
```bash 

./fetch
```  

This will start the service which will continuously check the urls table for URLs that need to be fetched and processed. If none are found, it will sleep for 1 minute before checking again.

#### Summary
 
You have now created a complete C++ application that interacts with a PostgreSQL database to fetch and store URL content. Here is a summary of the steps we covered:

1. Installed PostgreSQL and development libraries on Ubuntu.
2. Set up the PostgreSQL database and tables.
3. Wrote the C++ code (fetch.cpp) to fetch URL content and update the database.
4. Created a CMake configuration file (CMakeLists.txt).
5. Built the project using CMake and Make.
6. Ran the application.

#### Additional Notes 

- Error Handling: Ensure that your application has adequate error handling for production use, especially around database operations and network requests.
- Logging: Integrate a logging library for better traceability and debugging.
- Configuration Management: Consider externalizing database and network configurations to a configuration file or environment variables for easier management.
- Security: Ensure that passwords and sensitive data are not hard-coded and are stored securely.

#### Useful Links
 

- [PostgreSQL Documentation](https://www.postgresql.org/docs/) 
- [PostgreSQL: Insert](https://neon.tech/postgresql/postgresql-tutorial/postgresql-insert)
- [libpqxx Documentation](https://libpqxx.readthedocs.io/stable/)
- [libcurl Documentation](https://curl.se/libcurl/)
- [CMake Documentation](https://cmake.org/documentation/)

By following these steps, you can create a robust and maintainable C++ service for interacting with a PostgreSQL database and fetching URL content. If you have any specific questions or need further assistance, 
