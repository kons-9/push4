#include "push4_headder.h"
#define DISPLAY
// #define DEBUG
// #define TRAIN

// c++の練習

// 今後の改善点(やるとしたら)
// 実際はstateの三進数を2->1,1->2とすれば片方だけで良い
// memの使わないところが多いのでbinaryファイル多分結構小さくできる
// 2重にloopが絡んでくると確定できないと思う．1212,2121,1212,2121見たいのはできる？
// 配列をarrayに変える


class Environment {
  private:
    const State_type *pow_three;
    Player_val_type board[16] = {};

    State_type *gen_pow_three() {
        State_type *pow_three = (State_type *)malloc(sizeof(State_type) * 16);
        State_type pow = 1;
        for (size_t i = 0; i < 16; i++) {
            pow_three[i] = pow;
            pow *= 3;
        }
        return pow_three;
    }

  public:
    Environment()
        : pow_three(gen_pow_three()) {
        std::cout << "start\n";
    }
    ~Environment() {
        std::cout << "end\n";
    }
#ifdef DEBUG
    void set_board(Player_val_type li[4][4]) {
        for (size_t i = 0; i < 4; i++) {
            for (size_t j = 0; j < 4; j++) {
                board[board_index(i, j)] = li[i][j];
            }
        }
    }
#endif
    void reset() {
        for (size_t i = 0; i < 16; i++) {
            board[i] = 0;
        }
    }

    size_t board_index(const size_t x, const size_t y) {
        return x * 4 + y;
    }

    Player_val_type get_board(const size_t x, const size_t y) const {
        return board[x * 4 + y];
    }

    State_type board_to_state() {
        // 3進数で状態を表現
        State_type state = 0;
        for (size_t i = 0; i < 16; i++) {
            state += ((State_type)board[i] * pow_three[i]);
        }
        return state;
    }

    Player_val_type step(const Action_type action, const Player_val_type val) {
        // action: 0-3までの数値
        // val: 先攻1 or 後攻2
        // return 追い出された要素
        Player_val_type kicked = board[board_index(3, action)];
        for (size_t i = 0; i < 3; i++) {
            board[board_index(3 - i, action)] = board[board_index(2 - i, action)];
        }
        board[action] = val;
        return kicked;
    }
    Player_val_type redo(const Action_type action, const Player_val_type val) {
        // action: 列の場所
        // val: 追い出された数値
        // return: 行ったaction
        Player_val_type ret = board[action];
        for (size_t i = 0; i < 3; i++) {
            board[board_index(i, action)] = board[board_index(i + 1, action)];
        }
        board[board_index(3, action)] = val;
        return ret;
    }
    Judge_type judge() {
        // ななめ
        bool judge[3] = {false, false, false};
        Player_val_type val = board[board_index(0, 0)];
        judge[val] |= (val == board[board_index(1, 1)]) && (val == board[board_index(2, 2)])
                      && (val == board[board_index(3, 3)]);
        val = board[board_index(0, 3)];
        judge[val] |= (val == board[board_index(1, 2)]) && (val == board[board_index(2, 1)])
                      && (val == board[board_index(3, 0)]);
        // たて
        for (size_t i = 0; i < 4; i++) {
            val = board[board_index(0, i)];
            judge[val] |= (val == board[board_index(1, i)]) && (val == board[board_index(2, i)])
                          && (val == board[board_index(3, i)]);
        }
        // よこ
        for (size_t i = 0; i < 4; i++) {
            val = board[board_index(i, 0)];
            judge[val] |= (val == board[board_index(i, 1)]) && (val == board[board_index(i, 2)])
                          && (val == board[board_index(i, 3)]);
        }
        if (judge[1] && judge[2]) {
            // 引き分け
            return 3;
        } else if (judge[1]) {
            // 先攻の勝ち
            return 1;
        } else if (judge[2]) {
            // 後攻の勝ち
            return 2;
        } else {
            // まだ終わらん
            return 0;
        }
    }
};

// std::cout << envを可能にする
std::ostream &operator<<(std::ostream &os, const Environment &it) {
    for (size_t i = 0; i < 4; i++) {
        for (size_t j = 0; j < 4; j++) {
            os << (int)it.get_board(i, j) << ' ';
        }
        os << '\n';
    }

    return os;
}

struct Memory {
    // 未確定0, 勝ち1, 負け2, 引き分け3, loop4
    Judge_type judge;
    //確定時のアクション
    Action_type action;
    // actionごとに確定があるか
    Action_type action_val[4];
    // 深さ優先探索で通ったか
    bool is_passed;
    // 負けの時の最大の長さのaction
    Action_type lose_action;
    // 負けの時の最大の長さ
    Path_len_type lose_path_len;
    // 引き分けのaction
    Action_type draw_action;
    // loopのaction
    Action_type loop_action;
    Memory() {
        for (size_t i = 0; i < 4; i++) {
            action_val[i] = 0;
        }
        judge = 0;
        is_passed = false;
        lose_path_len = 0;
    }
    bool operator==(Memory obj) {
        bool flag = judge == obj.judge;
        flag &= action == obj.action;
        flag &= is_passed == obj.is_passed;
        flag &= lose_action == obj.lose_action;
        flag &= lose_path_len == obj.lose_path_len;
        flag &= draw_action == obj.draw_action;
        flag &= loop_action == obj.loop_action;
        for (size_t i = 0; i < 4; i++) {
            flag &= action_val[i] == obj.action_val[i];
        }
        return flag;
    }

    void renew(Judge_type judge,
               Action_type action,
               Action_type action_val[4],
               bool is_passed,
               Action_type lose_action,
               Path_len_type lose_path_len,
               Action_type draw_action,
               Action_type loop_action) {
        Memory::judge = judge;
        Memory::action = action;
        for (size_t i = 0; i < 4; i++) {
            Memory::action_val[i] = action_val[i];
        }
        Memory::is_passed = is_passed;
        Memory::lose_action = lose_action;
        Memory::lose_path_len = lose_path_len;
        Memory::draw_action = draw_action;
        Memory::loop_action = loop_action;
    }

    static constexpr bool binary = true;

    void load(std::ifstream &ifs) {
        if (binary) {
            for (size_t i = 0; i < 4; i++) {
                ifs.read(reinterpret_cast<char *>(&action_val[i]), sizeof action_val[i]);
            }

            ifs.read(reinterpret_cast<char *>(&action), sizeof action);
            ifs.read(reinterpret_cast<char *>(&lose_action), sizeof lose_action);
            ifs.read(reinterpret_cast<char *>(&draw_action), sizeof draw_action);
            ifs.read(reinterpret_cast<char *>(&loop_action), sizeof loop_action);
            ifs.read(reinterpret_cast<char *>(&is_passed), sizeof is_passed);
            ifs.read(reinterpret_cast<char *>(&lose_path_len), sizeof lose_path_len);
            ifs.read(reinterpret_cast<char *>(&judge), sizeof judge);
        } else {
            for (size_t j = 0; j < 4; j++) {
                ifs >> action_val[j];
            }

            ifs >> action >> lose_action >> draw_action >> loop_action;
            ifs >> is_passed >> lose_path_len >> judge;
        }
    }
    void save(std::ofstream &ofs) {
        if (binary) {
            for (size_t i = 0; i < 4; i++) {
                ofs.write(reinterpret_cast<char *>(&action_val[i]), sizeof action_val[i]);
            }

            ofs.write(reinterpret_cast<char *>(&action), sizeof action);
            ofs.write(reinterpret_cast<char *>(&lose_action), sizeof lose_action);
            ofs.write(reinterpret_cast<char *>(&draw_action), sizeof draw_action);
            ofs.write(reinterpret_cast<char *>(&loop_action), sizeof loop_action);
            ofs.write(reinterpret_cast<char *>(&is_passed), sizeof is_passed);
            ofs.write(reinterpret_cast<char *>(&lose_path_len), sizeof lose_path_len);
            ofs.write(reinterpret_cast<char *>(&judge), sizeof judge);
        } else {
            for (size_t j = 0; j < 4; j++) {
                ofs << action_val[j] << ' ';
            }

            ofs << action << ' ' << lose_action << ' ' << draw_action << ' ' << loop_action << ' ';
            ofs << is_passed << ' ' << lose_path_len << ' ' << judge << '\n';
        }
    }
};

class Agent_mem {
  private:
    static constexpr State_type MAX_STATE_SIZE = 43046721;
    Memory *first_mem;
    Memory *second_mem;

  public:
    Memory &get_memory(const State_type s, const Player_val_type player) {
        if (player == 1) {
            return first_mem[s];
        } else {
            return second_mem[s];
        }
    }
    Agent_mem() {
        first_mem = new Memory[MAX_STATE_SIZE];
        second_mem = new Memory[MAX_STATE_SIZE];
    }
    Agent_mem(std::string filename, int level) {
        first_mem = new Memory[MAX_STATE_SIZE];
        second_mem = new Memory[MAX_STATE_SIZE];
        if (level == 5) {
            load_mem(filename);
        }
    }
    ~Agent_mem() {
        delete[] first_mem;
        delete[] second_mem;
    }
    void load_mem(const std::string &file_path) {
        // 読み込む
        std::ifstream ifs(file_path, std::ios::binary);
        if (ifs.fail()) {
            std::cout << "Cannot load. Start using initialialized memory..." << std::endl;
            return;
        }
        for (size_t i = 0; i < MAX_STATE_SIZE; i++) {
            first_mem[i].load(ifs);
        }
        for (size_t i = 0; i < MAX_STATE_SIZE; i++) {
            second_mem[i].load(ifs);
        }
    }
    void save_mem(const std::string &file_path) {

        std::ofstream ofs(file_path, std::ios::binary);
        if (ofs.fail()) {
            std::cout << "Cannot save. sorry." << std::endl;
            return;
        }

        for (size_t i = 0; i < MAX_STATE_SIZE; i++) {
            first_mem[i].save(ofs);
        }
        for (size_t i = 0; i < MAX_STATE_SIZE; i++) {
            second_mem[i].save(ofs);
        }
        ofs << std::endl;
    }
};

class Agent {
  private:
    static constexpr Path_len_type levels[6] = {1, 3, 5, 7, 9, 100};
    static constexpr Action_type action[4] = {0, 1, 2, 3};

    Agent_mem &mem;
    const Player_val_type val;
    bool is_save = false;
    const int MAX_LEVEL = 5;
    int level;

    // 乱数生成器を用意する
    std::random_device seed_gen;
    std::mt19937 engine{seed_gen()};

  public:
    const Path_len_type MAX_DEPTH;
    Agent(Player_val_type val, Agent_mem &mem, size_t level = 2)
        : mem(mem)
        , val(val)
        , MAX_DEPTH(levels[level]) {
    }

  private:
    std::pair<Judge_type, Path_len_type> search(Environment &env, Player_val_type player_val, Path_len_type depth = 0) {
        // player_valのターン
        // return: judge, lose_path_len

        size_t state = env.board_to_state();

        Memory &state_mem = mem.get_memory(state, player_val);
        // loopしているか
        if (state_mem.is_passed) {
            return {4, 0};
        }
        // 確定済みの状態であるか
        if (state_mem.judge) {
            return {state_mem.judge, state_mem.lose_path_len};
        }

        // 状態が終わりかどうか
        // judgeが判断するのは先攻が勝ちか後攻が勝ちということだけなので
        // それが自分の値と等しいのかを判断する必要がある
        auto judge = env.judge();
        if (judge) {
            if (judge == 3) {
                state_mem.judge = 3;
            } else if (player_val == judge) {
                state_mem.judge = 1;
            } else {
                state_mem.judge = 2;
            }
            return {state_mem.judge, 0};
        }

        // max depthと比べる
        if (depth == MAX_DEPTH) {
            // 探索長不足，未確定
            return {0, 0};
        }

        // 探索開始
        bool flag_no_search = false;
        bool flag_draw = false;
        bool flag_loop = false;
        for (size_t i = 0; i < 4; i++) {
            Action_type act = action[i];
            // 再帰
            state_mem.is_passed = true;
            Player_val_type kicked = env.step(act, player_val);
            auto [opponent_judge, lose_turn] = search(env, 3 - player_val, depth + 1);
            env.redo(act, kicked);
            state_mem.is_passed = false;

            // 結果をもとにmemをupdate
            Judge_type my_judge = update_mem(state_mem, opponent_judge, lose_turn, act, depth);
            switch (my_judge) {
            case 0:
                flag_no_search = true;
                break;
            case 1:
                return {1, state_mem.lose_path_len};
            case 3:
                flag_draw = true;
                break;
            case 4:
                flag_loop = true;
                break;
            default:
                break;
            }
        }
        // 勝ちの経路は見当たらなかった
        if (flag_no_search) {
            //まだ未探索のところがある
            return {0, 0};
        } else if (flag_draw) {
            // 引き分けがある
            state_mem.judge = 3;
            state_mem.action = state_mem.draw_action;
            return {3, 0};
        } else if (flag_loop) {
            // 勝ち，未探索，引き分け以外に取れる行動がloopだけ
            if (depth == 0) {
                //ループが形成されるのでmemを書き換える
                loop_update_mem(env, player_val);
            }
            return {4, 0};
        } else {
            // 負けしかない
            state_mem.action = state_mem.lose_action;
            state_mem.judge = 2;
            return {2, state_mem.lose_path_len};
        }
    }

    void loop_update_mem(Environment &env, Player_val_type player_val) {
        size_t state = env.board_to_state();
        Memory &state_mem = mem.get_memory(state, player_val);

        if (state_mem.judge == 4) {
            return;
        }

        Action_type action;
        // for (size_t i = 0; i < 4; i++) {
        //     if (state_mem.action_val[i] == 4) {
        //         action = state_mem.loop_action;
        //     }
        // }


        action = state_mem.loop_action;
        state_mem.judge = 4;
        // state_mem.action = action;
        state_mem.action = state_mem.loop_action;

        Player_val_type tmp = env.step(action, player_val);
        loop_update_mem(env, 3 - player_val);
        env.redo(action, tmp);
    }

    Judge_type update_mem(Memory &state_mem,
                          Judge_type opponent_judge,
                          Path_len_type lose_turn,
                          Action_type action,
                          Path_len_type depth) {
        // judge_val, lose_turn
        switch (opponent_judge) {
        case 1:
            //相手が勝ち，自分は負け
            state_mem.action_val[action] = 2;
            if (state_mem.lose_path_len < lose_turn + 1) {
                state_mem.lose_path_len = lose_turn + 1;
                state_mem.lose_action = action;
            }
            return {2};
            break;
        case 2:
            // 相手負け，自分は勝ち
            state_mem.action = action;
            state_mem.judge = 1;
            state_mem.lose_path_len = lose_turn + 1;
// 冗長ではある
#ifdef DEBUG
            state_mem.action_val[action] = 1;
#endif
            return {1};
            break;
        case 3:
            // 引き分け
            state_mem.action_val[action] = 3;
            state_mem.draw_action = action;
            return {3};
            break;
        case 4:
            // loop
            // 未確定のactionがあればそちらを優先するのでループしない
            for (size_t i = 0; i < 4; i++) {
                if (i != action && state_mem.action_val[i] == 0) {
                    return {0};
                }
            }
            state_mem.loop_action = action;
            if (depth == 0) {
                // 元の場所まで戻ってきた
                state_mem.action_val[action] = 4;
            }
            return {4};

            break;
        default:
            // 未確定
            return 0;
            break;
        }
    }


  public:
    bool is_confirm(Environment &env) {
        return mem.get_memory(env.board_to_state(), Agent::val).judge != 0;
    }

    Action_type select_action(Environment &env) {

        search(env, Agent::val);

        size_t state = env.board_to_state();
        Memory &state_mem = mem.get_memory(state, Agent::val);

        if (state_mem.judge) {
#ifdef DISPLAY
            std::cout << "confirmed:val:" << (int)Agent::val << ' ' << "judge:" << (int)state_mem.judge << ' '
                      << "if_lose:" << (int)state_mem.lose_path_len << std::endl;
#endif
            return state_mem.action;
        }
        // 未確定がある.
        // 未確定はrandomにchoice
        std::vector<Action_type> not_confirm_actions;
        for (size_t action = 0; action < 4; action++) {
            if (state_mem.action_val[action] == 0) {
                not_confirm_actions.push_back(action);
            }
        }
        std::vector<Action_type> result;
        std::sample(not_confirm_actions.begin(), not_confirm_actions.end(), std::back_inserter(result), 1, engine);
        return result[0];
        // return 0;
    }

    Action_type *get_action_val(State_type state, State_type val) {
        return mem.get_memory(state, val).action_val;
    }
};


class Push4 {
  private:
    Environment env;
    Agent_mem mem;
    Agent first;
    Agent second;
    const int MAX_TURN = 100;

  public:
    Push4(int level = 2)
        : mem()
        , first(1, mem, level)
        , second(2, mem, level) {
    }
    Push4(std::string file_name, int level = 5)
        : mem(file_name, level)
        , first(1, mem, level)
        , second(2, mem, level) {
    }
    void load(std::string string) {
        mem.load_mem(string);
    }
    void sample() {
        env.reset();
        Action_type action;
        for (int i = 0; i < MAX_TURN; i++) {
            std::cout << env << std::endl;
            action = first.select_action(env);
            env.step(action, 1);

            switch (env.judge()) {
            case 1:
                std::cout << "player win!!" << std::endl;
                std::cout << env;
                return;
            case 2:
                std::cout << "player lose!!" << std::endl;
                std::cout << env;
                return;
            default:
                break;
            }

            std::cout << env << std::endl;
            action = second.select_action(env);
            env.step(action, 2);
            switch (env.judge()) {
            case 1:
                std::cout << "player win!!" << std::endl;
                std::cout << env;
                return;
            case 2:
                std::cout << "player lose!!" << std::endl;
                std::cout << env;
                return;
            default:
                break;
            }
        }
    }
    void pvp() {
        env.reset();
        Action_type action;
        for (int i = 0; i < MAX_TURN; i++) {
            do {
                int tmp;
                std::cout << env;
                first.select_action(env);
                std::cout << "Please input 1-4: ";
                std::cin >> tmp;
                action = tmp;
            } while (action < 1 || action > 4);
            std::cout << "your select: " << (int)action << std::endl;
            action--;
            env.step(action, 1);
            switch (env.judge()) {
            case 1:
                std::cout << "player win!!" << std::endl;
                std::cout << env;
                return;
            case 2:
                std::cout << "player lose!!" << std::endl;
                std::cout << env;
                return;
            default:
                break;
            }
            do {
                int tmp;
                std::cout << env;
                second.select_action(env);
                std::cout << "Please input 1-4: ";
                std::cin >> tmp;
                action = tmp;
            } while (action < 1 || action > 4);
            std::cout << "your select: " << (int)action << std::endl;
            action--;
            env.step(action, 2);
            switch (env.judge()) {
            case 1:
                std::cout << "player win!!" << std::endl;
                std::cout << env;
                return;
            case 2:
                std::cout << "player lose!!" << std::endl;
                std::cout << env;
                return;
            default:
                break;
            }
        }
    }
    void vs_player(Player_val_type val) {
        env.reset();
        if (val == 1) {
            std::cout << "Player vs computer" << std::endl;
            Action_type action;
            for (int i = 0; i < MAX_TURN; i++) {
                do {
                    int tmp;
                    std::cout << env;
                    std::cout << "Please input 1-4: ";
                    std::cin >> tmp;
                    action = tmp;
                } while (action < 1 || action > 4);
                std::cout << "your select: " << (int)action << std::endl;
                action--;
                env.step(action, val);
                switch (env.judge()) {
                case 1:
                    std::cout << "player win!!" << std::endl;
                    std::cout << env;
                    return;
                case 2:
                    std::cout << "player lose!!" << std::endl;
                    std::cout << env;
                    return;
                default:
                    break;
                }

                std::cout << env;
                action = second.select_action(env);
                std::cout << "conputer select: " << action + 1 << std::endl;
                env.step(action, 3 - val);
                switch (env.judge()) {
                case 1:
                    std::cout << "player win!!" << std::endl;
                    std::cout << env;
                    return;
                case 2:
                    std::cout << "player lose!!" << std::endl;
                    std::cout << env;
                    return;
                default:
                    break;
                }
            }

        } else if (val == 2) {
            std::cout << "computer vs Player" << std::endl;
            Action_type action;
            for (int i = 0; i < MAX_TURN; i++) {
                std::cout << env;
                action = first.select_action(env);
                std::cout << "conputer select: " << action + 1 << std::endl;
                env.step(action, 3 - val);
                switch (env.judge()) {
                case 2:
                    std::cout << "player win!!" << std::endl;
                    return;
                case 1:
                    std::cout << "player lose!!" << std::endl;
                    return;
                default:
                    break;
                }
                do {
                    int tmp;
                    std::cout << env;
                    std::cout << "Please input 1-4: ";
                    std::cin >> tmp;
                    action = tmp;
                } while (action < 1 || action > 4);
                action--;
                env.step(action, val);
                switch (env.judge()) {
                case 2:
                    std::cout << "player win!!" << std::endl;
                    return;
                case 1:
                    std::cout << "player lose!!" << std::endl;
                    return;
                default:
                    break;
                }
            }
        } else if (val == 3) {
            pvp();
        } else if (val == 4) {
            sample();
        } else {
            std::cout << "invalid player type." << std::endl;
            return;
        }
    }
#ifdef TRAIN
    void train(const std::string &file_path, const int num_train = 1000000, const int depth = 100) {
        Action_type action;

        for (int j = 0; j < num_train; j++) {
            if (j % 1000 == 0) {
                std::cout << "step: " << j << std::endl;
            }

            env.reset();
            for (size_t i = 0; i < 100; i++) {
                if (first.is_confirm(env))
                    break;
                action = first.select_action(env);
                env.step(action, 1);
                if (env.judge()) {
                    break;
                }
                if (second.is_confirm(env))
                    break;
                action = second.select_action(env);
                env.step(action, 2);
                if (env.judge()) {
                    break;
                }
            }
        }
        mem.save_mem(file_path);
    }
#endif
#ifdef DEBUG
    void all_test() {
        Memory_load_save_test();
        player_test();
    }
    bool list_test(Environment &env, Agent &player, Player_val_type list[4][4], Action_type expect_list[4]) {
        env.set_board(list);
        auto state = env.board_to_state();
        player.select_action(env);
        Action_type *action_val = player.get_action_val(state, 1);
        for (size_t i = 0; i < 4; i++) {
            if (expect_list[i] != 0 && action_val[i] != expect_list[i]) {
                return 1;
            }
        }
        return 0;
    }

    void player_test() {
        Agent &player = first;
        bool flag = true;
        static constexpr int num_int = 6;
        Player_val_type lists[num_int][4][4] = {{{1, 1, 1, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
                                                {{2, 2, 2, 1}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
                                                {{1, 1, 1, 2}, {2, 2, 2, 0}, {}, {}},
                                                {{2, 1, 2, 2}, {2, 2, 2, 1}, {}, {}},
                                                {{1, 2, 2, 0}, {2, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}},
                                                {{2, 2, 2, 1}, {1, 0, 0, 0}, {2, 0, 0, 0}, {1, 0, 0, 0}}};

        Action_type expect_lists[num_int][4] = {{0, 0, 0, 1}, {0, 0, 0, 2}, {0, 0, 0, 3}, {2, 2, 2, 2}, {}, {}};

        for (size_t i = 0; i < num_int; i++) {
            if (list_test(env, player, lists[i], expect_lists[i])) {
                flag = false;
                std::cout << "not passed " << i << '\n';
            }
        }

        if (flag) {
            std::cout << "all passed" << '\n';
        }
    }

    void ifst(std::string file_path, Memory &m) {
        Action_type tmp[4] = {1, 1, 1, 1};
        m.renew(2, 2, tmp, false, 1, 1000, 1, 1);
        std::ofstream ofs(file_path, std::ios::binary);
        if (ofs.fail()) {
            std::cout << "Cannot save. sorry." << std::endl;
            return;
        }
        m.save(ofs);
    }
    void Memory_load_save_test() {

        std::string file_path = "./hello.push4";
        Memory m;
        Memory mm;
        ifst(file_path, m);

        std::ifstream ifs(file_path, std::ios::binary);
        if (ifs.fail()) {
            std::cout << "Cannot load. Start using initialialized memory..." << std::endl;
            return;
        }
        mm.load(ifs);


        if (m == mm) {
            std::cout << "passed" << std::endl;
        } else {
            std::cout << "not passed" << std::endl;
        }
    }
#endif
};
int main() {

    std::string file_path = "./memory.push4";
#ifdef DEBUG
    Push4 d_game;
    d_game.all_test();
    return 0;
#endif
#ifdef TRAIN
    Push4 t_game(file_path);
    t_game.train(file_path);
    return 0;
#endif
    int level;
    int val;
    int restart = 1;

    std::cout << "select level 0-5 >> ";
    std::cin >> level;
    std::cout << "select 1(first),2(second) >> ";
    std::cin >> val;

    Push4 game(level);
    if (level >= 5) {
        // 本気モード
        std::cout << "loading...";
        game.load(file_path);
    }

    while (restart) {
        game.vs_player(val);
        std::cout << "if you want to restart, push 1 >> ";
        std::cin >> restart;
    }

    return 0;
}