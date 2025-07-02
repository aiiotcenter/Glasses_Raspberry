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
        serverUrl = "http://192.168.254.245:5000/process_frame";
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
        std::string responseFile = std::string(cwd) + "/response.txt";

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

        // Open file for response
        FILE *fp = fopen(responseFile.c_str(), "wb");
        if (!fp)
        {
            std::cerr << "❌ Failed to create response file\n";
            return "";
        }

        CURL *curl = curl_easy_init();
        if (!curl)
        {
            std::cerr << "❌ curl_easy_init failed.\n";
            fclose(fp);
            return "";
        }

        std::cout << "✅ curl initialized\n";

        curl_mime *form = curl_mime_init(curl);
        if (!form)
        {
            std::cerr << "❌ curl_mime_init failed.\n";
            fclose(fp);
            curl_easy_cleanup(curl);
            return "";
        }

        std::cout << "✅ mime form created\n";

        curl_mimepart *field = curl_mime_addpart(form);
        if (!field)
        {
            std::cerr << "❌ Failed to add mime part.\n";
            fclose(fp);
            curl_mime_free(form);
            curl_easy_cleanup(curl);
            return "";
        }

        curl_mime_name(field, "image");
        curl_mime_filedata(field, tempFilename.c_str()); // Send file
        curl_mime_filename(field, "frame.jpg");

        std::cout << "✅ File attached to form\n";

        // Error buffer for detailed error messages
        char errorBuffer[CURL_ERROR_SIZE];
        errorBuffer[0] = 0; // Empty string

        curl_easy_setopt(curl, CURLOPT_URL, serverUrl.c_str());
        curl_easy_setopt(curl, CURLOPT_MIMEPOST, form);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);             // Increased timeout
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L);       // Connection timeout
        curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);             // Prevent SIGPIPE
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);            // Write to file
        curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errorBuffer); // Store error details
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);              // Enable verbose logging

        std::cout << "📤 Performing HTTP request...\n";

        CURLcode res = curl_easy_perform(curl);

        // Close the file before further processing
        fclose(fp);

        // Get HTTP response code
        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        std::cout << "HTTP response code: " << http_code << std::endl;

        // Clean up curl resources
        if (form)
            curl_mime_free(form);
        if (curl)
            curl_easy_cleanup(curl);

        std::cout << "✅ After cleanup\n";

        // Clean up the temporary image file
        std::remove(tempFilename.c_str());

        if (res != CURLE_OK)
        {
            std::cerr << "HTTP Error: " << curl_easy_strerror(res) << "\n";
            std::cerr << "Error details: " << errorBuffer << "\n";
            return "";
        }

        // Read response from file
        std::ifstream responseStream(responseFile);
        std::string response;
        if (responseStream)
        {
            response = std::string((std::istreambuf_iterator<char>(responseStream)),
                                   std::istreambuf_iterator<char>());
            responseStream.close();
        }
        else
        {
            std::cerr << "❌ Failed to read response file\n";
        }

        // Clean up response file
        std::remove(responseFile.c_str());

        std::cout << "✅ Server response: " << response << "\n";

        return response;
    }

    // Fallback method using system command if the main method fails
    std::string sendFrameWithSystemCommand(const cv::Mat &frame)
    {
        char cwd[1024];
        getcwd(cwd, sizeof(cwd));
        std::string tempFilename = std::string(cwd) + "/temp_frame.jpg";
        std::string responseFile = std::string(cwd) + "/response.txt";

        if (!cv::imwrite(tempFilename, frame))
        {
            std::cerr << "Failed to save temp image.\n";
            return "";
        }

        // Use curl command line instead of libcurl
        std::string cmd = "curl -s -X POST -F \"image=@" + tempFilename + "\" " + serverUrl + " > " + responseFile;

        int result = system(cmd.c_str());
        if (result != 0)
        {
            std::cerr << "Failed to execute curl command (exit code: " << result << ")\n";
            std::remove(tempFilename.c_str());
            return "";
        }

        // Read response from file
        std::ifstream responseStream(responseFile);
        std::string response;
        if (responseStream)
        {
            response = std::string((std::istreambuf_iterator<char>(responseStream)),
                                   std::istreambuf_iterator<char>());
            responseStream.close();
        }
        else
        {
            std::cerr << "Failed to read response file\n";
        }

        // Clean up
        std::remove(tempFilename.c_str());
        std::remove(responseFile.c_str());

        return response;
    }

    // Method to test server connection
    bool testServerConnection()
    {
        // Try a simple HEAD request using system command
        std::string cmd = "curl -s -I " + serverUrl + " > /dev/null";
        int result = system(cmd.c_str());

        if (result == 0)
        {
            std::cout << "Server is reachable via system command\n";
            return true;
        }

        std::cout << "Warning: Server connection test failed\n";
        return false;
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