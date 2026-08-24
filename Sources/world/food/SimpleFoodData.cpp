#include "SimpleFoodData.h"
#include "FoodConstants.h"
#include "../item/FoodItem.h"
#include "../entity/player/Player.h"
#include "../level/Level.h"
#include "../level/Difficulty.h"
#include "../../AppConstants.h"

SimpleFoodData::SimpleFoodData()
:   foodLevel(MAX_FOOD),
    lastFoodLevel(MAX_FOOD),
    saturationLevel(5.0f),
    exhaustionLevel(0.0f),
    tickTimer(0)
{
}

void SimpleFoodData::eat(int food, float saturationModifier) {
    foodLevel = Mth::Min(food + foodLevel, MAX_FOOD);
    saturationLevel = Mth::Min(saturationLevel + (float)food * saturationModifier * 2.0f, (float)foodLevel);
}

void SimpleFoodData::eat(FoodItem* item) {
    if (item) {
        eat(item->getNutrition(), item->getSaturationModifier());
    }
}

void SimpleFoodData::addExhaustion(float amount) {
    exhaustionLevel = Mth::Min(exhaustionLevel + amount, 40.0f);
}

void SimpleFoodData::setFoodLevel(int l) {
    foodLevel = Mth::clamp(l, 0, MAX_FOOD);
}

void SimpleFoodData::setSaturation(float sat) {
    saturationLevel = Mth::clamp(sat, 0.0f, (float)foodLevel);
}

void SimpleFoodData::setExhaustion(float exh) {
    exhaustionLevel = Mth::clamp(exh, 0.0f, 40.0f);
}

void SimpleFoodData::tick(Player* player) {
    lastFoodLevel = foodLevel;

    VoxelDifficulty difficulty(Difficulty::NORMAL);
    if (player && player->level) {
        difficulty = VoxelDifficulty::fromId(player->level->difficulty);
    }

    // 1. Disminucion de agotamiento (Exhaustion -> Saturation / Food)
    if (exhaustionLevel > 4.0f) {
        exhaustionLevel -= 4.0f;
        if (saturationLevel > 0.0f) {
            saturationLevel = Mth::Max(saturationLevel - 1.0f, 0.0f);
        } else if (difficulty.shouldDepleteHunger()) {
            foodLevel = Mth::Max(foodLevel - 1, 0);
        }
    }

    // 2. Modo Pacifico: autorregeneracion continua de comida
    if (difficulty.shouldRegenFood()) {
        if (foodLevel < MAX_FOOD) {
            tickTimer++;
            if (tickTimer >= 20) {
                foodLevel = Mth::Min(foodLevel + 1, MAX_FOOD);
                tickTimer = 0;
            }
        }
    }

    // 3. Regeneracion natural de salud (foodLevel >= 18)
    if (foodLevel >= HEAL_LEVEL && player && player->isHurt()) {
        tickTimer++;
        if (tickTimer >= HEALTH_TICK_COUNT) {
            player->heal(1);
            addExhaustion(3.0f);
            tickTimer = 0;
        }
    }
    // 4. Inanicion / Danio por hambre (foodLevel <= 0)
    else if (foodLevel <= 0) {
        tickTimer++;
        if (tickTimer >= HEALTH_TICK_COUNT) {
            if (player && difficulty.canStarveAtHealth(player->health)) {
                player->hurt(NULL, 1);
            }
            tickTimer = 0;
        }
    } else {
        tickTimer = 0;
    }
}
