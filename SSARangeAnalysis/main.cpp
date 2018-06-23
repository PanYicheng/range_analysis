#include<iostream>
#include<ssa_reader.h>

int main(int argc, char **argv)
{
	SSAGraph ssagraph;
	if (!ssagraph.readFromFile("benchmark/t2.ssa"))
	{
		return 1;
	}
	ssagraph.Print();
	ssagraph.PrintDominate();
	/*ssagraph.convertToeSSA();
	std::cout <<std::endl<< "After converting to eSSA" << std::endl;
	ssagraph.Print();*/

	ssagraph.functions["foo"]->Simulate();
	system("pause");
	return 0;
}