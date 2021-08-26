#include <iostream>
#include "Durak.h"
#include "MCTS.h"
#include <ctime>
#include <chrono>

using std::cout;
using std::endl;
using std::cin;

using State = DurakState;
using Move = State::Move;
using MovePtr = State::MovePtr;

using AttackMove = State::AttackMove;
using DefendMove = State::DefendMove;
using ThrowInMove = State::ThrowInMove;
using Card = State::Card;

/*std::tuple<int, int, int, double, double> tournament(State::Move (*g)(const State&)) {
    int firstWins = 0;
    int secondWins = 0;
    int draws = 0;
    double t1 = 0;
    double t2 = 0;

    for (int i = 0; i < 100; ++i) {
        State s;
        MCTS<State> mcts;
        int turn = 1 - 2 * (rand() % 2);

        while (!s.isTerminal()) {
            if (turn == 1) {
                auto start = std::chrono::high_resolution_clock::now();
                State::Move move = mcts.getMove(2'500);
                s.makeMove(move);
                mcts.makeMove(move);
                auto end = std::chrono::high_resolution_clock::now();
                std::chrono::duration<double> ms_double = end - start;
                t1 += ms_double.count();
            } else {
                auto start = std::chrono::high_resolution_clock::now();
                Move move = g(s);
                s.makeMove(move);
                auto end = std::chrono::high_resolution_clock::now();
                std::chrono::duration<double> ms_double = end - start;
                t2 += ms_double.count();
                mcts.makeMove(move);
            }

            turn = -turn;
        }

        if (s.getResult() == 0) {
            ++draws;
        } else if (turn * s.getResult() == 1) {
            ++firstWins;
        } else {
            ++secondWins;
        }
    }

    return {firstWins, draws, secondWins, t1, t2};
}

std::pair<int, int> negamax(const State& s) {
    if (s.isTerminal())
        return {-s.getResult(), -1};

    int value = std::numeric_limits<int>::min() / 2;
    int best_move = -1;

    for (auto& [state, move] : s.getMovesAndStates()) {
        int new_value = negamax(state).first;

        if (new_value > value) {
            value = new_value;
            best_move = move;
        }
    }

    return {-value, best_move};
}

State::Move bestMove(const State& s) {
    auto [value, move] = negamax(s);
    return move;
}*/

int main() {
    // srand(time(nullptr));
    srand(5);

    /*State s;

    std::shared_ptr<DurakState::AttackMove> move = std::make_shared<DurakState::AttackMove>();
    move->cards.push_back(s.getHands().front().front());

    s.makeMove(move);*/

    /*State s;
    std::cout << s.toString();

    MCTS<State> mcts;
    MovePtr move = mcts.getMove();
    std::cout << static_cast<std::string>(*move) << std::endl;*/

    /*auto [f, d, s, t1, t2] = tournament(&bestMove);
    int total = f + d + s;

    printf("MCTS won %d of %d games (%.2f%%)\n", f, total, static_cast<double>(f) / total * 100);
    printf("Draws: %d games (%.2f%%)\n", d, static_cast<double>(d) / total * 100);
    printf("MCTS lost %d of %d games (%.2f%%)\n", s, total, static_cast<double>(s) / total * 100);
    printf("MCTS took %.2fs\n", t1);
    printf("Minimax took %.2fs\n", t2);*/

    std::vector<Card> deck;
    std::vector<std::vector<Card>> hands =
            {{Card("KS", false), Card("QS", false), Card("JS", false),
                     Card("10S", false), Card("9S", false), Card("8S", false),
                     Card("7S", false), Card("6S", false)},
             {Card("AS", false), Card("AC", false), Card("AH", false),
                     Card("AD", false)}};
    std::vector<Card> attack;
    std::vector<std::pair<Card, Card>> defended;
    std::vector<Card> discard;
    int trump = 0;
    bool defending = false;
    int defendingPlayer = -1;
    int attackingPlayer = -1;
    int playerToMove = 1;

    State s(deck, hands, attack, defended, discard, trump, defending, defendingPlayer, attackingPlayer, playerToMove);
    MCTS<State> mcts(0.7, s);

    /*std::cout << std::endl << "Initial state:" << std::endl << s.toString() << std::endl << std::endl;

    MovePtr move = mcts.getMove();

    std::cout << "The MCTS made the following move:" << std::endl << static_cast<std::string>(*move)
              << std::endl << std::endl;

    s.makeMove(move);
    mcts.makeMove(move);

    std::cout << std::endl << "State after the move:" << std::endl << s.toString() << std::endl << std::endl;*/

    int player = 2;

    std::cout << std::endl << "Initial state:" << std::endl << s.toString() << std::endl << std::endl;

    while (!s.isTerminal()) {
        if (s.playerToMove == player) {
            cout << "Current state: " << std::endl << s.toString() << endl;
            cout << "Enter move: " << std::flush;

            std::string str;
            getline(cin, str);
            MovePtr move = DurakState::stringToMove(str);

            s.makeMove(move);
            mcts.makeMove(move);

            std::cout << std::endl << "State after your move:" << std::endl << s.toString() << std::endl << std::endl;
        } else {
            MovePtr move = mcts.getMove();

            std::cout << "The MCTS made the following move:" << std::endl << static_cast<std::string>(*move)
                      << std::endl << std::endl;

            s.makeMove(move);
            mcts.makeMove(move);
        }
    }

    if (s.getResult(player) == 1) {
        cout << "You won!" << endl;
    } else {
        cout << "You lost! (or draw)" << endl;
    }

    return 0;
}