#include <iostream>
#include <string>
#include <curl/curl.h>
#include <json/json.h> 

class GPTService {
public:
    GPTService(const std::string& apiKey) : apiKey(apiKey) {
        std::cout<<"GPTService constructor"<<std::endl;
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
        jsonData["model"] = "text-davinci-003";  // You can use other models like "gpt-3.5-turbo" or "gpt-4"
        jsonData["prompt"] = prompt;
        jsonData["max_tokens"] = 100;  // Adjust as needed

        Json::StreamWriterBuilder writer;
        std::string jsonPayload = Json::writeString(writer, jsonData);

        // Set up the URL and headers for the HTTP POST request
        std::cout<<"Set up the URL and headers for the HTTP POST request"<<std::endl;
        const std::string url = "https://api.openai.com/v1/completions";
        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, ("Authorization: Bearer " + apiKey).c_str());
        headers = curl_slist_append(headers, "Content-Type: application/json");

        // Set the CURL options
        std::cout<<"Set the CURL options"<<std::endl;
        curl_easy_setopt(curlHandle, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curlHandle, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curlHandle, CURLOPT_POSTFIELDS, jsonPayload.c_str());

        // Set up the response handling
        std::cout<<"Set up the response handling"<<std::endl;
        std::string response;
        curl_easy_setopt(curlHandle, CURLOPT_WRITEFUNCTION, writeCallback);
        curl_easy_setopt(curlHandle, CURLOPT_WRITEDATA, &response);

        // Perform the request
        std::cout<<"Perform the request"<<std::endl;
        CURLcode res = curl_easy_perform(curlHandle);

        // Clean up headers
        std::cout<<"Clean up headers"<<std::endl;
        curl_slist_free_all(headers);

        // Check for errors
        if (res != CURLE_OK) {
            std::cerr << "CURL Error: " << curl_easy_strerror(res) << std::endl;
            return "";
        }

        // Parse the response (using JSON)
        std::cout<<"Parse the response (using JSON)"<<std::endl;
        Json::CharReaderBuilder reader;
        Json::Value jsonResponse;
        std::string errs;

        std::istringstream stream(response);
        if (!Json::parseFromStream(reader, stream, &jsonResponse, &errs)) {
            std::cerr << "JSON parsing error: " << errs << std::endl;
            return "";
        }

        // Extract the text from the response
        std::cout<<"Extract the text from the response"<<std::endl;
        if (jsonResponse.isMember("choices") && jsonResponse["choices"].isArray()) {
            const Json::Value& choices = jsonResponse["choices"];
            if (choices.size() > 0 && choices[0].isMember("text")) {
                std::cout<<"return choices[0][\"text\"].asString()"<<std::endl;
                return choices[0]["text"].asString();
            }
        }

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


