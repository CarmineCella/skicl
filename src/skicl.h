// skicl.h
//
#ifndef SKICL_H
#define SKICL_H

#include <deque>
#include <string>
#include <sstream>
#include <stdexcept>
#include <fstream>
#include <memory>
#include <iostream>

struct Node;
typedef std::shared_ptr<Node> Node_ptr;

enum NodeType {
	LEXEME,
	QUOTE,
	BLOCK
};

struct Node {
	Node (NodeType t) {
		type = t;
	}
	NodeType type;
	std::string lexeme;
	std::deque <Node_ptr> tail;
	static Node_ptr create (NodeType t) {
		return std::make_shared<Node> (t);
	}
};

std::string get_token (std::istream& in) {
	std::stringstream accum;
	while (!in.eof()) {
		char c = in.get ();
		switch(c) {
			case ';': case '[': case ']': case '{': case '}':
				if (accum.str ().size ()) {
					in.putback (c);
				} else {
					accum << c;
				}
				return accum.str ();
			break;
			case '\t': case ' ': case '\n': case '\r':
				if (accum.str ().size ()) {
					return accum.str ();
				} else continue;
			break;
			case '\"':
				accum << c;
				while (!in.eof ()) {
					c = in.get ();
					accum << c;
					if (c == '\"') break;
				}
				return accum.str ();
			break;
			default:
				if (!in.eof ()) accum << c;
			break;
		}
	}
	return accum.str ();
}

Node_ptr read (std::istream& in) {
	Node_ptr coll = Node::create (BLOCK);
	while (!in.eof ()) {
		std::string token = get_token (in);
		if (token == ";") break;

		if (token == "[" || token == "{") {
			Node_ptr l = Node::create (token == "[" ? BLOCK : QUOTE);
			std::string term = (token == "[" ? "]" : "}");
			std::string nterm = (token == "[" ? "}" : "]");
			while (!in.eof ()) {
				Node_ptr n = read (in);
				std::cout << "** " << n->lexeme << std::endl;
				if (n->lexeme == term) break;
				if (n->lexeme == nterm) {
					throw std::runtime_error ("invalid terminator in expression");
				}
				l->tail.push_back(n);
			}
			coll->tail.push_back(l);
		} else {
			Node_ptr s = Node::create (LEXEME);
			s->lexeme = token;
			coll->tail.push_back(s);
		}
	}
	return coll;
}
std::ostream& print (Node_ptr n, std::ostream& out) {
	switch (n->type) {
		case LEXEME:
			out << n->lexeme;
		break;
		case QUOTE:	case BLOCK:
		for (unsigned i = 0; i < n->tail.size (); ++i) {
			print (n->tail.at (i), out); out << " ";
		}
		break;
	}
	return out;
}

Node_ptr eval (Node_ptr node, Node_ptr env) {

	return node;
}

void repl (Node_ptr env) {
	std::cout << ">> ";
	while (true) {
		try {
			print (eval (read (std::cin), env), std::cout);
			std::cout << std::endl;
		} catch (std::exception& e) {
			std::cout << "error: " << e.what () << std::endl;
		}
	}
}
#endif // SKICL_H