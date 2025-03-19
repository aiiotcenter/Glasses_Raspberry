#include <iostream>
#include <fstream>
#include <string>
#include "GptService/GptService.cpp"
#include <cstdlib> // For system()
#include <curl/curl.h>

// Function to send GPT response to Flask TTS API
void sendToTTS(const std::string &text)
{
    std::string command = "curl -X POST -F \"text=" + text + "\" http://127.0.0.1:5000/tts --output output.mp3";
    system(command.c_str()); // Execute the curl command
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
    // Read the API key from "apikey.txt"
    std::string apiKey = readApiKey("apikey.txt");
    GPTService gptService(apiKey);

    // Simulated AI model output (replace this with real-time data)
    std::string detectedObjects =
        "- chair/meters/ahead.\n"
        "- table/3 meters/left.\n"
        "- door/5 meters/front.\n";

    // Generate the GPT prompt
    std::string prompt = generatePrompt(detectedObjects);

    // Get GPT response
    std::string response = gptService.getGPTResponse(prompt);

    // Output the paraphrased description
    std::cout << "AI Assistant: " << response << std::endl;

    // Send GPT response to Flask TTS server
    sendToTTS(response);

    // Play the generated speech file
    system("mpg123 output.mp3");

    return 0;
}

// while(true)
//{

// step 1 : send request to server and ask for objects

// step 2 : get response json parse it

// step 3 : send gpt request to shape paragraph

// step 4 : send gpt response to piper

// step 5 : play piper video in speakers

// step 6 : repaet

//}
