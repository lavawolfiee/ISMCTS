#ifndef MCTS_DURAK_H
#define MCTS_DURAK_H

#include <utility>
#include <vector>
#include <algorithm>
#include <random>
#include <set>
#include <memory>

#define MOVES_CHECKING

class DurakState {
public:
    static const int numberOfCards = 36;
    static const int numberOfSuits = 4;
    static const int numberOfRanks = 36 / numberOfSuits;

    struct Card {
        int n;
        bool hidden;

        Card(int n, bool hidden = true): n(n), hidden(hidden) {}
        Card(int suit, int rank): n(rank * numberOfSuits + suit) {}
        Card(const std::string& s, bool hidden = true);

        operator int() const { return n; }

        int suit() const { return n % numberOfSuits; }

        int rank() const { return n / numberOfSuits; }

        bool isHidden() const { return hidden; }

        void reveal() { hidden = false; }

        explicit operator std::string() const {
            return ranks[rank()] + suits[suit()];
        }

        bool operator<(const Card& other) const { return n < other.n; }

        bool operator==(const Card& other) const { return n == other.n; }

        bool beat(const Card& other, int trump) const {
            if (suit() == trump) {
                if (other.suit() != trump)
                    return true;
                else return rank() > other.rank();
            } else if (other.suit() == trump || suit() != other.suit())
                return false;
            else return rank() > other.rank();
        }
    };

    static const std::string ranks[numberOfRanks];
    static const std::string suits[numberOfSuits];

    struct Move {
        bool _null = false;

        Move(): _null(true) {}

        static std::shared_ptr<Move> null() { return std::make_shared<Move>(); }

        bool isNull() const { return _null; }

        virtual explicit operator std::string() const { return "Null move"; }
    };

    using MovePtr = std::shared_ptr<Move>;

    struct AttackMove: public Move {
        std::vector<Card> cards;

        AttackMove() { _null = false; }

        explicit operator std::string() const override {
            std::string s = "Attack move: ";
            s += static_cast<std::string>(cards.front());

            for (size_t i = 1; i < cards.size(); ++i) {
                s += ", ";
                s += static_cast<std::string>(cards.at(i));
            }

            return s;
        }
    };

    struct DefendMove: public Move {
        std::vector<std::pair<Card, Card>> cards; // pairs of {what to beat, than to beat}
        bool giveUp;

        explicit DefendMove(bool giveUp = false): giveUp(giveUp) { _null = false; }

        explicit operator std::string() const override {
            if (giveUp)
                return "Giving up defend move";
            std::string s = "Defend move:\n";
            s += "- beat " + static_cast<std::string>(cards.front().first) + " with " +
                 static_cast<std::string>(cards.front().second);

            for (size_t i = 1; i < cards.size(); ++i) {
                s += "\n- beat " + static_cast<std::string>(cards.at(i).first) + " with " +
                     static_cast<std::string>(cards.at(i).second);
            }

            return s;
        }
    };

    struct ThrowInMove: public Move {
        std::vector<Card> cards;

        ThrowInMove() { _null = false; }

        explicit operator std::string() const override {
            std::string s = "Throw-in move: ";
            s += cards.front();

            for (size_t i = 1; i < cards.size(); ++i) {
                s += ", ";
                s += cards.at(i);
            }

            return s;
        }
    };

private:
    std::vector<Card> deck;
    std::vector<std::vector<Card>> hands;
    std::vector<Card> attack;
    std::vector<std::pair<Card, Card>> defended;
    std::vector<Card> discard;

    int trump = -1;
    bool defending = false;
    int defendingPlayer = -1;
    int attackingPlayer = -1;

    std::random_device rd_dev;
    std::mt19937 rd;

public:
    const int numberOfPlayers = 2;
    int playerToMove = 1;

    DurakState();
    DurakState(std::vector<Card> deck, std::vector<std::vector<Card>> hands,
               std::vector<Card> attack, std::vector<std::pair<Card, Card>> defended,
               std::vector<Card> discard, int trump, bool defending, int defendingPlayer, int attackingPlayer,
               int playerToMove);
    DurakState(const DurakState& other);
    DurakState(DurakState&& other) noexcept;
    DurakState& operator=(const DurakState& other);

    void makeMove(const MovePtr& m);
    void randomizeHiddenState();
    void randomizeHiddenState(int observer);
    double getResult(int player) const;
    bool isTerminal() const;

    std::vector<MovePtr> getMoves();
    MovePtr randomMove();

    const std::vector<std::vector<Card>>& getHands() const { return hands; }
    std::string toString() const;
    static MovePtr stringToMove(const std::string& s) ;

private:
    void swap(DurakState& other);
    void nextTurn();
};

DurakState::DurakState(): rd(5) {// rd(rd_dev()) {
    deck.reserve(numberOfCards);
    for (int i = 0; i < numberOfCards; ++i) {
        deck.emplace_back(i);
    }
    std::shuffle(deck.begin(), deck.end(), rd);

    size_t _numberOfCards = 6;
    for (size_t i = 0; i < numberOfPlayers; ++i) {
        hands.emplace_back(deck.begin() + i * _numberOfCards, deck.begin() + (i + 1) * _numberOfCards);
    }

    deck.erase(deck.begin(), deck.begin() + _numberOfCards * numberOfPlayers);

    if (numberOfPlayers * _numberOfCards >= numberOfCards) {
        trump = hands.back().back().suit();
        hands.back().back().reveal();
    } else {
        trump = deck.front().suit();
        deck.front().reveal();
    }
}

const std::string DurakState::ranks[numberOfRanks] = {"6", "7", "8", "9", "10", "J", "Q", "K", "A"};
// const std::string DurakState::Card::suits[numberOfSuits] = {"♠S", "♣C", "♥H", "♦D"};
const std::string DurakState::suits[numberOfSuits] = {"S", "C", "H", "D"};

DurakState::Card::Card(const std::string& s, bool hidden): hidden(hidden) {
    std::string r = s.substr(0, s.size() - 1);
    std::string _suit(1, s.back());
    int suit = -1;
    int rank = -1;

    for (int i = 0; i < numberOfSuits; ++i)
        if (suits[i] == _suit) {
            suit = i;
            break;
        }

    if (suit == -1)
        throw std::runtime_error("Bad suit in converting from string to Card");

    for (int i = 0; i < numberOfRanks; ++i)
        if (ranks[i] == r) {
            rank = i;
            break;
        }

    if (suit == -1)
        throw std::runtime_error("Bad rank in converting from string to Card");

    n = rank * numberOfSuits + suit;
}

DurakState::DurakState(const DurakState& other):
        deck(other.deck), hands(other.hands), attack(other.attack), defended(other.defended), discard(other.discard),
        trump(other.trump), defending(other.defending), defendingPlayer(other.defendingPlayer),
        attackingPlayer(other.attackingPlayer), rd(5), // rd(rd_dev()),
        numberOfPlayers(other.numberOfPlayers), playerToMove(other.playerToMove) {

}

DurakState::DurakState(DurakState&& other) noexcept:
        deck(std::move(other.deck)), hands(std::move(other.hands)), attack(std::move(other.attack)),
        defended(std::move(other.defended)), discard(std::move(other.discard)), trump(other.trump),
        defending(other.defending), defendingPlayer(other.defendingPlayer), attackingPlayer(other.attackingPlayer),
        rd(5), // rd(rd_dev()),
        numberOfPlayers(other.numberOfPlayers), playerToMove(other.playerToMove) {

}

void DurakState::swap(DurakState& other) {
    std::swap(deck, other.deck);
    std::swap(hands, other.hands);
    std::swap(attack, other.attack);
    std::swap(defended, other.defended);
    std::swap(discard, other.discard);
    std::swap(trump, other.trump);
    std::swap(defending, other.defending);
    std::swap(defendingPlayer, other.defendingPlayer);
    std::swap(rd, other.rd);
    std::swap(playerToMove, other.playerToMove);
}

DurakState& DurakState::operator=(const DurakState& other) {
    DurakState copy(other);
    swap(copy);
    return *this;
}

void DurakState::makeMove(const std::shared_ptr<DurakState::Move>& m) {
    std::shared_ptr<AttackMove> attackMove = std::dynamic_pointer_cast<AttackMove>(m);

    if (attackMove) {
#ifdef MOVES_CHECKING
        if (!attack.empty() && attackMove->cards.front().rank() != attack.front().rank())
            throw std::runtime_error("Bad attack move: can't remit using card with different rank");

        for (size_t i = 1; i < attackMove->cards.size(); ++i)
            if (attackMove->cards.front().rank() != attackMove->cards.at(i).rank())
                throw std::runtime_error("Bad attack move: all cards should have same rank");

        if (attackMove->cards.size() + attack.size() > playerToMove % numberOfPlayers) {
            throw std::runtime_error("Bad attack move: cannot remit "
            + std::to_string(attackMove->cards.size() + attack.size()) + " cards to a player with "
            + std::to_string(playerToMove % numberOfPlayers) + " cards");
        }
#endif

        std::vector<Card>& hand = hands.at(playerToMove - 1);

        if (attack.empty())
            attackingPlayer = playerToMove;

        for (auto& card : attackMove->cards) {
            attack.push_back(card);

            auto it = std::find(hand.begin(), hand.end(), card);
#ifdef MOVES_CHECKING
            if (it == hand.end())
                throw std::runtime_error("Bad attack move: player don't have card " + static_cast<std::string>(card) +
                                         " in his hand");
#endif
            hand.erase(it);

            attack.back().reveal();
        }

        defending = true;
        nextTurn();
        defendingPlayer = playerToMove;

        return;
    }

    // it isn't an attack move

    std::shared_ptr<DefendMove> defendMove = std::dynamic_pointer_cast<DefendMove>(m);

    if (defendMove) {
#ifdef MOVES_CHECKING
        if (playerToMove != defendingPlayer)
            throw std::runtime_error("Bad defend move: current player isn't a defending player");
#endif

        if (defendMove->giveUp) {
            // moving all cards from attack and defended to defendingPlayer's hand. Dealing card to players
            auto& hand = hands.at(defendingPlayer - 1);

            std::move(attack.begin(), attack.end(), std::back_inserter(hand));
            attack.clear();

            for (auto& it : defended) {
                hand.push_back(it.first);
                hand.push_back(it.second);
            }
            defended.clear();

            // dealing cards
            int total = 0;
            for (int player = attackingPlayer, i = 0; i < numberOfPlayers;
                 ++i, player = (player % numberOfPlayers) + 1) {
                auto& pHand = hands.at(player - 1);
                int toGet = 6 - static_cast<int>(pHand.size());

                if (toGet <= 0)
                    continue;

                if (total + toGet >= deck.size()) {
                    pHand.insert(pHand.end(), deck.begin(), deck.end() - total);
                    total = deck.size();
                    break;
                } else {
                    pHand.insert(pHand.end(), deck.end() - total - toGet, deck.end() - total);
                    total += toGet;
                }
            }

            deck.erase(deck.end() - total, deck.end());

            defending = false;
            defendingPlayer = -1;
            attackingPlayer = -1;
            nextTurn();
        } else if (attack.empty()) {
            // player defended against all the cards. Moving defended to discard and dealing cards to players

            for (auto& it : defended) {
                discard.push_back(it.first);
                discard.push_back(it.second);
            }
            defended.clear();

            // dealing cards
            int total = 0;
            for (int player = attackingPlayer, i = 0; i < numberOfPlayers;
                 ++i, player = (player % numberOfPlayers) + 1) {
                auto& hand = hands.at(player - 1);
                int toGet = 6 - static_cast<int>(hand.size());

                if (toGet <= 0)
                    continue;

                if (total + toGet >= deck.size()) {
                    hand.insert(hand.end(), deck.begin(), deck.end() - total);
                    total = deck.size();
                    break;
                } else {
                    hand.insert(hand.end(), deck.end() - total - toGet, deck.end() - total);
                    total += toGet;
                }
            }

            deck.erase(deck.end() - total, deck.end());

            defending = false;
            defendingPlayer = -1;
            attackingPlayer = -1;
            nextTurn();
        } else {
            // defending
            auto& hand = hands.at(defendingPlayer - 1);

            for (const std::pair<Card, Card>& p : defendMove->cards) {
                if (!p.second.beat(p.first, trump))
                    std::cout << "Can't beat!" << std::endl;

                auto it = std::find(attack.begin(), attack.end(), p.first);

#ifdef MOVES_CHECKING
                if (it == attack.end())
                    throw std::runtime_error(
                            "Bad defend move: where is no card " + static_cast<std::string>(p.first)
                            + " in attack");
#endif

                auto it2 = std::find(hand.begin(), hand.end(), p.second);
#ifdef MOVES_CHECKING
                if (it2 == hand.end())
                    throw std::runtime_error("Bad defend move: player don't have card " +
                                             static_cast<std::string>(p.second) + " in his hand");
#endif
                hand.erase(it2);

                attack.erase(it);
                defended.push_back(p);
                defended.back().first.reveal();
                defended.back().second.reveal();
            }

            nextTurn();
        }

        return;
    }
    // it isn't a defend move

    std::shared_ptr<ThrowInMove> throwInMove = std::dynamic_pointer_cast<ThrowInMove>(m);

    if (throwInMove) {
#ifdef MOVES_CHECKING
        if (playerToMove == defendingPlayer)
            throw std::runtime_error("Bad throw-in move: defending player can't throw-in");
#endif

        auto& hand = hands.at(playerToMove - 1);

        for (const Card& c : throwInMove->cards) {
#ifdef MOVES_CHECKING
            bool was = false;

            for (auto& card : attack) {
                if (card.rank() == c.rank()) {
                    was = true;
                    break;
                }
            }

            if (!was) {
                for (auto& it : defended) {
                    if (it.first.rank() == c.rank() || it.second.rank() == c.rank()) {
                        was = true;
                        break;
                    }
                }
            }

            if (!was)
                throw std::runtime_error("Bad throw-in move: there is no card " + static_cast<std::string>(c) +
                                         " in field");
#endif
            attack.push_back(c);
            attack.back().reveal();

            auto it = std::find(hand.begin(), hand.end(), c);
#ifdef MOVES_CHECKING
            if (it == hand.end())
                throw std::runtime_error(
                        "Bad throw-in move: player don't have card " + static_cast<std::string>(c) +
                        " in his hand");
#endif
            hand.erase(it);
        }

        nextTurn();

        return;
    }
    // it isn't a throw-in move

    // the move is _null which is weird. Don't need to do anything
}

void DurakState::nextTurn() {
    playerToMove = (playerToMove % numberOfPlayers) + 1;
}

void DurakState::randomizeHiddenState() {
    randomizeHiddenState(playerToMove);
}

void DurakState::randomizeHiddenState(int observer) {
    // folding all hidden cards in one deck, shuffling them and dealing back

    // folding
    std::vector<std::vector<Card>> newHands(hands.size());

    bool hasTrump = !deck.empty();

    for (size_t i = 0; i < numberOfPlayers; ++i) {
        for (const Card& card : hands.at(i)) {
            if (card.isHidden() && (observer != i + 1))
                deck.push_back(card);
            else
                newHands.at(i).push_back(card);
        }
    }

    if (hasTrump) {
        if (deck.size() > 2) {
            // shuffling all hidden cards (all in deck except first - trump)
            std::shuffle(deck.begin() + 1, deck.end(), rd);
        }
    } else if (deck.size() > 1) {
        std::shuffle(deck.begin(), deck.end(), rd);
    }

    // dealing back
    int total = 0;
    for (int player = 0; player < numberOfPlayers; ++player) {
        auto& hand = newHands.at(player);
        int toGet = static_cast<int>(hands.at(player).size()) - static_cast<int>(hand.size());

        hand.insert(hand.end(), deck.end() - total - toGet, deck.end() - total);
        total += toGet;
    }

    deck.erase(deck.end() - total, deck.end());

    hands = std::move(newHands);
}

bool DurakState::isTerminal() const {
    return std::any_of(hands.begin(), hands.end(),
                       [](const std::vector<Card>& hand) { return hand.empty(); });
}

double DurakState::getResult(int player) const {
    int win = 0;
    for (size_t i = 0; i < hands.size(); ++i) {
        if (hands.at(i).empty()) {
            win = i + 1;
            break;
        }
    }

    return (win == player);
}

std::vector<DurakState::MovePtr> DurakState::getMoves() {
    if (isTerminal())
        return std::vector<MovePtr>();

    std::vector<MovePtr> moves;

    std::vector<Card>& hand = hands.at(playerToMove - 1);

    if (defending) {
        if (playerToMove == defendingPlayer) {
            // remitting, defending or giving up

            // remitting
            if (std::all_of(attack.begin() + 1, attack.end(), [&attack = attack](const Card& c) {
                return attack.front().rank() == c.rank();
            }) && defended.empty()) {
                std::vector<Card> remitCards;

                for (const Card& card : hand)
                    if (card.rank() == attack.front().rank())
                        remitCards.push_back(card);

                int limit = hands.at(playerToMove % numberOfPlayers).size();

                for (size_t i = 1; i < (1u << remitCards.size()); ++i) {
                    std::shared_ptr<ThrowInMove> move = std::make_shared<ThrowInMove>();
                    for (size_t j = 0; j < remitCards.size(); ++j)
                        if (i & (1u << j))
                            move->cards.push_back(remitCards.at(j));

                    if (move->cards.size() <= limit)
                        moves.push_back(move);
                }

                /*for (const Card& card : hand) {
                    if (card.rank() == attack.front().rank()) {
                        std::shared_ptr<AttackMove> move = std::make_shared<AttackMove>();
                        move->cards.push_back(card);
                        moves.push_back(move);
                    }
                }*/
            }

            // giving up
            std::shared_ptr<DefendMove> gupMove = std::make_shared<DefendMove>(true);
            moves.push_back(gupMove);

            // defending
            std::sort(hand.begin(), hand.end(), [this](const Card& c1, const Card& c2) {
                if (c1.suit() == trump) {
                    if (c2.suit() == trump)
                        return c1.rank() < c2.rank();
                    else
                        return false;
                } else {
                    if (c2.suit() == trump)
                        return true;
                    else if (c1.rank() == c2.rank())
                        return c1.suit() < c2.suit();
                    else
                        return c1.rank() < c2.rank();
                }
            });

            std::vector<bool> beaten(attack.size(), false);
            std::shared_ptr<DefendMove> move = std::make_shared<DefendMove>();

            for (const Card& card : hand) {
                for (size_t i = 0; i < attack.size(); ++i) {
                    if (!beaten.at(i) && card.beat(attack.at(i), trump)) {
                        beaten.at(i) = true;
                        move->cards.emplace_back(attack.at(i), card);
                        break;
                    }
                }
            }

            if (std::find(beaten.begin(), beaten.end(), false) == beaten.end())
                moves.push_back(move);
        } else {
            // throwing-in
            std::vector<bool> allowedRanks(numberOfRanks, false);

            for (auto& [c1, c2] : defended) {
                allowedRanks[c1.rank()] = true;
                allowedRanks[c2.rank()] = true;
            }

            for (auto& c : attack)
                allowedRanks[c.rank()] = true;

            std::vector<std::vector<Card>> cardsByRanks(numberOfRanks);

            for (auto& card : hand)
                if (allowedRanks[card.rank()])
                    cardsByRanks.at(card.rank()).push_back(card);

            for (auto& rank : cardsByRanks) {
                for (size_t i = 1; i < (1u << rank.size()); ++i) {
                    std::shared_ptr<ThrowInMove> move = std::make_shared<ThrowInMove>();
                    for (size_t j = 0; j < rank.size(); ++j)
                        if (i & (1u << j))
                            move->cards.push_back(rank.at(j));
                    moves.push_back(move);
                }
            }

            /*for (auto& card : hand) {
                bool added = false;
                for (auto&[c1, c2] : defended)
                    if (card.rank() == c1.rank() || card.rank() == c2.rank()) {
                        std::shared_ptr<ThrowInMove> move = std::make_shared<ThrowInMove>();
                        move->cards.push_back(card);
                        moves.push_back(move);
                        added = true;
                        break;
                    }

                if (added)
                    continue;

                for (auto& c : attack)
                    if (c.rank() == card.rank()) {
                        std::shared_ptr<ThrowInMove> move = std::make_shared<ThrowInMove>();
                        move->cards.push_back(card);
                        moves.push_back(move);
                        break;
                    }
            }*/

            moves.push_back(std::make_shared<ThrowInMove>());
        }
    } else {
        // attacking
        std::vector<std::vector<Card>> cardsByRanks(numberOfRanks);

        for (auto& card : hand) {
            cardsByRanks.at(card.rank()).push_back(card);
        }

        for (auto& rank : cardsByRanks) {
            for (size_t i = 1; i < (1u << rank.size()); ++i) {
                std::shared_ptr<AttackMove> move = std::make_shared<AttackMove>();
                for (size_t j = 0; j < rank.size(); ++j)
                    if (i & (1u << j))
                        move->cards.push_back(rank.at(j));
                moves.push_back(move);
            }
        }

        /*for (auto& card : hand) {
            std::shared_ptr<AttackMove> move = std::make_shared<AttackMove>();
            move->cards.push_back(card);
            moves.push_back(move);
        }*/
    }

    return moves;
}

DurakState::MovePtr DurakState::randomMove() {
    if (isTerminal())
        return Move::null();

    std::vector<MovePtr> moves = getMoves();
    return moves[rd() % moves.size()];
}

std::string DurakState::toString() const {
    if (isTerminal()) {
        std::string s;

        s += "Game ended. Player ";
        s += std::to_string((playerToMove + numberOfPlayers - 2) % numberOfPlayers + 1);
        s += " won.";

        return s;
    } else {
        std::string s;
        auto& hand = hands.at(playerToMove - 1);

        s += "Player hand: ";
        s += static_cast<std::string>(hand.front());

        for (size_t i = 1; i < hand.size(); ++i) {
            s += ", ";
            s += static_cast<std::string>(hand.at(i));
        }

        s += "\nTrump: ";

        if (!deck.empty())
            s += static_cast<std::string>(deck.front());
        else
            s += suits[trump];

        if (defending) {
            if (playerToMove == defendingPlayer)
                s += "\nYou must defend\n";
            else
                s += "\nPlayer " + std::to_string(defendingPlayer) + " is defending. You can throw-in\n";

            if (attack.empty())
                s += "Attack is empty\n";
            else {
                s += "Attack: ";
                s += static_cast<std::string>(attack.front());

                for (size_t i = 1; i < attack.size(); ++i) {
                    s += ", ";
                    s += static_cast<std::string>(attack.at(i));
                }

                s += '\n';
            }

            if (defended.empty())
                s += "Defended is empty\n";
            else {
                s += "Defended: (";
                s += static_cast<std::string>(defended.front().first);
                s += "; ";
                s += static_cast<std::string>(defended.front().second);
                s += ")";

                for (size_t i = 1; i < defended.size(); ++i) {
                    s += ", (";
                    s += static_cast<std::string>(defended.at(i).first);
                    s += "; ";
                    s += static_cast<std::string>(defended.at(i).second);
                    s += ")";
                }
            }
        } else {
            s += "\nYou must attack";
        }

        return s;
    }
}

DurakState::MovePtr DurakState::stringToMove(const std::string& _s) {
    char first = _s.front();
    std::string s = _s.substr(2, _s.size() - 2);

    if (first == 'A') {
        // attack move
        std::shared_ptr<AttackMove> move = std::make_shared<AttackMove>();
        int last = -1;

        for (int i = 0; i <= s.size(); ++i) {
            if (i == s.size() || s.at(i) == ' ') {
                move->cards.emplace_back(s.substr(last + 1, i - last - 1));
                last = i;
            }
        }

        return move;
    } else if (first == 'D') {
        // defend move
        std::shared_ptr<DefendMove> move = std::make_shared<DefendMove>();

        if (s.substr(0, 6) == "GIVEUP") {
            move->giveUp = true;
            return move;
        }

        std::vector<Card> _cards;
        int last = -1;

        for (int i = 0; i <= s.size(); ++i) {
            if (i == s.size() || s.at(i) == ' ') {
                _cards.emplace_back(s.substr(last + 1, i - last - 1));
                last = i;
            }
        }

        if (_cards.size() % 2 != 0)
            throw std::runtime_error("Defend move description must have even number of cards");

        for (int i = 0; i < _cards.size(); i += 2)
            move->cards.emplace_back(_cards.at(i), _cards.at(i + 1));

        return move;
    } else if (first == 'T') {
        // throw-in move
        std::shared_ptr<ThrowInMove> move = std::make_shared<ThrowInMove>();

        int last = -1;

        for (int i = 0; i <= s.size(); ++i) {
            if (i == s.size() || s.at(i) == ' ') {
                move->cards.emplace_back(s.substr(last + 1, i - last - 1));
                last = i;
            }
        }

        return move;
    } else
        throw std::runtime_error("Bad move type during converting string to Move");
}

DurakState::DurakState(std::vector<Card> deck, std::vector<std::vector<Card>> hands,
                       std::vector<Card> attack, std::vector<std::pair<Card, Card>> defended,
                       std::vector<Card> discard, int trump, bool defending, int defendingPlayer,
                       int attackingPlayer, int playerToMove):
        deck(std::move(deck)), hands(std::move(hands)), attack(std::move(attack)),
        defended(std::move(defended)), discard(std::move(discard)), trump(trump), defending(defending),
        defendingPlayer(defendingPlayer), attackingPlayer(attackingPlayer), playerToMove(playerToMove) {

}

namespace std {
    template<>
    struct hash<DurakState::Card> {
    private:
        size_t a, b, p;

    public:
        hash() {
            std::mt19937 rd((std::random_device()()));
            a = rd();
            b = rd();
            p = 3'313'483'909u;
        }

        size_t operator()(const DurakState::Card& card) const {
            return ((a * static_cast<unsigned int>(card.n)) % p + b) % p;
        }
    };

    template<>
    struct hash<DurakState::Move> {
        size_t operator()(const DurakState::Move& m) const {
            std::hash<DurakState::Card> h;

            try {
                const auto& move = dynamic_cast<const DurakState::AttackMove&>(m);

                size_t value = 0;

                for (const DurakState::Card& c : move.cards)
                    value ^= h(c);

                return value;
            } catch (const std::bad_cast& ex) {
                // it isn't an attack move
            }

            try {
                const auto& move = dynamic_cast<const DurakState::DefendMove&>(m);

                size_t value = 0;

                for (const auto&[c1, c2] : move.cards) {
                    value ^= h(c1);
                    value ^= h(c2);
                }

                value = (value << 8u) || (value >> 24u);

                if (move.giveUp)
                    value ^= 0b100000001u;

                return value;
            } catch (const std::bad_cast& ex) {
                // it isn't an attack move
            }

            try {
                const auto& move = dynamic_cast<const DurakState::ThrowInMove&>(m);

                size_t value = 0;

                for (const DurakState::Card& c : move.cards)
                    value ^= h(c);

                return (value << 16u) || (value >> 16u);
            } catch (const std::bad_cast& ex) {
                // it isn't an attack move
            }

            return 0;
        }
    };
}

bool operator==(const DurakState::Move& move, const DurakState::Move& other) {
    try {
        auto m1 = dynamic_cast<const DurakState::AttackMove&>(move);

        try {
            auto m2 = dynamic_cast<const DurakState::AttackMove&>(other);

            std::set<DurakState::Card> set1(m1.cards.begin(), m1.cards.end());
            std::set<DurakState::Card> set2(m2.cards.begin(), m2.cards.end());

            return set1 == set2;
        } catch (const std::bad_cast& ex) {
            return false;
        }
    } catch (const std::bad_cast& ex) {}

    try {
        auto m1 = dynamic_cast<const DurakState::DefendMove&>(move);

        try {
            auto m2 = dynamic_cast<const DurakState::DefendMove&>(other);

            std::set<std::pair<DurakState::Card, DurakState::Card>> set1(m1.cards.begin(), m1.cards.end());
            std::set<std::pair<DurakState::Card, DurakState::Card>> set2(m2.cards.begin(), m2.cards.end());

            return set1 == set2;
        } catch (const std::bad_cast& ex) {
            return false;
        }
    } catch (const std::bad_cast& ex) {}

    try {
        auto m1 = dynamic_cast<const DurakState::ThrowInMove&>(move);

        try {
            auto m2 = dynamic_cast<const DurakState::ThrowInMove&>(other);

            std::set<DurakState::Card> set1(m1.cards.begin(), m1.cards.end());
            std::set<DurakState::Card> set2(m2.cards.begin(), m2.cards.end());

            return set1 == set2;
        } catch (const std::bad_cast& ex) {
            return false;
        }
    } catch (const std::bad_cast& ex) {}

    return move._null == other._null;
}

#endif //MCTS_DURAK_H