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
class VarNode;


static int totalTime = 0;

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
	~Range() = default;
	Range(const Range &other) = default;
	Range(Range &&) = default;
	Range &operator=(const Range &other) = default;
	Range &operator=(Range &&) = default;
	Range(float lb, float ub, RangeType rtype = Regular);

	
	
	void setLower(float val) { lower = val; };
	void setUpper(float val) { upper = val; };
	float getLower() const { return lower; };
	float getUpper() const { return upper; };
	bool isUnknown() const { return type == Unknown; };
	bool isEmpty() const { return type == Empty; };
	bool isMaxRange() const { return MaxRangeLower && MaxRangeUpper; };
	void setMaxRange(bool low, bool upper) { MaxRangeLower = low; MaxRangeUpper = upper; };
	void setType(RangeType rtype) { type = rtype; };
	bool checkValid() const;



	Range add(const Range &other) const;
	Range sub(const Range &other) const;
	Range mul(const Range &other) const;
	Range div(const Range &other) const;
	Range intersectWith(const Range &other) const;
	Range unionWith(const Range &other) const;

	void Print() const;
	std::string getString() const;
};

enum IntervalId { BasicIntervalId, SymbIntervalId };

class BasicInterval {
public:
	Range range;

	BasicInterval();
	virtual ~BasicInterval() = default;
	BasicInterval(const BasicInterval &) = delete;
	BasicInterval(BasicInterval &&) = delete;
	BasicInterval &operator=(const BasicInterval &) = delete;
	BasicInterval &operator=(BasicInterval &&) = delete;

	// Methods for RTTI
	virtual IntervalId getValueId() const { return BasicIntervalId; }
	static bool classof(BasicInterval const * /*unused*/) { return true; }
	/// Returns the range of this interval.
	const Range &getRange() const { return this->range; }
	void setRange(const Range &newRange) {
		this->range = newRange;
		if (!range.checkValid())
		{
			range.setType(Empty);
		}
	}

	// inplement as virtual function to enable Polymorphisn
	virtual std::string getString();
	virtual void setCompareOpe(std::string) {};
	virtual void setBound(Variable* bound) {};
	virtual void Print() ;
};

class SymbolInterval : public BasicInterval{
public:
	const Variable * bound;
	std::string compareOpe;

	SymbolInterval();
	~SymbolInterval() ;
	SymbolInterval(const SymbolInterval &) = delete;
	SymbolInterval(SymbolInterval &&) = delete;
	SymbolInterval &operator=(const SymbolInterval &) = delete;
	SymbolInterval &operator=(SymbolInterval &&) = delete;
	
	// Polymorphisn functions
	std::string getString();
	void setCompareOpe(std::string ope) { compareOpe = ope; };
	void setBound(Variable *_bound) { bound = _bound; };

	// Methods for RTTI
	IntervalId getValueId() const override { return SymbIntervalId; }
	static bool classof(SymbolInterval const * /*unused*/) { return true; }
	static bool classof(BasicInterval const *BI) {
		return BI->getValueId() == SymbIntervalId;
	}
	/// Returns the opcode of the operation that create this interval.
	std::string getOperation() const { return this->compareOpe; }
	/// Returns the node which is the bound of this interval.
	const Variable *getBound() const { return this->bound; }
	/// Replace symbolic intervals with hard-wired constants.
	Range fixIntersects(VarNode *bound, VarNode *sink);
	/// Prints the content of the interval.
	void Print() ;
};


enum VarType { Int_type, Float_type};

class Variable
{
public:
	std::string name;
	VarType type;
	bool isConstant{ false };
	float constantVal = 0;

	Variable();
	Variable(std::string name);
	
	std::string GetTypeString();
	VarType GetType() { return this->type; }
	void setType(VarType _type) { this->type = _type; };
	float getValue() { return constantVal; };
	void setValue(float val) { constantVal = val; defTime = totalTime; };
	std::string getString();

	void ParseDef(std::string variableDefString);
	void ParseUse(std::string useString);
	 
	// simulation data
	int defTime = 0;
};

enum OperationType {
	PLUS_OPE, MINUS_OPE, MULTI_OPE, DIV_OPE, CALL_OPE, EQUAL_OPE, COMP_OPE, PHI_OPE,
    GOTO_OPE, RETURN_OPE, INT_TRANSFORM_OPE, FLOAT_TRANSFORM_OPE, INTERSECT_OPE,
	UNION_OPE};

class Statement
{
public:
	BasicBlock * parentBlock{NULL};

	BasicInterval *intersect;
	std::deque<Variable *> params;
	OperationType operation;
	std::string compareOpe;
	std::string functionCalled;
	Variable * result = NULL;

	std::string trueNextBlockName;
	std::string falseNextBlockName;
	int lineNumber{ -1 };

	Statement(int lines, BasicBlock* parent);
	Statement(OperationType, Variable*, Variable*, BasicInterval *);

	void Print();
	bool Parse(std::string statementString);
	void ParseBranch(std::vector<std::string>& ifelseString);
	void SetDefaultNextBlockName(std::string blockName);
	std::string removeD(std::string input);


	Variable* GetLocalVariable(Variable* var);
	//Range eval();

	void Simulate();
	bool JudgeCompare();
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
	static std::vector<BasicInterval*> generateInterval(std::string, Variable*);

	void ParseDef(std::string blockDefString);
	static bool checkIsDef(std::string unknownString);
	static bool checkIsLabelDef(std::string unknownString);
	void Print();

	void Simulate();
	std::string simulationNextBlock;
	int simulationStep = 0;
	BasicBlock *GetNextBlock();
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

	void Simulate();
	void PrintVars();
	void resetVarDefTime();
	void StoreReturnValue();
	Variable *returnVar = NULL;
	std::vector<float> resultPossibleValues;
	

	void convertToeSSA();
	void convertVariable(Variable *convert, BasicBlock*block, std::string ope, Variable*bound);
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

// Following is the classed used for range-analysis

class VarNode
{
public:
	Variable *V;
	Range interval;
	char abstractState;

	explicit VarNode(Variable *V);
	~VarNode() = default;
	void init(bool outside);
	Range& getRange() { return interval; };
	Variable* getVariable() { return V; };
	void setRange(const Range &newInterval)
	{
		this->interval = newInterval;
		// check if the new interval is valid
		if (!interval.checkValid()) {
			interval.setType(Empty);
		}
	}
	void Print() ;
	char getAbstractState() { return abstractState; };
	void storeAbstractState();
};

class BasicOp
{
public:
	BasicInterval * intersect;
	VarNode *sink;
	// the statement that originated this op node
	Statement *state;

	enum class OperationId
	{
		UnaryOpId,
		SigmaOpId,
		BinaryOpId,
		TernaryOpId,
		PhiOpId,
		ControlDepId
	};
	/// The dtor. It's virtual because this is a base class.
	virtual ~BasicOp();
	// We do not want people creating objects of this class.
	BasicOp(const BasicOp &) = delete;
	BasicOp(BasicOp &&) = delete;
	BasicOp &operator=(const BasicOp &) = delete;
	BasicOp &operator=(BasicOp &&) = delete;
	// Methods for RTTI
	virtual OperationId getValueId() const = 0;
	static bool classof(BasicOp const * /*unused*/) { return true; }
	/// Given the input of the operation and the operation that will be
	/// performed, evaluates the result of the operation.
	virtual Range eval() const = 0;
	/// Return the instruction that originated this op node
	const Statement *getInstruction() const { return state; }
	/// Replace symbolic intervals with hard-wired constants.
	void fixIntersects(VarNode *V);
	/// Returns the range of the operation.
	BasicInterval *getIntersect() const { return intersect; }
	/// Changes the interval of the operation.
	void setIntersect(const Range &newIntersect) {
		this->intersect->setRange(newIntersect);
	}
	/// Returns the target of the operation, that is,
	/// where the result will be stored.
	VarNode *getSink() { return sink; }
	/// Prints the content of the operation.
	virtual void Print() const = 0;
protected:
	// we don't want to create objects of this class, just inherit from it
	BasicOp(BasicInterval *intersect, VarNode* sink, Statement *state);
};
#endif