#include "tictactoe.h"

ttt::State::State(uint player, uint opponent): player(player), opponent(opponent), occupied(player | opponent) {
    if ((player & opponent) || (player & ~0b111111111u) || (opponent & ~0b111111111u)) {
        throw std::runtime_error("Wrong state");
    }

    checkTerminal();
}

ttt::State::State(uint player, uint opponent, uint occupied, bool terminal, int score, int turn):
        player(player), opponent(opponent), occupied(occupied), terminal(terminal), score(score), playerToMove(turn) {

}

void ttt::State::makeMove(Move move) {
    if (!checkMove(move))
        throw std::runtime_error("Wrong move");

    occupied |= 1u << move;
    player |= 1u << move;

    checkTerminal();

    std::swap(player, opponent);
    score = -score;

    playerToMove = (playerToMove % numberOfPlayers) + 1;
}

bool ttt::State::isTerminal() const {
    return terminal;
}

int ttt::State::getScore() const {
    return score;
}

void ttt::State::checkTerminal() {
    if (checkWin(player)) {
        // current player wins
        terminal = true;
        score = 1;
    } else if (checkWin(opponent)) {
        // current player loses
        terminal = true;
        score = -1;
    } else if (occupied == 0b111111111u) {
        // draw
        terminal = true;
        score = 0;
    } else {
        terminal = false;
        score = 0;
    }
}

bool ttt::State::checkWin(uint state) {
    std::vector<uint> masks = {0b111000000u, 0b000111000u, 0b000000111u,
                               0b100100100u, 0b010010010u, 0b001001001u,
                               0b100010001u, 0b001010100u};

    return std::any_of(masks.begin(), masks.end(),
                       [state](uint mask) { return (mask & state) == mask; });
}

std::vector<std::pair<ttt::State, ttt::State::Move>> ttt::State::getMovesAndStates() const {
    if (terminal)
        return std::vector<std::pair<State, Move>>();

    std::vector<std::pair<State, Move>> moves;

    for (uint move = 0; move < 9; ++move) {
        if (!(occupied & (1u << move))) {
            State new_state(*this);
            new_state.makeMove(move);
            moves.emplace_back(new_state, move);
        }
    }

    return moves;
}

std::vector<ttt::State::Move> ttt::State::getMoves() const {
    if (terminal)
        return std::vector<Move>();

    std::vector<Move> moves;

    for (uint move = 0; move < 9; ++move) {
        if (!(occupied & (1u << move))) {
            moves.emplace_back(move);
        }
    }

    return moves;
}

ttt::State& ttt::State::operator=(const State& other) {
    State copy(other);
    swap(copy);
    return *this;
}

ttt::State::State(const State& other):
        State(other.player, other.opponent, other.occupied, other.terminal, other.score, other.playerToMove) {

}

std::string ttt::State::print() const {
    std::string s;

    for (uint p = 0; p < 9; ++p) {
        if ((player & (1u << p))) {
            s.push_back('X');
        } else if ((opponent & (1u << p))) {
            s.push_back('O');
        } else {
            s.push_back('.');
        }

        if (p % 3 == 2)
            s.push_back('\n');
    }

    return s;
}

bool ttt::State::checkMove(Move move) const {
    return !(move > 8 || (occupied & (1u << move)));
}

void ttt::State::swap(State& other) {
    std::swap(player, other.player);
    std::swap(opponent, other.opponent);
    std::swap(occupied, other.occupied);
    std::swap(terminal, other.terminal);
    std::swap(score, other.score);
    std::swap(playerToMove, other.playerToMove);
}

double ttt::State::getScore(int p) const {
    if (playerToMove == p)
        if (score == 1) {
            return 1;
        } else if (score == -1) {
            return 0;
        } else {
            return 0.1;
        }
    else
        if (score == 1)
            return 0;
        else if (score == -1)
            return 1;
        else
            return 0.5;
}

ttt::State::Move ttt::State::randomMove() const {
    std::vector<Move> moves = getMoves();
    return moves[rand() % moves.size()];
}
