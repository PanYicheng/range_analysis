#include<iostream>
#include<ssa_reader.h>

int main(int argc, char **argv)
{
	SSAGraph ssagraph;
	if (!ssagraph.readFromFile("benchmark/t1.ssa"))
	{
		return 1;
	}
	ssagraph.Print();
	ssagraph.PrintDominate();
	ssagraph.convertToeSSA();
	std::cout <<std::endl<< "After converting to eSSA" << std::endl;
	ssagraph.Print();
	system("pause");
	return 0;
}