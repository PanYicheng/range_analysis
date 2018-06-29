#include<iostream>
#include<ssa_reader.h>

int main(int argc, char **argv)
{
	SSAGraph ssagraph;
	std::string fileName;
	std::cout << "Please input file name:" << std::endl;
	std::cin >> fileName;
	std::cout <<std::endl<< "Reading File: " << fileName << std::endl;
	if(!ssagraph.readFromFile(fileName))
	{
		std::cout << "File read failed ! Try again !"<<std::endl;
		return 1;
	}
	ssagraph.Print();
	//ssagraph.PrintDominate();
	/*ssagraph.convertToeSSA();
	std::cout <<std::endl<< "After converting to eSSA" << std::endl;
	ssagraph.Print();*/

	ssagraph.SimulateSolution();
	system("pause");
	return 0;
}