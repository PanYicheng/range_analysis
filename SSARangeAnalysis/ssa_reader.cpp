#include<ssa_reader.h>
#include<fstream>
#include<iostream>
#include<iomanip>
#include<cstdlib>
#include<deque>
#include<sstream>

#define MAX_LINE_LENGTH 255
#define MAX_SIMULATION_STEP 10


std::string reverseCompareOpe(std::string ope)
{
	if (ope == "<")
		return ">=";
	if (ope == "<=")
		return ">";
	if (ope == ">")
		return "<=";
	if (ope == ">=")
		return "<";
	if (ope == "!=")
		return "==";
	if (ope == "==")
		return "!=";
	return "Error Input";
}

// these defines are used to enable debug output during building of the SSA graph
//#define DEBUG

//#define DEBUG_VAR
//#define DEBUG_STATE
//#define DEBUG_BB
//#define DEBUG_FUNC


void DeleteNonConstantVar(Variable *var)
{
	if (var->isConstant)
	{
		return;
	}
	else
	{
		delete var;
		return;
	}
}

Range::Range()
	:lower(-1), upper(1)
{

}

Range::Range(float lb, float ub, RangeType rtype)
	: lower(lb), upper(ub), type(rtype)
{
	if (lb > ub)
	{
		type = Empty;
	}
}

bool Range::checkValid() const
{
	if (type != Regular)
		return true;
	else
	{
		if (!MaxRangeLower && !MaxRangeUpper)
		{
			if (lower > upper)
			{
				return false;
			}
		}
		return true;
	}
}


Range Range::add(const Range&other) const
{
	if (isUnknown() || other.isUnknown())
	{
		return Range(-1, 1, Unknown);
	}
	float a = this->getLower();
	float b = this->getUpper();
	float c = other.getLower();
	float d = other.getUpper();
	return Range(a + c, b + d);
}

Range Range::sub(const Range&other) const
{
	if (isUnknown() || other.isUnknown())
	{
		return Range(-1, 1, Unknown);
	}
	float a = this->getLower();
	float b = this->getUpper();
	float c = other.getLower();
	float d = other.getUpper();
	return Range(a - d, b - c);
}

Range Range::mul(const Range&other) const
{
	if (isUnknown() || other.isUnknown())
	{
		return Range(-1, 1, Unknown);
	}
	if (isMaxRange() || other.isMaxRange())
	{
		Range temp(-1, 1);
		temp.setMaxRange(true, true);
		return temp;
	}
	float a = this->getLower();
	float b = this->getUpper();
	float c = other.getLower();
	float d = other.getUpper();
	float res[4];
	res[0] = a * c;
	res[1] = a * d;
	res[2] = b * c;
	res[3] = b * d;
	float *min = &res[0];
	float *max = &res[0];
	for (int i = 0; i < 4; i++)
	{
		if (res[i] > *max)
		{
			max = &res[i];
		}
		else if (res[i] < *min)
		{
			min = &res[i];
		}
	}
	return Range(*min, *max);
}

Range Range::div(const Range&other) const
{
	if (isUnknown() || other.isUnknown())
	{
		return Range(-1, 1, Unknown);
	}
	float a = this->getLower();
	float b = this->getUpper();
	float c = other.getLower();
	float d = other.getUpper();
	// Divide 0 will produce Max Range
	if (c <= 0 && d >= 0)
	{
		Range temp(-1, 1);
		temp.setMaxRange(true, true);
		return temp;
	}
	float res[4];
	res[0] = a / c;
	res[1] = a / d;
	res[2] = b / c;
	res[3] = b / d;
	float *min = &res[0];
	float *max = &res[0];
	for (int i = 0; i < 4; i++)
	{
		if (res[i] > *max)
		{
			max = &res[i];
		}
		else if (res[i] < *min)
		{
			min = &res[i];
		}
	}
	return Range(*min, *max);
	Range temp;
	return temp;
}

Range Range::intersectWith(const Range&other) const
{
	if (isEmpty() || other.isEmpty())
	{
		return Range(-1, 1, Empty);
	}
	if (isUnknown())
	{
		return other;
	}
	if (other.isUnknown())
	{
		return *this;
	}
	float l = getLower() > other.getLower() ? getLower() : other.getLower();
	float u = getUpper() < other.getUpper() ? getUpper() : other.getUpper();
	return Range(l, u);
}

Range Range::unionWith(const Range&other) const
{
	if (isEmpty())
	{
		return other;
	}
	if (other.isEmpty())
	{
		return *this;
	}
	if (isUnknown())
	{
		return other;
	}
	if (other.isUnknown())
	{
		return *this;
	}
	float l = getLower() < other.getLower() ? getLower() : other.getLower();
	float u = getUpper() > other.getUpper() ? getUpper() : other.getUpper();
	return Range(l, u);
}

void Range::Print() const
{
	if (this->isUnknown())
	{
		std::cout << "Unknown";
		return;
	}
	if (this->isEmpty())
	{
		std::cout << "Empty";
		return;
	}
	if (this->MaxRangeLower)
	{
		std::cout << "[-inf, ";
	}
	else {
		std::cout << "[" << getLower() << ", ";
	}
	if (this->MaxRangeUpper) {
		std::cout << "+inf]";
	}
	else
	{
		std::cout << getUpper() << "]";
	}
}

std::string Range::getString() const
{
	std::string ret;
	if (this->isUnknown())
	{
		ret = "Unknown";
		return ret;
	}
	if (this->isEmpty())
	{
		ret = "Empty";
		return ret;
	}
	if (this->notEqual)
	{
		ret.append("[ !=");
		ret.append(std::to_string(lower));
		ret.append(" ]");
		return ret;
	}
	if (this->MaxRangeLower)
	{
		ret.append("[-inf, ");
	}
	else {
		ret.append("[");
		ret.append(std::to_string(getLower()));
		ret.append(", ");
	}
	if (this->MaxRangeUpper) {
		ret.append("+inf]");
		return ret;
	}
	else
	{
		ret.append(std::to_string(getUpper()));
		ret.append("]");
		return ret;
	}
}

BasicInterval::BasicInterval()
{

}

std::string BasicInterval::getString()
{
	return range.getString();
}

void BasicInterval::Print()
{
	std::cout << getString();
}

SymbolInterval::SymbolInterval()
{
	bound = NULL;
}

SymbolInterval::~SymbolInterval() = default;

std::string SymbolInterval::getString()
{
	std::string ret;
	if (compareOpe == "<")
	{
		ret.append("(-inf, ft(");
		ret.append(bound->name);
		ret.append(") )");
	}
	if (compareOpe == "<=")
	{
		ret.append("(-inf, ft(");
		ret.append(bound->name);
		ret.append(") ]");
	}
	if (compareOpe == ">")
	{
		ret.append("( ft(");
		ret.append(bound->name);
		ret.append(") ,inf)");
	}
	if (compareOpe == ">=")
	{
		ret.append("[ ft(");
		ret.append(bound->name);
		ret.append(") ,inf)");
	}
	if (compareOpe == "!=")
	{
		ret.append("[ != ft(");
		ret.append(bound->name);
		ret.append(") ]");
	}
	if (compareOpe == "==")
	{
		ret.append("[ == ft(");
		ret.append(bound->name);
		ret.append(") ]");
	}
	return ret;
}

void SymbolInterval::Print()
{
	std::cout << getString();
}

Variable::Variable()
	:type(Float_type), name("Unknown Variable")
{
#if (defined DEBUG) || (defined DEBUG_VAR)
	std::cout << "Variable " << "Unknown" << "created !" << std::endl;
#endif
}

Variable::Variable(std::string name)
	:name(name), type(Float_type)
{
#if (defined DEBUG) || (defined DEBUG_VAR)
	std::cout << "Variable " << name << "created !" << std::endl;
#endif
}

// parse info from definition : "int a" or "float b"
void Variable::ParseDef(std::string varDefString)
{
	if (varDefString.find("int", 0) != std::string::npos)
	{
		this->type = Int_type;
		this->name = varDefString.substr(varDefString.find("int", 0) + 4);
	}
	else if (varDefString.find("float", 0) != std::string::npos)
	{
		this->type = Float_type;
		this->name = varDefString.substr(varDefString.find("float", 0) + 6);
	}

}

// parse  info from useing : "a" ,"1.0"
void Variable::ParseUse(std::string useString)
{
	std::stringstream ss(useString);
	float val;
	if (ss >> val)
	{
		this->name = useString;
		this->isConstant = true;
		constantVal = val;
	}
	else
	{
		// remove parenthesis in "i_2(D)"
		if (useString.find("(", 0) != std::string::npos)
		{
			this->name = useString.substr(0, useString.find("(", 0) - 0);
			if (useString.find("(D)", 0) != std::string::npos)
				this->isInput = true;
		}
		else
		{
			this->name = useString;
		}
	}
}

std::string Variable::GetTypeString()
{
	if (this->type == Int_type)
		return "int";
	else
		return "float";
}

std::string Variable::getString()
{
	if (isConstant)
	{
		switch (type)
		{
		case Int_type:return std::to_string(int(constantVal));
		case Float_type:return std::to_string(constantVal);
		default:return "Invalid type";
		}
	}
	else
	{
		return name;
	}
}


Statement::Statement(int lines, BasicBlock*parent)
	:lineNumber(lines), parentBlock(parent)
{

}

Statement::Statement(OperationType ope, Variable*res, Variable*param1, BasicInterval* interval)
{
	operation = ope;
	params.push_back(param1);
	intersect = interval;
	result = res;
}

std::string Statement::removeD(std::string input)
{
	if (input.find("(D)", 0) != std::string::npos)
	{
		while (input.find("(D)", 0) != std::string::npos)
		{
			input.replace(input.find("(D)", 0), 3, "");
		}
		return input;
	}
	else
	{
		return input;
	}
}

bool Statement::Parse(std::string statementString)
{
	//statementString = removeD(statementString);
	Variable* var;
	// "# i_2 = PHI <i_5(3), i_7(4)>"
	if (statementString.find("PHI", 0) != std::string::npos)
	{
		this->operation = PHI_OPE;
		var = new Variable(statementString.substr(statementString.find("#", 0) + 2, statementString.find("=", 0) - statementString.find("#", 0) - 3));
		this->result = GetLocalVariable(var);
		delete var;

		std::string source1 = statementString.substr(statementString.find("<", 0) + 1, statementString.find(",", 0) - statementString.find("<", 0) - 1);
		std::string source2 = statementString.substr(statementString.find(",", 0) + 2, statementString.find(">", 0) - statementString.find(",", 0) - 2);
		var = new Variable();
		var->ParseUse(source1);
		this->params.push_back(GetLocalVariable(var));
		DeleteNonConstantVar(var);

		var = new Variable();
		var->ParseUse(source2);
		this->params.push_back(GetLocalVariable(var));
		DeleteNonConstantVar(var);

		return true;
	}
	// "goto <bb 7>;" "goto <bb 5> (<L2>)"
	if (statementString.find("goto", 0) != std::string::npos)
	{
		this->operation = GOTO_OPE;
		std::string nextBlockName = statementString.substr(statementString.find("<", 0) + 1,
			statementString.find(">", 0) - statementString.find("<") - 1);
		this->trueNextBlockName = this->falseNextBlockName = nextBlockName;
		//case for "goto <bb 5> (<L2>);"
		if (statementString.find("(") != std::string::npos)
		{
			nextBlockName = statementString.substr(statementString.find("(", 0) + 2,
				statementString.find(")", 0) - statementString.find("(", 0) - 3);
			this->trueNextBlockName = this->falseNextBlockName = nextBlockName;
		}
		return true;
	}
	// "return _10;" or "return;"
	if (statementString.find("return", 0) != std::string::npos)
	{
		this->operation = RETURN_OPE;
		// set next block to be exit
		SetDefaultNextBlockName("exit");
		// this judges whether return has a value to return
		if (statementString.find("return", 0) + 6 == statementString.find(";", 0))
		{
			return true;
		}
		var = new Variable();
		// pop tail ";"
		statementString.pop_back();
		var->ParseUse(statementString.substr(statementString.find("return", 0) + 7, std::string::npos));
		this->params.push_back(GetLocalVariable(var));
		DeleteNonConstantVar(var);

		return true;
	}
	//  "_9 = (int) ret_2;"
	if (statementString.find("(int)", 0) != std::string::npos)
	{
		var = new Variable(statementString.substr(2, statementString.find("=", 0) - 3));
		this->result = GetLocalVariable(var);
		DeleteNonConstantVar(var);
		this->operation = INT_TRANSFORM_OPE;

		var = new Variable();
		var->ParseUse(statementString.substr(statementString.find("(int)", 0) + 6, statementString.find(";", 0) - statementString.find("(int)", 0) - 6));
		params.push_back(GetLocalVariable(var));
		DeleteNonConstantVar(var);

		return true;
	}
	//  "j_6 = (float) _5;"
	if (statementString.find("(float)", 0) != std::string::npos)
	{
		var = new Variable(statementString.substr(2, statementString.find("=", 0) - 3));
		this->result = GetLocalVariable(var);
		DeleteNonConstantVar(var);
		this->operation = FLOAT_TRANSFORM_OPE;

		var = new Variable();
		var->ParseUse(statementString.substr(statementString.find("(float)", 0) + 8, statementString.find(";", 0) - statementString.find("(float)", 0) - 8));
		params.push_back(GetLocalVariable(var));
		DeleteNonConstantVar(var);

		return true;
	}
	// parse function call statements like : "bar (0, k_1);" or "j_4 = bar (i_2(D));"
	if (statementString.find(" (", 0) != std::string::npos)
	{
		this->operation = CALL_OPE;
		// ret = func(i,j)
		if (statementString.find("=", 0) != std::string::npos)
		{
			var = new Variable(statementString.substr(2, statementString.find("=", 0) - 3));
			this->result = GetLocalVariable(var);
			DeleteNonConstantVar(var);

			this->functionCalled = statementString.substr(statementString.find("=", 0) + 2,
				statementString.find(" (", 0) - statementString.find("=", 0) - 2);
		}
		// func(i, j)
		else
		{
			this->result = NULL;
			this->functionCalled = statementString.substr(2, statementString.find(" (", 0) - 2);
		}
		int paramStartPos = statementString.find(" (", 0) + 2;
		int paramEndPos = statementString.find(",", paramStartPos);
		// only one parameter, end with ");"
		if (paramEndPos == std::string::npos)
		{
			paramEndPos = statementString.find(";", 0) - 1;
		}
		while (paramEndPos != std::string::npos && (paramStartPos < paramEndPos))
		{
			std::string oneParamString = statementString.substr(paramStartPos, paramEndPos - paramStartPos);
			var = new Variable();
			var->ParseUse(oneParamString);
			this->params.push_back(GetLocalVariable(var));
			DeleteNonConstantVar(var);

			paramStartPos = paramEndPos + 2;
			paramEndPos = statementString.find(",", paramStartPos);
			if (paramEndPos == std::string::npos)
			{
				paramEndPos = statementString.find(";", paramStartPos) - 1;
			}
		}
		return true;
	}
	// "_3 = i_2(D) + 10;"
	if (statementString.find("=", 0) != std::string::npos)
	{
		var = new Variable(statementString.substr(2, statementString.find("=", 0) - 3));
		this->result = GetLocalVariable(var);
		DeleteNonConstantVar(var);

		// statement like " k_4 = 0;"
		std::string source = statementString.substr(statementString.find("=", 0) + 2, std::string::npos);
		//remove the symbol ";"
		source.pop_back();
		var = new Variable();
		var->ParseUse(source);
		// means there is only one right value
		if (source.find(" ", 0) == std::string::npos)
		{
			this->operation = EQUAL_OPE;
			this->params.push_back(GetLocalVariable(var));
			DeleteNonConstantVar(var);
			return true;
		}
		delete var;
	}
	if (statementString.find("*", 0) != std::string::npos)
	{
		this->operation = MULTI_OPE;
		std::string source1 = statementString.substr(statementString.find("=", 0) + 2, statementString.find("*", 0) - statementString.find("=", 0) - 3);
		std::string source2 = statementString.substr(statementString.find("*", 0) + 2, statementString.find(";", 0) - statementString.find("*", 0) - 2);

		var = new Variable();
		var->ParseUse(source1);
		this->params.push_back(GetLocalVariable(var));
		DeleteNonConstantVar(var);

		var = new Variable();
		var->ParseUse(source2);
		this->params.push_back(GetLocalVariable(var));
		DeleteNonConstantVar(var);
		return true;
	}
	else if (statementString.find("/", 0) != std::string::npos)
	{
		this->operation = DIV_OPE;
		std::string source1 = statementString.substr(statementString.find("=", 0) + 2, statementString.find("/", 0) - statementString.find("=", 0) - 3);
		std::string source2 = statementString.substr(statementString.find("/", 0) + 2, statementString.find(";", 0) - statementString.find("/", 0));

		var = new Variable();
		var->ParseUse(source1);
		this->params.push_back(GetLocalVariable(var));
		DeleteNonConstantVar(var);

		var = new Variable();
		var->ParseUse(source2);
		this->params.push_back(GetLocalVariable(var));
		DeleteNonConstantVar(var);
		return true;
	}
	else if (statementString.find("+", 0) != std::string::npos)
	{
		this->operation = PLUS_OPE;
		std::string source1 = statementString.substr(statementString.find("=", 0) + 2, statementString.find("+", 0) - statementString.find("=", 0) - 3);
		std::string source2 = statementString.substr(statementString.find("+", 0) + 2, statementString.find(";", 0) - statementString.find("+", 0) - 2);

		var = new Variable();
		var->ParseUse(source1);
		this->params.push_back(GetLocalVariable(var));
		DeleteNonConstantVar(var);

		var = new Variable();
		var->ParseUse(source2);
		this->params.push_back(GetLocalVariable(var));
		DeleteNonConstantVar(var);

		return true;
	}
	else if (statementString.find("-", 0) != std::string::npos)
	{
		this->operation = MINUS_OPE;
		std::string source1 = statementString.substr(statementString.find("=", 0) + 2, statementString.find("-", 0) - statementString.find("=", 0) - 3);
		std::string source2 = statementString.substr(statementString.find("-", 0) + 2, statementString.find(";", 0) - statementString.find("-", 0) - 2);

		var = new Variable();
		var->ParseUse(source1);
		this->params.push_back(GetLocalVariable(var));
		DeleteNonConstantVar(var);

		var = new Variable();
		var->ParseUse(source2);
		this->params.push_back(GetLocalVariable(var));
		DeleteNonConstantVar(var);

		return true;
	}
	if (statementString.length() == 0)
	{
		return false;
	}
#if (defined DEBUG) || (defined DEBUG_STATE)
	if (isValidStatement)
	{
		std::cout << "Statement[ " << this->lineNumber << "]" <<
			(this->result ? this->result->name : "None") << " = " <<
			this->operation << " ( ";
		for (Variable *it : this->params)
		{
			std::cout << it->name << " ";
		}
		std::cout << ")" << std::endl;
	}
#endif
	return false;
}

/* parse branch statement like:
"
if (k_1 < N_5(D))
	goto <bb 3>;
else
	goto <bb 5>;
"
*/
void Statement::ParseBranch(std::vector<std::string>& ifelseString)
{
	this->trueNextBlockName = ifelseString[1].substr(ifelseString[1].find("<", 0) + 1,
		ifelseString[1].find(">", 0) - ifelseString[1].find("<", 0) - 1);
	this->falseNextBlockName = ifelseString[3].substr(ifelseString[1].find("<", 0) + 1,
		ifelseString[3].find(">", 0) - ifelseString[1].find("<", 0) - 1);
	std::string comparisonExp = ifelseString[0].substr(ifelseString[0].find("(", 0) + 1,
		ifelseString[0].length() - 7);
	std::string source1 = comparisonExp.substr(0, comparisonExp.find(" ", 0) - 0);

	comparisonExp = comparisonExp.substr(comparisonExp.find(" ", 0) + 1, std::string::npos);
	this->operation = COMP_OPE;
	this->compareOpe = comparisonExp.substr(0, comparisonExp.find(" ", 0) - 0);

	comparisonExp = comparisonExp.substr(comparisonExp.find(" ", 0) + 1, std::string::npos);
	std::string source2 = comparisonExp;
	Variable *var;
	var = new Variable();
	var->ParseUse(source1);
	this->params.push_back(GetLocalVariable(var));
	DeleteNonConstantVar(var);

	var = new Variable();
	var->ParseUse(source2);
	this->params.push_back(GetLocalVariable(var));
	DeleteNonConstantVar(var);
#if (defined DEBUG) || (defined DEBUG_STATE)
	std::cout << "if( " << params[0]->name << " " << this->operation << " " << params[1]->name << " ) goto " <<
		trueNextBlockName << " else goto " << falseNextBlockName << std::endl;
#endif
}

void Statement::Print()
{
	//std::cout <<"["<<std::setw(4)<< lineNumber <<"]"<<"<"<<parentBlock->blockName<<">"<<"    ";
	std::cout << "    ";
	switch (this->operation)
	{
	case PLUS_OPE:std::cout << result->getString() << " = " << params[0]->getString() << " + " << params[1]->getString() << std::endl; break;
	case MINUS_OPE:std::cout << result->getString() << " = " << params[0]->getString() << " - " << params[1]->getString() << std::endl; break;
	case MULTI_OPE:std::cout << result->getString() << " = " << params[0]->getString() << " * " << params[1]->getString() << std::endl; break;
	case DIV_OPE:std::cout << result->getString() << " = " << params[0]->getString() << " / " << params[1]->getString() << std::endl; break;
	case EQUAL_OPE:std::cout << result->getString() << " = " << params[0]->getString() << std::endl; break;
	case CALL_OPE:std::cout << ((result != NULL) ? result->getString() + " = " : "") << functionCalled << " (";
		for (Variable*var : params) {
			std::cout << var->getString() << (var != *params.rbegin() ? ", " : "");
		}
		std::cout << ")" << std::endl;
		break;
	case COMP_OPE:std::cout << "if (" << params[0]->getString() << " " << compareOpe << " " << params[1]->getString() << ")" << std::endl;
		std::cout << "      goto <" << trueNextBlockName << ">" << std::endl;
		std::cout << "    else" << std::endl;
		std::cout << "      goto <" << falseNextBlockName << ">" << std::endl; break;
	case PHI_OPE:std::cout << result->getString() << " = " << "PHI <" << params[0]->getString() << ", " << params[1]->getString() << ">" << std::endl; break;
	case GOTO_OPE:std::cout << "goto <" << trueNextBlockName << ">" << std::endl; break;
	case RETURN_OPE:std::cout << "return " << (params.size() == 1 ? params[0]->getString() : "") << std::endl; break;
	case INT_TRANSFORM_OPE:std::cout << result->getString() << " = (int) " << params[0]->getString() << std::endl; break;
	case FLOAT_TRANSFORM_OPE:std::cout << result->getString() << " = (float) " << params[0]->getString() << std::endl; break;
	case INTERSECT_OPE:std::cout << result->getString() << " = " << params[0]->getString() <<
		" Inersect " << intersect->getString() << std::endl; break;
	default:std::cout << "Error Print Statement at " << lineNumber << std::endl;
	}
}

void Statement::SetDefaultNextBlockName(std::string blockName)
{
	trueNextBlockName = falseNextBlockName = blockName;
}

Variable* Statement::GetLocalVariable(Variable *var)
{
	// constant value, just return itself,else return the local variable
	if (var->isConstant)
	{
		return var;
	}
	Variable *ret = parentBlock->parentFunction->GetLocalVariable(var->name);
	if (var->isInput)
	{
		ret->isInput = true;
	}
	return ret;
}

bool Statement::JudgeCompare()
{
	if (compareOpe == "<")
	{
		return params[0]->getValue() < params[1]->getValue();
	}
	else if (compareOpe == "<=")
	{
		return params[0]->getValue() <= params[1]->getValue();
	}
	else if (compareOpe == ">")
	{
		return params[0]->getValue() > params[1]->getValue();
	}
	else if (compareOpe == ">=")
	{
		return params[0]->getValue() >= params[1]->getValue();
	}
	else if (compareOpe == "!=")
	{
		return params[0]->getValue() != params[1]->getValue();
	}
	else if (compareOpe == "==")
	{
		return params[0]->getValue() == params[1]->getValue();
	}
	else
		return false;
}

void Statement::Simulate()
{
	std::vector<float> param;
	switch (operation)
	{
	case PLUS_OPE:
		result->setValue(params[0]->getValue() + params[1]->getValue());break;
	case MINUS_OPE:result->setValue(params[0]->getValue() - params[1]->getValue()); break;
	case MULTI_OPE:result->setValue(params[0]->getValue() * params[1]->getValue()); break;
	case DIV_OPE:result->setValue(params[0]->getValue() / params[1]->getValue()); break;
	case EQUAL_OPE:result->setValue(params[0]->getValue()); break;
	case CALL_OPE:
		for (Variable *var : params)
		{
			param.push_back(var->getValue());
		}
		if (result != NULL)
		{
			result->setValue(
				parentBlock->parentFunction->parent->functions[functionCalled]->Simulate(param).getValue());
		}
		break;
	case COMP_OPE:
		if (JudgeCompare())
			parentBlock->simulationNextBlock = trueNextBlockName;
		else
			parentBlock->simulationNextBlock = falseNextBlockName;
		break;
	case PHI_OPE:
		if (params[0]->defTime > params[1]->defTime)
			result->setValue(params[0]->getValue());
		else
			result->setValue(params[1]->getValue());
		break;
	case GOTO_OPE:
		parentBlock->simulationNextBlock=trueNextBlockName; 
		break;
	case RETURN_OPE:
		parentBlock->simulationNextBlock = "exit";
		break;
	case INT_TRANSFORM_OPE:result->setValue(params[0]->getValue()); break;
	case FLOAT_TRANSFORM_OPE:result->setValue(params[0]->getValue()); break;
	case INTERSECT_OPE:; break;
	default:std::cout << "Error Print Statement at " << lineNumber << std::endl;
	}
	if (this->result != NULL)
	{
		//std::cout << result->getString() << " : " << result->getValue() << std::endl;
	}
}


//// evaluate the result range of this statement operation
//Range Statement::eval()
//{
//	switch (this->operation)
//	{
//	case PLUS_OPE:return params[0]->range.add(params[1]->range); break;
//	case MINUS_OPE:return params[0]->range.sub(params[1]->range); break;
//	case MULTI_OPE:return params[0]->range.mul(params[1]->range); break;
//	case DIV_OPE:return params[0]->range.div(params[1]->range); break;
//	case EQUAL_OPE:return params[0]->range; break;
//	case PHI_OPE:return params[0]->range.unionWith(params[1]->range); break;
//	case INTERSECT_OPE:return params[0]->range.intersectWith(params[1]->range); break;
//	default:return Range(); break;
//	}
//}

BasicBlock::BasicBlock(Function* parent)
	:parentFunction(parent), nextMethod(DefaultNext)
{
}

BasicBlock::BasicBlock(std::string name, Function*parent)
	: blockName(name), nextMethod(DefaultNext), parentFunction(parent)
{

}

// Parse block definition string like: <bb 1>:
void BasicBlock::ParseDef(std::string blockDefString)
{
	this->blockName = blockDefString.substr(blockDefString.find("<", 0) + 1, blockDefString.find(">", 0) - blockDefString.find("<", 0) - 1);
}

bool BasicBlock::checkIsDef(std::string unknownString)
{
	if ((unknownString.find("<", 0) != std::string::npos) &&
		(unknownString.find(">:", 0) != std::string::npos))
	{
		return true;
	}
	return false;
}

bool BasicBlock::checkIsLabelDef(std::string unknownString)
{
	if ((unknownString.find("<L", 0) != std::string::npos) &&
		(unknownString.find(">:", 0) != std::string::npos))
	{
		return true;
	}
	return false;
}

void BasicBlock::addStatement(Statement *state)
{
	this->statements.push_back(state);
	lastStatement = state;
}

void BasicBlock::addStatementBefore(Statement *state)
{
	statements.push_front(state);
}

// connect basic block according to the last statement
void BasicBlock::finishBlock()
{
#if defined(DEBUG) || defined(DEBUG_BB)
	std::cout << "Block : " << this->blockName << " Finished !" << std::endl;
#endif
	nextMethod = DefaultNext;
	if (lastStatement != NULL)
	{
		if (lastStatement->operation == COMP_OPE)
		{
			nextMethod = BranchNext;
			this->trueNextBlockName = lastStatement->trueNextBlockName;
			this->falseNextBlockName = lastStatement->falseNextBlockName;
		}
		else if (lastStatement->operation == GOTO_OPE)
		{
			nextMethod = GotoNext;
			defaultNextBlockName = lastStatement->trueNextBlockName;
			if (lastStatement->trueNextBlockName != lastStatement->falseNextBlockName)
			{
				std::cout << "Finish block <" << blockName << "> error !" << std::endl;
				exit(1);
			}
		}
		else if (lastStatement->operation == RETURN_OPE)
		{
			this->defaultNextBlockName = lastStatement->trueNextBlockName;
		}
	}
}

int BasicBlock::ReplaceVarUse(Variable *oldVar, Variable *newVar)
{
	int changedVarCount = 0;
	for (Statement *state : statements)
	{
		if (state->params.size() == 2)
		{
			if (state->params[0]->name == oldVar->name)
			{
				state->params[0] = newVar;
				changedVarCount++;
			}
			if (state->params[1]->name == oldVar->name)
			{
				state->params[1] = newVar;
				changedVarCount++;
			}
		}
		else if (state->params.size() == 1)
		{
			if (state->params[0]->name == oldVar->name)
			{
				state->params[0] = newVar;
				changedVarCount++;
			}
		}
	}
	return changedVarCount;
}

// generate interval for comparison ,(true interval, false interval)
std::vector<BasicInterval*> BasicBlock::generateInterval(std::string compareOpe, Variable *param)
{
	BasicInterval *intervalTrue, *intervalFalse;
	// "< 0"
	if (param->isConstant)
	{
		intervalTrue = new BasicInterval();
		intervalFalse = new BasicInterval();
		float constantVal = param->getValue();
		if (compareOpe == "<")
		{
			// True branch
			intervalTrue->range.setUpper(constantVal);
			intervalTrue->range.MaxRangeUpper = false;
			intervalTrue->range.MaxRangeLower = true;

			// False branch
			intervalFalse->range.setLower(constantVal);
			intervalFalse->range.MaxRangeLower = false;
			intervalFalse->range.MaxRangeUpper = true;
		}
		else if (compareOpe == "<=")
		{
			// True branch
			intervalTrue->range.setUpper(constantVal);
			intervalTrue->range.MaxRangeLower = true;
			intervalTrue->range.MaxRangeUpper = false;

			// False branch
			intervalFalse->range.setLower(constantVal);
			intervalFalse->range.MaxRangeLower = false;
			intervalFalse->range.MaxRangeUpper = true;
		}
		else if (compareOpe == ">")
		{
			// True branch
			intervalTrue->range.setLower(constantVal);
			intervalTrue->range.MaxRangeUpper = true;
			intervalTrue->range.MaxRangeLower = false;

			// False branch
			intervalFalse->range.setUpper(constantVal);
			intervalFalse->range.MaxRangeLower = true;
			intervalFalse->range.MaxRangeUpper = false;
		}
		else if (compareOpe == ">=")
		{
			// True branch
			intervalTrue->range.setLower(constantVal);
			intervalTrue->range.MaxRangeLower = false;
			intervalTrue->range.MaxRangeUpper = true;

			// False branch
			intervalFalse->range.setUpper(constantVal);
			intervalFalse->range.MaxRangeLower = true;
			intervalFalse->range.MaxRangeUpper = false;
		}
		else if (compareOpe == "!=")
		{
			// True branch
			intervalTrue->range.setLower(constantVal);
			intervalTrue->range.setUpper(constantVal);
			intervalTrue->range.notEqual = true;
			intervalTrue->range.MaxRangeLower = false;
			intervalTrue->range.MaxRangeUpper = false;

			// False branch
			intervalFalse->range.setLower(constantVal);
			intervalFalse->range.setUpper(constantVal);
			intervalFalse->range.MaxRangeLower = false;
			intervalFalse->range.MaxRangeUpper = false;
		}
		else if (compareOpe == "==")
		{
			// True branch
			intervalTrue->range.setLower(constantVal);
			intervalTrue->range.setUpper(constantVal);
			intervalTrue->range.MaxRangeLower = false;
			intervalTrue->range.MaxRangeUpper = false;

			// False branch
			intervalFalse->range.setLower(constantVal);
			intervalFalse->range.setUpper(constantVal);
			intervalFalse->range.notEqual = true;
			intervalFalse->range.MaxRangeLower = false;
			intervalFalse->range.MaxRangeUpper = false;
		}
	}
	// "< i_1"
	else
	{
		intervalTrue = new SymbolInterval();
		intervalFalse = new SymbolInterval();
		intervalTrue->setCompareOpe(compareOpe);
		intervalFalse->setCompareOpe(reverseCompareOpe(compareOpe));
		intervalTrue->setBound(param);
		intervalFalse->setBound(param);
	}
	std::vector<BasicInterval*> ret;
	ret.push_back(intervalTrue);
	ret.push_back(intervalFalse);
	return ret;
}

void BasicBlock::Print()
{
	std::cout << "  <" << this->blockName << ">:" << std::endl;
	for (Statement *st : this->statements)
	{
		st->Print();
	}
	if (nextMethod == DefaultNext)
		std::cout << "  ----->  <" << defaultNextBlockName << ">" << std::endl;
}

void BasicBlock::Simulate()
{
	//std::cout << this->blockName <<std::endl;
	for (Statement *state : statements)
	{
		state->Simulate();
	}
}

BasicBlock* BasicBlock::GetNextBlock()
{
	if (simulationNextBlock != "")
	{
		return parentFunction->bbs[simulationNextBlock];
	}
	else
	{
		return parentFunction->bbs[defaultNextBlockName];
	}
}

Function::Function()
{
#if defined(DEBUG) || defined(DEBUG_FUNC)
	std::cout << "Function " << "Unknown" << " created !" << std::endl;
#endif
	bbs["entry"] = new BasicBlock("entry", this);
	bbs["exit"] = new BasicBlock("exit", this);
}

Function::Function(std::string functionname)
	:functionName(functionname)
{
#if defined(DEBUG) || defined(DEBUG_FUNC)
	std::cout << "Function " << functionname << " created !" << std::endl;
#endif
	bbs["entry"] = new BasicBlock("entry", this);
	bbs["exit"] = new BasicBlock("exit", this);
}

// parse parameter like : foo(int i, int j)
void Function::ParseParams(std::string paramString)
{
	std::string oneParamString;
	Variable *var;
	int firstParamStartPos = paramString.find("(", 0) + 1;
	int paramEndPos = paramString.find(",", firstParamStartPos);
	if (paramEndPos == std::string::npos)
	{
		paramEndPos = paramString.find(")", firstParamStartPos);
	}
	while (paramEndPos != std::string::npos && (firstParamStartPos < paramEndPos))
	{
		oneParamString = paramString.substr(firstParamStartPos, paramEndPos - firstParamStartPos);
		var = new Variable();
		var->ParseDef(oneParamString);
		DeclareParameter(var);

		firstParamStartPos = paramEndPos + 2;
		paramEndPos = paramString.find(",", firstParamStartPos);
		if (paramEndPos == std::string::npos)
		{
			paramEndPos = paramString.find(")", firstParamStartPos);
		}
	}
}

void Function::DeclareLocalVar(Variable *var)
{
#if defined(DEBUG) || defined(DEBUG_FUNC)
	std::cout << var->type << " " << var->name << ";" << std::endl;
#endif
	localVarsMap[var->name] = var;
}

void Function::DeclareParameter(Variable *param)
{
	params.push_back(param);
	//	First fill it with function definition parameters
	inputParamNames.push_back(param->name);
	localVarsMap[param->name] = param;
}

// get local variable pointer
// if not find then add one Variable and return it
Variable* Function::GetLocalVariable(std::string varName)
{
	if (localVarsMap.find(varName) == localVarsMap.end())
	{
		Variable *var = new Variable(varName);
		localVarsMap[varName] = var;
		// resolute the type of the variable by removing the underscore
		std::string varNameWithoutUnderscore = varName.substr(0,
			varName.find("_", 0) - 0);
		std::map<std::string, Variable*>::iterator it = localVarsMap.find(varNameWithoutUnderscore);
		if (it != localVarsMap.end())
		{
			var->setType((*it).second->GetType());
		}
	}
	return localVarsMap[varName];
}

void Function::Print()
{
	std::cout << this->functionName << "(";
	for (Variable *var : this->params)
	{
		std::cout << var->GetTypeString() << " " << var->name << (var != *params.rbegin() ? ", " : "");
	}
	std::cout << "):" << std::endl;
	for (std::map<std::string, Variable*>::iterator it = localVarsMap.begin(); it != localVarsMap.end(); it++)
	{
		std::cout << "  " << it->second->GetTypeString() << " " << it->second->name << std::endl;
	}
	for (std::map<std::string, BasicBlock*>::iterator it = this->bbs.begin(); it != this->bbs.end(); it++)
	{
		it->second->Print();
	}
}

void Function::PrintVars()
{

	std::map<std::string, Variable*>::iterator it;
	for (it = localVarsMap.begin(); it != localVarsMap.end(); it++)
	{
		std::cout << it->first << " : " << it->second->getValue() << std::endl;
	}
}

void Function::resetVarDefTime()
{
	std::map<std::string, Variable*>::iterator it;
	for (it = localVarsMap.begin(); it != localVarsMap.end(); it++)
	{
		it->second->defTime = 0;
	}
}

Variable Function::Simulate(std::vector<float> inputParams)
{
	findInputParams();
	BasicBlock * currentBlock;
	int index;
	resetVarDefTime();
	index = 0;
	if (inputParams.size() != inputParamNames.size())
	{
		std::cout << "Error , parameter number isn't same "<<std::endl;
	}
	for (std::string input : inputParamNames)
	{
		localVarsMap[input]->setValue(inputParams[index]);
		index++;
	}
	currentBlock = bbs["entry"];
	while (currentBlock != bbs["exit"])
	{
		totalTime++;
		currentBlock->Simulate();
		currentBlock = currentBlock->GetNextBlock();
		if (currentBlock->defaultNextBlockName == "exit" &&
			this->returnVar == NULL)
		{
			this->returnVar = currentBlock->lastStatement->params[0];
		}
	}
	//PrintVars();
	Variable ret(returnVar->name);
	ret.setValue(localVarsMap[returnVar->getString()]->getValue());
	return ret;
}


void Function::findInputParams()
{
	std::map<std::string, Variable*>::iterator it;
	for (it = localVarsMap.begin(); it != localVarsMap.end(); it++)
	{
		if (it->second->isInput)
		{
			std::string varNameWithoutUnderscore = it->first.substr(0,
				it->first.find("_", 0) - 0);
			for (int i = 0; i < inputParamNames.size(); i++)
			{
				if (inputParamNames[i] == varNameWithoutUnderscore)
				{
					inputParamNames[i] = it->first;
				}
			}
		}
	}
}

void Function::convertVariable(Variable *convert, BasicBlock*block, std::string ope, Variable*bound)
{
	std::string varTrueName = convert->name + "_t";
	std::string varFalseName = convert->name + "_f";
	Variable *varTrue = GetLocalVariable(varTrueName);
	Variable *varFalse = GetLocalVariable(varFalseName);
	Variable *oldVar = convert;

	std::vector<BasicInterval*> intervals = BasicBlock::generateInterval(ope,
		bound);

	std::set<std::string> dominatingBlocks = findDom(block->trueNextBlockName);
	for (std::string domblock : dominatingBlocks)
	{
		bbs[domblock]->ReplaceVarUse(oldVar, varTrue);
	}
	bbs[block->trueNextBlockName]->addStatementBefore(
		new Statement(INTERSECT_OPE, varTrue, oldVar, intervals[0]));
	dominatingBlocks = findDom(block->falseNextBlockName);
	for (std::string domblock : dominatingBlocks)
	{
		bbs[domblock]->ReplaceVarUse(oldVar, varFalse);
	}
	bbs[block->falseNextBlockName]->addStatementBefore(
		new Statement(INTERSECT_OPE, varFalse, oldVar, intervals[1]));
}

void Function::convertToeSSA()
{
	BasicBlock *currentBlock;
	std::string varTrueName, varFalseName;
	Variable *varTrue, *varFalse, *oldVar;
	std::vector<BasicInterval*> intervals;
	std::string compareOpe;
	std::map<std::string, BasicBlock*>::iterator it;
	for (it = bbs.begin(); it != bbs.end(); it++)
	{
		currentBlock = it->second;
		if (currentBlock->nextMethod == BranchNext)
		{
			// i_1 < j_1
			if (!currentBlock->lastStatement->params[0]->isConstant &&
				!currentBlock->lastStatement->params[1]->isConstant)
			{
				convertVariable(currentBlock->lastStatement->params[0], currentBlock,
					currentBlock->lastStatement->compareOpe, currentBlock->lastStatement->params[1]);
				convertVariable(currentBlock->lastStatement->params[1], currentBlock,
					reverseCompareOpe(currentBlock->lastStatement->compareOpe), currentBlock->lastStatement->params[0]);
			}
			// i_1 < 0
			else
				if (!currentBlock->lastStatement->params[0]->isConstant &&
					currentBlock->lastStatement->params[1]->isConstant)
				{
					convertVariable(currentBlock->lastStatement->params[0], currentBlock,
						currentBlock->lastStatement->compareOpe, currentBlock->lastStatement->params[1]);
				}
		}
	}
}

// use fix point to caculate the dominate node
void Function::calculateDominate()
{
	// N constains all the basic block names
	std::set<std::string> N;
	std::map<std::string, BasicBlock*>::iterator it;
	std::map < std::string, std::set<std::string>> predNames;
	// construct the pred basic block connections
	for (it = bbs.begin(); it != bbs.end(); it++)
	{
		N.insert(it->first);
		// Branch block, add two pred edges
		if (it->second->nextMethod == BranchNext)
		{
			predNames[it->second->trueNextBlockName].insert(it->first);
			predNames[it->second->falseNextBlockName].insert(it->first);
		}
		// else only one pred edge
		else
		{
			predNames[it->second->defaultNextBlockName].insert(it->first);
		}
	}
	// Initialize the data flow algorithm
	OUT["entry"] = std::set<std::string>();
	OUT["entry"].insert("entry");
	for (std::string bb : N)
	{
		if (bb != "entry")
		{
			OUT[bb] = N;
		}
	}
	// start use dataflow algorithm
	bool changed = true;
	std::set<std::string> ret;
	while (changed)
	{
		changed = false;
		for (std::string basicBlockName : N)
		{
			if (basicBlockName != "entry")
			{
				ret = N;
				for (std::string predName : predNames[basicBlockName])
				{
					std::set<std::string> temp;
					temp = OUT[predName];
					ret = IntersectSet(ret, temp);
				}
				if (!checkEqual(ret, IN[basicBlockName]))
					changed = true;
				IN[basicBlockName] = ret;
				std::set<std::string> self;
				self.insert(basicBlockName);
				OUT[basicBlockName] = UnionSet(ret, self);
			}
		}
	}
	calculationFinished = true;
}

// 
std::set<std::string> Function::findDom(std::string start)
{
	if (!calculationFinished)
	{
		std::cout << "Error !, Dominate Calculation Not Finished !" << std::endl;
	}
	std::set<std::string> ret;
	std::map<std::string, std::set<std::string>>::iterator it;
	for (it = OUT.begin(); it != OUT.end(); it++)
	{
		// skip exit node for it has none statement
		if (it->first != "exit")
		{
			// if start is in the dominate node of it->first
			// add it to ret
			if (it->second.find(start) != it->second.end())
			{
				ret.insert(it->first);
			}
		}
	}
	return ret;
}

std::set<std::string> Function::UnionSet(std::set<std::string> &a, std::set<std::string> &b)
{
	std::set<std::string> ret(a);
	for (std::string str : b)
	{
		ret.insert(str);
	}
	return ret;
}

std::set<std::string> Function::IntersectSet(std::set<std::string> &a, std::set<std::string> &b)
{
	std::set<std::string> ret;
	for (std::string item : a)
	{
		//if item is just in a set, we erase it
		if ((b).find(item) != (b).end())
		{
			ret.insert(item);
		}
	}
	return ret;
}

bool Function::checkEqual(std::set<std::string> &a, std::set<std::string>&b)
{
	if (a.size() != b.size())
		return false;
	for (std::string str : a)
	{
		if (b.find(str) == b.end())
			return false;
	}
	return true;
}


void Function::PrintDominate()
{
	std::cout << "-----Function : " << this->functionName << " Dominate Start------" << std::endl;
	std::map<std::string, std::set<std::string>>::iterator it;
	for (it = OUT.begin(); it != OUT.end(); it++)
	{
		std::cout << "D[" << it->first << "]:";
		for (std::string str : OUT[it->first])
		{
			std::cout << " " << str;
		}
		std::cout << std::endl;
	}
	std::cout << "-----Function : " << this->functionName << " Dominate End------" << std::endl;
}

SSAGraph::SSAGraph()
{

}

bool SSAGraph::readFromFile(std::string filename)
{
	std::ifstream file(filename);
	if (!file.is_open())
	{
		std::cout << "File Input Error, Cannot Open!" << std::endl;
		return false;
	}
	char *line = new char[MAX_LINE_LENGTH];
	BasicBlock *lastBlock = NULL;
	BasicBlock *currentBlock = NULL;
	Function *currentFunction = NULL;
	Statement *currentStatement = NULL;
	std::string lineString;
	std::deque<std::string> functionNames;
	bool inFunction = true;
	bool inBlock = false;
	bool inEntry = true;
	int lineNum = 0;
	while (file.getline(line, MAX_LINE_LENGTH))
	{
		//std::cout << std::setw(5) << lineNum++ << ": " << line << std::endl;
		lineString = std::string(line);
		lineNum++;
		if (lineString.find(";;", 0) != std::string::npos)
		{
			int nameStartPos = lineString.find("(", 0) + 1;
			int nameEndPos = lineString.find(",", 0);
			functionNames.push_back(lineString.substr(nameStartPos, nameEndPos - nameStartPos));
			//std::cout << "Function Name Get: [" << functionNames.front() << "], length:" << functionNames.front().length() << std::endl;

			// skip the empty line after "Function" line
			file.getline(line, MAX_LINE_LENGTH);
			lineNum++;
			inFunction = false;
			functions[functionNames.back()] = new Function(functionNames.back());
			currentFunction = functions[functionNames.back()];
			currentFunction->parent = this;
			lastBlock = functions[functionNames.back()]->bbs["entry"];
			currentBlock = lastBlock;
			continue;
		}
		if (!inFunction) {
			// This means the paramter definition line
			if (lineString.find(functionNames.back(), 0) != std::string::npos)
			{
				inFunction = true;
				inEntry = true;
				functions[functionNames.back()]->ParseParams(lineString);
				// skip the "{" symbol
				file.getline(line, MAX_LINE_LENGTH);
				lineNum++;
				continue;
			}
		}
		else {
			// exit current function parse procedure
			if (lineString.find("}", 0) != std::string::npos)
			{
				inFunction = false;
				currentBlock->finishBlock();
				continue;
			}
			//if it is a if-else statement, we get 4 line total and process them together
			if (lineString.find("if", 0) != std::string::npos)
			{
				std::vector<std::string> ifelseString;
				ifelseString.push_back(lineString);
				int ct = 3;
				while (ct > 0)
				{
					file.getline(line, MAX_LINE_LENGTH);
					ifelseString.push_back(std::string(line));
					ct--;
				}
				lineNum += 3;
				currentStatement = new Statement(lineNum - 3, currentBlock);
				currentStatement->ParseBranch(ifelseString);
				currentBlock->addStatement(currentStatement);
				continue;
			}
			// check if it is define a block; "<bb 2>:" or "<L6>:"
			if (BasicBlock::checkIsDef(lineString))
			{
				inEntry = false;
				lastBlock = currentBlock;
				currentBlock = new BasicBlock(currentFunction);
				currentBlock->ParseDef(lineString);
				lastBlock->defaultNextBlockName = currentBlock->blockName;
				lastBlock->finishBlock();
				currentFunction->bbs[currentBlock->blockName] = currentBlock;
			}
			// otherwise all consider statements in the block
			else
			{
				if (inEntry && !lineString.empty())
				{
					Variable *var = new Variable();
					var->ParseDef(lineString.substr(0, lineString.length() - 1));
					currentFunction->DeclareLocalVar(var);
				}
				currentStatement = new Statement(lineNum, currentBlock);
				if (currentStatement->Parse(lineString))
				{
					currentBlock->addStatement(currentStatement);
				}
				else
				{
					delete currentStatement;
				}
			}
		}
	}
	file.close();
	delete line;
}

void SSAGraph::Print()
{
	for (std::map<std::string, Function*>::iterator it = functions.begin(); it != functions.end(); it++)
	{
		it->second->Print();
	}
}

void SSAGraph::PrintDominate()
{
	std::map<std::string, Function*>::iterator it;
	for (it = functions.begin(); it != functions.end(); it++)
	{
		it->second->calculateDominate();
		it->second->PrintDominate();
	}
}

void SSAGraph::convertToeSSA()
{
	for (std::map<std::string, Function*>::iterator it = functions.begin(); it != functions.end(); it++)
	{
		it->second->convertToeSSA();
	}
}

void SSAGraph::SimulateSolution()
{
	std::map<std::string, Function*>::iterator it;
	for (it = functions.begin(); it != functions.end(); it++)
	{
		it->second->findInputParams();
	}
	std::vector<float> fooInputStart;
	std::vector<float> fooInputEnd;
	std::vector<float> temp;
	float val;
	for (std::string str : functions["foo"]->inputParamNames)
	{
		std::cout << "Range for " << str << std::endl;
		std::cout << "Min: " << std::endl;
		std::cin >> val;
		fooInputStart.push_back(val);
		std::cout << "Max: " << std::endl;
		std::cin >> val;
		fooInputEnd.push_back(val);
		temp.push_back(val);
	}
	int index = 0;
	for (int i = -10; i <= 10; i++)
	{
		temp.clear();
		temp.push_back(i);
		std::cout<<(functions["foo"]->Simulate(temp)).getValue()<<std::endl;
	}
}

VarNode::VarNode(Variable *V)
	:V(V), interval(Range(-1, 1, Unknown)), abstractState(0) {}

void VarNode::init(bool outside)
{
	Variable *V = getVariable();
	if (V->isConstant)
	{
		float value = V->getValue();
		this, setRange(Range(value, value));
	}
	else
	{
		if (!outside)
		{
			this->setRange(Range(-1, 1, Unknown));
		}
		else
		{
			this->getRange().setMaxRange(true, true);
		}
	}
}

void VarNode::Print()
{
	std::cout << this->getVariable()->getString();
}

void VarNode::storeAbstractState()
{
	if (this->interval.isUnknown())
	{
		std::cout << "Error !, store abstract state doesn't handle empty set";
		return;
	}
	if (interval.MaxRangeLower)
	{
		if (interval.MaxRangeUpper)
		{
			abstractState = '?';
		}
		else
		{
			abstractState = '-';
		}
	}
	else if (interval.MaxRangeUpper)
	{
		abstractState = '+';
	}
	else
	{
		abstractState = '0';
	}
}

BasicOp::BasicOp(BasicInterval *_intersect, VarNode *_sink, Statement*_state)
	:intersect(_intersect), sink(_sink), state(_state) {}

BasicOp::~BasicOp()
{
	if (intersect != NULL)
		delete intersect;
}

void BasicOp::fixIntersects(VarNode *V)
{

}