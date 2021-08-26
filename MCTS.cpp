template<typename State, typename Agent>
MCTS<State, Agent>::MCTS(double exploration, const State& state, const Agent& agent):
        root(std::make_shared<Node>(Move::null(), nullptr, -1)),
        root_state(state), exploration(exploration), agent(agent) {

}

template<typename State, typename Agent>
typename State::Move MCTS<State, Agent>::getMove() const {
    loop();

    if (root->children.empty())
        return Move::null();

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
    State state = determinize(initial, 0);

    for (size_t i = 1; i <= iters; ++i)
        state = iterate(node, state, i);
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
    newState->randomizeHiddenState();
    return newState;
}

template<typename State, typename Agent>
typename MCTS<State, Agent>::NodePtr
MCTS<State, Agent>::select(MCTS::NodePtr node, State& state) const {
    std::vector<Move> legalMoves = state->getMoves();
    std::vector<Move> untried = node->getUntriedMoves(legalMoves);

    while (!state.isTerminal()) {
        if (!untried.empty()) {
            return expand(node, state, untried);
        }

        node = node->UCBSelectChild(legalMoves, this->exploration);
        state.makeMove(node->move);

        legalMoves = state->getMoves();
        untried = node->getUntriedMoves(legalMoves);
    }

    return node;
}

template<typename State, typename Agent>
typename MCTS<State, Agent>::NodePtr
MCTS<State, Agent>::expand(MCTS::NodePtr node, State& state, const std::vector<Move>& untried) const {
    Move move = untried[(rand() % untried.size())];
    int justMoved = state.playerToMove;
    state.makeMove(move);
    return node->addChild(move, justMoved);
}

template<typename State, typename Agent>
void MCTS<State, Agent>::rollout(State& state, Agent& agent) const {
    while (!state.isTerminal()) {
        Move move = agent.getMove(state);
        state.makeMove(move);
    }
}

template<typename State, typename Agent>
void MCTS<State, Agent>::makeMove(const Move& move) {
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
MCTS<State, Agent>::Node::Node(Move m, MCTS::Node::Ptr parent, int p):
        move(m), parent(parent), just_moved(p), wins(0.0), visits(0), avails(1) {

}

template<typename State, typename Agent>
void MCTS<State, Agent>::Node::update(const State& state) {
    visits += 1;

    if (just_moved != -1)
        wins += state->getScore(just_moved);
}

template<typename State, typename Agent>
typename MCTS<State, Agent>::NodePtr MCTS<State, Agent>::Node::addChild(Move m, int p) {
    NodePtr node = std::make_shared<Node>(m, this->shared_from_this(), p);
    children.push_back(node);
    return node;
}

template<typename State, typename Agent>
std::vector<typename State::Move> MCTS<State, Agent>::Node::getUntriedMoves(const std::vector<Move>& legalMoves) const {
    std::unordered_set<Move> tried;
    tried.reserve(children.size());

    for (auto it = children.begin(); it != children.end(); ++it) {
        tried.emplace((*it)->move);
    }

    std::vector<Move> untried;

    for (const Move& m : legalMoves) {
        if (tried.find(m) == tried.end())
            untried.push_back(m);
    }

    return untried;
}

template<typename State, typename Agent>
typename MCTS<State, Agent>::NodePtr
MCTS<State, Agent>::Node::UCBSelectChild(const std::vector<Move>& legalMoves, double exploration) const {
    std::unordered_set<Move> moves(legalMoves);
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
           + exploration * sqrt(log(avails) / static_cast<double>(visits));
}

template<typename State>
typename State::Move RandomAgent<State>::getMove(const State& state) const {
    return state.randomMove();
}