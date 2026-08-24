#ifndef NET_MINECRAFT_WORLD__Difficulty_H__
#define NET_MINECRAFT_WORLD__Difficulty_H__

//package net.minecraft.world;
// C++ equivalent of:
//   extends RefCounted
//   class_name VoxelDifficulty

class VoxelDifficulty
{
public:
	enum Difficulty {
		PEACEFUL	= 0,
		EASY		= 1,
		NORMAL		= 2,
		HARD		= 3,

		COUNT
	};

	VoxelDifficulty(Difficulty difficulty = NORMAL)
	:	current(byId((int)difficulty))
	{
	}

	explicit VoxelDifficulty(int id)
	:	current(byId(id))
	{
	}

	void setDifficulty(Difficulty difficulty) {
		current = byId((int)difficulty);
	}

	void setDifficulty(int id) {
		current = byId(id);
	}

	Difficulty getDifficulty() const {
		return current;
	}

	int getId() const {
		return (int)current;
	}

	bool isPeaceful() const { return current == PEACEFUL; }
	bool isEasy() const { return current == EASY; }
	bool isNormal() const { return current == NORMAL; }
	bool isHard() const { return current == HARD; }

	bool shouldSpawnMonsters() const {
		return current > PEACEFUL;
	}

	bool shouldDepleteHunger() const {
		return current > PEACEFUL;
	}

	bool shouldRegenFood() const {
		return current == PEACEFUL;
	}

	// Easy: stop at 5 hearts. Normal: stop at 1/2 heart. Hard: can kill.
	int getStarvationHealthFloor() const {
		if (current == HARD) return 0;
		if (current == NORMAL) return 1;
		if (current == EASY) return 10;
		return 0x7fffffff;
	}

	bool canStarveAtHealth(int health) const {
		if (current == PEACEFUL) return false;
		return health > getStarvationHealthFloor();
	}

	// Vanilla mob-damage scaling against the player.
	int scaleMobDamage(int damage) const {
		if (current == PEACEFUL) return 0;
		if (current == EASY) return damage / 3 + 1;
		if (current == HARD) return damage * 3 / 2;
		return damage;
	}

	static Difficulty byId(int id) {
		if (id < PEACEFUL) return PEACEFUL;
		if (id > HARD) return HARD;
		return (Difficulty)id;
	}

	static VoxelDifficulty fromId(int id) {
		return VoxelDifficulty(byId(id));
	}

	static const char* getName(Difficulty difficulty) {
		switch (byId((int)difficulty)) {
			case PEACEFUL: return "peaceful";
			case EASY:     return "easy";
			case NORMAL:   return "normal";
			case HARD:     return "hard";
			default:       return "normal";
		}
	}

	static const char* getTranslationKey(Difficulty difficulty) {
		switch (byId((int)difficulty)) {
			case PEACEFUL: return "options.difficulty.peaceful";
			case EASY:     return "options.difficulty.easy";
			case NORMAL:   return "options.difficulty.normal";
			case HARD:     return "options.difficulty.hard";
			default:       return "options.difficulty.normal";
		}
	}

	const char* getName() const {
		return getName(current);
	}

	const char* getTranslationKey() const {
		return getTranslationKey(current);
	}

	static bool shouldSpawnMonsters(int difficulty) {
		return fromId(difficulty).shouldSpawnMonsters();
	}

	static bool shouldDepleteHunger(int difficulty) {
		return fromId(difficulty).shouldDepleteHunger();
	}

	static bool shouldRegenFood(int difficulty) {
		return fromId(difficulty).shouldRegenFood();
	}

	static bool canStarveAtHealth(int difficulty, int health) {
		return fromId(difficulty).canStarveAtHealth(health);
	}

	static int scaleMobDamage(int difficulty, int damage) {
		return fromId(difficulty).scaleMobDamage(damage);
	}

	operator Difficulty() const { return current; }
	operator int() const { return (int)current; }

private:
	Difficulty current;
};

// Existing call sites use Difficulty::PEACEFUL / Difficulty::EASY / ...
typedef VoxelDifficulty::Difficulty Difficulty;

#endif /*NET_MINECRAFT_WORLD__Difficulty_H__*/
