#pragma once

#include "cell.h"
#include "common.h"

#include <functional>
#include <unordered_map>
#include <map>


class CellHasher {
public:
    size_t operator()(const Position p) const {
        return std::hash<std::string>()(p.ToString());
    }
};

class Sheet : public SheetInterface {
public:
    ~Sheet();

    void SetCell(Position pos, std::string text) override;

    const CellInterface* GetCell(Position pos) const override;
    CellInterface* GetCell(Position pos) override;

    void ClearCell(Position pos) override;

    Size GetPrintableSize() const override;

    void PrintValues(std::ostream& output) const override;
    void PrintTexts(std::ostream& output) const override;

private:
    std::unordered_map<Position, std::unique_ptr<Cell>, CellHasher> sheet_;
    std::map<int, int> rows_; // row and num of values in
    std::map<int, int> cols_; // col and num of values in
    Size size_{ 0, 0 };
};