/*
 * parenthesis_checker.cpp
 * this code uses a stack to check a mathematical expression if the left and right parenthesis are matched.
 * Such algorithms are generally used in GUI based environments.
 *  Created on: 15-Apr-2018
 *     
 */

#include <iostream>
#include<string.h>

using namespace std;

class Stack {					//class to create a stack
private:
	int *p;
	int length;

public:
	int top;
	Stack(int = 0);
	~Stack();
	void push(int);		//to push an element
	int pop();			// to pop an element
	void display();		//to display the contenets of the stack
	int empty();		// to check is stack is empty or not
};

	Stack::Stack(int size) {
	top = -1;
	length = size;
	if (size == 0)
		p = 0;
	else
		p = new int[length];
}

	Stack::~Stack() {
	if (p != 0)
		delete[] p;
}

	void Stack::push(int elem) {
	if (p == 0) //If the stack size is zero, allow user to mention it at runtime
			{
		cout << "Stack of zero size" << endl;
		cout << "Enter a size for stack : ";
		cin >> length;
		p = new int[length];
	}
	if (top == (length - 1))     //If the top reaches to the maximum stack size
			{
		cout << "\nCannot push " << elem << ", Stack full" << endl;
		return;
	} else {
		top++;
		p[top] = elem;
	}
}
	int Stack::pop() {
	if (p == 0 || top == -1) {
		//cout << "Stack empty!";
		return -1;
	}
	int ret = p[top];
	top--;
	return ret;
}

	void Stack::display() {
	for (int i = 0; i <= top; i++)
		cout << p[i] << " ";
	cout << endl;
}

	int Stack::empty() {
	if (top == -1) {
		return (1);
	} else {
		return (0);
	}

}



/*
int main() {
	int x;
	string e;
	//cout<<"Enter the Maximum Size of Expression you want to check: ";
	//gets>>e;
	std::cout << "\n Enter the expression you want to check:\n";
	std::cin>>e;
	x=sizeof(e)/4;
	//std::cout<<x;
	Stack expression(x);
	Stack expression2(x);
	Stack expression3(x);
	for (int i = 0; i != '\o'; i++) {

		if (e[i] == '(') {
			expression.push(e[i]);
		} else if (e[i] == ')') {
			int t=expression.pop();
			if(t==-1){
				cout<<"Check Expression";
				return(0);
			}
		}
	}
	for (int i = 0; i != '\o'; i++) {

			if (e[i] == '[') {
				expression2.push(e[i]);
			} else if (e[i] == ']') {
				expression2.pop();
			}
		}
	for (int i = 0; i != '\o'; i++) {

				if (e[i] == '{') {
					expression3.push(e[i]);
				} else if (e[i] == '}') {
					expression3.pop();
				}
			}
	if (expression.empty()&&expression2.empty()&&expression3.empty()) {
		std::cout << "\nParenthesis Matched!";
	}
	if (expression.empty()&&expression2.empty()&&expression3.empty() == 0) {
		std::cout << "Check Expression";
	}
	return (0);

}
*/


	bool areParenthesisBalanced(char expr[])
	{	int size= strlen(expr);
	    Stack s(size);
	    char a, b, c;
	    int t,q,r;
	    // Traversing the Expression
	    for (int i=0; i<strlen(expr); i++)
	    {
	        if (expr[i]=='('||expr[i]=='['||expr[i]=='{')
	        {
	            // Push the element in the stack
	            s.push(expr[i]);
	        }
	        else
	        {
	            switch (expr[i])
	            {
	            case ')':

	                // Store the top element in a
	                a = s.top;
	               t= s.pop();
	                if (s.p[a]=='{'||s.p[a]=='['||t==-1){
	                    cout<<"Not Balanced\n";
	                    s.push(a);
	                }
	                break;

	            case '}':
	                // Store the top element in b
	                b = s.top;
	                r=s.pop();
	                if (s.p[b]=='('||s.p[b]=='['||r==-1){
	                    cout<<"Not Balanced\n";
	                    s.push(b);
	                }
	                break;
	            case ']':

	                // Store the top element in c
	                c=s.top;
	                q=s.pop();
	                if (s.p[c]=='('||s.p[c]=='{'||q==-1){
	                    cout<<"Not Balanced\n";
	                    s.push(c);
	                }
	                break;
	            }
	        }
	    }

	    // Check Empty Stack
	    if (s.empty())
	        return true;
	    else
	        return false;
	}

int main()
{
	char expr[50];
	cout<<"ENTER THE EXPRESSION YOU WANT TO CHECK: ";
	cin>>expr;


	if(areParenthesisBalanced(expr))
		cout<<"EXPRESSION BALANCED";
	else
		cout<<"CHECK EXPRESSION!";


}


