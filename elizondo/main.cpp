#include <string>
#include <random>
#include <vector>
#include <map>
#include <iostream>

#include "greedy_player.hpp"
#include "player_controller.hpp"

int main(int argc, char **argv ) {
    cout << "-> main" << endl;
    player_controller<greedy_player> player(argv[1]);
    player.play();
    return 0;
}