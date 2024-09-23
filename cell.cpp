#include "cell.h"
#include "sheet.h"

#include <cassert>
#include <iostream>
#include <string>
#include <optional>

Cell::Cell(Sheet& sheet) : sheet_(sheet) {}
Cell::~Cell() {}

class Cell::Impl
{
public:
	Impl() = default;
	~Impl() = default;
	virtual Value GetValue() const = 0;
	virtual std::string GetText() const = 0;
	virtual std::vector<Position> GetReferencedCells() const { return {}; };
	virtual void InvalidateCache() const { return; }
protected:
	Value value_;
	std::string text_;
};

class Cell::EmptyImpl : public Impl
{
public:
	Value GetValue() const override
	{
		return "";
	}
	std::string GetText() const override
	{
		return "";
	}
};

class Cell::TextImpl : public Impl
{
public:
	TextImpl(Value val, std::string text)
	{
		value_ = val; text_ = text;
	}
	Value GetValue() const override
	{
		return value_;
	}
	std::string GetText() const override
	{
		return text_;
	}
};

class Cell::FormulaImpl : public Impl
{
public:
	explicit FormulaImpl(std::string text, const SheetInterface& sheet) : sheet_(sheet)
	{
		value_ = text.substr(1);
		formula_ptr = ParseFormula(std::get<std::string>(value_));
		text_ = "=" + formula_ptr->GetExpression();
	}
	Value GetValue() const override
	{
		if (cache_)
			return cache_.value();
		auto result = formula_ptr->Evaluate(sheet_);
		if (std::holds_alternative<double>(result)) {
			cache_ = std::get<double>(result);
			return std::get<double>(result);
		}
		return std::get<FormulaError>(result);
	}
	std::string GetText() const override
	{
		return text_;
	}
	std::vector<Position> GetReferencedCells() const override
	{
		return formula_ptr->GetReferencedCells();
	}
	void InvalidateCache() const override
	{
		cache_.reset();
	}
private:
	mutable std::optional<double> cache_;
	std::unique_ptr<FormulaInterface> formula_ptr;
	const SheetInterface& sheet_;
};

void Cell::Set(std::string text)
{
	std::unique_ptr<Impl> impl;
	if (text.size() == 0)
	{
		impl = std::move(std::make_unique<EmptyImpl>());
	}
	else if (text[0] == '=' && text.size() > 1)
	{
		impl = std::move(std::make_unique<FormulaImpl>(text, sheet_));
	}
	else if (text[0] == '\'')
	{
		auto val = text.substr(1);
		auto impl_ptr = std::make_unique<TextImpl>(val, text);
		impl = std::move(impl_ptr);
	}
	else
	{
		auto impl_ptr = std::make_unique<TextImpl>(text, text);
		impl = std::move(impl_ptr);
	}


	bool just_created = false;
	Position erase_pos;
	for (const auto& pos : impl->GetReferencedCells())
	{
		auto cell_ptr = reinterpret_cast<Cell*>(sheet_.GetCell(pos));
		if (cell_ptr == nullptr)
		{
			sheet_.SetCell(pos, "");
			just_created = true;
			erase_pos = pos;
			cell_ptr = reinterpret_cast<Cell*>(sheet_.GetCell(pos));
		}

		referenced_.insert(cell_ptr);
		cell_ptr->references_.insert(this);
	}

	std::pair<bool, Cell*> temp = IsCircularDependency(*impl.get());
	bool is_circular = temp.first;
	if (is_circular)
	{
		Cell* cell_to_erase_reference = temp.second;
		if (just_created)
		{
			auto to_erase_cell_ptr = reinterpret_cast<Cell*>(sheet_.GetCell(erase_pos));
			this->referenced_.erase(to_erase_cell_ptr);
			to_erase_cell_ptr->Clear();
			sheet_.DeleteCell(erase_pos);
		}
		else
		{
			this->referenced_.erase(cell_to_erase_reference);
			cell_to_erase_reference->references_.erase(this);
		}
		throw CircularDependencyException("Circular Reference has found!");
	}

	if (IsReferenced())
	{
		InvalidateCache();
	}

	impl_ = std::move(impl);
}

Cell::Value Cell::GetValue() const {
	return impl_->GetValue();
}

std::string Cell::GetText() const {
	return impl_->GetText();
}

void Cell::Clear() {
	impl_ = std::make_unique<EmptyImpl>();
}

std::vector<Position> Cell::GetReferencedCells() const
{
	return impl_->GetReferencedCells();
}

std::pair<bool, Cell*> Cell::IsCircularDependency(const Impl& new_impl) const
{
	std::unordered_set<const Cell*> visited;
	std::deque<const Cell*> have_to_visit;
	have_to_visit.push_front(this);
	visited.insert(this);
	while (have_to_visit.size() != 0)
	{
		for (const auto& cell : have_to_visit.front()->references_)
		{
			if (visited.count(cell))
			{
				return { true, const_cast<Cell*>(have_to_visit.front()) };
			}
			visited.insert(cell);
			if (cell->references_.size() != 0)
			{
				have_to_visit.push_front(cell);
			}
		}
		have_to_visit.pop_back();
	}
	return { false, nullptr };
}

void Cell::InvalidateCache()
{
	if (this->impl_ == nullptr)
		return;

	std::deque<const Cell*> have_to_visit;
	have_to_visit.push_front(this);
	this->impl_->InvalidateCache();
	while (have_to_visit.size() != 0)
	{
		for (const auto& cell : referenced_)
		{
			cell->impl_->InvalidateCache();
			if (cell->referenced_.size() != 0)
			{
				for (const auto& ref_cell : cell->referenced_)
				{
					have_to_visit.push_front(ref_cell);
				}
			}
		}
		have_to_visit.pop_back();
	}
}

bool Cell::IsReferenced() const
{
	if (referenced_.size() == 0)
		return false;
	return true;
}

