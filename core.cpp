#include <iostream>
#include "GptService/GptService.cpp"



int main() {

    GPTService gptService("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");
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
