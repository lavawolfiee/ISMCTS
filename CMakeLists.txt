cmake_minimum_required(VERSION 3.17)
project(MCTS)

set(CMAKE_CXX_STANDARD 17)

add_executable(MCTS main.cpp tictactoe.h tictactoe.cpp MCTS.h Durak.h)