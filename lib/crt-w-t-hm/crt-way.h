#pragma once
#include <iostream>
#include <nlohmann/json.hpp>
#include <cpr/cpr.h>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <vector>
#include <map>
#include <string>
#include <optional>
#include <format>
#include <chrono>
#include <ctime>


namespace Constants {
    const int8_t kTokenSize = 36;
    static std::string null_str = "";
    const static std::unordered_map<std::string, std::string> transportMap = {
        {"bus", "Автобус🚍"},
        {"plane", "Самолет✈️"},
        {"suburban", "Элктричка🚊"},
        {"train", "Поезд🚆"},
        {"water", "Корабль🚢"},
        {"helicopter", "Вертолет🚁"}
    };

    const static std::unordered_map<std::string, std::string> cityMap = {
        {"Москва", "c213"},
        {"Санкт-Петербург", "c2"},
        {"Краснодар", "c35"},
        {"Екатеринбург", "c54"}
    };
    
    const std::vector<std::string> cities = {"Москва", "Санкт-Петербург", "Краснодар", "Екатеринбург"};
    const std::string API_path = "../data/API_token.txt";
    constexpr size_t limit = 40;
}

class Errors {
public:
    static void fileOpenErrorMessage(bool open_file);
    
    static void validateTokenCheck(const std::string& token);

    static void DataFormatError();

    static void BadRequest();
};
bool isLeapYear(int year);
bool isValidDate(const std::string& date);
std::string convertCityToYandexCode(std::string& city);

class TArgs {
public:
    TArgs(std::string from = "", std::string to = "", std::string transfers = "true", int limits = Constants::limit, std::string date = "");

    std::string from;
    std::string to;
    std::string transfers;
    std::string date;
    int limits;
};

class JsonFields {
public:
    std::string fromCity = "";
    std::string toCity = "";
    std::string departureTime = "";
    std::string arrivalTime = "";
    int transfers = 0;
    std::string transferCity = "";
    std::vector<std::string> transport;
    std::string price = "";
    std::string codeOfVoyage = "";
    int duration = 0;
    int parseTimeDuration[3];


    friend std::ostream& operator<<(std::ostream& stream, JsonFields& obj);
};

class Request {
public:
    Request(TArgs arguments, std::string token_path);

    std::string getTokenFromFile();
    JsonFields extractJsonFields(nlohmann::json& json_request, size_t iterator);
    nlohmann::json SendRequestToAPI();
    std::vector<JsonFields> FilterResJson(nlohmann::json json_data);

private:
    TArgs args_;
    std::string tokenPath_;
    const std::string TOKEN_ = getTokenFromFile();
};

std::string PrintRequests(std::vector<JsonFields>& schedule, TArgs& args, size_t limit);

std::string PrintDuration(int duration);

std::string print_cities();

std::string getCurrentDate();