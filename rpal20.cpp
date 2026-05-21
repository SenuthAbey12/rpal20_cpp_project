#include <bits/stdc++.h>
using namespace std;

/*
 * RPAL Interpreter Project - C++ version
 * Implements: manual lexical analyzer, recursive-descent parser, AST printer,
 * basic standardizer, and evaluator for common RPAL programs.
 * Build: make
 * Run:   ./rpal20 file_name
 */

struct Token {
    string type;   // ID, INT, STR, SYM, END
    string text;
    int line;
};

class Lexer {
    string s;
    size_t i = 0;
    int line = 1;
    vector<string> twoOps = {"->", ">=", "<=", "**"};
public:
    explicit Lexer(string src) : s(std::move(src)) {}

    vector<Token> scan() {
        vector<Token> out;
        while (i < s.size()) {
            char c = s[i];
            if (isspace((unsigned char)c)) {
                if (c == '\n') line++;
                i++;
                continue;
            }
            if (c == '/' && i + 1 < s.size() && s[i + 1] == '/') {
                while (i < s.size() && s[i] != '\n') i++;
                continue;
            }
            if (isalpha((unsigned char)c)) {
                size_t st = i++;
                while (i < s.size() && (isalnum((unsigned char)s[i]) || s[i] == '_')) i++;
                out.push_back({"ID", s.substr(st, i - st), line});
                continue;
            }
            if (isdigit((unsigned char)c)) {
                size_t st = i++;
                while (i < s.size() && isdigit((unsigned char)s[i])) i++;
                out.push_back({"INT", s.substr(st, i - st), line});
                continue;
            }
            if (c == '\'') {
                i++;
                string val;
                while (i < s.size() && s[i] != '\'') {
                    if (s[i] == '\\' && i + 1 < s.size()) {
                        char n = s[i + 1];
                        if (n == 'n') val += '\n';
                        else if (n == 't') val += '\t';
                        else val += n;
                        i += 2;
                    } else {
                        val += s[i++];
                    }
                }
                if (i < s.size() && s[i] == '\'') i++;
                out.push_back({"STR", val, line});
                continue;
            }
            if (c == '(' || c == ')' || c == ',' || c == ';' || c == '.') {
                out.push_back({"SYM", string(1, c), line});
                i++;
                continue;
            }
            bool matched = false;
            if (i + 1 < s.size()) {
                string op2 = s.substr(i, 2);
                for (auto &op : twoOps) {
                    if (op2 == op) {
                        out.push_back({"SYM", op2, line});
                        i += 2;
                        matched = true;
                        break;
                    }
                }
            }
            if (matched) continue;
            string opchars = "+-*/<>&@/:=~|$!#%^_[]{}\"`?";
            if (opchars.find(c) != string::npos) {
                out.push_back({"SYM", string(1, c), line});
                i++;
                continue;
            }
            throw runtime_error("Unknown character at line " + to_string(line) + ": " + string(1, c));
        }
        out.push_back({"END", "<END>", line});
        return out;
    }
};

struct Node {
    string label;
    vector<shared_ptr<Node>> ch;
    explicit Node(string l = "") : label(std::move(l)) {}
};
using N = shared_ptr<Node>;
static N node(const string &l, vector<N> c = {}) { auto p = make_shared<Node>(l); p->ch = std::move(c); return p; }

class Parser {
    vector<Token> t;
    size_t p = 0;
public:
    explicit Parser(vector<Token> toks) : t(std::move(toks)) {}
    N parse() {
        N r = E();
        if (!is("<END>")) err("Unexpected token after complete expression");
        return r;
    }
private:
    bool is(const string &x) const { return t[p].text == x; }
    bool type(const string &x) const { return t[p].type == x; }
    Token get() { return t[p++]; }
    void need(const string &x) { if (!is(x)) err("Expected '" + x + "'"); p++; }
    void err(const string &m) const { throw runtime_error(m + " near '" + t[p].text + "' at line " + to_string(t[p].line)); }
    bool reservedWord(const string &w) const {
        static const unordered_set<string> kw = {
            "let","in","fn","where","aug","or","not","gr","ge","ls","le","eq","ne",
            "within","and","rec","true","false","nil","dummy"
        };
        return kw.count(w) > 0;
    }
    bool idToken() const { return type("ID") && !reservedWord(t[p].text); }
    bool startsRn() const {
        return idToken() || type("INT") || type("STR") || is("true") || is("false") || is("nil") || is("dummy") || is("(");
    }
    bool startsVb() const { return idToken() || is("("); }

    N E() {
        if (is("let")) { get(); N d = D(); need("in"); return node("let", {d, E()}); }
        if (is("fn")) {
            get(); vector<N> c;
            if (!startsVb()) err("Expected variable binding after fn");
            while (startsVb()) c.push_back(Vb());
            need("."); c.push_back(E());
            return node("lambda", c);
        }
        return Ew();
    }
    N Ew() { N a = T(); if (is("where")) { get(); return node("where", {a, Dr()}); } return a; }
    N T() {
        vector<N> v; v.push_back(Ta());
        while (is(",")) { get(); v.push_back(Ta()); }
        if (v.size() > 1) return node("tau", v);
        return v[0];
    }
    N Ta() { N a = Tc(); while (is("aug")) { get(); a = node("aug", {a, Tc()}); } return a; }
    N Tc() { N a = B(); if (is("->")) { get(); N b = Tc(); need("|"); return node("->", {a, b, Tc()}); } return a; }
    N B() { N a = Bt(); while (is("or")) { get(); a = node("or", {a, Bt()}); } return a; }
    N Bt() { N a = Bs(); while (is("&")) { get(); a = node("&", {a, Bs()}); } return a; }
    N Bs() { if (is("not")) { get(); return node("not", {Bp()}); } return Bp(); }
    N Bp() {
        N a = A();
        string op;
        if (is("gr") || is(">")) op = "gr";
        else if (is("ge") || is(">=")) op = "ge";
        else if (is("ls") || is("<")) op = "ls";
        else if (is("le") || is("<=")) op = "le";
        else if (is("eq")) op = "eq";
        else if (is("ne")) op = "ne";
        if (!op.empty()) { get(); return node(op, {a, A()}); }
        return a;
    }
    N A() {
        if (is("+")) { get(); return At(); }
        if (is("-")) { get(); return node("neg", {At()}); }
        N a = At();
        while (is("+") || is("-")) { string op = get().text; a = node(op, {a, At()}); }
        return a;
    }
    N At() { N a = Af(); while (is("*") || is("/")) { string op = get().text; a = node(op, {a, Af()}); } return a; }
    N Af() { N a = Ap(); if (is("**")) { get(); a = node("**", {a, Af()}); } return a; }
    N Ap() { N a = R(); while (is("@")) { get(); if (!type("ID")) err("Expected identifier after @"); N id = node("<ID:" + get().text + ">"); a = node("@", {a, id, R()}); } return a; }
    N R() { N a = Rn(); while (startsRn()) a = node("gamma", {a, Rn()}); return a; }
    N Rn() {
        if (idToken()) return node("<ID:" + get().text + ">");
        if (type("INT")) return node("<INT:" + get().text + ">");
        if (type("STR")) return node("<STR:" + get().text + ">");
        if (is("true")) { get(); return node("true"); }
        if (is("false")) { get(); return node("false"); }
        if (is("nil")) { get(); return node("nil"); }
        if (is("dummy")) { get(); return node("dummy"); }
        if (is("(")) { get(); N e = E(); need(")"); return e; }
        err("Expected Rn"); return nullptr;
    }
    N D() { N a = Da(); if (is("within")) { get(); return node("within", {a, D()}); } return a; }
    N Da() { vector<N> v; v.push_back(Dr()); while (is("and")) { get(); v.push_back(Dr()); } if (v.size() > 1) return node("and", v); return v[0]; }
    N Dr() { if (is("rec")) { get(); return node("rec", {Db()}); } return Db(); }
    N Db() {
        if (is("(")) { get(); N d = D(); need(")"); return d; }
        if (!idToken()) err("Expected identifier in definition");
        string name = get().text;
        if (is(",")) {
            vector<N> vars; vars.push_back(node("<ID:" + name + ">"));
            while (is(",")) { get(); if (!idToken()) err("Expected identifier after comma in variable list"); vars.push_back(node("<ID:" + get().text + ">")); }
            need("="); return node("=", {node(",", vars), E()});
        }
        if (is("=")) { get(); return node("=", {node("<ID:" + name + ">"), E()}); }
        vector<N> c; c.push_back(node("<ID:" + name + ">"));
        if (!startsVb()) err("Expected '=' or variable binding in definition");
        while (startsVb()) c.push_back(Vb());
        need("="); c.push_back(E());
        return node("fcn_form", c);
    }
    N Vb() {
        if (idToken()) return node("<ID:" + get().text + ">");
        if (is("(")) {
            get();
            if (is(")")) { get(); return node("()"); }
            vector<N> vars;
            if (!idToken()) err("Expected identifier in variable list");
            vars.push_back(node("<ID:" + get().text + ">"));
            while (is(",")) { get(); if (!idToken()) err("Expected identifier after comma in variable list"); vars.push_back(node("<ID:" + get().text + ">")); }
            need(")");
            if (vars.size() == 1) return vars[0];
            return node(",", vars);
        }
        err("Expected variable binding"); return nullptr;
    }
};

static string idName(const string &label) {
    if (label.rfind("<ID:", 0) == 0) return label.substr(4, label.size() - 5);
    return label;
}
static bool isIdNode(const N &n) { return n && n->label.rfind("<ID:", 0) == 0; }
static N makeLambda(vector<N> params, N body) {
    for (int i = (int)params.size() - 1; i >= 0; --i) body = node("lambda", {params[i], body});
    return body;
}

N standardize(const N &x) {
    vector<N> c; for (auto &k : x->ch) c.push_back(standardize(k));
    string l = x->label;
    if (l == "let") { // let X=E in P => gamma(lambda(X,P),E)
        N d = c[0], p = c[1];
        if (d->label == "=") return node("gamma", {node("lambda", {d->ch[0], p}), d->ch[1]});
    }
    if (l == "where") { // P where X=E => gamma(lambda(X,P),E)
        N p = c[0], d = c[1];
        if (d->label == "=") return node("gamma", {node("lambda", {d->ch[0], p}), d->ch[1]});
    }
    if (l == "fcn_form") {
        N name = c[0]; N body = c.back(); vector<N> params(c.begin()+1, c.end()-1);
        return node("=", {name, makeLambda(params, body)});
    }
    if (l == "lambda" && c.size() > 2) {
        N body = c.back(); vector<N> params(c.begin(), c.end()-1);
        return makeLambda(params, body);
    }
    if (l == "within") { // X=E1 within Y=E2 => Y = gamma(lambda(X,E2),E1)
        N d1 = c[0], d2 = c[1];
        if (d1->label == "=" && d2->label == "=") return node("=", {d2->ch[0], node("gamma", {node("lambda", {d1->ch[0], d2->ch[1]}), d1->ch[1]})});
    }
    if (l == "and") {
        vector<N> vars, vals;
        for (auto &d : c) if (d->label == "=") { vars.push_back(d->ch[0]); vals.push_back(d->ch[1]); }
        return node("=", {node(",", vars), node("tau", vals)});
    }
    if (l == "rec") { // rec X=E => X = gamma(Y*,lambda(X,E))
        N d = c[0];
        if (d->label == "=") return node("=", {d->ch[0], node("gamma", {node("<ID:Y*>") , node("lambda", {d->ch[0], d->ch[1]})})});
    }
    return node(l, c);
}

void printTree(const N &n, int depth = 0) {
    for (int i = 0; i < depth; ++i) cout << ".";
    cout << n->label << "\n";
    for (auto &c : n->ch) printTree(c, depth + 1);
}

struct Value;
struct Env;
using EnvPtr = shared_ptr<Env>;

struct Closure {
    vector<N> params;
    N body;
    EnvPtr env;
};
struct Builtin {
    string name;
    vector<Value> args;
};
struct Value {
    enum Kind { INT, STR, BOOL, TUPLE, CLOSURE, BUILTIN, NIL, DUMMY } kind = DUMMY;
    long long i = 0;
    string s;
    bool b = false;
    vector<Value> tuple;
    shared_ptr<Closure> clo;
    shared_ptr<Builtin> bi;
};
struct Env {
    unordered_map<string, Value> m;
    EnvPtr parent;
    explicit Env(EnvPtr p = nullptr) : parent(std::move(p)) {}
};

static bool printed = false;
Value eval(N x, EnvPtr env);

Value makeInt(long long v){ Value r; r.kind=Value::INT; r.i=v; return r; }
Value makeStr(string v){ Value r; r.kind=Value::STR; r.s=std::move(v); return r; }
Value makeBool(bool v){ Value r; r.kind=Value::BOOL; r.b=v; return r; }
Value makeTuple(vector<Value> v){ Value r; r.kind=Value::TUPLE; r.tuple=std::move(v); return r; }
Value makeNil(){ Value r; r.kind=Value::NIL; return r; }
Value makeDummy(){ Value r; r.kind=Value::DUMMY; return r; }
Value makeBuiltin(string name, vector<Value> args={}){ Value r; r.kind=Value::BUILTIN; r.bi=make_shared<Builtin>(); r.bi->name=std::move(name); r.bi->args=std::move(args); return r; }
Value makeClosure(vector<N> params, N body, EnvPtr env){ Value r; r.kind=Value::CLOSURE; r.clo=make_shared<Closure>(); r.clo->params=std::move(params); r.clo->body=body; r.clo->env=env; return r; }

string show(const Value &v) {
    switch(v.kind) {
        case Value::INT: return to_string(v.i);
        case Value::STR: return v.s;
        case Value::BOOL: return v.b ? "true" : "false";
        case Value::NIL: return "nil";
        case Value::DUMMY: return "dummy";
        case Value::TUPLE: {
            string r = "(";
            for (size_t i=0;i<v.tuple.size();++i){ if(i) r += ", "; r += show(v.tuple[i]); }
            return r + ")";
        }
        case Value::CLOSURE: return "<function>";
        case Value::BUILTIN: return "<builtin:" + v.bi->name + ">";
    }
    return "";
}

Value lookup(EnvPtr e, const string &name) {
    for (auto p=e; p; p=p->parent) if (p->m.count(name)) return p->m[name];
    static unordered_set<string> builtins = {"Print","Conc","Stern","Stem","ItoS","Order","Null","Isinteger","Isstring","Istuple","Istruthvalue","Isfunction","Isdummy"};
    if (builtins.count(name)) return makeBuiltin(name);
    throw runtime_error("Unbound identifier: " + name);
}

void bindPattern(EnvPtr env, N pat, const Value &v) {
    if (isIdNode(pat)) { env->m[idName(pat->label)] = v; return; }
    if (pat->label == "()") return;
    if (pat->label == ",") {
        if (v.kind != Value::TUPLE || v.tuple.size() != pat->ch.size()) throw runtime_error("Tuple pattern arity mismatch");
        for (size_t i=0;i<pat->ch.size();++i) bindPattern(env, pat->ch[i], v.tuple[i]);
        return;
    }
    throw runtime_error("Invalid binding pattern: " + pat->label);
}

bool truth(const Value &v){ if(v.kind!=Value::BOOL) throw runtime_error("Boolean expected"); return v.b; }
long long integer(const Value &v){ if(v.kind!=Value::INT) throw runtime_error("Integer expected"); return v.i; }

bool equalVal(const Value &a, const Value &b) {
    if (a.kind != b.kind) return false;
    if (a.kind == Value::INT) return a.i == b.i;
    if (a.kind == Value::STR) return a.s == b.s;
    if (a.kind == Value::BOOL) return a.b == b.b;
    if (a.kind == Value::NIL || a.kind == Value::DUMMY) return true;
    if (a.kind == Value::TUPLE) {
        if (a.tuple.size()!=b.tuple.size()) return false;
        for(size_t i=0;i<a.tuple.size();++i) if(!equalVal(a.tuple[i], b.tuple[i])) return false;
        return true;
    }
    return false;
}

Value apply(Value f, Value arg) {
    if (f.kind == Value::TUPLE) {
        long long n = integer(arg);
        if (n < 1 || n > (long long)f.tuple.size()) throw runtime_error("Tuple index out of range");
        return f.tuple[(size_t)n - 1];
    }
    if (f.kind == Value::CLOSURE) {
        auto newEnv = make_shared<Env>(f.clo->env);
        bindPattern(newEnv, f.clo->params[0], arg);
        if (f.clo->params.size() == 1) return eval(f.clo->body, newEnv);
        vector<N> rest(f.clo->params.begin()+1, f.clo->params.end());
        return makeClosure(rest, f.clo->body, newEnv);
    }
    if (f.kind == Value::BUILTIN) {
        string name = f.bi->name;
        auto args = f.bi->args; args.push_back(arg);
        if (name == "Print" && args.size()==1) { cout << show(args[0]); printed = true; return makeDummy(); }
        if (name == "ItoS" && args.size()==1) return makeStr(to_string(integer(args[0])));
        if (name == "Order" && args.size()==1) { if(args[0].kind!=Value::TUPLE) throw runtime_error("Order expects tuple"); return makeInt((long long)args[0].tuple.size()); }
        if (name == "Stem" && args.size()==1) { if(args[0].kind!=Value::STR) throw runtime_error("Stem expects string"); return makeStr(args[0].s.empty()?"":args[0].s.substr(0,1)); }
        if (name == "Stern" && args.size()==1) { if(args[0].kind!=Value::STR) throw runtime_error("Stern expects string"); return makeStr(args[0].s.size()<=1?"":args[0].s.substr(1)); }
        if (name == "Null" && args.size()==1) return makeBool((args[0].kind==Value::NIL) || (args[0].kind==Value::TUPLE && args[0].tuple.empty()));
        if (name == "Isinteger" && args.size()==1) return makeBool(args[0].kind==Value::INT);
        if (name == "Isstring" && args.size()==1) return makeBool(args[0].kind==Value::STR);
        if (name == "Istuple" && args.size()==1) return makeBool(args[0].kind==Value::TUPLE);
        if (name == "Istruthvalue" && args.size()==1) return makeBool(args[0].kind==Value::BOOL);
        if (name == "Isfunction" && args.size()==1) return makeBool(args[0].kind==Value::CLOSURE || args[0].kind==Value::BUILTIN);
        if (name == "Isdummy" && args.size()==1) return makeBool(args[0].kind==Value::DUMMY);
        if (name == "Conc" && args.size()==2) { if(args[0].kind!=Value::STR || args[1].kind!=Value::STR) throw runtime_error("Conc expects strings"); return makeStr(args[0].s + args[1].s); }
        return makeBuiltin(name, args);
    }
    throw runtime_error("Attempted to apply non-function: " + show(f));
}

unordered_map<string, Value> evalDef(N d, EnvPtr env) {
    if (d->label == "=") {
        Value v = eval(d->ch[1], env);
        unordered_map<string, Value> r;
        if (isIdNode(d->ch[0])) r[idName(d->ch[0]->label)] = v;
        else if (d->ch[0]->label == ",") {
            if (v.kind != Value::TUPLE || v.tuple.size()!=d->ch[0]->ch.size()) throw runtime_error("Definition tuple mismatch");
            for(size_t i=0;i<d->ch[0]->ch.size();++i) r[idName(d->ch[0]->ch[i]->label)] = v.tuple[i];
        }
        return r;
    }
    if (d->label == "fcn_form") {
        string name = idName(d->ch[0]->label);
        vector<N> params(d->ch.begin()+1, d->ch.end()-1);
        Value clo = makeClosure(params, d->ch.back(), env);
        return {{name, clo}};
    }
    if (d->label == "and") {
        unordered_map<string, Value> r;
        for (auto &x : d->ch) { auto m = evalDef(x, env); r.insert(m.begin(), m.end()); }
        return r;
    }
    if (d->label == "within") {
        auto m1 = evalDef(d->ch[0], env);
        auto e2 = make_shared<Env>(env); for(auto &kv:m1) e2->m[kv.first]=kv.second;
        return evalDef(d->ch[1], e2);
    }
    if (d->label == "rec") {
        auto recEnv = make_shared<Env>(env);
        N child = d->ch[0];
        if (child->label == "fcn_form") {
            string name = idName(child->ch[0]->label);
            vector<N> params(child->ch.begin()+1, child->ch.end()-1);
            Value clo = makeClosure(params, child->ch.back(), recEnv);
            recEnv->m[name] = clo;
            return {{name, clo}};
        } else if (child->label == "=") {
            string name = idName(child->ch[0]->label);
            recEnv->m[name] = makeDummy();
            Value v = eval(child->ch[1], recEnv);
            recEnv->m[name] = v;
            return {{name, v}};
        }
    }
    throw runtime_error("Invalid definition node: " + d->label);
}

Value eval(N x, EnvPtr env) {
    string l = x->label;
    if (l.rfind("<INT:",0)==0) return makeInt(stoll(l.substr(5, l.size()-6)));
    if (l.rfind("<STR:",0)==0) return makeStr(l.substr(5, l.size()-6));
    if (l.rfind("<ID:",0)==0) return lookup(env, idName(l));
    if (l == "true") return makeBool(true);
    if (l == "false") return makeBool(false);
    if (l == "nil") return makeNil();
    if (l == "dummy") return makeDummy();
    if (l == "tau") { vector<Value> v; for(auto &c:x->ch) v.push_back(eval(c,env)); return makeTuple(v); }
    if (l == "lambda") { vector<N> params(x->ch.begin(), x->ch.end()-1); return makeClosure(params, x->ch.back(), env); }
    if (l == "gamma") return apply(eval(x->ch[0], env), eval(x->ch[1], env));
    if (l == "let") { auto e2=make_shared<Env>(env); auto m=evalDef(x->ch[0], e2); for(auto &kv:m) e2->m[kv.first]=kv.second; return eval(x->ch[1], e2); }
    if (l == "where") { auto e2=make_shared<Env>(env); auto m=evalDef(x->ch[1], e2); for(auto &kv:m) e2->m[kv.first]=kv.second; return eval(x->ch[0], e2); }
    if (l == "->") return truth(eval(x->ch[0], env)) ? eval(x->ch[1], env) : eval(x->ch[2], env);
    if (l == "neg") return makeInt(-integer(eval(x->ch[0], env)));
    if (l == "not") return makeBool(!truth(eval(x->ch[0], env)));
    if (l == "or") return makeBool(truth(eval(x->ch[0], env)) || truth(eval(x->ch[1], env)));
    if (l == "&") return makeBool(truth(eval(x->ch[0], env)) && truth(eval(x->ch[1], env)));
    if (l == "aug") { Value a=eval(x->ch[0],env), b=eval(x->ch[1],env); vector<Value> v; if(a.kind==Value::TUPLE) v=a.tuple; else v.push_back(a); v.push_back(b); return makeTuple(v); }
    if (l == "+" || l == "-" || l == "*" || l == "/" || l == "**") {
        long long a=integer(eval(x->ch[0],env)), b=integer(eval(x->ch[1],env));
        if(l=="+") return makeInt(a+b);
        if(l=="-") return makeInt(a-b);
        if(l=="*") return makeInt(a*b);
        if(l=="/") return makeInt(a/b);
        long long r=1;
        for(long long k=0;k<b;k++) r*=a;
        return makeInt(r);
    }
    if (l == "eq" || l == "ne") { bool e=equalVal(eval(x->ch[0],env), eval(x->ch[1],env)); return makeBool(l=="eq"?e:!e); }
    if (l == "gr" || l == "ge" || l == "ls" || l == "le") {
        Value a=eval(x->ch[0],env), b=eval(x->ch[1],env);
        if(a.kind==Value::INT && b.kind==Value::INT){ if(l=="gr") return makeBool(a.i>b.i); if(l=="ge") return makeBool(a.i>=b.i); if(l=="ls") return makeBool(a.i<b.i); return makeBool(a.i<=b.i); }
        if(a.kind==Value::STR && b.kind==Value::STR){ if(l=="gr") return makeBool(a.s>b.s); if(l=="ge") return makeBool(a.s>=b.s); if(l=="ls") return makeBool(a.s<b.s); return makeBool(a.s<=b.s); }
        throw runtime_error("Comparison expects both integer or both string");
    }
    if (l == "@") { // E @ f R  ==> gamma(gamma(f,E),R) for common operator forms
        Value f = lookup(env, idName(x->ch[1]->label));
        return apply(apply(f, eval(x->ch[0],env)), eval(x->ch[2],env));
    }
    throw runtime_error("Cannot evaluate node: " + l);
}

int main(int argc, char **argv) {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    try {
        bool showAst=false, showSt=false;
        string file;
        for(int i=1;i<argc;i++) {
            string a=argv[i];
            if(a=="-ast") showAst=true;
            else if(a=="-st") showSt=true;
            else file=a;
        }
        if(file.empty()) { cerr << "Usage: ./rpal20 [-ast] [-st] file_name\n"; return 1; }
        ifstream in(file);
        if(!in) { cerr << "Cannot open input file: " << file << "\n"; return 1; }
        string src((istreambuf_iterator<char>(in)), istreambuf_iterator<char>());
        Lexer lx(src);
        Parser ps(lx.scan());
        N ast = ps.parse();
        if(showAst) { printTree(ast); return 0; }
        N st = standardize(ast);
        if(showSt) { printTree(st); return 0; }
        auto global = make_shared<Env>();
        Value ans = eval(ast, global);
        if(!printed && ans.kind != Value::DUMMY) cout << show(ans);
        return 0;
    } catch(const exception &e) {
        cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
