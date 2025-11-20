#include <psdik.h>

using json = nlohmann::json;
using namespace boost::asio;
using namespace boost::interprocess;

// Генератор уникальных числовых ID

IdGenerator::IdGenerator() : gen(rd()), dis(1, 1LL << 62) {}

int64_t IdGenerator::generate() {
    // Используем комбинацию счетчика и случайного числа для уникальности
    return dis(gen) + (++counter);
}

// Для восстановления ID из конфига
void IdGenerator::setCounter(int64_t value) {
    counter.store(value);
}

int64_t IdGenerator::getCurrentCounter() const {
    return counter.load();
}




// Система логирования
static Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
};

void Logger::setLevel(Level level) { currentLevel = level; };

void Logger::log(Level level, const std::string& message) {
    if (level < currentLevel) return;
    
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    
    std::string levelStr;
    switch(level) {
        case DEBUG: levelStr = "DEBUG"; break;
        case INFO: levelStr = "INFO"; break;
        case WARNING: levelStr = "WARNING"; break;
        case ERROR: levelStr = "ERROR"; break;
    }
    
    std::cout << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S")
                << " [" << levelStr << "] " << message << std::endl;
};


#define LOG_DEBUG(msg) Logger::getInstance().log(Logger::DEBUG, msg)
#define LOG_INFO(msg) Logger::getInstance().log(Logger::INFO, msg)
#define LOG_WARNING(msg) Logger::getInstance().log(Logger::WARNING, msg)
#define LOG_ERROR(msg) Logger::getInstance().log(Logger::ERROR, msg)


// Кэш данных с историей

void DataCache::updateValue(int64_t id, const std::string& name, const json& value, const std::string& quality = "good") {
    std::lock_guard<std::mutex> lock(mutex);
    HistoricalValue hv{value, std::chrono::system_clock::now(), quality};
    
    currentValues[id] = hv;
    idToName[id] = name;
    history[id].push_back(hv);
    
    if (history[id].size() > maxHistorySize) {
        history[id].pop_front();
    }
    
    LOG_DEBUG("Updated value for " + name + " (ID: " + std::to_string(id) + "): " + value.dump());
}

std::vector<HistoricalValue> DataCache::getHistory(int64_t id, size_t count) {
    std::lock_guard<std::mutex> lock(mutex);
    auto it = history.find(id);
    if (it == history.end()) return {};
    
    count = std::min(count, it->second.size());
    return std::vector<HistoricalValue>(
        it->second.end() - count, 
        it->second.end()
    );
}

json DataCache::getCurrentValue(int64_t id) {
    std::lock_guard<std::mutex> lock(mutex);
    auto it = currentValues.find(id);
    if (it != currentValues.end()) {
        return it->second.value;
    }
    return json();
}

json DataCache::getAllCurrentValues() {
    std::lock_guard<std::mutex> lock(mutex);
    json result;
    for (const auto& [id, value] : currentValues) {
        auto nameIt = idToName.find(id);
        std::string name = (nameIt != idToName.end()) ? nameIt->second : "Unknown";
        
        result[std::to_string(id)] = {
            {"n", name}, // "n" вместо "name" для экономии места
            {"v", value.value}, // "v" вместо "value"
            {"t", std::chrono::duration_cast<std::chrono::milliseconds>(
                value.timestamp.time_since_epoch()).count()}, // "t" вместо "timestamp"
            {"q", value.quality} // "q" вместо "quality"
        };
    }
    return result;
}

std::string DataCache::getNameById(int64_t id) {
    std::lock_guard<std::mutex> lock(mutex);
    auto it = idToName.find(id);
    return it != idToName.end() ? it->second : "Unknown";
}

bool DataCache::idExists(int64_t id) {
    std::lock_guard<std::mutex> lock(mutex);
    return idToName.find(id) != idToName.end();
}


// Базовый класс для протоколов с улучшенной обработкой ошибок

ProtocolHandler::ProtocolHandler(const std::string& protoName, DataCache& cache) 
    : name(protoName), dataCache(cache) {}
    
virtual ~ProtocolHandler() = default;

void ProtocolHandler::setConnectionParameters(const json& config) {
    connectionParams.clear();
    connectionParams.push_back(config["primary"]);
    
    if (config.contains("secondary")) {
        for (const auto& secondary : config["secondary"]) {
            connectionParams.push_back(secondary);
        }
    }
}

virtual bool ProtocolHandler::connect() {
    if (connectionParams.empty()) {
        LOG_ERROR("No connection parameters for " + name);
        return false;
    }
    
    auto now = std::chrono::steady_clock::now();
    if (connectionAttempts > 0) {
        auto timeSinceLastAttempt = std::chrono::duration_cast<std::chrono::seconds>(
            now - lastConnectionAttempt);
        if (timeSinceLastAttempt.count() < (1 << connectionAttempts)) {
            return false; // Экспоненциальная задержка
        }
    }
    
    lastConnectionAttempt = now;
    
    for (size_t i = 0; i < connectionParams.size(); ++i) {
        size_t idx = (currentConnectionIndex + i) % connectionParams.size();
        LOG_INFO("Attempting to connect to " + name + " via " + 
                    connectionParams[idx]["host"].get<std::string>());
        
        if (trySpecificConnect(connectionParams[idx])) {
            connected = true;
            connectionAttempts = 0;
            currentConnectionIndex = idx;
            LOG_INFO("Successfully connected to " + name);
            return true;
        }
    }
    
    connectionAttempts++;
    connected = false;
    LOG_ERROR("All connection attempts failed for " + name);
    return false;
}

virtual void ProtocolHandler::disconnect() {
    connected = false;
    LOG_INFO("Disconnected from " + name);
}

bool ProtocolHandler::isConnected() const { return connected; }

void ProtocolHandler::updateData(int64_t id, const std::string& varName, const json& value, const std::string& quality = "good") {
    dataCache.updateValue(id, varName, value, quality);
    onDataReceived(id, varName, value);
}


// Modbus handler
bool ModbusTcpHandler::trySpecificConnect(const json& connectionParams) override {
    try {
        // Здесь будет реальная реализация подключения Modbus
        context = std::make_unique<ModbusContext>();
        context->host = connectionParams["host"];
        context->port = connectionParams["port"];
        
        // Имитация подключения
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        bool success = (rand() % 4 != 0); // 75% успешных подключений для демо
        
        if (success) {
            LOG_INFO("Modbus connected to " + context->host + ":" + 
                        std::to_string(context->port));
            return true;
        }
    } catch (const std::exception& e) {
        LOG_ERROR("Modbus connection error: " + std::string(e.what()));
    }
    return false;
}

json ModbusTcpHandler::readData(const json& variables) override {
    if (!connected) {
        if (!connect()) {
            return json::object();
        }
    }
    
    json result;
    try {
        for (const auto& [key, var] : variables.items()) {
            try {
                // Имитация чтения данных
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                
                // Генерация тестовых данных
                json value;
                std::string type = var["type"];
                if (type == "float32") {
                    value = static_cast<float>(rand() % 1000) / 10.0f;
                } else if (type == "uint16") {
                    value = rand() % 65535;
                } else if (type == "bool") {
                    value = (rand() % 2) == 1;
                } else if (type == "string") {
                    value = "test_string_" + std::to_string(rand() % 100);
                } else {
                    value = "unknown_type";
                }
                
                int64_t varId = var["id"];
                std::string varName = var["name"];
                
                result[std::to_string(varId)] = {
                    {"n", varName}, // Сокращенные ключи для экономии места
                    {"v", value},
                    {"t", type}
                };
                
                updateData(varId, varName, value);
                
            } catch (const std::exception& e) {
                LOG_ERROR("Error reading variable " + var["name"].get<std::string>() + 
                            ": " + e.what());
                updateData(var["id"], var["name"], json(), "bad");
            }
        }
    } catch (const std::exception& e) {
        LOG_ERROR("Modbus read error: " + std::string(e.what()));
        disconnect();
    }
    
    return result;
}

// Система подписки
void SubscriptionManager::addSubscriber(int64_t variableId, ip::tcp::socket socket) {
    std::lock_guard<std::mutex> lock(mutex);
    subscribers[variableId].push_back(std::move(socket));
    LOG_INFO("New subscription for variable ID: " + std::to_string(variableId));
}

void SubscriptionManager::notifySubscribers(int64_t variableId, const std::string& variableName, const json& value) {
    std::lock_guard<std::mutex> lock(mutex);
    auto it = subscribers.find(variableId);
    if (it == subscribers.end()) return;
    
    // Компактный формат для экономии трафика
    json message = {
        {"i", variableId}, // "i" вместо "id"
        {"n", variableName}, // "n" вместо "name"
        {"v", value}, // "v" вместо "value"
        {"t", std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count()}, // "t" вместо "timestamp"
        {"type", "data_update"}
    };
    
    std::string messageStr = message.dump() + "\n";
    
    auto& sockets = it->second;
    for (auto itSocket = sockets.begin(); itSocket != sockets.end(); ) {
        try {
            write(*itSocket, buffer(messageStr));
            ++itSocket;
        } catch (const std::exception& e) {
            LOG_WARNING("Subscriber disconnected: " + std::string(e.what()));
            itSocket = sockets.erase(itSocket);
        }
    }
}

void SubscriptionManager::removeDisconnected() {
    std::lock_guard<std::mutex> lock(mutex);
    for (auto& [variable, sockets] : subscribers) {
        sockets.erase(
            std::remove_if(sockets.begin(), sockets.end(),
                [](const ip::tcp::socket& socket) {
                    return !socket.is_open();
                }),
            sockets.end()
        );
    }
}


// Главный класс сервера
DataServer::DataServer() : subscriptionManager(dataCache) {
    lastConfigCheck = std::chrono::steady_clock::now();
}

void DataServer::loadConfig(const std::string& filename) {
    configFile = filename;
    std::ifstream f(filename);
    if (!f.is_open()) {
        throw std::runtime_error("Cannot open config file: " + filename);
    }
    
    config = json::parse(f);
    LOG_INFO("Configuration loaded from " + filename);
    
    // Восстановление счетчика ID из конфига
    restoreIdCounter();
    
    // Генерация ID для переменных, если их нет
    generateMissingIds();
    
    // Инициализация обработчиков протоколов
    initializeProtocols();
}

void DataServer::restoreIdCounter() {
    int64_t maxId = 0;
    for (auto& [proto, proto_config] : config.items()) {
        if (proto_config.contains("variables")) {
            for (auto& [key, var] : proto_config["variables"].items()) {
                if (var.contains("id") && var["id"].is_number()) {
                    int64_t id = var["id"];
                    if (id > maxId) maxId = id;
                }
            }
        }
    }
    
    if (maxId > 0) {
        idGenerator.setCounter(maxId);
        LOG_INFO("Restored ID counter to: " + std::to_string(maxId));
    }
}

void DataServer::generateMissingIds() {
    for (auto& [proto, proto_config] : config.items()) {
        if (proto_config.contains("variables")) {
            for (auto& [key, var] : proto_config["variables"].items()) {
                if (!var.contains("id") || !var["id"].is_number() || var["id"] == 0) {
                    var["id"] = idGenerator.generate();
                    LOG_INFO("Generated ID for variable: " + var["name"].get<std::string>() + 
                            " -> " + std::to_string(var["id"].get<int64_t>()));
                }
            }
        }
    }
}

void DataServer::saveConfig(const std::string& filename = "") {
    std::string targetFile = filename.empty() ? configFile : filename;
    std::ofstream f(targetFile);
    if (!f.is_open()) {
        throw std::runtime_error("Cannot open config file for writing: " + targetFile);
    }
    
    f << config.dump(4);
    LOG_INFO("Configuration saved to " + targetFile);
}

void DataServer::initializeProtocols() {
    protocols.clear();
    
    for (auto& [proto, proto_config] : config.items()) {
        std::unique_ptr<ProtocolHandler> handler;
        
        if (proto == "modbus_tcp") {
            handler = std::make_unique<ModbusTcpHandler>(dataCache);
        } else if (proto == "iec104") {
            // handler = std::make_unique<IEC104Handler>(dataCache);
        } else if (proto == "snmp") {
            // handler = std::make_unique<SNMPHandler>(dataCache);
        }
        
        if (handler) {
            handler->setConnectionParameters(proto_config["connection_parameters"]);
            
            // Подписка на события данных
            handler->onDataReceived.connect(
                [this](int64_t id, const std::string& name, const json& value) {
                    subscriptionManager.notifySubscribers(id, name, value);
                });
            
            handler->onConnectionStatusChanged.connect(
                [this, proto](const std::string&, bool connected) {
                    LOG_INFO(proto + " connection status: " + 
                            (connected ? "connected" : "disconnected"));
                });
            
            protocols[proto] = std::move(handler);
        }
    }
}

void DataServer::startPolling() {
    running = true;
    
    for (auto& [proto, handler] : protocols) {
        auto& vars = config[proto]["variables"];
        int pollingInterval = config[proto]["polling_interval_ms"];
        
        pollingThreads.emplace_back([this, handler = handler.get(), vars, pollingInterval]() {
            while (running) {
                if (!handler->isConnected()) {
                    handler->connect();
                } else {
                    auto data = handler->readData(vars);
                    // Данные автоматически обновляются в кэше через callback
                }
                
                std::this_thread::sleep_for(std::chrono::milliseconds(pollingInterval));
            }
        });
    }
    
    // Поток для проверки обновления конфигурации
    pollingThreads.emplace_back([this]() {
        while (running) {
            checkConfigUpdate();
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    });
}

void DataServer::checkConfigUpdate() {
    auto now = std::chrono::steady_clock::now();
    if (std::chrono::duration_cast<std::chrono::seconds>(now - lastConfigCheck).count() < 5) {
        return;
    }
    
    lastConfigCheck = now;
    
    try {
        std::ifstream f(configFile);
        if (!f.is_open()) return;
        
        json newConfig = json::parse(f);
        if (newConfig != config) {
            LOG_INFO("Configuration file changed, reloading...");
            config = newConfig;
            restoreIdCounter();
            generateMissingIds();
            initializeProtocols();
        }
    } catch (const std::exception& e) {
        LOG_ERROR("Error checking config update: " + std::string(e.what()));
    }
}

void DataServer::startTcpServer() {
    io_service service;
    ip::tcp::acceptor acceptor(service, ip::tcp::endpoint(ip::tcp::v4(), 8080));
    
    LOG_INFO("TCP server started on port 8080");
    
    while (running) {
        try {
            ip::tcp::socket socket(service);
            acceptor.accept(socket);
            
            std::thread([this, sock = std::move(socket)]() mutable {
                handleTcpClient(std::move(sock));
            }).detach();
        } catch (const std::exception& e) {
            LOG_ERROR("TCP server error: " + std::string(e.what()));
        }
    }
}

void DataServer::handleTcpClient(ip::tcp::socket socket) {
    try {
        streambuf buffer;
        read_until(socket, buffer, '\n');
        std::istream is(&buffer);
        std::string request;
        std::getline(is, request);
        
        json requestJson;
        try {
            requestJson = json::parse(request);
        } catch (const json::parse_error&) {
            // Простой текстовый запрос
            if (request.find("SUBSCRIBE") == 0) {
                // Формат: SUBSCRIBE variable_id
                auto pos = request.find(' ');
                if (pos != std::string::npos) {
                    std::string varIdStr = request.substr(pos + 1);
                    try {
                        int64_t varId = std::stoll(varIdStr);
                        if (dataCache.idExists(varId)) {
                            subscriptionManager.addSubscriber(varId, std::move(socket));
                            return; // Сокет будет использоваться для push-уведомлений
                        } else {
                            std::string response = "{\"error\": \"Unknown variable ID\"}\n";
                            write(socket, buffer(response));
                        }
                    } catch (const std::exception& e) {
                        std::string response = "{\"error\": \"Invalid variable ID format\"}\n";
                        write(socket, buffer(response));
                    }
                }
            } else if (request == "GET_ALL") {
                auto data = dataCache.getAllCurrentValues();
                std::string response = data.dump() + "\n";
                write(socket, buffer(response));
            } else if (request.find("GET_HISTORY") == 0) {
                // Формат: GET_HISTORY variable_id count
                auto pos1 = request.find(' ');
                auto pos2 = request.find(' ', pos1 + 1);
                if (pos1 != std::string::npos && pos2 != std::string::npos) {
                    std::string varIdStr = request.substr(pos1 + 1, pos2 - pos1 - 1);
                    int count = std::stoi(request.substr(pos2 + 1));
                    try {
                        int64_t varId = std::stoll(varIdStr);
                        auto history = dataCache.getHistory(varId, count);
                        json historyJson = json::array();
                        for (const auto& item : history) {
                            historyJson.push_back({
                                {"v", item.value}, // Сокращенные ключи
                                {"t", std::chrono::duration_cast<std::chrono::milliseconds>(
                                    item.timestamp.time_since_epoch()).count()},
                                {"q", item.quality}
                            });
                        }
                        std::string response = historyJson.dump() + "\n";
                        write(socket, buffer(response));
                    } catch (const std::exception& e) {
                        std::string response = "{\"error\": \"Invalid variable ID\"}\n";
                        write(socket, buffer(response));
                    }
                }
            } else if (request == "GET_CONFIG") {
                std::string response = config.dump(4) + "\n";
                write(socket, buffer(response));
            } else if (request.find("SAVE_CONFIG") == 0) {
                // Формат: SAVE_CONFIG [filename]
                auto pos = request.find(' ');
                std::string filename;
                if (pos != std::string::npos) {
                    filename = request.substr(pos + 1);
                }
                saveConfig(filename);
                std::string response = "{\"status\": \"success\", \"message\": \"Configuration saved\"}\n";
                write(socket, buffer(response));
            }
        }
        
        // JSON запрос
        if (!requestJson.empty()) {
            json response = handleJsonRequest(requestJson);
            std::string responseStr = response.dump() + "\n";
            write(socket, buffer(responseStr));
        }
        
    } catch (const std::exception& e) {
        LOG_ERROR("TCP client handling error: " + std::string(e.what()));
    }
}

json DataServer::handleJsonRequest(const json& request) {
    json response;
    
    if (request.contains("action")) {
        std::string action = request["action"];
        
        if (action == "get_all") {
            response = dataCache.getAllCurrentValues();
        } else if (action == "get_history") {
            int64_t variableId = request["variable_id"];
            int count = request.value("count", 10);
            auto history = dataCache.getHistory(variableId, count);
            response = json::array();
            for (const auto& item : history) {
                response.push_back({
                    {"v", item.value}, // Сокращенные ключи
                    {"t", std::chrono::duration_cast<std::chrono::milliseconds>(
                        item.timestamp.time_since_epoch()).count()},
                    {"q", item.quality}
                });
            }
        } else if (action == "get_config") {
            response = config;
        } else if (action == "save_config") {
            try {
                std::string filename = request.value("filename", "");
                saveConfig(filename);
                response = {{"status", "success"}, {"message", "Configuration saved successfully"}};
            } catch (const std::exception& e) {
                response = {{"status", "error"}, {"message", e.what()}};
            }
        } else if (action == "update_config") {
            try {
                json newConfig = request["config"];
                config = newConfig;
                restoreIdCounter();
                generateMissingIds();
                initializeProtocols();
                saveConfig();
                response = {{"status", "success"}, {"message", "Configuration updated and saved"}};
            } catch (const std::exception& e) {
                response = {{"status", "error"}, {"message", e.what()}};
            }
        } else if (action == "get_id_map") {
            // Возвращает маппинг ID -> имя для всех переменных
            json idMap;
            for (auto& [proto, proto_config] : config.items()) {
                if (proto_config.contains("variables")) {
                    for (auto& [key, var] : proto_config["variables"].items()) {
                        int64_t id = var["id"];
                        std::string name = var["name"];
                        idMap[std::to_string(id)] = name;
                    }
                }
            }
            response = idMap;
        }
    }
    
    return response;
}

void DataServer::stop() {
    running = false;
    for (auto& thread : pollingThreads) {
        if (thread.joinable()) thread.join();
    }
    pollingThreads.clear();
}


// Глобальный генератор ID
static IdGenerator idGenerator;

int main() {
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);
    
    try {
        DataServer server;
        server.loadConfig("config.json");
        server.startPolling();
        
        // Запуск TCP сервера в отдельном потоке
        std::thread tcpThread([&server]() {
            server.startTcpServer();
        });
        
        LOG_INFO("Data server started successfully");
        
        // Ожидание сигнала завершения
        while (!shutdownRequested) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        
        LOG_INFO("Shutting down...");
        server.stop();
        if (tcpThread.joinable()) tcpThread.join();
        
    } catch (const std::exception& e) {
        LOG_ERROR("Fatal error: " + std::string(e.what()));
        return 1;
    }
    
    return 0;
}
