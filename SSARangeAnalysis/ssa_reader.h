#pragma once
#ifndef SSA_READER_H
#define SSA_READER_H

#include<string>
#include<vector>
#include<deque>
#include<map>
#include<set>

class Variable;
class BasicBlock;
class Function;


enum BoundType {InitialBound, ReachableBound,UnreacheableBound,FutureBound, InfiniteBound};
enum RangeType {Unknown,Regular, Empty};

class Range
{
public:
	float lower, upper;
	// Flags for "!=" compare range
	bool notEqual{ false };
	bool MaxRangeLower{ false };
	bool MaxRangeUpper{ false };
	RangeType type{Regular};

	Range();
	Range(float lb, float ub, RangeType rtype = Regular);

	
	
	void setLower(float val) { lower = val; };
	void setUpper(float val) { upper = val; };
	float getLower() const { return lower; };
	float getUpper() const { return upper; };
	bool isUnknown() const { return type == Unknown; };
	bool isEmpty() const { return type == Empty; };
	bool isMaxRange() const { return MaxRangeLower && MaxRangeUpper; };
	void setMaxRange(bool low, bool upper) { MaxRangeLower = low; MaxRangeUpper = upper; };

	Range add(const Range &other) const;
	Range sub(const Range &other) const;
	Range mul(const Range &other) const;
	Range div(const Range &other) const;
	Range intersectWith(const Range &other) const;
	Range unionWith(const Range &other) const;

	void Print() const;
};



enum VarType { Int_type, Float_type};

class Variable
{
public:
	std::string name;
	Range range;
	VarType type;
	bool isConstant{ false };
	Variable();
	Variable(std::string name, Range range = Range(0, 0));
	
	std::string GetTypeString();
	VarType GetType() { return this->type; }
	void setType(VarType _type) { this->type = _type; };
	void ParseDef(std::string variableDefString);
	void ParseUse(std::string useString);
};

enum OperationType {
	PLUS_OPE, MINUS_OPE, MULTI_OPE, DIV_OPE, CALL_OPE, EQUAL_OPE, COMP_OPE, PHI_OPE,
    GOTO_OPE, RETURN_OPE, INT_TRANSFORM_OPE, FLOAT_TRANSFORM_OPE, INTERSECT_OPE,
	UNION_OPE};

class Statement
{
public:
	BasicBlock * parentBlock{NULL};
	std::deque<Variable *> params;
	OperationType operation;
	std::string compareOpe;
	std::string functionCalled;
	Variable * result = NULL;
	std::string trueNextBlockName;
	std::string falseNextBlockName;
	int lineNumber{ -1 };
	Statement(int lines, BasicBlock* parent);
	Statement(OperationType ope, Variable*res, Variable*param1, Variable*param2);

	void Print();
	bool Parse(std::string statementString);
	void ParseBranch(std::vector<std::string>& ifelseString);
	void SetDefaultNextBlockName(std::string blockName);

	Variable* GetLocalVariable(Variable* var);
	Range eval();
};

enum BBNextMethod {DefaultNext, BranchNext, GotoNext};

class BasicBlock
{
public:
	Function * parentFunction{NULL};

	std::string blockName;
	std::deque<Statement*> statements;
	std::string trueNextBlockName;
	std::string falseNextBlockName;
	std::string defaultNextBlockName;
	BBNextMethod nextMethod;
	Statement *lastStatement = NULL;
	BasicBlock(Function* parent);
	BasicBlock(std::string name, Function* parent);
	void addStatement(Statement *state);
	void addStatementBefore(Statement *state);
	void finishBlock();

	int ReplaceVarUse(Variable* oldVar, Variable* newVar);

	void ParseDef(std::string blockDefString);
	static bool checkIsDef(std::string unknownString);
	static bool checkIsLabelDef(std::string unknownString);
	void Print();
};

class Function
{
public:
	std::string functionName;
	std::map<std::string, BasicBlock*> bbs;
	std::vector<Variable *> params;
	std::map<std::string, Variable*> localVarsMap;

	//Dataflow Data of Dominate point
	bool calculationFinished = false; // flags marking whether finish dominate calculation
	std::map<std::string, std::set<std::string>> IN;
	std::map<std::string, std::set<std::string>> OUT;
	void calculateDominate();
	std::set<std::string> findDom(std::string start);
	void PrintDominate();
	std::set<std::string> UnionSet(std::set<std::string> &, std::set<std::string> &);
	std::set<std::string> IntersectSet(std::set<std::string> &,std::set<std::string>&);
	bool checkEqual(std::set<std::string> &a, std::set<std::string>&b);


	Function();
	Function(std::string functionname);



	Variable *GetLocalVariable(std::string varName);
	void DeclareParameter(Variable *param);
	void DeclareLocalVar(Variable *var);
	void ParseParams(std::string paramString);
	void Print();



	void convertToeSSA();
};

class SSAGraph
{
public:
	Function * mainFunction = NULL;
	std::map<std::string, Function*> functions;
	SSAGraph();
	bool readFromFile(std::string filename);

	void PrintDominate();
	void Print();
	void convertToeSSA();
};

#endif