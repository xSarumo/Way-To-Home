#include "crt-way.h"


void Errors::fileOpenErrorMessage(bool open_file = false) {
    if (!open_file){
        std::cerr << "[-] Error: File could not be opened!\n";
        exit(1);
    } else {
        std::cerr << "[+] File successfully opened!\n";
    }
    return;
}

void Errors::validateTokenCheck(const std::string& token) {
    bool include_spaces = token.find(" ") != std::string::npos;
    if ((token.size() < Constants::kTokenSize) || (include_spaces)) {
        std::cerr << "[-] Error: Not valid token!\n";
        if (include_spaces) {
            std::cerr << "[!] Check the token for spaces. If there are any, please remove them and try again." << std::endl;
            exit(1);
        }
    } else {
        std::cerr << "[+] Right token!\n";
        return;
    }
    return;
}

void Errors::DataFormatError(){
    std::cerr << "[-] Error: Wrong data input!" << std::endl;
    exit(1);
    return;
}

void Errors::BadRequest(){
    std::cerr << "[-] Error: API URL is empty!" << std::endl;
    exit(1);
    return;
}

TArgs::TArgs(std::string from, std::string to, std::string transfers, int limits, std::string date):
    from(convertCityToYandexCode(from)), to(convertCityToYandexCode(to)), transfers(transfers), limits(limits), date(date) {}

Request::Request(TArgs arguments, std::string token_path = Constants::API_path)
    : tokenPath_(token_path), args_(arguments), TOKEN_(getTokenFromFile()) {}

std::string Request::getTokenFromFile() { 
    std::ifstream file(tokenPath_);
    if (!file) {
        Errors::fileOpenErrorMessage();
        return Constants::null_str;
    } else {
        std::string token;

        std::getline(file, token);
        token = token.substr(0, Constants::kTokenSize);
        Errors::fileOpenErrorMessage(true);
        file.close(); 
        Errors::validateTokenCheck(token);
        return token;
    }  
}

template<typename T>
std::optional<T> getArgumentSafe(const nlohmann::json& j, const std::vector<std::string>& path) {
    const nlohmann::json* current = &j;
    for (const auto& key : path) {
        if (!current->contains(key) || (*current)[key].is_null()) {
            return std::nullopt;
        }
        current = &((*current)[key]);
        
        if (!current->is_object() && key != path.back()) {
            return std::nullopt;
        }
    }
    return current->get<T>();
}

std::time_t convertTimeFromDeparture(const std::string& segmentTime) {
    int hours, minutes, seconds;
    char colon;

    std::istringstream iss(segmentTime);
    iss >> hours >> colon >> minutes >> colon >> seconds;

    std::time_t now = std::time(nullptr);
    std::tm* now_tm = std::localtime(&now);

    std::tm curTm = *now_tm; 
    curTm.tm_hour = hours;  
    curTm.tm_min = minutes;
    curTm.tm_sec = seconds;

    return std::mktime(&curTm);
}

void clearOldData(nlohmann::json& cacheFileJson) {
    std::time_t now = std::time(nullptr);
    std::string currentDate = getCurrentDate();

    for (size_t i = 0; i < cacheFileJson["segments"].size(); ) {
        auto& segment = cacheFileJson["segments"][i];
        std::string departureDate = getArgumentSafe<std::string>(segment, {"departure"}).value_or("").substr(0, 10);
        std::string departureTime = getArgumentSafe<std::string>(segment, {"departure"}).value_or("").substr(11, 8);

        if (departureDate != currentDate) {
            ++i;
            continue;
        }

        std::time_t segmentTime = convertTimeFromDeparture(departureTime);

        if (now > segmentTime) {
            cacheFileJson["segments"].erase(i);
        } else {
            ++i;
        }
    }
}

nlohmann::json Request::SendRequestToAPI() {
    if (!isValidDate(args_.date)) {
        Errors::DataFormatError();
        exit(1);
    }

    std::string cacheFilePath = "../data/cache/" + args_.date + args_.from + "-" + args_.to + ".json";
    std::ifstream inFile(cacheFilePath);
    nlohmann::json request;

    if (inFile) {
        std::stringstream buffer;
        buffer << inFile.rdbuf();
        inFile.close();

        request = nlohmann::json::parse(buffer.str());
        std::size_t originalSize = request["segments"].size();

        clearOldData(request);

        if (request["segments"].size() != originalSize) {
            std::ofstream outFile(cacheFilePath, std::ios::trunc);
            outFile << request.dump(4);
        }
    } else {
        std::string API_Req_link = "https://api.rasp.yandex.net/v3.0/search/?apikey=" + TOKEN_ +
                                   "&format=json&from=" + args_.from + "&to=" + args_.to +
                                   "&lang=ru_RU&page=1&date=" + args_.date +
                                   "&limit=20&transfers=" + args_.transfers;
        if (API_Req_link.empty()) {
            Errors::BadRequest();
        }
        std::cout << "[?] Sending request..." << std::endl;
        try {
            cpr::Response resp_full = cpr::Get(cpr::Url(API_Req_link));
            std::cout << "[+] Request successfully sent!" << std::endl;
            if (resp_full.status_code / 100 != 2) {
                std::cerr << "[-] Request failed with status code: " << resp_full.status_code << std::endl;
                exit(1);
            }
            request = nlohmann::json::parse(resp_full.text);
            clearOldData(request);
            std::ofstream outFile(cacheFilePath);
            outFile << request.dump(4);
        } catch (const std::exception& e) {
            std::cerr << "[-] Exception occurred: " << e.what() << std::endl;
            exit(1);
        }
    }
    return request;
}


JsonFields Request::extractJsonFields(nlohmann::json& json_request, size_t iterator) {
    JsonFields fields;

    fields.fromCity = getArgumentSafe<std::string>(json_request, {"search","from","title"}).value_or("");
    fields.toCity   = getArgumentSafe<std::string>(json_request, {"search","to","title"}).value_or("");

    const auto& segment = json_request["segments"][iterator];

    fields.arrivalTime   = getArgumentSafe<std::string>(segment, {"arrival"}).value_or("");
    fields.departureTime = getArgumentSafe<std::string>(segment, {"departure"}).value_or("");

    if (getArgumentSafe<bool>(segment, {"has_transfers"}).value_or(false)) {
        if (segment.contains("transfers") && segment["transfers"].is_array()) {
            fields.transfers = segment["transfers"].size();
            auto& segment_tmp = segment["transfers"][0];

            if (fields.transfers == 1) {
                fields.transferCity = getArgumentSafe<std::string>(segment_tmp, {"title"}).value_or("");
            }
        }

        if (segment.contains("transport_types")) {
            for (auto& transport : segment["transport_types"]) {
                fields.transport.push_back(transport.get<std::string>());
            }
        }
    } else {
        fields.transfers = 0;
    }
    if (fields.transport.empty()){
        fields.transport.push_back(getArgumentSafe<std::string>(segment, {"from", "transport_type"}).value_or(""));
    }
    fields.duration = getArgumentSafe<int>(segment, {"duration"}).value_or(0);
    if (fields.duration == 0) {
        int tmp_dur = 0;
        auto& tmp_segment = segment["details"];
        for (auto& seg : tmp_segment) {
            tmp_dur += getArgumentSafe<int>(seg, {"duration"}).value_or(0);
        }
        fields.duration = tmp_dur;
    }
    return fields;
}

std::vector<JsonFields> Request::FilterResJson(nlohmann::json json_data) {
    std::vector<JsonFields> travelSchadules;
    for(size_t i = 0; i < json_data["segments"].size(); ++i) {
        JsonFields tmp_Road = extractJsonFields(json_data, i);
        if (tmp_Road.transfers < 2) {
            travelSchadules.push_back(tmp_Road);
        }
    }
    return travelSchadules;
}

std::string convertTransport(std::string& jformatString) {
    if (Constants::transportMap.find(jformatString) != Constants::transportMap.end()) {
        return Constants::transportMap.at(jformatString);
    } else {
        return "Неизвестно";
    }
}

std::string convertCityToYandexCode(std::string& city) {
    if (Constants::cityMap.find(city) != Constants::cityMap.end()) {
        return Constants::cityMap.at(city);
    } else {
        return Constants::null_str;
    }
}

bool isLeapYear(int year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

std::string getCurrentDate() {
    std::time_t now = std::time(nullptr);
    std::tm* localTime = std::localtime(&now);
    
    std::ostringstream oss;
    oss << std::put_time(localTime, "%Y-%m-%d");
    return oss.str();
}

bool isValidDate(const std::string& date) {
    int year, month, day;
    char dash1, dash2;
    std::istringstream ss(date);
    
    if (!(ss >> year >> dash1 >> month >> dash2 >> day)) {
        return false;
    }
    if (dash1 != '-' || dash2 != '-') {
        return false;
    }
    if (month < 1 || month > 12 || day < 1 || day > 31 || year < 2025) {
        return false;
    }
    int daysInMonth[] = { 31, isLeapYear(year) ? 29 : 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    if (day > daysInMonth[month - 1]) {
        return false;
    }
    
    std::string currentDate = getCurrentDate();
    if (date < currentDate) {
        return false;
    }
    
    return true;
}
std::string PrintDuration(int duration) {
    int hours = duration / 60 / 60;
    int minutes = (duration / 60) - (hours * 60);
    return std::to_string(hours) + " ч. " + std::to_string(minutes) + " мин.";
}

std::ostream& operator<<(std::ostream& stream, JsonFields& obj) {
    stream << "◆ Путь🛤️: \n"
           << "|➜ " << obj.fromCity << " ➜{ " << convertTransport(obj.transport[0]) << " }➜" 
           << (obj.transferCity != ""? " " + obj.transferCity + " ➜{ " + convertTransport(obj.transport[1]) + " }➜" : "") << " " << obj.toCity <<  " \n|\n"
           << "◆ Время отправки🛫: \n|➜ Дата: " << obj.departureTime.replace(10, 1, " Время: ") 
           << " \n|\n◆ Время прибытия🛬: \n|➜ Дата: " + obj.arrivalTime.replace(10, 1, " Время: ") +  " \n|\n"
           << "◆Общая продолжительность пути⏳: " << "\n|➜ " << PrintDuration(obj.duration) << "\n"; 
    return stream;
}

std::string PrintRequests(std::vector<JsonFields>& schedule, TArgs& args, size_t limit) {
    std::ostringstream stream;
    size_t counter = 0;
    for (auto& req : schedule) {
        if (counter < limit) {
            counter++;
            stream << "🚏 Маршрут №" << counter << " :\n";
            stream << req << "\n";
            stream << "🚧---🚧---🚧\n\n";
        }
    }
    stream << "🔍 Всего найдено маршрутов: " << counter << "\n\n";
    stream << "🧳 Ссылка на сайт для покупки билетов: https://rasp.yandex.ru/search/?fromId=" + args.from + "&toId="+ args.to +"&when=" + args.date + "\n";
    return stream.str();
}

std::string print_cities() {
    std::string str_tmp;
    for(size_t i; i < Constants::cities.size(); ++i) {
       str_tmp += Constants::cities[i] + "\n";
    }
    return str_tmp;
}