#include <iostream>
#include <fstream>
#include <string>
#include "GptService/GptService.cpp"
#include <cstdlib> // For system()
#include <curl/curl.h>
#include "CameraService/CameraService.cpp"
#include "httpService/httpService.cpp"
#include <thread>
#include <chrono>
#include <sstream>

// Function to send GPT response to Flask TTS API
void sendToTTS(const std::string &text)
{
    std::ostringstream cmd;
    cmd << "curl -X POST -F \"text=" << text << "\" http://127.0.0.1:5000/tts | mpg123 -";
    std::cout << "TTS Command: " << cmd.str() << std::endl;

    int result = system(cmd.str().c_str());
    if (result != 0)
        std::cerr << "❌ system() command failed with code " << result << "\n";
}

// Function to read the API key from a file
std::string readApiKey(const std::string &filename)
{
    std::ifstream file(filename);
    if (!file)
    {
        std::cerr << "Failed to open " << filename << std::endl;
        exit(1);
    }
    std::string apiKey;
    std::getline(file, apiKey);
    file.close();
    return apiKey;
}

// Function to format AI model data for the prompt
std::string generatePrompt(const std::string &detectedObjects)
{
    return "You are an AI assistant helping a blind person navigate their environment. "
           "Based on AI vision analysis, the following objects are detected in front of the user:\n" +
           detectedObjects +
           "\nParaphrase this data into a natural spoken description that helps the user understand their surroundings.";
}

int main()
{
    CameraService cameraService;
    HttpService httpService;

    // Read the API key from "apikey.txt"
    std::string apiKey = readApiKey("apikey.txt");
    GPTService gptService(apiKey);

    // while(true)
    //{

    // step 1 : take frame

    // step 2 : use http service and send to server await the response

    // step 3 : parse the response get the message and send to promotion builder

    // step 4 : send the promotion to gpt and get the response

    // step 5 : send the response to gtts python server and play the audio

    // step 6 : repaet

    //}

    while (true)
    {
        std::cout << "\nLoop started\n";

        // Step 1: Capture frame
        std::cout << "Step 1: Capturing frame...\n";
        cv::Mat frame = cameraService.captureFrame();
        if (frame.empty())
        {
            std::cerr << "Skipping: empty frame.\n";
            std::this_thread::sleep_for(std::chrono::seconds(2));
            continue;
        }

        // Step 2: Send to HTTP server and get detection
        std::cout << "Step 2: Sending frame to server...\n";
        std::string detectedObjects = httpService.sendFrame(frame);
        if (detectedObjects.empty())
        {
            std::cerr << "Skipping: no objects detected.\n";
            std::this_thread::sleep_for(std::chrono::seconds(2));
            continue;
        }

        std::cout << "Detected: " << detectedObjects << "\n";

        // Step 3: Generate prompt
        std::cout << "Step 3: Building prompt...\n";
        std::string prompt = generatePrompt(detectedObjects);

        // Step 4: Ask GPT
        std::cout << "Step 4: Getting GPT response...\n";
        std::string response = gptService.getGPTResponse(prompt);
        if (response.empty())
        {
            std::cerr << "Skipping: no GPT response.\n";
            std::this_thread::sleep_for(std::chrono::seconds(2));
            continue;
        }

        std::cout << "GPT Response: " << response << "\n";

        // Step 5: Speak via TTS
        std::cout << "Step 5: Speak via TTS...\n";
        sendToTTS(response);

        std::cout << "\nLoop ended\n";

        // Step 6: Wait before next round (adjust as needed)
        std::this_thread::sleep_for(std::chrono::seconds(3));
    }

    return 0;
}
