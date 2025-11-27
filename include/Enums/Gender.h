#ifndef GAME_ENUMS_H
#define GAME_ENUMS_H

/// Gender a Pokemon can have
/// Provided to function for Encounter template values
enum class Gender
{
    Male,
    Female,
    Genderless,
    Random = Genderless,
};

#endif