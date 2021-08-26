#ifndef TICTACTOE_TICTACTOE_H
#define TICTACTOE_TICTACTOE_H

#include <string>
#include <vector>
#include <stdexcept>
#include <algorithm>

typedef unsigned int uint;

namespace ttt {
    class State {
    private:
        uint player = 0;
        uint opponent = 0;
        uint occupied = 0;
        bool terminal = false;
        int score = 0;

        State(uint player, uint opponent, uint occupied, bool terminal, int score, int turn);

        void checkTerminal(); // checks whether the state is terminal and sets 'terminal' and 'score' variables
        static bool checkWin(uint state); // checks whether the state is winning
        void swap(State& other);

    public:
        struct Move {
            int m;

            Move(int i): m(i) {}

            Move(const Move& other): m(other.m) {}

            Move(const Move&& other): m(other.m) {}

            Move& operator=(const Move& other) {
                m = other.m;
                return *this;
            }

            operator int() const { return m; }

            bool operator==(const Move& other) const { return other.m == m; }

            static Move null() { return Move(-1); }

            bool isNull() const { return (m == -1); }
        };

        const int numberOfPlayers = 2;
        int playerToMove = 1;

        State() = default;
        State(uint player, uint opponent);
        ~State() = default;
        State(const State& other);
        State& operator=(const State& other);

        std::vector<std::pair<State, Move>> getMovesAndStates() const; // returns vector of pairs {new_state, move_to_this_state}
        std::vector<Move> getMoves() const; // returns vector of legal moves
        bool isTerminal() const; // returns whether the node is terminal
        int getScore() const; // returns score of ended game; -1 - opponent wins, 0 - draw, 1 - player wins.
        // If the game isn't ended, the behaviour is undefined
        double getScore(int p) const;
        void makeMove(Move move); // move: int from 0 to 8
        bool checkMove(Move move) const; // return whether the move is correct
        Move randomMove() const;

        std::string print() const;

        void randomizeHiddenState() {};
    };
}

namespace std {
    template<>
    struct hash<ttt::State::Move> {
        inline size_t operator()(const ttt::State::Move& m) const {
            return m.m;
        }
    };
}

#endif //TICTACTOE_TICTACTOE_H
