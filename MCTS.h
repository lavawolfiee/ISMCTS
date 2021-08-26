#ifndef MCTS_MCTS_H
#define MCTS_MCTS_H

#include <memory>
#include <vector>
#include <algorithm>
#include <unordered_set>
#include <cmath>

template<typename State>
struct RandomAgent {
    typename State::MovePtr getMove(State& s) const;
};

template<typename T>
struct SharedHash {
private:
    std::hash<T> h;
public:
    size_t operator()(const std::shared_ptr<T>& ptr) const {
        return h(*ptr);
    }
};

template <typename T>
struct SharedEqual {
    bool operator()(const std::shared_ptr<T>& ptr1, const std::shared_ptr<T>& ptr2) const {
        return (*ptr1) == (*ptr2);
    }
};

template<typename State, typename Agent = RandomAgent<State>>
class MCTS {

    using MovePtr = typename State::MovePtr;
    using Move = typename State::Move;

    struct Node: public std::enable_shared_from_this<Node> {
        using Ptr = std::shared_ptr<Node>;
        using wPtr = std::weak_ptr<Node>;

        MovePtr move;
        wPtr parent;
        int just_moved;

        std::vector<Ptr> children;
        double wins;
        size_t visits;
        size_t avails;

        Node(MovePtr m, Ptr parent, int p);

        void update(const State& state);
        std::vector<MovePtr> getUntriedMoves(const std::vector<MovePtr>& legalMoves) const;
        Ptr UCBSelectChild(const std::vector<MovePtr>& legalMoves, double exploration = 0.7) const;
        double ucb(double exploration = 0.7) const;
        Ptr addChild(MovePtr move, int just_moved);
    };

    using NodePtr = std::shared_ptr<Node>;

private:
    std::shared_ptr<Node> root;
    State root_state;
    Agent agent;

    void loop(NodePtr node, const State& initial, size_t iters = 10'000) const;
    State iterate(NodePtr node, const State& initial) const;

    State determinize(const State& state) const;

    NodePtr select(NodePtr node, State& state) const;
    NodePtr expand(NodePtr node, State& state, const std::vector<MovePtr>& untried) const;

    void rollout(State& state, const Agent& agent) const;

public:
    const double exploration;

    explicit MCTS(double exploration = 0.7, const State& state = State(),
                  const Agent& agent = RandomAgent<State>());

    MovePtr getMove(size_t iters = 10'000) const; // getMove in one thread for the best move

    void loop(size_t iters = 10'000) const; // makes one loop of iters iterations to increase the tree
    State iterate() const; // makes one iteration to increase the tree

    void makeMove(const MovePtr& move); // makes move, changing root and root_state, removing redundant Nodes
};

template<typename State, typename Agent>
MCTS<State, Agent>::MCTS(double exploration, const State& state, const Agent& agent):
        root(std::make_shared<Node>(State::Move::null(), nullptr, -1)),
        root_state(state), exploration(exploration), agent(agent) {

}

template<typename State, typename Agent>
typename State::MovePtr MCTS<State, Agent>::getMove(size_t iters) const {
    loop(iters);

    if (root->children.empty())
        return State::Move::null();

    std::cout << root->wins << "/" << root->visits << std::endl;
    for (auto it = root->children.begin(); it != root->children.end(); ++it) {
        std::cout << (*it)->wins << "/" << (*it)->visits << " (" << static_cast<std::string>(*((*it)->move)) << "); ";
    }
    std::cout << std::endl;

    auto best = std::max_element(begin(root->children),
                                 end(root->children),
                                 [](const auto& a, const auto& b) {
                                     return a->visits < b->visits;
                                 });

    return (*best)->move;
}

template<typename State, typename Agent>
void MCTS<State, Agent>::loop(size_t iters) const {
    loop(root, root_state, iters);
}

template<typename State, typename Agent>
State MCTS<State, Agent>::iterate() const {
    return iterate(root, root_state);
}

template<typename State, typename Agent>
void MCTS<State, Agent>::loop(MCTS::NodePtr node, const State& initial, size_t iters) const {
    State state = determinize(initial);

    for (size_t i = 1; i <= iters; ++i)
        iterate(node, state);
}

template<typename State, typename Agent>
State MCTS<State, Agent>::iterate(MCTS::NodePtr node, const State& initial) const {

    // Determinize
    State state = determinize(initial);

    // Selection and expansion
    node = select(node, state);

    // Simulation
    rollout(state, agent);

    // Backpropagation
    while (node) {
        node->update(state);
        node = node->parent.lock();
    }

    return state;
}

template<typename State, typename Agent>
State MCTS<State, Agent>::determinize(const State& state) const {
    State newState(state);
    newState.randomizeHiddenState();
    return newState;
}

template<typename State, typename Agent>
typename MCTS<State, Agent>::NodePtr
MCTS<State, Agent>::select(MCTS::NodePtr node, State& state) const {
    std::vector<MovePtr> legalMoves = state.getMoves();
    std::vector<MovePtr> untried = node->getUntriedMoves(legalMoves);

    while (!state.isTerminal()) {
        if (!untried.empty()) {
            return expand(node, state, untried);
        }

        node = node->UCBSelectChild(legalMoves, this->exploration);
        state.makeMove(node->move);

        legalMoves = state.getMoves();
        untried = node->getUntriedMoves(legalMoves);
    }

    return node;
}

template<typename State, typename Agent>
typename MCTS<State, Agent>::NodePtr
MCTS<State, Agent>::expand(MCTS::NodePtr node, State& state, const std::vector<MovePtr>& untried) const {
    MovePtr move = untried[(rand() % untried.size())];
    int justMoved = state.playerToMove;
    state.makeMove(move);
    return node->addChild(move, justMoved);
}

template<typename State, typename Agent>
void MCTS<State, Agent>::rollout(State& state, const Agent& agent) const {
    while (!state.isTerminal()) {
        MovePtr move = agent.getMove(state);
        state.makeMove(move);
    }
}

template<typename State, typename Agent>
void MCTS<State, Agent>::makeMove(const MovePtr& move) {
    NodePtr child;

    for (const NodePtr& n : root->children) {
        if (n->move == move) {
            child = n;
            break;
        }
    }

    if (!child) {
        child = root->addChild(move, root_state.playerToMove);
    }

    root = child;
    root_state.makeMove(move);
}

template<typename State, typename Agent>
MCTS<State, Agent>::Node::Node(MovePtr m, MCTS::Node::Ptr parent, int p):
        move(m), parent(parent), just_moved(p), wins(0.0), visits(0), avails(1) {

}

template<typename State, typename Agent>
void MCTS<State, Agent>::Node::update(const State& state) {
    visits += 1;

    if (just_moved != -1)
        wins += state.getResult(just_moved);
}

template<typename State, typename Agent>
typename MCTS<State, Agent>::NodePtr MCTS<State, Agent>::Node::addChild(MovePtr m, int p) {
    NodePtr node = std::make_shared<Node>(m, this->shared_from_this(), p);
    children.push_back(node);
    return node;
}

template<typename State, typename Agent>
std::vector<typename State::MovePtr>
MCTS<State, Agent>::Node::getUntriedMoves(const std::vector<MovePtr>& legalMoves) const {
    std::unordered_set<MovePtr, SharedHash<Move>, SharedEqual<Move>> tried;
    tried.reserve(children.size());

    for (auto it = children.begin(); it != children.end(); ++it) {
        tried.emplace((*it)->move);
    }

    std::vector<MovePtr> untried;

    for (const MovePtr& m : legalMoves) {
        auto it = tried.find(m);
        if (it == tried.end())
            untried.push_back(m);
    }

    return untried;
}

template<typename State, typename Agent>
typename MCTS<State, Agent>::NodePtr
MCTS<State, Agent>::Node::UCBSelectChild(const std::vector<MovePtr>& legalMoves, double exploration) const {
    std::unordered_set<MovePtr, SharedHash<Move>, SharedEqual<Move>> moves(legalMoves.begin(), legalMoves.end());
    std::vector<NodePtr> legalChildren;

    for (const NodePtr& n : children) {
        if (moves.find(n->move) != moves.end())
            legalChildren.push_back(n);
    }

    std::vector<double> ucbs;
    ucbs.reserve(legalChildren.size());

    for (const NodePtr& n : legalChildren) {
        ucbs.push_back(n->ucb());
    }

    double max = ucbs.at(0);
    long long ind = 0;

    for (long long i = 1; i < ucbs.size(); ++i) {
        if (ucbs.at(i) > max) {
            max = ucbs.at(i);
            ind = i;
        }
    }

    for (const NodePtr& n : legalChildren) {
        n->avails += 1;
    }

    return legalChildren.at(ind);
}

template<typename State, typename Agent>
double MCTS<State, Agent>::Node::ucb(double exploration) const {
    return static_cast<double>(wins) / static_cast<double>(visits) +
           +exploration * sqrt(log(avails) / static_cast<double>(visits));
}

template<typename State>
typename State::MovePtr RandomAgent<State>::getMove(State& state) const {
    return state.randomMove();
}

#endif //MCTS_MCTS_H