#include "formula.h"

#include "FormulaAST.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <sstream>

using namespace std::literals;

std::ostream& operator<<(std::ostream& output, FormulaError fe) {
    return output << "#ARITHM!";
}

namespace {

    class Formula : public FormulaInterface {
    public:
        // Реализуйте следующие методы:
        explicit Formula(std::string expression) try : ast_(ParseFormulaAST(expression))
        {}
        catch (const std::exception& e) {
            std::throw_with_nested(FormulaException(e.what()));
        }

        Value Evaluate(const SheetInterface& sheet) const override
        {
            double val = 0;
            try
            {
                val = ast_.Execute(sheet);
                if (std::isinf(val))
                    throw FormulaError(FormulaError::Category::Arithmetic);
            }
            catch (const FormulaError& exc)
            {
                return exc;
            }
            return val;
        }
        std::string GetExpression() const override
        {
            std::ostringstream out;
            ast_.PrintFormula(out);
            return out.str();
        }

        std::vector<Position> GetReferencedCells() const
        {
            std::vector<Position> cells;
            for (auto cell : ast_.GetCells()) 
            {
                if (cell.IsValid())
                    cells.push_back(cell);
            }
            std::sort(cells.begin(), cells.end());
            std::vector<Position>::iterator it;
            it = std::unique(cells.begin(), cells.end());

            cells.resize(std::distance(cells.begin(), it));
            return cells;
        }

    private:
        FormulaAST ast_;
    };
}  // namespace

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
    return std::make_unique<Formula>(std::move(expression));
}