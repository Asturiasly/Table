#pragma once

#include "common.h"
#include "formula.h"

#include <deque>
#include <functional>
#include <unordered_set>
#include <optional>

class Sheet;

class Cell : public CellInterface {
public:
    Cell(Sheet& sheet);
    ~Cell();

    void Set(std::string text);
    void Clear();
    bool IsReferenced() const;
    Value GetValue() const override;
    std::string GetText() const override;
    std::vector<Position> GetReferencedCells() const override;

private:
    class Impl;
    class EmptyImpl;
    class TextImpl;
    class FormulaImpl;
    std::pair<bool, Cell*> IsCircularDependency(const Impl& new_impl) const;
    void InvalidateCache();

    std::unique_ptr<Impl> impl_;
    Sheet& sheet_;
    std::unordered_set<Cell*> references_; //какие ячейки зависимы от этой
    std::unordered_set<Cell*> referenced_; //от каких ячеек зависит текущая
};