#include <array>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iostream>
#include <limits>
#include <list>
#include <utility>
#include <vector>

// typedef std::pair<int, int> Point;
struct Point {
    int x, y;
    Point() : Point(0, 0) {}
    Point(int x, int y) : x(x), y(y) {}
    bool operator==(const Point& rhs) const {
        return x == rhs.x && y == rhs.y;
    }
    bool operator!=(const Point& rhs) const {
        return !operator==(rhs);
    }
    Point operator+(const Point& rhs) const {
        return Point(x + rhs.x, y + rhs.y);
    }
    Point operator-(const Point& rhs) const {
        return Point(x - rhs.x, y - rhs.y);
    }
};

enum SPOT_STATE {
    EMPTY = 0,
    BLACK = 1,
    WHITE = 2
};
const int SIZE = 8;
const int MAXITER = 10000;
const int MAX_DURATION = 2000;
const double DIV_DELTA = 1e-9;
const std::array<Point, 8> directions{{Point(-1, -1), Point(-1, 0), Point(-1, 1),
                                       Point(0, -1), /*{0, 0}, */ Point(0, 1),
                                       Point(1, -1), Point(1, 0), Point(1, 1)}};

struct Node {
    int partial_wins;
    int partial_games;
    int* total_games;
    bool isLeaf;
    bool expanded;

    int cur_player;  // 1: O, 2: X
    std::array<std::array<int, SIZE>, SIZE> board;
    std::array<int8_t, 3> disc_count;
    Node* parent;
    std::vector<Node*> children;
    std::vector<Point> next_spots;

    Node(std::array<std::array<int, SIZE>, SIZE>& board, int player) {
        this->partial_wins = 0;
        this->partial_games = 0;
        this->total_games = &(this->partial_games);
        this->isLeaf = true;
        this->expanded = false;

        this->cur_player = player;
        this->board = board;
        this->disc_count[0] = this->disc_count[1] = this->disc_count[2] = 0;
        for (int i = 0; i < SIZE; i++) {
            for (int j = 0; j < SIZE; j++) {
                this->disc_count[this->board[i][j]]++;
            }
        }
        this->parent = nullptr;
    }
    Node(Node* node) {
        this->partial_wins = 0;
        this->partial_games = 0;
        this->total_games = node->total_games;
        this->isLeaf = true;
        this->expanded = false;

        this->cur_player = node->cur_player;
        this->board = node->board;
        this->disc_count[0] = node->disc_count[0];
        this->disc_count[1] = node->disc_count[1];
        this->disc_count[2] = node->disc_count[2];
        this->parent = nullptr;
    }
    ~Node() {
        for (auto child : children) {
            delete child;
        }
    }
};

bool isTerminal(Node* node);

void monte_carlo_tree_search(Node* root, std::chrono::_V2::system_clock::time_point start_time);
Node* traversal(Node* root);
void expansion(Node* node);
int rollout(Node* node, int player);
void backPropagation(Node* node, int win);

int main(int argc, char** argv) {
    assert(argc == 3);
    srand(time(NULL));
    std::ifstream fin(argv[1]);
    std::ofstream fout(argv[2]);
    int player;
    std::array<std::array<int, SIZE>, SIZE> board;
    // === Read state ===
    fin >> player;
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            fin >> board[i][j];
        }
    }
    // === Write move ===

    Node* root = new Node(board, player);
    auto start_time = std::chrono::high_resolution_clock::now();
    monte_carlo_tree_search(root, start_time);
    auto end_time = std::chrono::high_resolution_clock::now();
    int duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
    printf("duration of mcts: %d milliseconds\n", duration);

    Point p;
    double max_val = -std::numeric_limits<double>::max();
    for (int idx = 0; idx < root->children.size(); idx++) {
        double val = (double)root->children[idx]->partial_wins / (root->children[idx]->partial_games + DIV_DELTA);
        if (val > max_val) {
            max_val = val;
            p = root->next_spots[idx];
        }
    }
    fout << p.x << " " << p.y << std::endl;
    fout.flush();

    /* Node* node = root;
    while (node != nullptr) {
        int selection;
        std::cout << "isLeaf: " << node->isLeaf << "\n";
        std::cout << "cur_player: " << node->cur_player << "\n";
        std::cout << "disc_count: " << (int)node->disc_count[0] << ", " << (int)node->disc_count[1] << ", " << (int)node->disc_count[2] << "\n";
        std::cout << node->partial_wins << "/" << node->partial_games << "\n";
        std::cout << "+---------------+\n";
        for (int i = 0; i < SIZE; i++) {
            std::cout << "|";
            for (int j = 0; j < SIZE; j++) {
                if (j != 0)
                    std::cout << " ";
                if (board[i][j] == 1) {
                    std::cout << "O";
                } else if (board[i][j] == 2) {
                    std::cout << "X";
                } else {
                    std::cout << " ";
                }
            }
            std::cout << "|\n";
        }
        std::cout << "+---------------+\n";
        std::cout << "child size: " << node->children.size() << "\n";
        for (int i = 0; i < node->children.size(); i++) {
            std::cout << i << ": " << node->children[i]->partial_wins << "/" << node->children[i]->partial_games << " = " << (double)node->children[i]->partial_wins / (node->children[i]->partial_games + DIV_DELTA) << "\n";
        }
        std::cin >> selection;
        if (selection >= node->children.size() || selection < 0)
            node = node->parent;
        else
            node = node->children[selection];
    } */

    // === Finalize ===
    fin.close();
    fout.close();
    delete root;
    return 0;
}

bool isTerminal(Node* node) {
    if (node->disc_count[0] != 0) {
        int toCheck = node->disc_count[0];
        for (int i = 0; i < SIZE && toCheck; i++) {
            for (int j = 0; j < SIZE && toCheck; j++) {
                if (node->board[i][j] == EMPTY) {
                    toCheck--;
                    for (auto dir : directions) {
                        Point p = Point(i, j) + dir;
                        if (0 <= p.x && p.x < SIZE && 0 <= p.y && p.y < SIZE) {
                            int firstPly = node->board[p.x][p.y];
                            while (0 <= p.x && p.x < SIZE && 0 <= p.y && p.y < SIZE && node->board[p.x][p.y] != EMPTY) {
                                if (node->board[p.x][p.y] != firstPly)
                                    return false;
                                p = p + dir;
                            }
                        }
                    }
                }
            }
        }
    }
    return true;
}

void monte_carlo_tree_search(Node* root, std::chrono::_V2::system_clock::time_point start_time) {
    int iteration = 0;
    while (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start_time).count() < MAX_DURATION) {
        iteration++;
        Node* curnode;
        curnode = traversal(root);
        if (!isTerminal(curnode) && curnode->partial_games != 0) {
            expansion(curnode);
            curnode = curnode->children[0];
        }
        int win = rollout(curnode, root->cur_player);
        backPropagation(curnode, win);
    }
    std::cout << "iterations: " << iteration << "\n";
    std::cout << root->partial_wins << "/" << root->partial_games << " = " << (double)root->partial_wins / (root->partial_games + DIV_DELTA) << "\n";
}

Node* traversal(Node* root) {
    Node* curnode = root;
    while (!curnode->isLeaf) {
        Node* tmpnode = nullptr;
        double max_UCT = 0;
        for (auto child : curnode->children) {
            double tmp = ((double)child->partial_wins / (child->partial_games + DIV_DELTA)) + sqrt(2 * log(*child->total_games) / (child->partial_games + DIV_DELTA));
            if (tmp > max_UCT) {
                max_UCT = tmp;
                tmpnode = child;
            }
        }
        curnode = tmpnode;
    }
    return curnode;
}

void expansion(Node* node) {
    if (node->expanded) {
        node->isLeaf = false;
        return;
    }
    if (isTerminal(node))
        return;

    node->isLeaf = false;

    if (node->next_spots.empty())
        for (int i = 0; i < SIZE; i++) {
            for (int j = 0; j < SIZE; j++) {
                if (node->board[i][j] == EMPTY) {
                    bool point_availible = false;
                    for (auto dir : directions) {
                        // Move along the direction while testing.
                        Point p = Point(i, j) + dir;
                        if (0 <= p.x && p.x < SIZE && 0 <= p.y && p.y < SIZE) {
                            if (node->board[p.x][p.y] != 3 - node->cur_player)
                                continue;
                            p = p + dir;
                            while (0 <= p.x && p.x < SIZE && 0 <= p.y && p.y < SIZE && node->board[p.x][p.y] != EMPTY) {
                                if (node->board[p.x][p.y] == node->cur_player) {
                                    point_availible = true;
                                    break;
                                }
                                p = p + dir;
                            }
                        }
                    }
                    if (point_availible) {
                        node->next_spots.emplace_back(i, j);
                    }
                }
            }
        }

    if (node->next_spots.size() == 0) {
        Node* child_node = new Node(node);
        child_node->cur_player = 3 - child_node->cur_player;
        child_node->parent = node;
        node->children.emplace_back(child_node);
    } else {
        for (int spotIdx = 0; spotIdx < node->next_spots.size(); spotIdx++) {
            Node* child_node = new Node(node);
            for (auto dir : directions) {
                Point p = node->next_spots[spotIdx] + dir;

                while (0 <= p.x && p.x < SIZE && 0 <= p.y && p.y < SIZE && node->board[p.x][p.y] != EMPTY) {
                    if (node->board[p.x][p.y] == node->cur_player) {
                        p = p - dir;
                        while (p != node->next_spots[spotIdx]) {
                            child_node->disc_count[child_node->cur_player]++;
                            child_node->disc_count[3 - child_node->cur_player]--;
                            child_node->board[p.x][p.y] = child_node->cur_player;
                            p = p - dir;
                        }
                        break;
                    }
                    p = p + dir;
                }
            }
            child_node->disc_count[child_node->cur_player]++;
            child_node->disc_count[0]--;
            child_node->board[node->next_spots[spotIdx].x][node->next_spots[spotIdx].y] = child_node->cur_player;
            child_node->cur_player = 3 - child_node->cur_player;
            child_node->parent = node;
            node->children.emplace_back(child_node);
        }
    }
    node->expanded = true;
}

int rollout(Node* node, int player) {
    Node* curnode = node;
    while (!isTerminal(curnode)) {
        expansion(curnode);
        node->isLeaf = true;
        curnode = curnode->children[rand() % curnode->children.size()];
    }
    return (curnode->disc_count[player] - curnode->disc_count[3 - player] > 0 ? 1 : 0);
}

void backPropagation(Node* node, int win) {
    Node* curnode = node;
    while (curnode != nullptr) {
        curnode->partial_wins += win;
        curnode->partial_games++;
        curnode = curnode->parent;
    }
}

/*
g++ --std=c++17 -O3 -Wextra -Wall -fsanitize=address -g player.cpp -o player_dbg; ./player_dbg testcase/state0 action
g++ --std=c++17 -O3 player.cpp -o player_2sec
g++ --std=c++17 -O3 player.cpp -o player_9sec
g++ --std=c++17 -O3 player.cpp -o player; ./player testcase/state0 action
make; ./main ./player_9sec ./player_2sec
make; ./main ./player ./players/player_random
make; ./main ./player ./players/baseline1
make; ./main ./player ./players/baseline2
make; ./main ./player ./players/baseline3
make; ./main ./player ./players/baseline4
make; ./main ./player ./players/baseline5
make; ./main ./players/player_random ./player
make; ./main ./players/baseline1 ./player
make; ./main ./players/baseline2 ./player
make; ./main ./players/baseline3 ./player
make; ./main ./players/baseline4 ./player
make; ./main ./players/baseline5 ./player
make; ./main ./players/player_109062131 ./players/baseline1
make; ./main ./players/player_109062131 ./players/baseline2
make; ./main ./players/player_109062131 ./players/baseline3
make; ./main ./players/player_109062131 ./players/baseline4
make; ./main ./players/player_109062131 ./players/baseline5
make; ./main ./player ./players/player_109062131
make; ./main ./players/player_109062131 ./player
 */