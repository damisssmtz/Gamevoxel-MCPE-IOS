#ifndef NET_MINECRAFT_WORLD_FOOD__SimpleFoodData_H__
#define NET_MINECRAFT_WORLD_FOOD__SimpleFoodData_H__

class FoodItem;
class Player;

class SimpleFoodData
{
public:
    static const int MAX_FOOD = 20;
    static const int HEAL_LEVEL = 18;
    static const int HEALTH_TICK_COUNT = 80;

    SimpleFoodData();

    void eat(int food, float saturationModifier = 0.6f);
    void eat(FoodItem* item);

    void tick(Player* player);
    void addExhaustion(float amount);

    bool needsFood() const { return foodLevel < MAX_FOOD; }

    int getFoodLevel() const { return foodLevel; }
    int getLastFoodLevel() const { return lastFoodLevel; }
    float getSaturationLevel() const { return saturationLevel; }
    float getExhaustionLevel() const { return exhaustionLevel; }

    void setFoodLevel(int l);
    void setSaturation(float sat);
    void setExhaustion(float exh);

private:
    int foodLevel;
    int lastFoodLevel;
    float saturationLevel;
    float exhaustionLevel;
    int tickTimer;
};

#endif /*NET_MINECRAFT_WORLD_FOOD__SimpleFoodData_H__*/
