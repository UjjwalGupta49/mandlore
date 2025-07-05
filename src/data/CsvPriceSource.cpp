#include "data/CsvPriceSource.h"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <vector>

CsvPriceSource::CsvPriceSource(std::string csvPath) : path_{std::move(csvPath)} {}

std::vector<Bar> CsvPriceSource::fetch() {
    std::ifstream file(path_);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open CSV file: " + path_);
    }

    std::vector<Bar> bars;
    std::string line;
    // Skip header
    if (!std::getline(file, line)) {
        return bars; // empty file
    }

    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string cell;
        Bar bar;
        
        // timestamp,open,high,low,close,volume
        std::getline(ss, cell, ',');
        bar.timestamp = std::stoll(cell);
        std::getline(ss, cell, ',');
        bar.open = std::stod(cell);
        std::getline(ss, cell, ',');
        bar.high = std::stod(cell);
        std::getline(ss, cell, ',');
        bar.low = std::stod(cell);
        std::getline(ss, cell, ',');
        bar.close = std::stod(cell);
        std::getline(ss, cell, ',');
        bar.volume = std::stod(cell);

        bars.push_back(bar);
    }

    return bars;
} 