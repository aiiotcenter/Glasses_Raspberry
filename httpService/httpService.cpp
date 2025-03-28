#include <curl/curl.h>
#include <opencv2/opencv.hpp>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <thread>

class HttpService
{
public:
    HttpService()
    {
        std::cout << "constructing http service";
        // loadEnvFile(".env");
        // const char *url = std::getenv("SERVER_URL");
        // if (!url)
        // {
        //     std::cerr << "SERVER_URL not set in .env file!\n";
        //     exit(1);
        // }
        serverUrl = "http://livebees.aiiot.center/detect_objects";
        curl_global_init(CURL_GLOBAL_ALL);
    }

    ~HttpService()
    {
        curl_global_cleanup();
    }

    std::string sendFrame(const cv::Mat &frame)
    {
        std::cout << "entered send frame\n";

        std::string tempFilename = "temp_frame.jpg";
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
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);

        std::string response;
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,
                         [](void *contents, size_t size, size_t nmemb, void *userp) -> size_t
                         {
                             ((std::string *)userp)->append((char *)contents, size * nmemb);
                             return size * nmemb;
                         });
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

        std::cout << "📤 Performing HTTP request...\n";

        CURLcode res = curl_easy_perform(curl);

        std::cout << "res is " << res << "\n";

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

private:
    std::string serverUrl;

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