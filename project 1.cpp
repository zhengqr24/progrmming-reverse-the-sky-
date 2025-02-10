#include <iostream>
#include <vector>
#include <deque>
#include <cstdlib>
#include <ctime>
#include <conio.h>
#include <thread>
#include <chrono>
#include <fstream>
#include <sstream>
#include <random>

using namespace std;
using namespace chrono;

const int WIDTH = 100;
const int HEIGHT = 35;
const int NUM_ITEMS = 8;
const int INITIAL_DELAY = 150;
const int SCORE_PER_LEVEL = 50;
const string HIGHSCORE_FILE = "highscore.txt";

enum Direction { UP, DOWN, LEFT, RIGHT };
enum Weather { SUNNY, RAINY, SNOWY, THUNDERSTORM, WINDY, SMOG };
enum ItemType { NORMAL, BOOST, SLOWDOWN, SHIELD, DOUBLE_POINTS, REVERSE, MAGNET }; //食物种类
enum GameMode { CLASSIC, SURVIVAL }; // 添加生存模式
enum RandomEvent { NONE, FOOD_RESET, SUPER_FOOD, HELL_MODE, SCORE_RUSH };//随机事件

struct Point {
    int x, y;
    ItemType type;
};

class SnakeGame {
public:
    SnakeGame(int numObstacles, int initialDelay, bool isMultiplayer, GameMode gameMode)
        : direction(RIGHT), direction2(RIGHT), gameOver(false), paused(false), score1(0), score2(0),
        level(1), delay(initialDelay), numObstacles(numObstacles), isMultiplayer(isMultiplayer), weatherDuration(20), baseDelay(initialDelay),
        combo1(0), combo2(0),
        lastEatTime1(steady_clock::now()), lastEatTime2(steady_clock::now()),
        magnetActive1(false), magnetActive2(false), magnetDuration1(0), magnetDuration2(0){ // 初始化状态

        // 初始化玩家1
        snake.push_back({ WIDTH / 2, HEIGHT / 2 });

    // 初始化玩家2
    if (isMultiplayer) {
        player2.push_back({ WIDTH / 2 - 10, HEIGHT / 2 });
    }

    generateItems();
    generateObstacles();
    lastWeatherChange = steady_clock::now();
    startTime = steady_clock::now();
    loadHighScore();//加载历史最高分
    loadHighSurvivalTime(); // 加载历史最长生存时间
    currentSurvivalTime = 0; // 初始化当前生存时
    currentWeather = SUNNY;
    player1Alive = true;
    player2Alive = isMultiplayer;
    shieldDuration1 = 0;
    shieldDuration2 = 0;
    shieldActive1 = false;
    shieldActive2 = false;
    // 添加双倍分数的独立时间变量
    doublePointsActive1 = false;
    doublePointsActive2 = false;
    doublePointsDuration1 = 0;
    doublePointsDuration2 = 0;
}

void run() {
    while (!gameOver) {
        if (!paused) {
            draw();
            input();
            logic();
        }
        else {
            drawPaused();
            input();
        }
        this_thread::sleep_for(chrono::milliseconds(delay));
    }
    // 游戏结束时显示信息
    cout << "Game Over! Player 1 Score: " << score1 << ", Player 2 Score: " << score2
        << ", Level: " << level << ", Time: " << getElapsedTime() << " seconds" << endl;
    if (score1 > highScore) {
        highScore = score1;
        saveHighScore();
        cout << "New High Score: " << highScore << "!" << endl;
    }
    if (gameMode == SURVIVAL) {
        currentSurvivalTime = getElapsedTime();
        if (currentSurvivalTime > highSurvivalTime) {
            highSurvivalTime = currentSurvivalTime;
            saveHighSurvivalTime();
            cout << "New High Survival Time: " << highSurvivalTime << " seconds!" << endl;
        }
    }
}

private:
    deque<Point> snake;
    deque<Point> player2;
    vector<Point> items;
    vector<Point> obstacles;
    Direction direction;
    Direction direction2;
    bool gameOver;
    bool paused;
    bool player1Alive;
    bool player2Alive;
    int score1;
    int score2;
    int level;
    int delay;
    int numObstacles;
    int highScore;
    int baseDelay; // 基础延迟用于天气计算
    steady_clock::time_point startTime;
    Weather currentWeather;
    bool isMultiplayer;
    bool shieldActive1 = false; // 玩家1护盾
    bool shieldActive2 = false; // 玩家2护盾
    int shieldDuration1 = 0; // 玩家1护盾剩余时间
    int shieldDuration2 = 0; // 玩家2护盾剩余时间
    bool doublePointsActive1 = false; // 玩家1双倍分数状态
    bool doublePointsActive2 = false; // 玩家2双倍分数状态
    int doublePointsDuration1 = 0; // 玩家1双倍分数剩余时间
    int doublePointsDuration2 = 0; // 玩家2双倍分数剩余时间
    //反转道具时间变量
    bool reverseActive1 = false;
    bool reverseActive2 = false;
    int reverseDuration1 = 0;
    int reverseDuration2 = 0;
    bool magnetActive1;
    bool magnetActive2;
    int magnetDuration1;
    int magnetDuration2;
    steady_clock::time_point magnetStart1;
    steady_clock::time_point magnetStart2;
    steady_clock::time_point reverseStart1;
    steady_clock::time_point reverseStart2;
    steady_clock::time_point shieldStart1;
    steady_clock::time_point shieldStart2;
    steady_clock::time_point doublePointsStart1;
    steady_clock::time_point doublePointsStart2;
    steady_clock::time_point lastWeatherChange;
    int weatherDuration;
    vector<pair<Point, steady_clock::time_point>> tempObstacles; // 暂时障碍物
    //添加连击数的独立变量
    int combo1;
    int combo2;
    steady_clock::time_point lastEatTime1;
    steady_clock::time_point lastEatTime2;
    GameMode gameMode; // 添加游戏模式变量
    steady_clock::time_point lastObstacleTime; // 上次生成障碍物的时间
    double currentSurvivalTime; // 当前游戏的生存时间
    double highSurvivalTime; // 历史最长生存时间
    RandomEvent randomEvent = NONE;
    steady_clock::time_point lastEventTime;
    steady_clock::time_point eventEndTime;
    bool isEventActive = false;

    void checkRandomEvent() {
        auto now = steady_clock::now();

        // 如果已经过去 20 秒，触发新事件
        if (duration_cast<seconds>(now - lastEventTime).count() >= 20) {
            lastEventTime = now;
            isEventActive = true;
            eventEndTime = now + seconds(5);  // 持续 5 秒
            int randomChoice = rand() % 4;  // 随机选择 4 种事件

            switch (randomChoice) {
            case 0:  // 食物重置
                randomEvent = FOOD_RESET;
                items.clear();
                generateItems();
                break;
            case 1:  // 奖励掉落
                randomEvent = SUPER_FOOD;
                generateSuperFood();
                break;
            case 2:  // 地狱模式
                randomEvent = HELL_MODE;
                delay /= 2;  // 速度加快 100%
                generateRandomObstacles(50); // 生成 50 个随机障碍物
                break;
            case 3:  // 刷分时刻
                randomEvent = SCORE_RUSH;
                generateRandomFood(20); // 生成 20 个食物
                break;
            }
        }

        // 如果事件已进行 5 秒，则结束
        if (isEventActive && now >= eventEndTime) {
            isEventActive = false;
            switch (randomEvent) {
            case HELL_MODE:
                delay *= 2;  // 恢复正常速度
                obstacles.clear();  // 清除临时障碍
                break;
            case SCORE_RUSH:
                items.clear();  // 移除未吃掉的食物
                generateItems(); // 恢复正常的食物数量
                break;
            default:
                break;
            }
            randomEvent = NONE;
        }
    }

    void generateSuperFood() {
        Point superFood;
        do {
            superFood.x = rand() % WIDTH;
            superFood.y = rand() % HEIGHT;
        } while (isSnake(superFood.x, superFood.y) || isObstacle(superFood.x, superFood.y));
        superFood.type = DOUBLE_POINTS;  // 让它表现得像双倍得分道具
        items.push_back(superFood);
    }

    void generateRandomObstacles(int count) {
        for (int i = 0; i < count; ++i) {
            Point obstacle;
            do {
                obstacle.x = rand() % WIDTH;
                obstacle.y = rand() % HEIGHT;
            } while (isSnake(obstacle.x, obstacle.y) || isObstacle(obstacle.x, obstacle.y));
            obstacles.push_back(obstacle);
        }
    }

    void generateRandomFood(int count) {
        for (int i = 0; i < count; ++i) {
            Point item;
            do {
                item.x = rand() % WIDTH;
                item.y = rand() % HEIGHT;
            } while (isSnake(item.x, item.y) || isObstacle(item.x, item.y));
            item.type = NORMAL;
            items.push_back(item);
        }
    }

    void drawPaused() {
        ostringstream buffer;
        buffer << "\033[2J\033[1;1H";
        buffer << "Game Paused. Press 'p' to resume." << endl;
        cout << buffer.str();
    }

    ItemType getItemType(int x, int y) {
        for (const auto& item : items) {
            if (item.x == x && item.y == y) {
                return item.type;
            }
        }
        return NORMAL; // 默认返回普通道具
    }

    void draw() {
        ostringstream buffer;
        buffer << "\033[2J\033[1;1H";
        for (int i = 0; i < WIDTH + 2; ++i) buffer << "#";
        buffer << endl;

        for (int y = 0; y < HEIGHT; ++y) {
            for (int x = 0; x < WIDTH; ++x) {
                if (x == 0) buffer << "#";
                // 统一使用isSnake判断并绘制
                if (isSnake(x, y)) {
                    buffer << (isPlayer1Snake(x, y) ? "O" : "X");  // 玩家1蛇用"O"显示，玩家2用"X"
                }
                else if (isItem(x, y)) {
                    char itemChar = getItemChar(x, y);
                    buffer << itemChar;
                }
                else if (isObstacle(x, y)) {
                    buffer << "#";
                }
                else {
                    buffer << " ";
                }
                if (x == WIDTH - 1) buffer << "#";
            }
            buffer << endl;
        }

        for (int i = 0; i < WIDTH + 2; ++i) buffer << "#";
        buffer << endl;
        buffer << "Player 1 Score: " << score1 << "  Player 2 Score: " << score2 << endl;
        buffer << "Level: " << level << "  Time: " << getElapsedTime() << " seconds" << endl;
        buffer << "High Score: " << highScore << endl;

        // 显示当前生存时间和历史最长生存时间
        if (gameMode == SURVIVAL) {
            buffer << "Current Survival Time: " << getElapsedTime() << " seconds" << endl;
            buffer << "High Survival Time: " << highSurvivalTime << " seconds" << endl;
        }

        //显示连击数量
        buffer << "Player 1 Combo: " << combo1 << endl;
        if (isMultiplayer) {
            buffer << "Player 2 Combo: " << combo2 << endl;
        }

        // 在draw()方法中添加天气显示
        buffer << "Current Weather: ";
        switch (currentWeather) {
        case SUNNY: buffer << "SUNNY"; break;
        case RAINY: buffer << "RAINY"; break;
        case SNOWY: buffer << "SNOWY"; break;
        case THUNDERSTORM: buffer << "THUNDERSTORM"; break;
        case WINDY:buffer << "WINDY";break;
        case SMOG:buffer << "SMOG";break;
        }
        buffer << " (" << (weatherDuration - duration_cast<seconds>(steady_clock::now() - lastWeatherChange).count()) << "s remaining)\n";

        //显示随机事件
        buffer << "Current Event: ";
        switch (randomEvent) {
        case FOOD_RESET: buffer << " 食物重置"; break;
        case SUPER_FOOD: buffer << " 奖励掉落"; break;
        case HELL_MODE: buffer << " 地狱模式"; break;
        case SCORE_RUSH: buffer << " 刷分时刻"; break;
        default: buffer << "无"; break;
        }
        buffer << endl;

        // 显示双倍分数道具剩余时间
        if (doublePointsActive1) {
            buffer << "Player 1 Double Points Remaining: " << doublePointsDuration1 << "s" << endl;
        }
        if (doublePointsActive2) {
            buffer << "Player 2 Double Points Remaining: " << doublePointsDuration2 << "s" << endl;
        }

        // 显示反转道具剩余时间
        if (reverseActive1) {
            buffer << "Player 1 Reverse: " << (reverseDuration1 > 0 ? reverseDuration1 : 0) << "s\n";
        }
        if (reverseActive2) {
            buffer << "Player 2 Reverse: " << (reverseDuration2 > 0 ? reverseDuration2 : 0) << "s\n";
        }

        // 显示玩家1和玩家2护盾剩余时间
        if (shieldActive1) {
            buffer << "Player 1 Shield: "
                << (shieldDuration1 > 0 ? shieldDuration1 : 0)
                << "s\n";
        }
        if (shieldActive2) {
            buffer << "Player 2 Shield: "
                << (shieldDuration2 > 0 ? shieldDuration2 : 0)
                << "s\n";
        }

        // 显示磁铁剩余时间
        if (magnetActive1) {
            buffer << "Player 1 Magnet: " << magnetDuration1 << "s\n";
        }
        if (magnetActive2) {
            buffer << "Player 2 Magnet: " << magnetDuration2 << "s\n";
        }

        cout << buffer.str();
    }

    void input() {
        if (_kbhit()) {
            int ch = _getch();
            // 处理玩家1输入（考虑反转）
            if (ch == 'w' || ch == 's' || ch == 'a' || ch == 'd') {
                if (reverseActive1) {  // 反转状态时按键映射反转
                    switch (ch) {
                    case 'w': ch = 's'; break;
                    case 's': ch = 'w'; break;
                    case 'a': ch = 'd'; break;
                    case 'd': ch = 'a'; break;
                    }
                }
                switch (ch) {
                case 'w': if (direction != DOWN) direction = UP; break;
                case 's': if (direction != UP) direction = DOWN; break;
                case 'a': if (direction != RIGHT) direction = LEFT; break;
                case 'd': if (direction != LEFT) direction = RIGHT; break;
                }
            }
            // 处理玩家2输入（仅限双人模式）
            else if (ch == 224 && isMultiplayer) {
                int arrow = _getch();
                if (reverseActive2) {  // 反转玩家2的箭头方向
                    switch (arrow) {
                    case 72: arrow = 80; break;  // 上变下
                    case 80: arrow = 72; break;  // 下变上
                    case 75: arrow = 77; break;  // 左变右
                    case 77: arrow = 75; break;  // 右变左
                    }
                }
                switch (arrow) {
                case 72: if (direction2 != DOWN) direction2 = UP; break;
                case 80: if (direction2 != UP) direction2 = DOWN; break;
                case 75: if (direction2 != RIGHT) direction2 = LEFT; break;
                case 77: if (direction2 != LEFT) direction2 = RIGHT; break;
                }
            }
            // 处理暂停键
            else if (ch == 'p') paused = !paused;
        }
    }

    // 新增生成临时障碍物的方法
    void generateTempObstacles() {
        Point obstacle;
        do {
            obstacle.x = rand() % WIDTH;
            obstacle.y = rand() % HEIGHT;
        } while (isSnake(obstacle.x, obstacle.y) || isItem(obstacle.x, obstacle.y) || isObstacle(obstacle.x, obstacle.y));

        tempObstacles.emplace_back(obstacle, steady_clock::now());
    }

    void logic() {
        checkRandomEvent();
        //随机事件逻辑
        // 天气系统逻辑
        auto now = steady_clock::now();
        int weatherElapsed = duration_cast<seconds>(now - lastWeatherChange).count();
        int weatherRemaining = weatherDuration - weatherElapsed;

        if (weatherElapsed >= weatherDuration) {
            currentWeather = static_cast<Weather>(rand() % 4);
            lastWeatherChange = now;
            weatherDuration = 20;
            tempObstacles.clear(); // 清除旧障碍物

            // 重置速度到基础值
            delay = baseDelay;
        }

        // 处理天气效果
        auto it = tempObstacles.begin();
        switch (currentWeather) {
        case SUNNY:
            delay = baseDelay;
            break;
        case RAINY:
            delay = baseDelay * 0.7;
            break;
        case SNOWY:
            delay = baseDelay * 1.3;
            break;
        case THUNDERSTORM:
            // 每3秒生成新障碍物
            if (duration_cast<seconds>(now - lastWeatherChange).count() % 3 == 0) {
                generateTempObstacles();
            }
            // 移除过期障碍物
            while (it != tempObstacles.end()) {
                if (duration_cast<seconds>(now - it->second).count() >= 3) {
                    it = tempObstacles.erase(it);
                }
                else {
                    ++it;
                }
            }
            break;
        case WINDY:
            // 大风天气反向控制
            if (duration_cast<seconds>(now - lastWeatherChange).count() <= 20) {
                reverseActive1 = true;
                reverseActive2 = isMultiplayer;
            }
            else {
                reverseActive1 = false;
                reverseActive2 = false;
            }
            break;
        case SMOG:
            // 雾霾天气减少得分
            if (duration_cast<seconds>(now - lastWeatherChange).count() <= 20) {
                doublePointsActive1 = false;
                doublePointsActive2 = false;
            }
            break;
        }

        // 生存模式逻辑：每秒生成8个新的障碍物
        if (gameMode == SURVIVAL) {
            if (duration_cast<seconds>(now - lastObstacleTime).count() >= 1) {
                for (int i = 0; i < 8; ++i) {
                    generateObstacle();
                }
                lastObstacleTime = now;
            }
        }

        // 玩家1移动逻辑
        Point newHead = snake.front();
        switch (direction) {
        case UP: newHead.y--; break;
        case DOWN: newHead.y++; break;
        case LEFT: newHead.x--; break;
        case RIGHT: newHead.x++; break;
        }

        bool ateItem = isItem(newHead.x, newHead.y);

        // 碰撞检测
        if (!shieldActive1 && (newHead.x < 0 || newHead.x >= WIDTH ||
            newHead.y < 0 || newHead.y >= HEIGHT ||
            isSnake(newHead.x, newHead.y) ||
            isObstacle(newHead.x, newHead.y))) {
            player1Alive = false;
        }

        if (ateItem) {
            ItemType type = getItemType(newHead.x, newHead.y);
            removeItem(newHead.x, newHead.y);
            generateItem();

            score1 += 10 * (doublePointsActive1 ? 2 : 1);
            if (score1 % SCORE_PER_LEVEL == 0) levelUp();

            auto now = steady_clock::now();
            long long timeDiff = duration_cast<milliseconds>(now - lastEatTime1).count();
            if (timeDiff <= 2000) {
                combo1++;
            }
            else {
                combo1 = 1;
            }
            lastEatTime1 = now;

            switch (type) {
            case SHIELD:
                shieldActive1 = true;
                shieldStart1 = steady_clock::now();
                shieldDuration1 = 3;
                break;
            case BOOST:
                delay = max(50, delay - 30);
                break;
            case SLOWDOWN:
                delay = min(500, delay + 50);
                break;
            case DOUBLE_POINTS:
                doublePointsActive1 = true;
                doublePointsStart1 = steady_clock::now();
                doublePointsDuration1 = 5; // 5秒
                break;
            case REVERSE:
                reverseActive1 = true;
                reverseStart1 = steady_clock::now();
                reverseDuration1 = 5;
                break;
            case MAGNET:
                magnetActive1 = true;
                magnetStart1 = steady_clock::now();
                magnetDuration1 = 5;
                break;
            default: break;
            }
            snake.push_front(newHead);
        }
        else {
            snake.push_front(newHead);
            snake.pop_back();
        }

        // 磁铁生效时自动吸收食物
        if (magnetActive1 && player1Alive) {
            auto now = steady_clock::now();
            magnetDuration1 = 5 - duration_cast<seconds>(now - magnetStart1).count();
            if (magnetDuration1 <= 0) {
                magnetActive1 = false;
            }
            else {
                // 检查5x5范围
                Point head = snake.front();
                for (int dx = -2; dx <= 2; ++dx) {
                    for (int dy = -2; dy <= 2; ++dy) {
                        int x = head.x + dx;
                        int y = head.y + dy;
                        if (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT) {
                            if (isItem(x, y)) {
                                ItemType type = getItemType(x, y);
                                removeItem(x, y);
                                generateItem();
                                score1 += 10 * (doublePointsActive1 ? 2 : 1);
                                if (score1 % 50 == 0) levelUp();

                                //处理特殊道具效果
                                switch (type) {
                                case SHIELD:
                                    shieldActive1 = true;
                                    shieldStart1 = steady_clock::now();
                                    shieldDuration1 = 3;
                                    break;
                                case BOOST:
                                    delay = max(50, delay - 30);
                                    break;
                                case SLOWDOWN:
                                    delay = min(500, delay + 50);
                                    break;
                                case DOUBLE_POINTS:
                                    doublePointsActive1 = true;
                                    doublePointsStart1 = steady_clock::now();
                                    doublePointsDuration1 = 5; // 5秒
                                    break;
                                case REVERSE:
                                    reverseActive1 = true;
                                    reverseStart1 = steady_clock::now();
                                    reverseDuration1 = 5;
                                    break;
                                case MAGNET:
                                    magnetActive1 = true;
                                    magnetStart1 = steady_clock::now();
                                    magnetDuration1 = 5;
                                    break;
                                default: break;
                                }
                            }
                        }
                    }
                }
            }
        }

        // 更新反转倒计时
        if (reverseActive1) {
            auto now = steady_clock::now();
            reverseDuration1 = 5 - duration_cast<seconds>(now - reverseStart1).count();
            if (reverseDuration1 <= 0) reverseActive1 = false;
        }

        // 更新护盾时间（如果激活）
        if (shieldActive1) {
            auto now = steady_clock::now();
            shieldDuration1 = 3 - duration_cast<seconds>(now - shieldStart1).count();
            if (shieldDuration1 <= 0) {
                shieldActive1 = false;
            }
        }

        // 更新双倍得分状态（非阻塞式）
        if (doublePointsActive1) {
            auto now = steady_clock::now();
            doublePointsDuration1 = 5 - duration_cast<seconds>(now - doublePointsStart1).count();
            if (doublePointsDuration1 <= 0) {
                doublePointsActive1 = false;
            }
        }

        // 玩家2逻辑：仅在双人模式下才执行
        if (isMultiplayer && player2Alive) {
            Point newHead2 = player2.front();
            switch (direction2) {
            case UP: newHead2.y--; break;
            case DOWN: newHead2.y++; break;
            case LEFT: newHead2.x--; break;
            case RIGHT: newHead2.x++; break;
            }

            bool ateItem2 = isItem(newHead2.x, newHead2.y);

            if (!shieldActive2 && (newHead2.x < 0 || newHead2.x >= WIDTH ||
                newHead2.y < 0 || newHead2.y >= HEIGHT ||
                isSnake(newHead2.x, newHead2.y) ||
                isObstacle(newHead2.x, newHead2.y))) {
                player2Alive = false;
            }

            if (ateItem2) {
                ItemType type = getItemType(newHead2.x, newHead2.y);
                removeItem(newHead2.x, newHead2.y);
                generateItem();

                score2 += 10 * (doublePointsActive2 ? 2 : 1);

                auto now = steady_clock::now();
                long long timeDiff = duration_cast<milliseconds>(now - lastEatTime2).count();
                if (timeDiff <= 2000) {
                    combo2++;
                }
                else {
                    combo2 = 1;
                }
                lastEatTime2 = now;

                switch (type) {
                case SHIELD:
                    shieldActive2 = true;
                    shieldStart2 = steady_clock::now();
                    shieldDuration2 = 3;
                    break;
                case BOOST:
                    delay = max(50, delay - 30);
                    break;
                case SLOWDOWN:
                    delay = min(300, delay + 30);
                    break;
                case DOUBLE_POINTS:
                    doublePointsActive2 = true;
                    doublePointsStart2 = steady_clock::now();
                    doublePointsDuration2 = 5; // 5秒
                    break;
                case REVERSE:
                    reverseActive2 = true;
                    reverseStart2 = steady_clock::now();
                    reverseDuration2 = 5;
                    break;
                case MAGNET:
                    magnetActive1 = true;
                    magnetStart1 = steady_clock::now();
                    magnetDuration1 = 5;
                    break;
                default: break;
                }
                player2.push_front(newHead2);
            }
            else {
                player2.push_front(newHead2);
                player2.pop_back();
            }
        }

        // 玩家2磁铁生效
        if (magnetActive2 && player2Alive) {
            auto now = steady_clock::now();
            magnetDuration2 = 5 - duration_cast<seconds>(now - magnetStart2).count();
            if (magnetDuration2 <= 0) {
                magnetActive2 = false;
            }
            else {
                Point head2 = player2.front();
                for (int dx = -2; dx <= 2; ++dx) {
                    for (int dy = -2; dy <= 2; ++dy) {
                        int x = head2.x + dx;
                        int y = head2.y + dy;
                        if (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT) {
                            if (isItem(x, y)) {
                                ItemType type = getItemType(x, y);
                                removeItem(x, y);
                                generateItem();
                                score2 += 10 * (doublePointsActive2 ? 2 : 1);

                                //处理特殊道具效果
                                switch (type) {
                                case SHIELD:
                                    shieldActive1 = true;
                                    shieldStart1 = steady_clock::now();
                                    shieldDuration1 = 3;
                                    break;
                                case BOOST:
                                    delay = max(50, delay - 30);
                                    break;
                                case SLOWDOWN:
                                    delay = min(500, delay + 50);
                                    break;
                                case DOUBLE_POINTS:
                                    doublePointsActive1 = true;
                                    doublePointsStart1 = steady_clock::now();
                                    doublePointsDuration1 = 5; // 5秒
                                    break;
                                case REVERSE:
                                    reverseActive1 = true;
                                    reverseStart1 = steady_clock::now();
                                    reverseDuration1 = 5;
                                    break;
                                case MAGNET:
                                    magnetActive1 = true;
                                    magnetStart1 = steady_clock::now();
                                    magnetDuration1 = 5;
                                    break;
                                default: break;
                                }
                            }
                        }
                    }
                }
            }
        }

        // 更新状态
        now = steady_clock::now();

        //更新护盾倒计时
        if (shieldActive1) {
            shieldDuration1 = 3 - duration_cast<seconds>(now - shieldStart1).count();
            if (shieldDuration1 <= 0) shieldActive1 = false;
        }
        if (shieldActive2) {
            shieldDuration2 = 3 - duration_cast<seconds>(now - shieldStart2).count();
            if (shieldDuration2 <= 0) shieldActive2 = false;
        }

        // 更新反转倒计时
        if (reverseActive1) {
            reverseDuration1 = 5 - duration_cast<seconds>(now - reverseStart1).count();
            if (reverseDuration1 <= 0) reverseActive1 = false;
        }
        if (reverseActive2) {
            reverseDuration2 = 5 - duration_cast<seconds>(now - reverseStart2).count();
            if (reverseDuration2 <= 0) reverseActive2 = false;
        }

        //更新双倍得分倒计时
        if (doublePointsActive1) {
            doublePointsDuration1 = 5 - duration_cast<seconds>(now - doublePointsStart1).count();
            if (doublePointsDuration1 <= 0) {
                doublePointsActive1 = false;
            }
        }
        if (doublePointsActive2) {
            doublePointsDuration2 = 5 - duration_cast<seconds>(now - doublePointsStart2).count();
            if (doublePointsDuration2 <= 0) {
                doublePointsActive2 = false;
            }
        }

        // 竞争逻辑
        if (player1Alive && player2Alive && isMultiplayer) {
            if (snake.size() > player2.size() &&
                isSnake(player2.front().x, player2.front().y)) {
                player2Alive = false;
            }
            else if (player2.size() > snake.size() &&
                isSnake(snake.front().x, snake.front().y)) {
                player1Alive = false;
            }
        }

        // 结束条件
        if ((!isMultiplayer && !player1Alive) ||
            (isMultiplayer && !player1Alive && !player2Alive)) {
            gameOver = true;
        }
    }

    void levelUp() {
        level++;
        if (delay > 50) delay -= 20; // 每升一级减少延迟时间，加快速度
        generateItems(); // 每升一级生成更多的物品
        generateObstacles(); // 每升一级生成更多的障碍物
    }

    bool isSnake(int x, int y) {
        for (const auto& p : snake) {
            if (p.x == x && p.y == y) return true;
        }
        if (isMultiplayer) {  // 仅在双人模式检查玩家2
            for (const auto& p : player2) {
                if (p.x == x && p.y == y) return true;
            }
        }
        return false;
    }

    bool isItem(int x, int y) {
        for (const auto& item : items) {
            if (item.x == x && item.y == y) return true;
        }
        return false;
    }

    bool isObstacle(int x, int y) {
        for (const auto& obstacle : obstacles) {
            if (obstacle.x == x && obstacle.y == y) return true;
        }
        for (const auto& temp : tempObstacles) {
            if (temp.first.x == x && temp.first.y == y) return true;
        }
        return false;
    }

    void generateItems() {
        for (int i = 0; i < NUM_ITEMS; ++i) {
            generateItem();
        }
    }

    void generateItem() {
        Point item;
        item.type = static_cast<ItemType>(rand() % 7); // 随机生成7种道具
        do {
            item.x = rand() % WIDTH;
            item.y = rand() % HEIGHT;
        } while (isSnake(item.x, item.y) || isItem(item.x, item.y) || isObstacle(item.x, item.y));
        items.push_back(item);
    }

    void removeItem(int x, int y) {
        items.erase(remove_if(items.begin(), items.end(), [x, y](const Point& item) {
            return item.x == x && item.y == y;
            }), items.end());
    }

    void generateObstacles() {
        for (int i = 0; i < numObstacles; ++i) {
            generateObstacle();
        }
    }

    void generateObstacle() {
        Point obstacle;
        do {
            obstacle.x = rand() % WIDTH;
            obstacle.y = rand() % HEIGHT;
        } while (isSnake(obstacle.x, obstacle.y) || isItem(obstacle.x, obstacle.y) || isObstacle(obstacle.x, obstacle.y));
        obstacles.push_back(obstacle);
    }

    double getElapsedTime() {
        auto now = steady_clock::now();
        auto elapsed = duration_cast<seconds>(now - startTime).count();
        return elapsed;
    }

    void loadHighScore() {
        ifstream file(HIGHSCORE_FILE);
        if (file.is_open()) {
            file >> highScore;
            file.close();
        }
        else {
            highScore = 0;
        }
    }

    void saveHighScore() {
        ofstream file(HIGHSCORE_FILE);
        if (file.is_open()) {
            file << highScore;
            file.close();
        }
    }

    void loadHighSurvivalTime() {
        ifstream file("highsurvivaltime.txt");
        if (file.is_open()) {
            file >> highSurvivalTime;
            file.close();
        }
        else {
            highSurvivalTime = 0;
        }
    }

    void saveHighSurvivalTime() {
        ofstream file("highsurvivaltime.txt");
        if (file.is_open()) {
            file << highSurvivalTime;
            file.close();
        }
    }

    bool isPlayer1Snake(int x, int y) {
        for (const auto& p : snake) {
            if (p.x == x && p.y == y) return true;
        }
        return false;
    }

    char getItemChar(int x, int y) {
        for (const auto& item : items) {
            if (item.x == x && item.y == y) {
                switch (item.type) {
                case REVERSE: return '$';  //反转道具
                case BOOST: return '+';   // 加速道具
                case SLOWDOWN: return '-'; // 减速道具
                case SHIELD: return '&';   // 护盾道具，改为&避免与墙壁重叠
                case DOUBLE_POINTS: return '@'; // 双倍分数道具
                case MAGNET: return '^';  // 磁铁道具显示
                default: return '*';     // 普通食物
                }
            }
        }
        return ' '; // 默认空白字符
    }
};

int main() {
    int difficulty;
    bool isMultiplayer;
    GameMode gameMode;

    // 选择难度
    cout << "Select difficulty level (1: Easy, 2: Medium, 3: Hard): ";
    cin >> difficulty;

    // 选择游戏模式
    cout << "Select mode (1: Single Player, 2: Multiplayer, 3: Survival): ";
    int mode;
    cin >> mode;
    if (mode == 3) {
        gameMode = SURVIVAL;
        isMultiplayer = false; // 生存模式仅支持单人
    }
    else {
        gameMode = CLASSIC;
        isMultiplayer = mode == 2;
    }

    int numObstacles;
    int initialDelay;

    // 根据难度选择障碍物数量和初始延迟
    switch (difficulty) {
    case 1:
        numObstacles = 20;
        initialDelay = 200;
        break;
    case 2:
        numObstacles = 30;
        initialDelay = 150;
        break;
    case 3:
        numObstacles = 40;
        initialDelay = 100;
        break;
    default:
        cout << "Invalid difficulty level. Defaulting to Medium." << endl;
        numObstacles = 30;
        initialDelay = 150;
        break;
    }

    // 初始化游戏并开始运行
    SnakeGame game(numObstacles, initialDelay, isMultiplayer, gameMode);
    game.run();
    return 0;
}

