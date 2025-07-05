#pragma once

#include "data/PriceSource.h"
#include <string>

class CsvPriceSource : public PriceSource {
public:
    explicit CsvPriceSource(std::string csvPath);

    std::vector<Bar> fetch() override;

private:
    std::string path_;
}; 