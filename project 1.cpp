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
enum ItemType { NORMAL, BOOST, SLOWDOWN, SHIELD, DOUBLE_POINTS, REVERSE, MAGNET }; //ʳ������
enum GameMode { CLASSIC, SURVIVAL }; // �������ģʽ
enum RandomEvent { NONE, FOOD_RESET, SUPER_FOOD, HELL_MODE, SCORE_RUSH };//����¼�

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
        magnetActive1(false), magnetActive2(false), magnetDuration1(0), magnetDuration2(0){ // ��ʼ��״̬

        // ��ʼ�����1
        snake.push_back({ WIDTH / 2, HEIGHT / 2 });

    // ��ʼ�����2
    if (isMultiplayer) {
        player2.push_back({ WIDTH / 2 - 10, HEIGHT / 2 });
    }

    generateItems();
    generateObstacles();
    lastWeatherChange = steady_clock::now();
    startTime = steady_clock::now();
    loadHighScore();//������ʷ��߷�
    loadHighSurvivalTime(); // ������ʷ�����ʱ��
    currentSurvivalTime = 0; // ��ʼ����ǰ����ʱ
    currentWeather = SUNNY;
    player1Alive = true;
    player2Alive = isMultiplayer;
    shieldDuration1 = 0;
    shieldDuration2 = 0;
    shieldActive1 = false;
    shieldActive2 = false;
    // ���˫�������Ķ���ʱ�����
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
    // ��Ϸ����ʱ��ʾ��Ϣ
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
    int baseDelay; // �����ӳ�������������
    steady_clock::time_point startTime;
    Weather currentWeather;
    bool isMultiplayer;
    bool shieldActive1 = false; // ���1����
    bool shieldActive2 = false; // ���2����
    int shieldDuration1 = 0; // ���1����ʣ��ʱ��
    int shieldDuration2 = 0; // ���2����ʣ��ʱ��
    bool doublePointsActive1 = false; // ���1˫������״̬
    bool doublePointsActive2 = false; // ���2˫������״̬
    int doublePointsDuration1 = 0; // ���1˫������ʣ��ʱ��
    int doublePointsDuration2 = 0; // ���2˫������ʣ��ʱ��
    //��ת����ʱ�����
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
    vector<pair<Point, steady_clock::time_point>> tempObstacles; // ��ʱ�ϰ���
    //����������Ķ�������
    int combo1;
    int combo2;
    steady_clock::time_point lastEatTime1;
    steady_clock::time_point lastEatTime2;
    GameMode gameMode; // �����Ϸģʽ����
    steady_clock::time_point lastObstacleTime; // �ϴ������ϰ����ʱ��
    double currentSurvivalTime; // ��ǰ��Ϸ������ʱ��
    double highSurvivalTime; // ��ʷ�����ʱ��
    RandomEvent randomEvent = NONE;
    steady_clock::time_point lastEventTime;
    steady_clock::time_point eventEndTime;
    bool isEventActive = false;

    void checkRandomEvent() {
        auto now = steady_clock::now();

        // ����Ѿ���ȥ 20 �룬�������¼�
        if (duration_cast<seconds>(now - lastEventTime).count() >= 20) {
            lastEventTime = now;
            isEventActive = true;
            eventEndTime = now + seconds(5);  // ���� 5 ��
            int randomChoice = rand() % 4;  // ���ѡ�� 4 ���¼�

            switch (randomChoice) {
            case 0:  // ʳ������
                randomEvent = FOOD_RESET;
                items.clear();
                generateItems();
                break;
            case 1:  // ��������
                randomEvent = SUPER_FOOD;
                generateSuperFood();
                break;
            case 2:  // ����ģʽ
                randomEvent = HELL_MODE;
                delay /= 2;  // �ٶȼӿ� 100%
                generateRandomObstacles(50); // ���� 50 ������ϰ���
                break;
            case 3:  // ˢ��ʱ��
                randomEvent = SCORE_RUSH;
                generateRandomFood(20); // ���� 20 ��ʳ��
                break;
            }
        }

        // ����¼��ѽ��� 5 �룬�����
        if (isEventActive && now >= eventEndTime) {
            isEventActive = false;
            switch (randomEvent) {
            case HELL_MODE:
                delay *= 2;  // �ָ������ٶ�
                obstacles.clear();  // �����ʱ�ϰ�
                break;
            case SCORE_RUSH:
                items.clear();  // �Ƴ�δ�Ե���ʳ��
                generateItems(); // �ָ�������ʳ������
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
        superFood.type = DOUBLE_POINTS;  // �������ֵ���˫���÷ֵ���
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
        return NORMAL; // Ĭ�Ϸ�����ͨ����
    }

    void draw() {
        ostringstream buffer;
        buffer << "\033[2J\033[1;1H";
        for (int i = 0; i < WIDTH + 2; ++i) buffer << "#";
        buffer << endl;

        for (int y = 0; y < HEIGHT; ++y) {
            for (int x = 0; x < WIDTH; ++x) {
                if (x == 0) buffer << "#";
                // ͳһʹ��isSnake�жϲ�����
                if (isSnake(x, y)) {
                    buffer << (isPlayer1Snake(x, y) ? "O" : "X");  // ���1����"O"��ʾ�����2��"X"
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

        // ��ʾ��ǰ����ʱ�����ʷ�����ʱ��
        if (gameMode == SURVIVAL) {
            buffer << "Current Survival Time: " << getElapsedTime() << " seconds" << endl;
            buffer << "High Survival Time: " << highSurvivalTime << " seconds" << endl;
        }

        //��ʾ��������
        buffer << "Player 1 Combo: " << combo1 << endl;
        if (isMultiplayer) {
            buffer << "Player 2 Combo: " << combo2 << endl;
        }

        // ��draw()���������������ʾ
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

        //��ʾ����¼�
        buffer << "Current Event: ";
        switch (randomEvent) {
        case FOOD_RESET: buffer << " ʳ������"; break;
        case SUPER_FOOD: buffer << " ��������"; break;
        case HELL_MODE: buffer << " ����ģʽ"; break;
        case SCORE_RUSH: buffer << " ˢ��ʱ��"; break;
        default: buffer << "��"; break;
        }
        buffer << endl;

        // ��ʾ˫����������ʣ��ʱ��
        if (doublePointsActive1) {
            buffer << "Player 1 Double Points Remaining: " << doublePointsDuration1 << "s" << endl;
        }
        if (doublePointsActive2) {
            buffer << "Player 2 Double Points Remaining: " << doublePointsDuration2 << "s" << endl;
        }

        // ��ʾ��ת����ʣ��ʱ��
        if (reverseActive1) {
            buffer << "Player 1 Reverse: " << (reverseDuration1 > 0 ? reverseDuration1 : 0) << "s\n";
        }
        if (reverseActive2) {
            buffer << "Player 2 Reverse: " << (reverseDuration2 > 0 ? reverseDuration2 : 0) << "s\n";
        }

        // ��ʾ���1�����2����ʣ��ʱ��
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

        // ��ʾ����ʣ��ʱ��
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
            // �������1���루���Ƿ�ת��
            if (ch == 'w' || ch == 's' || ch == 'a' || ch == 'd') {
                if (reverseActive1) {  // ��ת״̬ʱ����ӳ�䷴ת
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
            // �������2���루����˫��ģʽ��
            else if (ch == 224 && isMultiplayer) {
                int arrow = _getch();
                if (reverseActive2) {  // ��ת���2�ļ�ͷ����
                    switch (arrow) {
                    case 72: arrow = 80; break;  // �ϱ���
                    case 80: arrow = 72; break;  // �±���
                    case 75: arrow = 77; break;  // �����
                    case 77: arrow = 75; break;  // �ұ���
                    }
                }
                switch (arrow) {
                case 72: if (direction2 != DOWN) direction2 = UP; break;
                case 80: if (direction2 != UP) direction2 = DOWN; break;
                case 75: if (direction2 != RIGHT) direction2 = LEFT; break;
                case 77: if (direction2 != LEFT) direction2 = RIGHT; break;
                }
            }
            // ������ͣ��
            else if (ch == 'p') paused = !paused;
        }
    }

    // ����������ʱ�ϰ���ķ���
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
        //����¼��߼�
        // ����ϵͳ�߼�
        auto now = steady_clock::now();
        int weatherElapsed = duration_cast<seconds>(now - lastWeatherChange).count();
        int weatherRemaining = weatherDuration - weatherElapsed;

        if (weatherElapsed >= weatherDuration) {
            currentWeather = static_cast<Weather>(rand() % 4);
            lastWeatherChange = now;
            weatherDuration = 20;
            tempObstacles.clear(); // ������ϰ���

            // �����ٶȵ�����ֵ
            delay = baseDelay;
        }

        // ��������Ч��
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
            // ÿ3���������ϰ���
            if (duration_cast<seconds>(now - lastWeatherChange).count() % 3 == 0) {
                generateTempObstacles();
            }
            // �Ƴ������ϰ���
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
            // ��������������
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
            // �����������ٵ÷�
            if (duration_cast<seconds>(now - lastWeatherChange).count() <= 20) {
                doublePointsActive1 = false;
                doublePointsActive2 = false;
            }
            break;
        }

        // ����ģʽ�߼���ÿ������8���µ��ϰ���
        if (gameMode == SURVIVAL) {
            if (duration_cast<seconds>(now - lastObstacleTime).count() >= 1) {
                for (int i = 0; i < 8; ++i) {
                    generateObstacle();
                }
                lastObstacleTime = now;
            }
        }

        // ���1�ƶ��߼�
        Point newHead = snake.front();
        switch (direction) {
        case UP: newHead.y--; break;
        case DOWN: newHead.y++; break;
        case LEFT: newHead.x--; break;
        case RIGHT: newHead.x++; break;
        }

        bool ateItem = isItem(newHead.x, newHead.y);

        // ��ײ���
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
                doublePointsDuration1 = 5; // 5��
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

        // ������Чʱ�Զ�����ʳ��
        if (magnetActive1 && player1Alive) {
            auto now = steady_clock::now();
            magnetDuration1 = 5 - duration_cast<seconds>(now - magnetStart1).count();
            if (magnetDuration1 <= 0) {
                magnetActive1 = false;
            }
            else {
                // ���5x5��Χ
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

                                //�����������Ч��
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
                                    doublePointsDuration1 = 5; // 5��
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

        // ���·�ת����ʱ
        if (reverseActive1) {
            auto now = steady_clock::now();
            reverseDuration1 = 5 - duration_cast<seconds>(now - reverseStart1).count();
            if (reverseDuration1 <= 0) reverseActive1 = false;
        }

        // ���»���ʱ�䣨������
        if (shieldActive1) {
            auto now = steady_clock::now();
            shieldDuration1 = 3 - duration_cast<seconds>(now - shieldStart1).count();
            if (shieldDuration1 <= 0) {
                shieldActive1 = false;
            }
        }

        // ����˫���÷�״̬��������ʽ��
        if (doublePointsActive1) {
            auto now = steady_clock::now();
            doublePointsDuration1 = 5 - duration_cast<seconds>(now - doublePointsStart1).count();
            if (doublePointsDuration1 <= 0) {
                doublePointsActive1 = false;
            }
        }

        // ���2�߼�������˫��ģʽ�²�ִ��
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
                    doublePointsDuration2 = 5; // 5��
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

        // ���2������Ч
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

                                //�����������Ч��
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
                                    doublePointsDuration1 = 5; // 5��
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

        // ����״̬
        now = steady_clock::now();

        //���»��ܵ���ʱ
        if (shieldActive1) {
            shieldDuration1 = 3 - duration_cast<seconds>(now - shieldStart1).count();
            if (shieldDuration1 <= 0) shieldActive1 = false;
        }
        if (shieldActive2) {
            shieldDuration2 = 3 - duration_cast<seconds>(now - shieldStart2).count();
            if (shieldDuration2 <= 0) shieldActive2 = false;
        }

        // ���·�ת����ʱ
        if (reverseActive1) {
            reverseDuration1 = 5 - duration_cast<seconds>(now - reverseStart1).count();
            if (reverseDuration1 <= 0) reverseActive1 = false;
        }
        if (reverseActive2) {
            reverseDuration2 = 5 - duration_cast<seconds>(now - reverseStart2).count();
            if (reverseDuration2 <= 0) reverseActive2 = false;
        }

        //����˫���÷ֵ���ʱ
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

        // �����߼�
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

        // ��������
        if ((!isMultiplayer && !player1Alive) ||
            (isMultiplayer && !player1Alive && !player2Alive)) {
            gameOver = true;
        }
    }

    void levelUp() {
        level++;
        if (delay > 50) delay -= 20; // ÿ��һ�������ӳ�ʱ�䣬�ӿ��ٶ�
        generateItems(); // ÿ��һ�����ɸ������Ʒ
        generateObstacles(); // ÿ��һ�����ɸ�����ϰ���
    }

    bool isSnake(int x, int y) {
        for (const auto& p : snake) {
            if (p.x == x && p.y == y) return true;
        }
        if (isMultiplayer) {  // ����˫��ģʽ������2
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
        item.type = static_cast<ItemType>(rand() % 7); // �������7�ֵ���
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
                case REVERSE: return '$';  //��ת����
                case BOOST: return '+';   // ���ٵ���
                case SLOWDOWN: return '-'; // ���ٵ���
                case SHIELD: return '&';   // ���ܵ��ߣ���Ϊ&������ǽ���ص�
                case DOUBLE_POINTS: return '@'; // ˫����������
                case MAGNET: return '^';  // ����������ʾ
                default: return '*';     // ��ͨʳ��
                }
            }
        }
        return ' '; // Ĭ�Ͽհ��ַ�
    }
};

int main() {
    int difficulty;
    bool isMultiplayer;
    GameMode gameMode;

    // ѡ���Ѷ�
    cout << "Select difficulty level (1: Easy, 2: Medium, 3: Hard): ";
    cin >> difficulty;

    // ѡ����Ϸģʽ
    cout << "Select mode (1: Single Player, 2: Multiplayer, 3: Survival): ";
    int mode;
    cin >> mode;
    if (mode == 3) {
        gameMode = SURVIVAL;
        isMultiplayer = false; // ����ģʽ��֧�ֵ���
    }
    else {
        gameMode = CLASSIC;
        isMultiplayer = mode == 2;
    }

    int numObstacles;
    int initialDelay;

    // �����Ѷ�ѡ���ϰ��������ͳ�ʼ�ӳ�
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

    // ��ʼ����Ϸ����ʼ����
    SnakeGame game(numObstacles, initialDelay, isMultiplayer, gameMode);
    game.run();
    return 0;
}

