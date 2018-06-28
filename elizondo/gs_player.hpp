#include <string>
#include <random>
#include <vector>
#include <map>
#include <fstream>

#include "board_status.hpp"
#include "constants.hpp"
#include "auxiliars.hpp"

using namespace std;

random_device rd;
mt19937 generator(rd());

class gs_player {
    
    int columns, rows, steps;
    string team, side;
    vector<pair <int, int> > own_goal;
    vector<pair <int, int> > opponnent_goal;
    vector<player> players;
    vector<player> opponnents;

public:

    gs_player() = default;

    gs_player(
        int columns,
        int rows,
        int steps,
        string side,
        const vector<player>& players,
        const vector<player>& opponnent_players
    ) {
        this->columns = columns;
        this->rows = rows;
        this->steps = steps;
        this->side = side;
        this->team = team;
        this->players = players;
        this->opponnents = opponnent_players;
        this->get_goal_positions();

    }

    void get_goal_positions() {
        int mid_row = this->rows / 2;
        int column_own_goal = this->columns;
        int column_opponnent_goal = -1;
        if (this->side == IZQUIERDA) {
            column_own_goal = -1;
            column_opponnent_goal = this->columns;
        }

        for (int i = -1; i < 2; i++) {
            own_goal.push_back(make_pair(mid_row + i, column_own_goal));
            opponnent_goal.push_back(make_pair(mid_row + i, column_opponnent_goal));
        }

    }

    void starting_positions(vector<player_status>& positions) {
        int column = this->columns - 1;
        if (this->side == IZQUIERDA) {
            column = 0;
        }

        for (int i = 0; i < 3; i++) {
            positions.emplace_back(i, i, column, false);
        }
    }

    // Aca se usa la función punteadora, greedy, genetica, etc
    void make_move(const board_status& current_board, vector<player_move>& made_moves) {
        made_moves.clear();
        player_move p_move;
        made_moves.push_back(p_move);
        made_moves.push_back(p_move);
        made_moves.push_back(p_move);

        vector<double> pesos = grid_search(current_board);
        vector<int> found_move = search_move(current_board, pesos);
        update_moves(current_board, made_moves, found_move[0], found_move[1], found_move[2], found_move[3], found_move[4]);
    }

    void finish(string result) { }

private:

    vector<int> search_move(const board_status& current_board, const vector<double>& pesos) {
        double max_rank = -9999999;
        double current_rank = -9999999;
        
        int i = 0;
        int j = 0;
        int k = 0;
        int jugador = 0;
        int steps = 0;
        vector<int> result = {i,j,k,jugador,steps};

        while (i < moves.size()) {
            if (inside_board(current_board.team[0], i, this->opponnent_goal, this->rows, this->columns)) {        
                while (j < moves.size()) {
                    if (inside_board(current_board.team[1], j, this->opponnent_goal, this->rows, this->columns)) {
                        while (k < moves.size()) {
                            if (inside_board(current_board.team[2], k, this->opponnent_goal, this->rows, this->columns)) {
                                vector<int> player_moves { i, j, k };
                                for (int jugador = 0; jugador < 3; jugador++) {
                                    vector<int> player_moves_for_pass { i, j, k };
                                    player_moves_for_pass[jugador] = 0;
                                    if (current_board.team[jugador].in_posetion && player_moves[jugador] != 0 && in_different_positions(current_board.team, player_moves_for_pass)) {
                                        int max_steps = calculate_max_steps(current_board.team[jugador], player_moves[jugador], this->rows, this->columns, this->opponnent_goal);
                                        for (int steps = 1; steps <= max_steps; steps++) {
                                            current_rank = evaluate_board(current_board, i, j, k, pesos, jugador, steps);
                                            if (current_rank > max_rank) {
                                                max_rank = current_rank;
                                                result = {i,j,k,jugador,steps};
                                            }
                                        }
                                    }
                                }
                                if (in_different_positions(current_board.team, player_moves)) {
                                    current_rank = evaluate_board(current_board, i, j, k, pesos);
                                    if (current_rank > max_rank) {
                                        max_rank = current_rank;
                                        result = {i,j,k,0,0};
                                    }
                                }
                            }
                            k++;    
                        }
                        k = 0;
                    }
                    j++;    
                }
                j = 0;
            }
            i++;
        }
        return result;
    }

    void update_moves(const board_status& current_board, vector<player_move>& made_moves, int p0, int p1, int p2, int player_with_ball = 0, int steps = 0) {
        //si nadie patea todos tienen step 0
        vector<int> steps_players(3, 0);
        //si algun jugador pateo, actualizo su step
        steps_players[player_with_ball] = steps;

        vector<int> player_moves = {p0,p1,p2};

        for (int i = 0; i < 3; i++) {
            made_moves[i].player_id = current_board.team[i].id;
            made_moves[i].move_type = steps_players[i] == 0 ? MOVIMIENTO : PASE;
            made_moves[i].dir = player_moves[i];
            made_moves[i].steps = steps_players[i];
        }
    }

    double evaluate_board(const board_status& current_board, int i, int j, int k, const vector<double> &pesos, int jugador = 0, int steps = 0) {
        
        board_status updated_board = update_board(current_board, i, j, k, jugador, steps);

        double result = 0;

        for (auto goal : this->opponnent_goal) {
            if (get_ball_position(updated_board) == goal) {
                result = 999999;
            }
        }

        if (who_has_the_ball(updated_board) == "GREEDY") {
            for (auto p : updated_board.team) {
                result -= distance_player_opponnent_goal(p, this->opponnent_goal) * pesos[0]; //distancia al arco contrario
                result += distance_player_closest_opponnent(updated_board, p) * pesos[1]; //distancia al oponente mas cercano
                result += dispersion(updated_board.team) * pesos[2];
            }
        } 
        
        if (who_has_the_ball(updated_board) == "OPPONNENT") {
            for (auto p : updated_board.team) {
                result -= distance_player_ball(updated_board, p) * pesos[3]; //distancia a la pelota
                result -= distance_player_closest_opponnent(updated_board, p) * pesos[4]; //distancia al oponente mas cercano
                result += dispersion(updated_board.team) * pesos[5];
            }
        }

        if (who_has_the_ball(updated_board) == "FREE") {
            for (auto p : updated_board.team) {
                result -= distance_player_ball(updated_board, p) * pesos[6]; //distancia a la pelota
                result -= distance_player_opponnent_goal(p, this->opponnent_goal) * pesos[7]; //distancia al arco contrario
                result += dispersion(updated_board.team) * pesos[8];
            }
            for (auto p : updated_board.oponent_team) {
                result += distance_player_ball(updated_board, p) * pesos[9]; //distancia a la pelota del contrario
            }
        }

        return result;
    }

    board_status update_board(const board_status& current_board, int i, int j, int k, int jugador = 0, int steps = 0) {
        board_status updated_board = current_board;

        // si nadie tenia la pelota
        if (updated_board.ball.is_free) {
            // si venia moviendose actualizo su posicion
            if (updated_board.ball.steps > 0) {
                updated_board.ball.i += 2*moves[updated_board.ball.dir].first;
                updated_board.ball.j += 2*moves[updated_board.ball.dir].second;
            }
        }

        updated_board.team[0].i += moves[i].first;
        updated_board.team[0].j += moves[i].second;
        updated_board.team[1].i += moves[j].first;
        updated_board.team[1].j += moves[j].second;
        updated_board.team[2].i += moves[k].first;
        updated_board.team[2].j += moves[k].second;

        vector<int> player_moves { i, j, k };
        for (auto p : updated_board.team) {
            if (p.id == jugador && steps > 0) {
                p.in_posetion = false;
                updated_board.ball.is_free = true;
                updated_board.ball.steps = steps;
                updated_board.ball.dir = player_moves[jugador];
                updated_board.ball.i += 2*moves[player_moves[jugador]].first;
                updated_board.ball.j += 2*moves[player_moves[jugador]].second;
                p.i -= moves[player_moves[jugador]].first;
                p.j -= moves[player_moves[jugador]].second;
            }
        }
        
        for (int h = 0; h < 3; h++) {
            if (current_board.ball.is_free 
                && updated_board.ball.is_free 
                && updated_board.team[h].i == updated_board.ball.i 
                && updated_board.team[h].j == updated_board.ball.j 
                && player_moves[h] == 0) {
                updated_board.ball.is_free = false;
                updated_board.team[h].in_posetion = true;
            }
        }

        for (int h = 0; h < 3; h++) {
            if (current_board.ball.is_free 
                && updated_board.ball.is_free 
                && updated_board.oponent_team[h].i == updated_board.ball.i 
                && updated_board.oponent_team[h].j == updated_board.ball.j 
                && player_moves[h] == 0) {
                updated_board.ball.is_free = false;
                updated_board.oponent_team[h].in_posetion = true;
            }
        }

        return updated_board;
    }

    vector<double> grid_search(const board_status& current_board) {
        vector<double> pesos (10, 0);
        int turnos = 10;

        for (double a = 0.3; a <= 1; a += 0.3) { // en vez de hacer 10 for anidados se puede hacer recursivamente pero que lo piense otro
            for (double b = 0.3; b <= 1; b += 0.3) {
                for (double c = 0.3; c <= 1; c += 0.3) {
                    for (double d = 0.3; d <= 1; d += 0.3) {
                        for (double e = 0.3; e <= 1; e += 0.3) {
                            for (double f = 0.3; f <= 1; f += 0.3) {
                                for (double g = 0.3; g <= 1; g += 0.3) {
                                    for (double h = 0.3; h <= 1; h += 0.3) {
                                        for (double i = 0.3; i <= 1; i += 0.3) {
                                            for (double j = 0.3; j <= 1; j += 0.3) {
                                                //jugar hasta que haga un gol y guardar la cantidad de TURNOS
                                                // hay que llamar SEARCH MOVE Y UPDATE BOARD reiteradas veces para esto
                                                // es posible que no haga un gol y que haga un gol el contrario
                                                // (tal vez en el caso de que hace un gol el contrario que sea en la mayor cantidad de turnos posibles)
                                                int current_turns = 0;

                                                vector<double> pesos_tmp = {a,b,c,d,e,f,g,h,i,j};
                                                board_status updated_board = current_board;
                                                while(current_turns < turnos) {
                                                    vector<int> player_moves = search_move(updated_board, pesos_tmp);
                                                    updated_board = update_board(updated_board, player_moves[0], player_moves[1], player_moves[2], player_moves[3], player_moves[4]);
                                                    current_turns++;
                                                    if (scored(updated_board, this->opponnent_goal)) {
                                                        break;
                                                    }
                                                }

                                                //si la cantidad de TURNOS actual es menor que la minima hasta ahora, actualizar TURNOS y PESOS
                                                // if (current_turns < turnos) {
                                                    pesos = pesos_tmp;
                                                    // turnos = current_turns;
                                                // }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        return pesos;
    }
};

// string fileName = "searchmove.log";
// ofstream fs;
// fs.open(fileName, fstream::out);
// fs << "search move" << endl;
// for (auto c : current_board.team) {
//     fs << "id: " << c.id << " i: " << c.i << " j: " << c.j << " ball: " << c.in_posetion << endl;
// }
// for (auto c : current_board.oponent_team) {
//     fs << "id: " << c.id << " i: " << c.i << " j: " << c.j << " ball: " << c.in_posetion << endl;
// }
// for (auto d : pesos) {
//     fs << d << endl;
// }
// fs.close();

// string fileName = "gs.log";
// ofstream fs;
// fs.open(fileName, fstream::out);
// fs << "inspect updated board" << endl;
// for (auto c : updated_board.team) {
//     fs << "id: " << c.id << " i: " << c.i << " j: " << c.j << " ball: " << c.in_posetion << endl;
// }
// for (auto c : updated_board.oponent_team) {
//     fs << "id: " << c.id << " i: " << c.i << " j: " << c.j << " ball: " << c.in_posetion << endl;
// }
// fs.close();