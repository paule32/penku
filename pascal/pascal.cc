// --------------------------------------------------------------------------------
// MIT License
//
// Copyright (c) 2021 Jens Kallup
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
// --------------------------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <ctype.h>
#include <unistd.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <istream>
#include <typeinfo> 		// operator typeid
#include <exception>		// std::exception
#include <vector>
#include <map>
#include <list> 			// linked list
#include <algorithm>
#include <iterator>

using namespace std;		// std namespace

std::fstream yyin ;		 	// input stream file
std::fstream yyout;         // output

int yylineno = 1;			// current line number

bool debug_on = true;		// debug messages: default = false;

// ------------------------------------------------
// exception handling class for syntax error's ...
// ------------------------------------------------
class yyerr_exception: public std::exception
{
public:
	yyerr_exception(string s) { this->s = s; }
	virtual ~yyerr_exception()       throw() { }
	virtual const char* what() const throw() {
		return s.c_str();
	}
private:
	string s;
};

// ---------------------------------------
// C function for global use error's ...
// ---------------------------------------
int yyerror(string const &p) {
	stringstream str;
	str << "error:"
		<< yylineno
		<< ":"
		<< p
		<< std::endl;
    throw yyerr_exception(str.str());
}
extern "C" int yyinfo(string const &p) {
	stringstream str;
	std::cout
	<< "info:"
	<< yylineno
	<< ":"
	<< p
	<< std::endl;
	throw yyerr_exception(str.str());
}

struct cnv_toupper { void operator()(char &c) { c = toupper((unsigned char) c); } };
struct cnv_tolower { void operator()(char &c) { c = tolower((unsigned char) c); } };

std::string UpperCase(std::string s) { for_each(s.begin(), s.end(), cnv_toupper()); return s; }
std::string LowerCase(std::string s) { for_each(s.begin(), s.end(), cnv_tolower()); return s; }

// ---------------------------------------
// forward declarations
// ---------------------------------------
std::string check_ident(char);
bool        check_line ();

// --------------------------------------------
// internal structures / variable's ...
// --------------------------------------------
struct compiler_directive {
	std::string name;
	bool        exec;
	bool operator == (compiler_directive rhs) const noexcept {
		return (name == rhs.name);
	}
};
std::list<compiler_directive> directive;

int commentA_level	 = 0;	// { }
int commentB_level 	 = 0;	// (* *)

int level_define	 = 0;	// {$define name}
int level_undef 	 = 0;	// {$undef  name}
int level_ifdef 	 = 0;	// {$ifdef}
int level_else		 = 0;	// {$else}
int level_endif 	 = 0;	// {$endif}

bool check_char      = false;
bool check_comment   = false;

stringstream ss_code;		// code output stream
streampos    yyfile_size = 0;

// for multiple use ...
const string ERR_COMMENT			= "comment error.";
const string ERR_COMMENT_UNBALANCED = "comment unbalanced.";

// ------------------------------------------------------
// compiler comment directives ...
// ------------------------------------------------------
void scan_4_bracket()
{
	char c;
	while (1) {
		yyin.get(c);
		weiter:
		if (c == ' ' || c == '\t' || c == '\r')
			continue;
		else if (c == '\n') {
			++yylineno;
			continue;
		}
		else if (c == '{') {
			++commentA_level;
			while (1) {
				yyin.get(c);
				if (c == ' ' || c == '\t' || c == '\r')
					continue;
				else if (c == '\n') {
					++yylineno;
					continue;
				}
				else if (c == '}') {
					--commentA_level;
					if (commentA_level < 0)
						yyerror(ERR_COMMENT_UNBALANCED);
					break;
				}
				else if (c == EOF) {
					yyerror(ERR_COMMENT_UNBALANCED);
				}
				else {
					goto weiter;
				}
			}
		}
		else if (c == '}') {
			--commentA_level;
			if (commentA_level < 0)
				yyerror(ERR_COMMENT_UNBALANCED);
			break;
		}
		else if (c == EOF) {
			yyerror(ERR_COMMENT);
		}
	}
}

string scan_directive()
{
	stringstream id;
	char c;
	check_char = false;
	while (1) {
		yyin.get(c);
		weiter:
		if (c == ' ' || c == '\t' || c == '\r') {
			if (check_char)
				break;
			continue;
		}
		else if (c == '\n') {
			++yylineno;
			if (check_char)
				break;
			continue;
		}
		else if (c == '{') {
			yyin.putback(c);
			scan_4_bracket();
			break;
		}
		else if (c == EOF) {
			yyerror(ERR_COMMENT);
		}
		else {
			if (((c >= 'a') && (c <= 'z'))
			||  ((c >= 'A') && (c <= 'Z'))
			||  ((c >= '0') && (c <= '9'))
			||   (c == '_')
			||   (c == '.')
			||   (c == ':')
			||   (c == '\'')
			||   (c == '\\')) {
				check_char = true;
				id << static_cast<char>(c);
				continue;
			}
			else if (c == ' ' || c == '\t' || c == '\r') {
				if (check_char)
					break;
				continue;
			}
			else if (c == '\n') {
				++yylineno;
				if (check_char)
					break;
				continue;
			}
			else if (c == EOF) {
				yyerror(ERR_COMMENT);
			}
			else {
				scan_4_bracket();
				break;
			}
		}
	}
	
	if (commentA_level < 0)
		yyerror(ERR_COMMENT_UNBALANCED);
		
	return id.str();
}

void func_else () { scan_4_bracket(); }
void func_endif() { scan_4_bracket(); }

void func_apptype()
{
	string t = LowerCase(scan_directive());
	if (t == "console") { cout << "console application" << endl; return; } else
	if (t == "gui"    ) { cout << "graphical app"       << endl; return; }
	
	yyerror("unknown apptype");
}
void func_define()
{
	string t = LowerCase(scan_directive());
	cout << "define token: " << t << endl;
}
void func_ifdef()
{
	string t = LowerCase(scan_directive());
	cout << "ifdef token: " << t << endl;
}

void func_input()
{
	string t = LowerCase(scan_directive());
	cout << "input token: " << t << endl;
}
void func_undef()
{
	string t = LowerCase(scan_directive());
	cout << "undef token: " << t << endl;
}

bool check_directive(string const &t)
{
	typedef void (*func_p)(void);
	struct dirc_struct {
		string name;
		func_p func;
	};
	vector<dirc_struct> dircs {
		{"apptype", &func_apptype},
		{"define" , &func_define },
		{"else"   , &func_else   },
		{"endif"  , &func_endif  },
		{"ifdef"  , &func_ifdef  },
		{"include", &func_input  },
		{"undef"  , &func_undef  }
	};
	for (auto &&it: dircs) {
		if (LowerCase(t) == it.name) {
			cout << "funcer: " << it.name << endl;
			it.func();
			return true;
		}
	}
	return false;
}

// ------------------------------------------------------
// get token ident - sub routine, to minimaaze code size
// ------------------------------------------------------
std::string check_ident(char ch)
{
	stringstream id;
	char c = ch;
	check_char = false;
	printf("------> %d, %c\n",c,c);

	weiter:
	if (c == ' ' || c == '\t' || c == '\r') {
		yyin.get(c);
		goto weiter;
	}
	else if (c == '\n') {
		++yylineno;
		yyin.get(c);
		goto weiter;
	}
	else if (c == '}') {
		printf("zuszu\n");
		yyin.get(c);
		goto weiter;
	}
	else if (c == '{') {
		++commentA_level;
		printf("klo\n");
		while (1) {
			yyin.get(c);
			if (c == ' ' || c == '\t' || c == '\r')
				continue;
			else if (c == '\n') {
				printf("newline:22: %d, %c\n",c,c);
				++yylineno;
				continue;
			}
			else if (c == '$') {
				printf("direct found.\n");
				id.str(string(""));
				check_char = false;
				while (1) {
					matz:
					yyin.get(c);
					if (((c >= 'a') && (c <= 'z'))
					||  ((c >= 'A') && (c <= 'Z'))
					||  ((c >= '0') && (c <= '9'))
					||   (c == '_')
					||   (c == '.')
					||   (c == ':')
					||   (c == '\\')) {
						check_char = true;
						id << static_cast<char>(c);
						continue;
					}
					else if (c == ' ' || c == '\t' || c == '\r') {
						if (check_char)
							goto checker;
						continue;
					}
					else if (c == '\n') {
						++yylineno;
						if (check_char)
							goto checker;
						continue;
					}
					else if (c == '}') {
						--commentA_level;
						if (check_char)
							goto checker;	
						continue;
					}
					else if (c == EOF) {
						yyerror("comment directive error.");
					}
				}
				checker:
				cout << "idstr: " << id.str() << endl;
				if (check_directive(id.str())) {
					cout << "found directive sign: " << id.str() << endl;
					while (1) {
						yyin.get(c);
						if (c == ' ' || c == '\t' || c == '\r')
							continue;
						else if (c == '\n') {
							++yylineno;
							continue;
						}
						else if (c == '}') {
							--commentA_level;
							printf("comment zu\n");
							//goto weiter;
							break;
						}
						else if (c == EOF) {
							yyerror("cOMMent error.");
						}
					}
				}
				cout << "weiter: " << id.str() << endl;
				id.str(string(""));
				goto matz;
			}
			else if (c == EOF) {
				yyerror("comment eof.");
			}
		}
		yyin.get(c);
		goto weiter;
	}
	else if (c == '(') {
		loops:
		yyin.get(c);
		if (c == '*') {
			++commentB_level;
			while (1) {
				yyin.get(c);
				if (c == '*') {
					yyin.get(c);
					if (c == ')') {
						yyin.get(c);
						goto weiter;
					}
					else if (c == '\n') {
						++yylineno;
						continue;
					}
					else if (c == EOF) {
						yyerror("comment syntax error.");
					}
				}
				else if (c == ' ' || c == '\t' || c == '\r')
					continue;
				else if (c == '\n') {
					++yylineno;
					continue;
				}
				else if (c == EOF) {
					yyerror("Comment syntax error.");
				}
			}
		}
		else if (c == ' ' || c == '\t' || c == '\r')
			goto loops;
		else if (c == '\n') {
			++yylineno;
			goto loops;
		}
		else if (c == EOF) {
			yyerror("Commment error.");
		}
	}
	else if (c == '/') {
		printf("comm\n");
		yyin.get(c);
		if (c == '/') {
			printf("comm22\n");
			while (1) {
				yyin.get(c);
				if (c == ' ' || c == '\t' || c == '\r')
					continue;
				else if (c == '\n') {
					printf("newline:1\n");
					++yylineno;
					goto weiter;
				}
				else if (c == EOF) {
					return string("<eof>");
				}
			}
		}
		else if (c == ' ' || c == '\t' || c == '\r')
			goto weiter;
		else if (c == '\n') {
			++yylineno;
			goto weiter;
		}
		else if (c == EOF) {
			yyerror("comsst error");
		}
	}
	else {
		nex:
		if (((c >= 'a') && (c <= 'z'))
		||  ((c >= 'A') && (c <= 'Z'))
		||  ((c >= '0') && (c <= '9'))
		||   (c == '_')
		||   (c == '.')
		||   (c == ':')
		||   (c == '\\')) {
			if (c == '.') {
				if (id.str().size() > 2) {
					id << static_cast<char>(c);
					yyin.get(c);
					goto nex;
				}
				else {
					//yyin.putback(c);
					return id.str();
				}
			}
			if (check_char && c == '.') {
				//yyin.putback(c);
				return id.str();
			}
			check_char = true;
			id << static_cast<char>(c);
			goto nex;
		}
		else if (c == ' ' || c == '\t' || c == '\r')
			goto ende;
		else if (c == '\n') {
			++yylineno;
			goto ende;
		}
		else {
			if (c == '{' || c == '}'
			||  c == '/' || c == '(')
				goto weiter;
			else if (c == EOF) {
				return string("<eof>");
			}
		}
	}

	yyerror("UNknown character found.");
	ende:
	return id.str();
}

bool find_define(string const &name)
{
	bool found = false;
	for (auto it: directive) {
		if (it.name == name) {
			found = true;
			break;
		}
	}
	return found;
}

void push_define(string const &name)
{
	struct compiler_directive tmp;
	tmp.name = name;
	directive.push_front(tmp);
}

void del_define(string const &name)
{
	for (auto &&it: directive) {
		if (it.name == name) {
			directive.remove(it);
			break;
		}
	}
}

// ------------------------------------------------------
// main entry / start of program ...
// ------------------------------------------------------
int main(int argc, char **argv)
{
	stringstream ssA, id, ident;
	stringstream ssB;
	
	bool comment_end = false;
	int  result  = 0;
	char c;

	// token helpers
	#define TOK_PROGRAM 1
	#define TOK_LIBRARY 2
	#define TOK_UNIT    3
	#define TOK_BEGIN   4
	#define TOK_END     5
	
	int     TOK = 0;

	std::cout
	<< "Pascal 2 VM"
	<< std::endl
	<< "Copyright (c) 2021 Jens Kallup <kallup.jens@web.de"
	<< std::endl;
	
	if (argc < 2) {
		cout
		<< endl
		<< "Usage: pascal <infile> <outfile>"
		<< endl;
		return 1;
	}
	else if (argc < 3) {
		ssA << argv[1];
		ssB << argv[1] << ".out";
	}
	else {
		ssA << argv[1];
		ssB << argv[2];
	}
	
	yyin.open(ssA.str(), fstream::in);
	if (!yyin.is_open())
		yyerror("can not open input file!");
		
	yyout.open(ssB.str(), fstream::out | fstream::trunc);
	if (!yyout.is_open())
		yyerror("can not open write file!");

	// get file size
	yyfile_size = yyin.tellg(); yyin.seekg(0,ios::end);
	yyfile_size = yyin.tellg() - 
	yyfile_size;
	
	yyin.seekg(0,ios::beg);
	yylineno = 1;
	
	// parse code ...
	try {
		while (1) {
			id.str(string(""));
			yyin.get(c);			
			id << check_ident(c);
			string t = LowerCase(id.str());
			
			cout << "token: " << t << endl;
			
#if 0
			#define EXPECT(x)			 \
				if (!expect_char(x))	 \
				yyerror("syntax error 3");
			#define ID_MACRO(x,y,T)	 	 \
				if (id.str() == x)  { 	 \
				id.str(string("")); 	 \
				id << expect_ident(' '); \
				EXPECT(y); TOK = T; }
				
			if (!check_directive) {
				cout << "exece: '" << id.str() << "'" << endl;
				cout << "line:  "  << yylineno << endl;
				ID_MACRO("program",';', TOK_PROGRAM); 
				ID_MACRO("library",';', TOK_LIBRARY);
				ID_MACRO("unit"   ,';', TOK_UNIT   );
				
				if (TOK == TOK_PROGRAM)
				ID_MACRO("begin"  ,' ', TOK_BEGIN);
				ID_MACRO("end"    ,'.', TOK_END);
				
				if (TOK == TOK_END)
				break;
			}
#endif
		}
		
		ende:
		
		if (level_ifdef > 0) {
			//yyerror("$ifdef unbalanced.");
			printf("ifdde: %d\n",level_ifdef);
		}
		
		//if (commentA_level != 0) yyerror("unbalanced commentA.");
		//if (commentB_level != 0) yyerror("unbalanced commentB ");
		
		yyout
			<< ss_code.str()
			<< endl;
	}
	catch (yyerr_exception& err) {
		std::cout << err.what();
		result = 1;
	}
	catch (...) {
		std::cout << "unknow exception." <<
		std::endl ;
		result = 1;
	}
	
	yyin .close();
	yyout.close();
	
	if (result) {
		fprintf(stdout,"rs--> %s\n",ss_code.str().c_str());
		return result;
	}
	
	std::cout
	<< endl
	<< "Lines: " << yylineno << endl
	<< "done."   <<             endl ;
	
    return result;
}
