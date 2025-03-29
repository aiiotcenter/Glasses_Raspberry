#include <curl/curl.h>
#include <opencv2/opencv.hpp>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <thread>
#include <unistd.h>

class HttpService
{
public:
    HttpService()
    {
        std::cout << "Constructing HTTP service...\n";
        serverUrl = "http://livebees.aiiot.center/detect_objects";
        curl_global_init(CURL_GLOBAL_ALL);
    }

    ~HttpService()
    {
        curl_global_cleanup();
    }

    std::string sendFrame(const cv::Mat &frame)
    {
        std::cout << "Entered send frame\n";

        char cwd[1024];
        getcwd(cwd, sizeof(cwd));
        std::string tempFilename = std::string(cwd) + "/temp_frame.jpg";

        if (!cv::imwrite(tempFilename, frame))
        {
            std::cerr << "❌ Failed to save temp image.\n";
            return "";
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        if (FILE *testFile = fopen(tempFilename.c_str(), "rb"))
        {
            fclose(testFile);
            std::cout << "✅ File is readable before upload\n";
        }
        else
        {
            std::cerr << "❌ File is NOT readable before upload\n";
            return "";
        }

        CURL *curl = curl_easy_init();
        if (!curl)
        {
            std::cerr << "❌ curl_easy_init failed.\n";
            return "";
        }

        std::cout << "✅ curl initialized\n";

        curl_mime *form = curl_mime_init(curl);
        if (!form)
        {
            std::cerr << "❌ curl_mime_init failed.\n";
            curl_easy_cleanup(curl);
            return "";
        }

        std::cout << "✅ mime form created\n";

        curl_mimepart *field = curl_mime_addpart(form);
        if (!field)
        {
            std::cerr << "❌ Failed to add mime part.\n";
            curl_mime_free(form);
            curl_easy_cleanup(curl);
            return "";
        }

        curl_mime_name(field, "image");
        curl_mime_filedata(field, tempFilename.c_str()); // Send file
        curl_mime_filename(field, "frame.jpg");

        std::cout << "✅ File attached to form\n";

        curl_easy_setopt(curl, CURLOPT_URL, serverUrl.c_str());
        curl_easy_setopt(curl, CURLOPT_MIMEPOST, form);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);       // Increased timeout to 10 seconds
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L); // 5 second connection timeout
        curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);       // Prevent SIGPIPE from crashing the application

        std::string response;
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

        // Enable verbose logging for debugging
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

        std::cout << "📤 Performing HTTP request...\n";

        CURLcode res = curl_easy_perform(curl);

        // Get HTTP response code
        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        std::cout << "HTTP response code: " << http_code << std::endl;

        // Do NOT call curl_mime_free if curl is already cleaned up
        if (form)
            curl_mime_free(form);
        if (curl)
            curl_easy_cleanup(curl);

        std::cout << "✅ After cleanup\n";

        std::remove(tempFilename.c_str());

        if (res != CURLE_OK)
        {
            std::cerr << "HTTP Error: " << curl_easy_strerror(res) << "\n";
            return "";
        }

        std::cout << "✅ Server response: " << response << "\n";

        return response;
    }

    // Method to test server connection
    bool testServerConnection()
    {
        CURL *curl = curl_easy_init();
        if (!curl)
        {
            return false;
        }

        curl_easy_setopt(curl, CURLOPT_URL, serverUrl.c_str());
        curl_easy_setopt(curl, CURLOPT_NOBODY, 1L); // HEAD request
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 3L);

        CURLcode res = curl_easy_perform(curl);
        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

        curl_easy_cleanup(curl);

        std::cout << "Server test - HTTP code: " << http_code << ", Result: " << (res == CURLE_OK ? "OK" : "Failed") << std::endl;

        return (res == CURLE_OK && http_code > 0 && http_code < 500);
    }

private:
    std::string serverUrl;

    // Static callback function for curl
    static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
    {
        ((std::string *)userp)->append((char *)contents, size * nmemb);
        return size * nmemb;
    }

    void loadEnvFile(const std::string &filepath)
    {
        std::ifstream file(filepath);
        std::string line;
        while (std::getline(file, line))
        {
            size_t eqPos = line.find('=');
            if (eqPos != std::string::npos)
            {
                std::string key = line.substr(0, eqPos);
                std::string value = line.substr(eqPos + 1);
                setenv(key.c_str(), value.c_str(), 1);
            }
        }
    }
};