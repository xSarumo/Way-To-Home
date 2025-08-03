#include <iostream>
#include <cstring>
#include <crt-way.h>
#include <road-bot.h>

bool include_in(char* city){ 
    for (int i = 0; i < Constants::cities.size(); i++){
        if (city == Constants::cities[i]){
            return true;
        }
    }
    return false;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cout << "[-] Error: Missing argument\n";
        return 1;
    } 
    if (strcmp(argv[1], "--bot") == 0){
        Bot();
        return 0;
    } else if (strcmp(argv[1], "--help") == 0) {
        std::cout << "--bot -> starts telegram bot\n" << "If you want use the console version of program, enter two cities from this list:\n" + print_cities();
        return 0;
    } else if (argc == 4) {
        if (include_in(argv[1]) && include_in(argv[2])){
            TArgs* arguments = new TArgs;
            std::vector<std::string> val_list{argv[1], argv[2], argv[3]};
            arguments->from = convertCityToYandexCode(val_list[0]);
            arguments->to = convertCityToYandexCode(val_list[1]);
            arguments->date = val_list[2];
            Request req(*arguments, Constants::API_path);
            auto filteredResponse = req.FilterResJson(req.SendRequestToAPI());
            std::cout << PrintRequests(filteredResponse, *arguments, Constants::limit) << std::endl;
            return 0;
        } else {
            std::cout << "[-] Error: Wrong city input\n";
            return 1;
        }
    } else {
        std::cout << "[-] Error: Wrong input\n";
        return 1;
    }

    return 0;
}