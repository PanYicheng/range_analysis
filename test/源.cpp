
#include<iostream>
#include<cstdlib>
#include<memory>
#include<sstream>
using namespace std;
int main()
{
	string input;
	char str[100];
	cin >> input;
	stringstream ss(input);
	float val;
	ss >> val;
	cout <<"Read Value: "<< val << endl;
	cout << int((unsigned int(-1) >> 1)) << endl;
	system("pause");
	return 0;
}