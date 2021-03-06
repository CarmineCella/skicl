// skicl.cpp
// 
// TODO: primitive, if, while, upval, eval, serializzazione, test
// speed, tail ricorsione, lazy eval, static scoping, numlinea, return, break, continue
#include <iostream>
#include <fstream>
#include <deque>
#include <sstream>
#include <map>
#include <stdexcept>

using namespace std;

// ast
typedef double Real;
typedef std::deque<std::string> Block;
struct Namespace;
typedef std::string (*Action) (Block& b, Namespace&);
struct Namespace {
    std::map<std::string, std::string> variables;
    std::map<std::string, Block> proceduers;
    std::map<std::string, Action> functors;
};

// lexing
std::string get_token (std::istream& input) {
	std::stringstream accum;
	while (!input.eof ()) {
		char c = input.get ();
		switch (c) {
			case '[': case ']': case '\n': 
 				if (accum.str ().size ()) {
					input.unget();
					return accum.str ();
				} else {
					accum << c;
					return accum.str ();
				}
			break;
		    case '#':
				while (c != '\n' && !input.eof ())  { c = input.get (); }
                input.unget();
			break;            
			case ' ': case '\t': case '\r':
				if (accum.str ().size ()) return accum.str ();
				else continue;
			break;
			case '{':
		        accum << c;
	            do  {
	                c = input.get ();
	                accum << c;
	            } while (c != '}' && !input.eof ());
				return accum.str ().substr (1, accum.str ().size () - 2);
			break;             
			default:
				if (c > 0) accum << c;
		}
	}
	return accum.str ();
}

// parsing and evaluation
Block& check_args (const std::string& cmd, Block& b, int i) {
    if (b.size () != i) {
        std::stringstream err;
        err << "wrong number of arguments in " << cmd;
        throw std::runtime_error (err.str ());
    }
    return b;
}
std::string eval (std::istream& in, Namespace& nspace) {
    Block b;
    while (!in.eof ()) {
        std::string token = get_token (in);
        if (token.size () == 0) continue;
        if (token == "\n") break;
        if (token[0] == '$') {
            std::string key = token.substr (1, token.size () - 1);
            if (nspace.variables.find (key) != nspace.variables.end ()) {
                b.push_back (nspace.variables[key]);
            } else {
                std::stringstream err;
                err << "undeclared identifier " << key;
                throw std::runtime_error (err.str ());
            }
        } else if (token == "[") {
            std::stringstream code;
            while (!in.eof ()) {
                token = get_token(in);
                if (token == "]") break;
                code << token << " ";
            }
            if (token != "]") {
                std::stringstream err;
                err << "invalid syntax in " << code.str ();
                throw std::runtime_error (err.str ());
            } 
            b.push_back (eval (code, nspace));
        } else b.push_back (token);
    }
    if (b.size () == 0) return "";
    std::string cmd = b.at (0);
    b.pop_front ();
    if (nspace.proceduers.find (cmd) != nspace.proceduers.end ()) {
        std::stringstream argstream (nspace.proceduers[cmd].at (0));
        std::stringstream code (nspace.proceduers[cmd].at (1));
        Block args;
        while (!argstream.eof ()) {
            args.push_back (get_token (argstream));
        }
        Namespace nnspace = nspace;
        if (b.size () != args.size ()) {
            std::stringstream err;
            err << "invalid number of arguments in " << cmd;
            throw std::runtime_error (err.str ());
        }
        for (unsigned i = 0; i < b.size (); ++i) {
            nnspace.variables[args.at (i)] = b.at (i);
        }
        std::string res;
        while (!code.eof ()) {
            res = eval (code, nnspace);
        }
        return res;
    } else if (nspace.functors.find (cmd) != nspace.functors.end ()) {
        return nspace.functors[cmd](b, nspace);
    } else {
        std::stringstream err;
        err << "undeclared command " << cmd;
        throw std::runtime_error (err.str());
    }
    return "";
}

// functors
std::string fn_puts (Block& b, Namespace& nspace) {
    check_args ("puts", b, 1);
    std::cout << b.at (0) << std::endl;
    return "";
}
std::string fn_set (Block& b,  Namespace& nspace) {
    check_args ("set", b, 2);
    std::string key = b.at (0);
    nspace.variables[key] = b.at (1);
    return b.at (1);
}
std::string fn_proc (Block& b, Namespace& nspace) {
    check_args ("proc", b, 3);
   std::string key = b.at (0);
    b.pop_front ();
    nspace.proceduers[key] = b;
    return key;
}
Real to_number (const std::string& s) {
	std::istringstream iss (s.c_str ());
	Real dummy;
	iss >> std::noskipws >> dummy;
	if (!(iss && iss.eof ())) {
        std::stringstream err;
        err << "invalid argument " << s;
        throw std::runtime_error (err.str ());
    }
    return atof (s.c_str ());
}
template <char op>
std::string fn_binop (Block& b,  Namespace& nspace) {
    check_args ("bin_op", b, 2);
    std::stringstream val;
    Real first = to_number (b.at (0)); 
    Real second = to_number (b.at (1));
    switch (op) {
        case '+':  val << first + second;  break;
        case '-':  val << first - second;  break;
        case '*':  val << first * second;  break;
        case '/':  val << first / second;  break;
        case '<':  val << (first < second);  break;
        case '>':  val << (first > second);  break;
        case 'L':  val << (first <= second);  break;
        case 'G':  val << (first >= second);  break;
        case '=':  val << (first == second);  break;
    }
    return val.str ();
}
std::string load (const std::string& name, Namespace& nspace) {
	std::ifstream in (name.c_str ());
	if (!in.good ()) {
		std::string longname = getenv("HOME");
		longname += "/.skicl/" + name;
		in.open (longname.c_str());
		if (!in.good ()) return "";
	}
    std::string res;
	while (!in.eof ()) {
		res = eval (in, nspace);
	}
	return res;
}
std::string fn_load (Block& b,  Namespace& nspace) {
    check_args ("load", b, 1);
    return load (b.at (0), nspace);
}

// interface
void init_namespace (Namespace& nspace) {
    nspace.functors["puts"] = fn_puts;
    nspace.functors["load"] = fn_load;
    nspace.functors["set"] = fn_set;
    nspace.functors["proc"] = fn_proc;
    nspace.functors["+"] = fn_binop<'+'>;
    nspace.functors["-"] = fn_binop<'-'>;
    nspace.functors["*"] = fn_binop<'*'>;
    nspace.functors["/"] = fn_binop<'/'>;
    nspace.functors["<"] = fn_binop<'<'>;
    nspace.functors[">"] = fn_binop<'>'>;
    nspace.functors["<="] = fn_binop<'L'>;
    nspace.functors[">="] = fn_binop<'G'>;
    nspace.functors["=="] = fn_binop<'='>;
}
void repl (std::istream& in, Namespace& nspace){
    while (true) { 
        cout << ">> ";
        try {
            std::cout << eval (in, nspace) << std::endl;
        } catch (std::exception& err) {
            std::cout << "error: " << err.what () << std::endl;
        }
    } 
}
int main (int argc, char* argv[]) {
	try {
	    Namespace nspace;
        init_namespace (nspace);
		if (argc > 1) {
			for (unsigned i = 1; i < argc; ++i) {
				std::string res = load (argv[i], nspace);
			}
		} else {
			cout << "[skicl, ver 0.1]" << endl << endl;
			cout << "a tiny tcl dialect" << endl;
			cout << "(c) 2020, www.carminecella.com" << endl << endl;
            repl (cin, nspace);
		}
	} catch (exception& e) {
		cout << "error: " << e.what () << endl;
	} catch (...) {
		cout << "fatal error; quitting" << endl;
	}

    return 0;
}