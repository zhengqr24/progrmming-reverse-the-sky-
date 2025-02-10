
# progrmming-reverse-the-sky-
贪吃蛇-游戏设计文档

## 一．总设置
1. 模式设置：游戏分为单人模式，双人模式和生存模式（仅支持单人）
2. 难度设置：游戏可以选择三个难度：Easy, Medium, Hard。难度越大蛇的初始速度越快。
3. 地图设置：’#’为地图的边界，地图大小为100*35
4. 食物设置：
    - 普通食物：‘*’，分数+10，长度+1
    - 加速食物：‘+’，分数+10，长度+1，速度加快，延迟变低
    - 减速食物：‘-’，分数+10，长度+1，速度减慢，延迟变长
    - 双倍食物：‘@’，分数+20，长度+1，获得双倍得分buff持续5s
    - 护盾食物：‘&’，分数+10，获得护盾，长度+1，可以穿越障碍物和墙，持续3s
    - 反转食物：‘$’，分数+10，长度+1，吃了之后操作按键效果上下颠倒，左右反转，持续5s
    - 磁铁食物：‘^’，分数+10，长度+1，吃了之后蛇头5x5范围之内的食物都可以被吸入，持续5s
5. 初始化设置：地图中随机生成障碍‘#’，食物和道具（数量不定随机生成），蛇的初始化长度为1
6. 效果设置：若无护盾，蛇头碰到‘#’立即死亡，吃下食物以及道具则按照（4）食物设置来执行相应效果，在2s之内蛇连续吃到食物就会增加蛇的连击数量，若超过2s则连击数量返回为1
7. 天气系统：共有SUNNY, RAINY, SNOWY, THUNDERSTORM, WINDY, SMOG六种天气，显示在地图下方，每20s切换一次天气
    - SUNNY：无特殊效果，一切正常。
    - RAINY：蛇速度加快30%（延迟x1.3）
    - SNOWY：蛇速度减慢30%（延迟x0.7）
    - THUNDERSTORM：地图内障碍物数量随机快速增多（每个障碍物出现3s后消失）
    - WINDY：蛇的操作方向上下颠倒，左右反转
    - SMOG：蛇的得分减少，双倍得分道具失效
8. 显示设置：
    - 地图下方显示Player1score和Player2score
    - 游戏时间Time
    - 显示关卡等级level（仅限单人模式）
    - 吃到护盾或者双倍或者反转食物时显示护盾或双倍或反转食物的剩余时间（玩家1和玩家2都会显示）
    - 显示历史最高分数High Score!
    - 游戏结束时显示Game over!以及Player1score和Player2score和游玩的时间Time
    - 生存模式之下额外显示当前生存的时间currentsurvivalTime以及历史最大生存时间High Time!
    - 显示玩家当前的连击数量comb
9. 暂停设置：按下’p’键暂停游戏，再按下’p’重新开始游戏
10. 说明：程序在Visual Studio上运行，代码为C++
11. 随机事件系统：共有无，食物重置，地狱模式，超级食物，刷分时刻这五种事件，显示在地图下方，每20s生成一次随机事件
    - 无：一切正常，无特殊效果
    - 食物重置：地图上的食物位置全部重新随机分配
    - 地狱模式：短时间5秒内 蛇速加快100%，并且在这5秒之内随机出现许多障碍物（50个），障碍物5秒之后会消失难度提升。
    - 超级食物：地图某处出现 超级食物'？'（吃了加50分）
    - 刷分时刻：短时间5秒之内，地图上随机出现20个食物，5秒之后没被吃掉的剩余食物消失

## 二．单人模式
1. 操作设置：用wsad操控上下左右
2. 蛇身绘制：每一节的蛇都用’0’表示
3. 关卡设置：单人模式为闯关游戏，当分数为50的倍数，进入下一关。随着关卡等级的提升，蛇的速度变快，延迟降低。进入新关卡时障碍物和食物都重置（有可能你一进入新关卡立马撞死）。

## 三．双人模式
1. 操作设置：玩家1用wsad操作上下左右，玩家2用箭头操作上下左右
2. 游戏玩法：双人模式为竞争游戏，在限定时间120s之内，谁的得分高谁就胜利。注意：长的蛇可以吃掉短的蛇（短蛇的蛇头碰到长蛇的身体就会死亡），但是短的蛇不能吃长的蛇。
3. 显示设置：游戏结束时在原来基础上多播报一条“胜利者名单“

## 四. 生存模式
1.操作设置与单人模式相同但没有关卡
2.游戏玩法：每一秒地图上都会随机多生成8个障碍物，你的目标是活地尽可能长
