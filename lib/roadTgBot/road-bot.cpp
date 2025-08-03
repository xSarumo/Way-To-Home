#include "road-bot.h"
#include <unordered_map>



std::string makeDataString(std::vector<JsonFields>& schedule) {
    std::stringstream string_stream;
    for (auto& request : schedule) {
        string_stream << request << "\n";
    }
    return string_stream.str();
}

std::string getTgApiToken(){
    std::ifstream file(tokenTgPATH);
    if (!file) {
        return "";
    } else {
        std::string token;
        std::getline(file, token);
        file.close();
        return token;
    } 
}

int Bot() {

    TgBot::Bot bot(getTgApiToken());

    std::unordered_map<long, int> userMessageId;
    std::unordered_map<long, int> userState;
    std::unordered_map<long, std::string> userData;
    TArgs arguments;

    auto createCityKeyboard = [](const std::string& action) {
        TgBot::InlineKeyboardMarkup::Ptr keyboard(new TgBot::InlineKeyboardMarkup);
        std::vector<TgBot::InlineKeyboardButton::Ptr> row1;
        std::vector<TgBot::InlineKeyboardButton::Ptr> row2;

        TgBot::InlineKeyboardButton::Ptr btn1(new TgBot::InlineKeyboardButton);
        btn1->text = "Москва";
        btn1->callbackData = action + "_Москва";
        row1.push_back(btn1);

        TgBot::InlineKeyboardButton::Ptr btn2(new TgBot::InlineKeyboardButton);
        btn2->text = "Санкт-Петербург";
        btn2->callbackData = action + "_Санкт-Петербург";
        row1.push_back(btn2);

        TgBot::InlineKeyboardButton::Ptr btn3(new TgBot::InlineKeyboardButton);
        btn3->text = "Краснодар";
        btn3->callbackData = action + "_Краснодар";
        row2.push_back(btn3);

        TgBot::InlineKeyboardButton::Ptr btn4(new TgBot::InlineKeyboardButton);
        btn4->text = "Екатеринбург";
        btn4->callbackData = action + "_Екатеринбург";
        row2.push_back(btn4);

        keyboard->inlineKeyboard.push_back(row1);
        keyboard->inlineKeyboard.push_back(row2);
        return keyboard;
    };

    bot.getEvents().onCommand("start", [&bot, &userState, &userMessageId, &createCityKeyboard](TgBot::Message::Ptr message) {
        TgBot::Message::Ptr sentMessage = bot.getApi().sendMessage(
            message->chat->id, "🗺️ Выберите город отправления:\n\n👇👇👇", nullptr, 0, createCityKeyboard("from")
        );
        userMessageId[message->chat->id] = sentMessage->messageId;
        userState[message->chat->id] = 1;
    });

    bot.getEvents().onCallbackQuery([&bot, &userState, &userData, &userMessageId, &createCityKeyboard, &arguments](TgBot::CallbackQuery::Ptr query) {
        long chatId = query->message->chat->id;
        int messageId = query->message->messageId;
        std::string callbackData = query->data;

        int state = userState[chatId];

        if (callbackData.find("from_") == 0 && state == 1) { 
            std::string city = callbackData.substr(5);
            userData[chatId + 1] = city;
            arguments.from = convertCityToYandexCode(city);

            bot.getApi().editMessageText("📍 Выберите город прибытия:\n\n👇👇👇", chatId, messageId, "", "", nullptr, createCityKeyboard("to"));
            userState[chatId] = 2;
        }
        else if (callbackData.find("to_") == 0 && state == 2) { 
            std::string city = callbackData.substr(3);
            userData[chatId + 2] = city;
            if (userData[chatId + 1] != userData[chatId + 2]) {
                arguments.to = convertCityToYandexCode(city);
                bot.getApi().editMessageText("🗓️ Теперь введите дату (например, 2025-02-25):", chatId, messageId);
                userState[chatId] = 3;
                bot.getApi().answerCallbackQuery(query->id);
            } else {
                bot.getApi().sendMessage(chatId, "🛑 Вы выбрали тот же город, введите /start, чтобы начать сначала");
            }
        }
    });

    bot.getEvents().onAnyMessage([&bot, &userState, &userData, &userMessageId, &arguments](TgBot::Message::Ptr message) {
        long chatId = message->chat->id;

        if (userState.find(chatId) == userState.end() and message->text != "/start") {
            bot.getApi().sendMessage(chatId, "🛎️ Отправь /start для начала поиска.");
            return;
        }

        int state = userState[chatId];

        if (state == 3) {
            userData[chatId + 3] = message->text;
            arguments.date = message->text;
            if(isValidDate(arguments.date)) {

                std::string response = "Вы выбрали маршрут:\n";
                response += "🏙 Отправление: " + userData[chatId + 1] + "\n";
                response += "🏙 Прибытие: " + userData[chatId + 2] + "\n\n";
                response += "📅 Дата: " + userData[chatId + 3] + "\n\n";
                response += "🔍 Ищу доступные рейсы...";

                TgBot::Message::Ptr sentMessage = bot.getApi().sendMessage(chatId, response);
                userMessageId[chatId] = sentMessage->messageId;
                
                std::string path = "../data/API_token.txt";
                arguments.limits = 1;
                Request req(arguments, path);
                std::vector<JsonFields> schedule = req.FilterResJson(req.SendRequestToAPI());
                response = PrintRequests(schedule, arguments, 12);
            
                bot.getApi().editMessageText(response, chatId, userMessageId[chatId]);

                
                userState.erase(chatId);
            } else {
                bot.getApi().sendMessage(chatId, "Неверный формат даты. Попробуйте еще раз. /start");
            }
        }
    });

    try {
        std::cout << "[+] Bot started..." << std::endl;
        bot.getApi().deleteWebhook();
        TgBot::TgLongPoll longPoll(bot);

        while (true) {
            longPoll.start();
        }
    } catch (const std::exception& e) {
        std::cerr << "[-] Error: " << e.what() << std::endl;
    }

    return 0;
}
