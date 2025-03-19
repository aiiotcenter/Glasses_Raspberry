#include <iostream>
#include <string>
#include <curl/curl.h>
#include <json/json.h>
#include <sstream>

class GPTService {
public:
    GPTService(const std::string& apiKey) : apiKey(apiKey) {
        std::cout << "GPTService constructor" << std::endl;
        curl_global_init(CURL_GLOBAL_DEFAULT);
        curlHandle = curl_easy_init();
    }

    ~GPTService() {
        if (curlHandle) {
            curl_easy_cleanup(curlHandle);
        }
        curl_global_cleanup();
    }

    std::string getGPTResponse(const std::string& prompt) {
        if (!curlHandle) {
            std::cerr << "Curl initialization failed!" << std::endl;
            return "";
        }

        // Create the JSON payload
        Json::Value jsonData;
        jsonData["model"] = "gpt-3.5-turbo";  // Updated to latest available model
        jsonData["messages"][0]["role"] = "user"; // OpenAI expects "messages" format
        jsonData["messages"][0]["content"] = prompt;
        jsonData["max_tokens"] = 300;  // Increased to get a more meaningful response

        Json::StreamWriterBuilder writer;
        std::string jsonPayload = Json::writeString(writer, jsonData);

        // Set up the URL and headers for the HTTP POST request
        std::cout << "Setting up the URL and headers for the HTTP POST request" << std::endl;
        const std::string url = "https://api.openai.com/v1/chat/completions";
        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, ("Authorization: Bearer " + apiKey).c_str());
        headers = curl_slist_append(headers, "Content-Type: application/json");

        // Set the CURL options
        std::cout << "Setting CURL options" << std::endl;
        curl_easy_setopt(curlHandle, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curlHandle, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curlHandle, CURLOPT_POSTFIELDS, jsonPayload.c_str());

        // Set up the response handling
        std::cout << "Setting up response handling" << std::endl;
        std::string response;
        curl_easy_setopt(curlHandle, CURLOPT_WRITEFUNCTION, writeCallback);
        curl_easy_setopt(curlHandle, CURLOPT_WRITEDATA, &response);

        // Perform the request
        std::cout << "Performing the request" << std::endl;
        CURLcode res = curl_easy_perform(curlHandle);

        // Clean up headers
        curl_slist_free_all(headers);

        // Check for errors
        if (res != CURLE_OK) {
            std::cerr << "CURL Error: " << curl_easy_strerror(res) << std::endl;
            return "";
        }

        // Debugging: Print the raw JSON response
        std::cout << "Raw JSON response: " << response << std::endl;

        // Parse the response (using JSON)
        Json::CharReaderBuilder reader;
        Json::Value jsonResponse;
        std::string errs;
        std::istringstream stream(response);
        if (!Json::parseFromStream(reader, stream, &jsonResponse, &errs)) {
            std::cerr << "JSON parsing error: " << errs << std::endl;
            return "";
        }

        // Check for API error response
        if (jsonResponse.isMember("error")) {
            std::cerr << "OpenAI API Error: " << jsonResponse["error"]["message"].asString() << std::endl;
            return "Error from GPT: " + jsonResponse["error"]["message"].asString();
        }

        // Extract the text from the response
        if (jsonResponse.isMember("choices") && jsonResponse["choices"].isArray() && !jsonResponse["choices"].empty()) {
            const Json::Value& choices = jsonResponse["choices"];
            if (choices[0].isMember("message") && choices[0]["message"].isMember("content")) {
                std::cout << "Returning GPT response text" << std::endl;
                return choices[0]["message"]["content"].asString();
            }
        }

        std::cerr << "No valid choices in response." << std::endl;
        return "No response from GPT";
    }

private:
    std::string apiKey;
    CURL* curlHandle;

    // Callback function to handle the response data
    static size_t writeCallback(void* contents, size_t size, size_t nmemb, void* userp) {
        size_t totalSize = size * nmemb;
        ((std::string*)userp)->append((char*)contents, totalSize);
        return totalSize;
    }
};