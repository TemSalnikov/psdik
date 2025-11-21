// test_psdic.h
#ifndef test_psdic_H
#define test_psdic_H

#include <string>
#include <fstream>
#include <nlohmann/json.hpp>

class TestUtilities {
public:
    static std::string createTempConfig(const nlohmann::json& config) {
        std::string filename = "test_config_" + std::to_string(std::time(nullptr)) + ".json";
        std::ofstream file(filename);
        file << config.dump(4);
        file.close();
        return filename;
    }
    
    static void deleteFile(const std::string& filename) {
        std::remove(filename.c_str());
    }
    
    static nlohmann::json createSampleModbusConfig() {
        return {
            {"modbus_tcp", {
                {"connection_parameters", {
                    {"primary", {
                        {"host", "localhost"},
                        {"port", 502},
                        {"timeout_ms", 1000}
                    }}
                }},
                {"variables", {
                    {"temperature", {
                        {"id", 1001},
                        {"name", "Temperature"},
                        {"address", 100},
                        {"type", "float32"},
                        {"polling_interval_ms", 1000}
                    }},
                    {"pressure", {
                        {"id", 1002},
                        {"name", "Pressure"},
                        {"address", 104},
                        {"type", "float32"},
                        {"polling_interval_ms", 2000}
                    }}
                }},
                {"polling_interval_ms", 100}
            }}
        };
    }
};

#endif // test_psdic_H