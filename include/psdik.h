#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <thread>
#include <chrono>
#include <memory>
#include <mutex>
#include <atomic>
#include <deque>
#include <unordered_map>
#include <boost/asio.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/signals2.hpp>
#include <nlohmann/json.hpp>
#include <csignal>
#include <algorithm>
#include <cstdint>
#include <random>
#include <iomanip>
#include <sstream>

class IdGenerator {
private:
    std::atomic<int64_t> counter{0};
    std::random_device rd;
    std::mt19937_64 gen;
    std::uniform_int_distribution<int64_t> dis;
    
public:
    IdGenerator() : gen(rd()), dis(1, 1LL << 62);
    
    int64_t generate();
    // Для восстановления ID из конфига
    void setCounter(int64_t value);
    
    int64_t getCurrentCounter() const ;
};

// Система логирования
class Logger {
public:
    enum Level { DEBUG, INFO, WARNING, ERROR };
    
    static Logger& getInstance();
    
    void setLevel(Level level);
    
    void log(Level level, const std::string& message);
    
private:
    Logger() = default;
    Level currentLevel = INFO;
};

// Хеш-функция для int64_t
struct Int64Hash {
    std::size_t operator()(int64_t key) const {
        return std::hash<int64_t>{}(key);
    }
};

// Структура для хранения исторических данных
struct HistoricalValue {
    json value;
    std::chrono::system_clock::time_point timestamp;
    std::string quality; // "good", "bad", "uncertain"
};

// Кэш данных с историей
class DataCache {
private:
    std::mutex mutex;
    std::unordered_map<int64_t, std::deque<HistoricalValue>, Int64Hash> history;
    std::unordered_map<int64_t, HistoricalValue, Int64Hash> currentValues;
    std::unordered_map<int64_t, std::string, Int64Hash> idToName; // Маппинг ID -> имя
    size_t maxHistorySize = 100;
    
public:
    void updateValue(int64_t id, const std::string& name, const json& value, const std::string& quality = "good");
    std::vector<HistoricalValue> getHistory(int64_t id, size_t count);
    json getCurrentValue(int64_t id);
    json getAllCurrentValues();
    std::string getNameById(int64_t id);
    bool idExists(int64_t id);
};

// Базовый класс для протоколов с улучшенной обработкой ошибок
class ProtocolHandler {
protected:
    std::string name;
    std::atomic<bool> connected{false};
    std::atomic<int> connectionAttempts{0};
    std::chrono::steady_clock::time_point lastConnectionAttempt;
    std::vector<json> connectionParams; // Основной + резервные
    size_t currentConnectionIndex = 0;
    DataCache& dataCache;
    
public:
    ProtocolHandler(const std::string& protoName, DataCache& cache) 
        : name(protoName), dataCache(cache) {}  
    virtual ~ProtocolHandler() = default;
    void setConnectionParameters(const json& config);
    virtual bool connect();
    virtual void disconnect();
    virtual json readData(const json& variables) = 0;
    bool isConnected() const;
    
    // Callback система
    boost::signals2::signal<void(int64_t, const std::string&, const json&)> onDataReceived;
    boost::signals2::signal<void(const std::string&, bool)> onConnectionStatusChanged;

protected:
    virtual bool trySpecificConnect(const json& connectionParams) = 0;
    void updateData(int64_t id, const std::string& varName, const json& value, const std::string& quality = "good");
};

// Modbus handler
class ModbusTcpHandler : public ProtocolHandler {
private:
    // Заглушки для Modbus структур
    struct ModbusContext {
        std::string host;
        int port;
        // ... реальная реализация будет использовать libmodbus
    };
    
    std::unique_ptr<ModbusContext> context;
    
public:
    ModbusTcpHandler(DataCache& cache) : ProtocolHandler("modbus_tcp", cache) {}
    
protected:
    bool trySpecificConnect(const json& connectionParams) override ;
    
public:
    json readData(const json& variables) override ;
};

// Система подписки
class SubscriptionManager {
private:
    std::mutex mutex;
    DataCache& dataCache;
    std::unordered_map<int64_t, std::vector<ip::tcp::socket>, Int64Hash> subscribers;
    
public:
    SubscriptionManager(DataCache& cache) : dataCache(cache) {}
    
    void addSubscriber(int64_t variableId, ip::tcp::socket socket) ;
    
    void notifySubscribers(int64_t variableId, const std::string& variableName, const json& value) ;
    
    void removeDisconnected() ;
};

// Главный класс сервера
class DataServer {
private:
    std::map<std::string, std::unique_ptr<ProtocolHandler>> protocols;
    json config;
    DataCache dataCache;
    SubscriptionManager subscriptionManager;
    std::atomic<bool> running{false};
    std::vector<std::thread> pollingThreads;
    std::string configFile;
    std::chrono::steady_clock::time_point lastConfigCheck;
    
public:
    DataServer() : subscriptionManager(dataCache) ;
    void loadConfig(const std::string& filename) ;
    void restoreIdCounter() ;    
    void generateMissingIds() ;    
    void saveConfig(const std::string& filename = "") ;    
    void initializeProtocols() ;    
    void startPolling() ;    
    void checkConfigUpdate() ;    
    void startTcpServer() ;    
    void handleTcpClient(ip::tcp::socket socket) ;    
    json handleJsonRequest(const json& request) ;    
    void stop() ;
};


// Обработчик сигналов для graceful shutdown
std::atomic<bool> shutdownRequested{false};

void signalHandler(int signal) {
    shutdownRequested = true;
}