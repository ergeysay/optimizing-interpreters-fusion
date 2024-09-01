#include <stdint.h>
#include <stdio.h>
#include <initializer_list>

namespace Simplest {
    struct Context {
        bool stopForReturn;
        uint32_t returnValue;
        uint32_t* stack;
        uint32_t stackTop;

        Context() 
            : stopForReturn(false), returnValue(0), stack(new uint32_t[4096]), stackTop(0) {}

        ~Context() {
            delete[] stack;
        }
    };

    struct Node {
        virtual ~Node() {}

        virtual uint32_t eval(Context* ctx) { return 0; }
    };

    struct ConstNode : Node {
        uint32_t value;

        ConstNode(uint32_t value) : value(value) {}

        uint32_t eval(Context* ctx) override {
            return value;
        }
    };

    struct AddNode : Node {
        Node* lhs;
        Node* rhs;

        AddNode(Node* lhs, Node* rhs) : lhs(lhs), rhs(rhs) {}

        ~AddNode() {
            delete lhs;
            delete rhs;
        }

        uint32_t eval(Context* ctx) override {
            return lhs->eval(ctx) + rhs->eval(ctx);
        }
    };

    struct SubNode : Node {
        Node* lhs;
        Node* rhs;

        SubNode(Node* lhs, Node* rhs) : lhs(lhs), rhs(rhs) {}

        ~SubNode() {
            delete lhs;
            delete rhs;
        }

        uint32_t eval(Context* ctx) override {
            return lhs->eval(ctx) - rhs->eval(ctx);
        }
    };

    struct LessNode : Node {
        Node* lhs;
        Node* rhs;

        ~LessNode() {
            delete lhs;
            delete rhs;
        }

        LessNode(Node* lhs, Node* rhs) : lhs(lhs), rhs(rhs) {}

        uint32_t eval(Context* ctx) override {
            return lhs->eval(ctx) < rhs->eval(ctx);
        }
    };

    struct IfNode : Node {
        Node* condition;
        Node* body;

        IfNode(Node* condition, Node* body)
            : condition(condition), body(body) {}

        ~IfNode() {
            delete condition;
            delete body;
        }

        uint32_t eval(Context* ctx) override {
            if (condition->eval(ctx)) {
                body->eval(ctx);
            }

            return 0;
        }
    };

    struct Function {
        Node** body;
        uint32_t numNodes;

        Function() : body(0), numNodes(0) {}

        void init(std::initializer_list<Node*> body) {
            numNodes = (uint32_t) body.size();
            this->body = new Node * [numNodes];

            uint32_t i = 0;

            for (Node* statement : body) {
                this->body[i++] = statement;
            }
        }

        ~Function() {
            for (uint32_t i = 0; i < numNodes; i++) {
                delete body[i];
            }
            delete[] body;
        }
    };

    struct CallNode : Node {
        Function* function;
        Node* arg;

        CallNode(Function* function, Node* arg)
            : function(function), arg(arg) {}

        ~CallNode() {
            delete arg;
        }

        uint32_t eval(Context* ctx) override {
            ctx->stack[ctx->stackTop] = arg->eval(ctx);
            ctx->stackTop += 1;

            for (uint32_t i = 0, end_i = function->numNodes; i < end_i; i++) {
                function->body[i]->eval(ctx);
                if (ctx->stopForReturn) {
                    break;
                }
            }

            ctx->stopForReturn = false;
            ctx->stackTop -= 1;

            return ctx->returnValue;
        }
    };

    struct ReturnNode : Node {
        Node* rhs;

        ReturnNode(Node* rhs) : rhs(rhs) {}

        ~ReturnNode() {
            delete rhs;
        }

        uint32_t eval(Context* ctx) override {
            ctx->returnValue = rhs->eval(ctx);
            ctx->stopForReturn = true;

            // Since we pass the result in the ctx->retval field,
            // we don't need to return anything here
            return 0;
        }
    };

    struct ArgNode : Node {
        ArgNode() {}

        uint32_t eval(Context* ctx) override {
            return ctx->stack[ctx->stackTop - 1];
        }
    };

    uint32_t fib(uint32_t n) {
        Context ctx;

        Function* function = new Function();

        function->init({
            new IfNode(
                new LessNode(new ArgNode(), new ConstNode(2)),
                new ReturnNode(new ArgNode())),
            new ReturnNode(
                new AddNode(
                    new CallNode(function,
                        new SubNode(new ArgNode(), new ConstNode(1))),
                    new CallNode(function,
                        new SubNode(new ArgNode(), new ConstNode(2)))))
        });

        CallNode* call = new CallNode(function, new ConstNode(n));

        uint32_t result = call->eval(&ctx);

        delete function;
        delete call;

        return result;
    }
}

namespace SimpleFusion {
    using namespace Simplest;

    struct LessConstNode : Node {
        Node* lhs;
        uint32_t constant;

        LessConstNode(Node* lhs, uint32_t constant) 
            : lhs(lhs), constant(constant) {}

        ~LessConstNode() {
            delete lhs;
        }

        uint32_t eval(Context* ctx) {
            return lhs->eval(ctx) < constant;
        }
    };

    struct SubConstNode : Node {
        Node* lhs;
        uint32_t constant;

        SubConstNode(Node* lhs, uint32_t constant) 
            : lhs(lhs), constant(constant) {}

        ~SubConstNode() {
            delete lhs;
        }

        uint32_t eval(Context* ctx) {
            return lhs->eval(ctx) - constant;
        }
    };

    uint32_t fib(uint32_t n) {
        Context ctx;

        Function* function = new Function();

        function->init({
            new IfNode(
                new LessConstNode(new ArgNode(), 2),
                new ReturnNode(new ArgNode())),
            new ReturnNode(
                new AddNode(
                    new CallNode(function,
                        new SubConstNode(new ArgNode(), 1)),
                    new CallNode(function,
                        new SubConstNode(new ArgNode(), 2))))
            });

        CallNode* call = new CallNode(function, new ConstNode(n));

        uint32_t result = call->eval(&ctx);

        delete function;
        delete call;

        return result;
    }
}

#if defined(__clang__)
#define FORCEINLINE __attribute__((always_inline))
#elif defined(_MSC_VER) // clang defines _MSC_VER on Windows for some reason
#define FORCEINLINE __forceinline 
#endif

namespace BetterFusion {
    using namespace Simplest;

    struct ConstNode : Node {
        uint32_t value;

        ConstNode(uint32_t value) : value(value) {}

        uint32_t eval(Context* ctx) override {
            return compute(ctx);
        }

        uint32_t FORCEINLINE compute(Context* ctx) {
            return value;
        }
    };

    struct ArgNode : Node {
        ArgNode() {}

        uint32_t eval(Context* ctx) override {
            return compute(ctx);
        }

        uint32_t FORCEINLINE compute(Context* ctx) {
            return ctx->stack[ctx->stackTop - 1];
        }
    };

    struct LessArgConstNode : Node {
        ArgNode* lhs;
        ConstNode* rhs;

        LessArgConstNode(ArgNode* lhs, ConstNode* rhs) : lhs(lhs), rhs(rhs) {}

        uint32_t eval(Context* ctx) override {
            return compute(ctx);
        }

        uint32_t FORCEINLINE compute(Context* ctx) {
            return lhs->compute(ctx) < rhs->compute(ctx);
        }
    };

    struct SubArgConstNode : Node {
        ArgNode* lhs;
        ConstNode* rhs;

        SubArgConstNode(ArgNode* lhs, ConstNode* rhs) : lhs(lhs), rhs(rhs) {}

        uint32_t eval(Context* ctx) {
            return compute(ctx);
        }

        uint32_t FORCEINLINE compute(Context* ctx) {
            return lhs->compute(ctx) - rhs->compute(ctx);
        }
    };

    struct Function {
        Node** body;
        uint32_t numNodes;

        Function() : body(0), numNodes(0) {}

        void init(std::initializer_list<Node*> body_list) {
            numNodes = (uint32_t) body_list.size();
            body = new Node * [numNodes];

            uint32_t i = 0;

            for (Node* statement : body_list) {
                body[i++] = statement;
            }
        }

        ~Function() {
            for (uint32_t i = 0; i < numNodes; i++) {
                delete body[i];
            }
            delete[] body;
        }
    };

    struct CallNode : Node {
        Function* function;
        Node* arg;

        CallNode(Function* function, Node* arg)
            : function(function), arg(arg) {}

        ~CallNode() {
            delete arg;
        }

        uint32_t eval(Context* ctx) override {
            ctx->stack[ctx->stackTop] = arg->eval(ctx);
            ctx->stackTop += 1;

            for (uint32_t i = 0, end_i = function->numNodes; i < end_i; i++) {
                function->body[i]->eval(ctx);
                if (ctx->stopForReturn) {
                    break;
                }
            }

            ctx->stopForReturn = false;
            ctx->stackTop -= 1;

            return ctx->returnValue;
        }
    };

    uint32_t fib(uint32_t n) {
        Context ctx;

        Function* function = new Function();

        function->init({
            new IfNode(
                new LessArgConstNode(new ArgNode(), new ConstNode(2)),
                new ReturnNode(new ArgNode())),
            new ReturnNode(
                new AddNode(
                    new CallNode(function, new SubArgConstNode(new ArgNode(), new ConstNode(1))),
                    new CallNode(function, new SubArgConstNode(new ArgNode(), new ConstNode(2)))))
            });

        CallNode* call = new CallNode(function, new ConstNode(n));

        uint32_t result = call->eval(&ctx);

        delete function;
        delete call;

        return result;
    }
}

namespace SimplifyCalls {
    using namespace BetterFusion;

    struct CallAnyNode : Node {
        Node* function;
        Node* arg;

        CallAnyNode(Node* function, Node* arg) : function(function), arg(arg) {}

        ~CallAnyNode() {
            delete arg;
        }

        uint32_t eval(Context* ctx) override {
            ctx->stack[ctx->stackTop] = arg->eval(ctx);
            ctx->stackTop += 1;

            uint32_t result = function->eval(ctx);

            ctx->stopForReturn = false;
            ctx->stackTop -= 1;

            return result;
        }
    };

    struct IfElseNode : Node {
        Node* condition;
        Node* ifBody;
        Node* elseBody;

        IfElseNode(Node* condition, Node* ifBody, Node* elseBody)
            : condition(condition), ifBody(ifBody), elseBody(elseBody) {}

        ~IfElseNode() {
            delete condition;
            delete ifBody;
            delete elseBody;
        }

        uint32_t eval(Context* ctx) override {
            if (condition->eval(ctx)) {
                return ifBody->eval(ctx);
            }
            else {
                return elseBody->eval(ctx);
            }
        }
    };

    uint32_t fib(uint32_t n) {
        using BetterFusion::ArgNode;
        using BetterFusion::ConstNode;
        
        Context ctx;

        IfElseNode function(0, 0, 0);

        function.condition = new LessArgConstNode(new ArgNode(), new ConstNode(2));
        function.ifBody = new ArgNode();
        function.elseBody = new AddNode(
            new CallAnyNode(&function, new SubArgConstNode(new ArgNode(), new ConstNode(1))),
            new CallAnyNode(&function, new SubArgConstNode(new ArgNode(), new ConstNode(2))));

        CallAnyNode call(&function, new ConstNode(n));

        uint32_t result = call.eval(&ctx);

        return result;
    }
}

uint32_t fib(uint32_t n) {
    if (n < 2) {
        return n;
    }
    return fib(n - 1) + fib(n - 2);
}

int main(int, const char**) {
    uint32_t n = 42;

#if defined(BASELINE)
    printf("%d\n", fib(n));
#elif defined(SIMPLEST)
    printf("%d\n", Simplest::fib(n));
#elif defined(SIMPLE_FUSION)
    printf("%d\n", SimpleFusion::fib(n));
#elif defined(BETTER_FUSION)
    printf("%d\n", BetterFusion::fib(n));
#elif defined(SIMPLIFY_CALLS)
    printf("%d\n", SimplifyCalls::fib(n));
#else
#error Please define one of: BASELINE, SIMPLEST, SIMPLE_FUSION, BETTER_FUSION, or SIMPLIFY_CALLS
#endif

	return 0;
}
