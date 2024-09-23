#include "sheet.h"

#include "cell.h"
#include "common.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <optional>

using namespace std::literals;

Sheet::~Sheet() {}

void Sheet::SetCell(Position pos, std::string text)
{
    if (!pos.IsValid())
    {
        throw InvalidPositionException("Invalid position");
    }

    if (sheet_.count(pos))
    {
        sheet_[pos].get()->Set(text);
    }
    else
    {
        auto cell_ptr = std::make_unique<Cell>(*this);
        cell_ptr->Set(text);
        if (sheet_.count(pos))
        {
            this->ClearCell(pos);
            throw CircularDependencyException("Self reference has found!");
        }
        sheet_[pos] = std::move(cell_ptr);
    }

    if (pos.row + 1 >= size_.rows)
        size_.rows = pos.row + 1;
    if (pos.col + 1 >= size_.cols)
        size_.cols = pos.col + 1;

    if (!cols_.count(pos.col))
    {
        cols_[pos.col] = 1;
    }
    else
        ++cols_[pos.col];

    if (!rows_.count(pos.row))
    {
        rows_[pos.row] = 1;
    }
    else
        ++rows_[pos.row];
}

const CellInterface* Sheet::GetCell(Position pos) const
{
    if (!pos.IsValid())
    {
        throw InvalidPositionException("Invalid position");
    }

    if (sheet_.count(pos))
        return sheet_.at(pos).get();
    else
        return nullptr;
}
CellInterface* Sheet::GetCell(Position pos)
{
    if (!pos.IsValid())
    {
        throw InvalidPositionException("Invalid position");
    }

    if (sheet_.count(pos))
        return sheet_[pos].get();
    else
    {
        return nullptr;
    }
}

void Sheet::ClearCell(Position pos)
{
    if (!pos.IsValid())
    {
        throw InvalidPositionException("Invalid position");
    }

    if (!sheet_.count(pos))
    {
        return;
    }

    int empty_rows = 0; int empty_cols = 0;
    auto prev_it_row = std::prev(rows_.end());
    auto prev_it_col = std::prev(cols_.end());

    if (prev_it_row == rows_.begin())
        empty_rows = (*prev_it_row).first;
    else
        empty_rows = (*prev_it_row).first - (*(std::prev(prev_it_row))).first - 1;

    if (prev_it_col == cols_.begin())
        empty_cols = (*prev_it_col).first;
    else
        empty_cols = (*prev_it_col).first - (*(std::prev(prev_it_col))).first - 1;

    bool not_edge = pos.row + 1 < size_.rows && pos.col + 1 < size_.cols;
    if (not_edge)
    {
        --rows_[pos.row];
        --cols_[pos.col];

        if (rows_[pos.row] == 0)
            rows_.erase(pos.row);
        if (cols_[pos.col] == 0)
            cols_.erase(pos.col);
        sheet_[pos].release();
        sheet_.erase(pos);
    }
    else
    {
        if (rows_[pos.row] == 1)
        {
            size_.rows -= empty_rows + 1;
            rows_.erase(pos.row);
        }
        else
        {
            size_.rows -= empty_rows;
            --rows_[pos.row];
        }

        if (cols_[pos.col] == 1)
        {
            size_.cols -= empty_cols + 1;
            cols_.erase(pos.col);
        }
        else
        {
            size_.cols -= empty_rows;
            --cols_[pos.col];
        }
        sheet_[pos].release();
        sheet_.erase(pos);
    }
}

Size Sheet::GetPrintableSize() const
{
    return size_;
}

void Sheet::PrintValues(std::ostream& output) const
{
    if (sheet_.size() == 0)
        return;

    if (rows_.size() == 0 && cols_.size() == 0)
    {
        if (sheet_.count(Position{ 0, 0 }))
        {
            auto temp = sheet_.at(Position{ 0,0 }).get()->GetValue();
            std::visit([&](auto arg) { output << arg; }, temp);
        }
        output << "\n";
        return;
    }

    int max_row = 0;
    int max_col = 0;
    if (rows_.size() != 0)
    {
        max_row = (*std::prev(rows_.end())).first;
    }
    if (cols_.size() != 0)
    {
        max_col = (*std::prev(cols_.end())).first;
    }

    for (int row = 0; row < max_row + 1; ++row)
    {
        for (int col = 0; col < max_col + 1; ++col)
        {
            Position pos{ row, col };
            if (sheet_.count({ row, col }))
            {
                auto temp = sheet_.at(pos).get()->GetValue();
                std::visit([&](auto arg) { output << arg; }, temp);
            }
            if (max_col != col)
                output << "\t";
        }
        output << "\n";
    }
}
void Sheet::PrintTexts(std::ostream& output) const
{
    if (sheet_.size() == 0)
        return;

    if (rows_.size() == 0 && cols_.size() == 0)
    {
        if (sheet_.count(Position{ 0, 0 }))
            output << sheet_.at(Position{ 0, 0 }).get()->GetText();
        output << "\n";
        return;
    }

    int max_row = 0;
    int max_col = 0;
    if (rows_.size() != 0)
    {
        max_row = (*std::prev(rows_.end())).first;
    }
    if (cols_.size() != 0)
    {
        max_col = (*std::prev(cols_.end())).first;
    }

    for (int row = 0; row < max_row + 1; ++row)
    {
        for (int col = 0; col < max_col + 1; ++col)
        {
            Position pos{ row, col };
            if (sheet_.count(pos))
            {
                output << sheet_.at(pos).get()->GetText();
            }
            if (max_col != col)
                output << "\t";
        }
        output << "\n";
    }
}

std::unique_ptr<SheetInterface> CreateSheet()
{
    return std::make_unique<Sheet>();
}