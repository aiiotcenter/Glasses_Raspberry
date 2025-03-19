
#include <iostream>
#include <fstream>
#include <string>
#include "GptService/GptService.cpp"

int main() {
    // Read the API key from "apikey.txt"
    std::ifstream file("apikey.txt");
    if (!file) {
        std::cerr << "Failed to open apikey.txt" << std::endl;
        return 1;
    }
    std::string apiKey;
    std::getline(file, apiKey);
    file.close();

    // Initialize the GPTService with the API key read from the file
    GPTService gptService(apiKey);

    std::string prompt = "Tell me a joke!";
    std::string response = gptService.getGPTResponse(prompt);
    std::cout << "GPT Response: " << response << std::endl;

    //while(true)
    //{

        //step 1 : send request to server and ask for objects

        //step 2 : get response json parse it 

        //step 3 : send gpt request to shape paragraph 

        //step 4 : send gpt response to piper 

        //step 5 : play piper video in speakers 

        //step 6 : repaet 

    //}
    
    return 0;
}

